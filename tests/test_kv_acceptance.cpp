#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "kv_client.h"
#include "kv_server.h"
#include "paxos.h"
#include "socket_directory.h"

namespace {

using paxos::KvClient;
using paxos::KvServer;
using paxos::testing::Partition;
using paxos::testing::SocketDirectory;
using Servers = std::vector<std::unique_ptr<KvServer>>;

std::mt19937& Random() {
  static std::mt19937 random{std::random_device{}()};
  return random;
}

int RandomInt() { return static_cast<int>(Random()() % 1000000000); }

std::string Str(int value) { return std::to_string(value); }

void SleepFor(std::chrono::milliseconds duration) {
  std::this_thread::sleep_for(duration);
}

// Returns: true if `done` becomes set within `timeout`.
bool WaitFor(const std::atomic<bool>& done, std::chrono::milliseconds timeout) {
  auto deadline = std::chrono::steady_clock::now() + timeout;
  while (!done && std::chrono::steady_clock::now() < deadline) {
    SleepFor(std::chrono::milliseconds(10));
  }
  return done;
}

void Check(KvClient& client, const std::string& key,
           const std::string& value) {
  EXPECT_EQ(client.Get(key), value) << "Get(" << key << ")";
}

Servers MakeServers(const SocketDirectory& directory, int server_count) {
  Servers servers;
  for (int i = 0; i < server_count; ++i) {
    servers.push_back(
        std::make_unique<KvServer>(directory.Ports(server_count), i));
  }
  return servers;
}

// Returns: servers that reach each other only through partition links.
Servers MakePartitionedServers(const SocketDirectory& directory,
                               int server_count) {
  Servers servers;
  for (int i = 0; i < server_count; ++i) {
    servers.push_back(std::make_unique<KvServer>(
        directory.PartitionedPorts(server_count, i), i));
  }
  return servers;
}

// Returns: a client trying `ports` in a random order.
std::unique_ptr<KvClient> RandomClient(std::vector<std::string> ports) {
  std::shuffle(ports.begin(), ports.end(), Random());
  return std::make_unique<KvClient>(ports);
}

// Checks that every known append is in `value` once, in order per client.
void CheckAppends(const std::string& value, const std::vector<int>& counts) {
  for (std::size_t i = 0; i < counts.size(); ++i) {
    std::size_t last_offset = std::string::npos;
    for (int j = 0; j < counts[i]; ++j) {
      std::string wanted = "x " + Str(i) + " " + Str(j) + " y";
      std::size_t offset = value.find(wanted);
      ASSERT_NE(offset, std::string::npos)
          << "missing element in Append result";
      ASSERT_EQ(value.rfind(wanted), offset)
          << "duplicate element in Append result";
      ASSERT_TRUE(last_offset == std::string::npos || offset > last_offset)
          << "wrong order for element in Append result";
      last_offset = offset;
    }
  }
}

TEST(KvAcceptanceTest, Basic) {
  const int kServerCount = 3;
  SocketDirectory directory;
  Servers servers = MakeServers(directory, kServerCount);

  KvClient client(directory.Ports(kServerCount));
  std::vector<std::unique_ptr<KvClient>> clients;
  for (int i = 0; i < kServerCount; ++i) {
    clients.push_back(std::make_unique<KvClient>(
        std::vector<std::string>{directory.Port(i)}));
  }

  // Basic put/append/get.
  client.Append("app", "x");
  client.Append("app", "y");
  Check(client, "app", "xy");

  client.Put("a", "aa");
  Check(client, "a", "aa");

  clients[1]->Put("a", "aaa");
  Check(*clients[2], "a", "aaa");
  Check(*clients[1], "a", "aaa");
  Check(client, "a", "aaa");

  // Concurrent clients.
  for (int iteration = 0; iteration < 20; ++iteration) {
    std::vector<std::thread> threads;
    for (int n = 0; n < 15; ++n) {
      threads.emplace_back([&] {
        KvClient my_client({directory.Port(RandomInt() % kServerCount)});
        if (RandomInt() % 1000 < 500) {
          my_client.Put("b", Str(RandomInt()));
        } else {
          my_client.Get("b");
        }
      });
    }
    for (std::thread& thread : threads) {
      thread.join();
    }

    std::string first = clients[0]->Get("b");
    for (int i = 1; i < kServerCount; ++i) {
      ASSERT_EQ(clients[i]->Get("b"), first) << "mismatch at server " << i;
    }
  }

  SleepFor(std::chrono::seconds(1));
}

TEST(KvAcceptanceTest, Done) {
  const int kServerCount = 3;
  const int kItems = 10;
  const std::size_t kValueSize = 1000000;
  SocketDirectory directory;
  Servers servers = MakeServers(directory, kServerCount);

  KvClient client(directory.Ports(kServerCount));
  std::vector<std::unique_ptr<KvClient>> clients;
  for (int i = 0; i < kServerCount; ++i) {
    clients.push_back(std::make_unique<KvClient>(
        std::vector<std::string>{directory.Port(i)}));
  }

  client.Put("a", "aa");
  Check(client, "a", "aa");

  for (int iteration = 0; iteration < 2; ++iteration) {
    for (int i = 0; i < kItems; ++i) {
      std::string value(kValueSize, '\0');
      for (char& c : value) {
        c = static_cast<char>(RandomInt() % 100 + 1);
      }
      client.Put(Str(i), value);
      Check(*clients[i % kServerCount], Str(i), value);
    }
  }
  // Puts and Gets decided so far.
  const int kEarlyOps = 2 + 2 * 2 * kItems;

  // Done info may be piggybacked on proposer messages.
  for (int iteration = 0; iteration < 2; ++iteration) {
    for (int i = 0; i < kServerCount; ++i) {
      clients[i]->Put("a", "aa");
      Check(*clients[i], "a", "aa");
    }
  }

  SleepFor(std::chrono::seconds(1));

  for (int i = 0; i < kServerCount; ++i) {
    EXPECT_GE(servers[i]->paxos()->Min(), kEarlyOps)
        << "server " << i << " did not free its Paxos log";
  }
}

TEST(KvAcceptanceTest, Partition) {
  const int kServerCount = 5;
  SocketDirectory directory;
  Servers servers = MakePartitionedServers(directory, kServerCount);

  std::vector<std::shared_ptr<KvClient>> clients;
  for (int i = 0; i < kServerCount; ++i) {
    clients.push_back(std::make_shared<KvClient>(
        std::vector<std::string>{directory.Port(i)}));
  }

  // No partition.
  ASSERT_TRUE(Partition(directory, kServerCount, {{0, 1, 2, 3, 4}}));
  clients[0]->Put("1", "12");
  clients[2]->Put("1", "13");
  Check(*clients[3], "1", "13");

  // Progress in majority.
  ASSERT_TRUE(Partition(directory, kServerCount, {{2, 3, 4}, {0, 1}}));
  clients[2]->Put("1", "14");
  Check(*clients[4], "1", "14");

  // No progress in minority. Threads are detached so a stuck call can't hang
  // the test.
  auto done0 = std::make_shared<std::atomic<bool>>(false);
  auto done1 = std::make_shared<std::atomic<bool>>(false);
  std::thread([client = clients[0], done0] {
    client->Put("1", "15");
    *done0 = true;
  }).detach();
  std::thread([client = clients[1], done1] {
    client->Get("1");
    *done1 = true;
  }).detach();

  SleepFor(std::chrono::seconds(1));
  ASSERT_FALSE(*done0) << "Put in minority completed";
  ASSERT_FALSE(*done1) << "Get in minority completed";

  Check(*clients[4], "1", "14");
  clients[3]->Put("1", "16");
  Check(*clients[4], "1", "16");

  // Completion after heal.
  ASSERT_TRUE(Partition(directory, kServerCount, {{0, 2, 3, 4}, {1}}));
  ASSERT_TRUE(WaitFor(*done0, std::chrono::seconds(3)))
      << "Put did not complete";
  ASSERT_FALSE(*done1) << "Get in minority completed";

  Check(*clients[4], "1", "15");
  Check(*clients[0], "1", "15");

  ASSERT_TRUE(Partition(directory, kServerCount, {{0, 1, 2}, {3, 4}}));
  ASSERT_TRUE(WaitFor(*done1, std::chrono::seconds(10)))
      << "Get did not complete";

  Check(*clients[1], "1", "15");
}

TEST(KvAcceptanceTest, Unreliable) {
  const int kServerCount = 3;
  SocketDirectory directory;
  Servers servers = MakeServers(directory, kServerCount);
  for (int i = 0; i < kServerCount; ++i) {
    servers[i]->SetUnreliable(true);
  }
  std::vector<std::string> ports = directory.Ports(kServerCount);

  KvClient client(ports);
  std::vector<std::unique_ptr<KvClient>> clients;
  for (int i = 0; i < kServerCount; ++i) {
    clients.push_back(std::make_unique<KvClient>(
        std::vector<std::string>{directory.Port(i)}));
  }

  // Basic put/get, unreliable.
  client.Put("a", "aa");
  Check(client, "a", "aa");

  clients[1]->Put("a", "aaa");
  Check(*clients[2], "a", "aaa");
  Check(*clients[1], "a", "aaa");
  Check(client, "a", "aaa");

  // Sequence of puts, unreliable.
  for (int iteration = 0; iteration < 6; ++iteration) {
    std::vector<std::thread> threads;
    for (int n = 0; n < 5; ++n) {
      threads.emplace_back([&, n] {
        std::unique_ptr<KvClient> my_client = RandomClient(ports);
        std::string key = Str(n);
        std::string expected = my_client->Get(key);
        for (const char* suffix : {"0", "1", "2"}) {
          my_client->Append(key, suffix);
          expected += suffix;
        }
        SleepFor(std::chrono::milliseconds(100));
        EXPECT_EQ(my_client->Get(key), expected) << "wrong value";
        EXPECT_EQ(my_client->Get(key), expected) << "wrong value";
      });
    }
    for (std::thread& thread : threads) {
      thread.join();
    }
    ASSERT_FALSE(HasFailure());
  }

  // Concurrent clients, unreliable.
  for (int iteration = 0; iteration < 20; ++iteration) {
    std::vector<std::thread> threads;
    for (int n = 0; n < 15; ++n) {
      threads.emplace_back([&] {
        std::unique_ptr<KvClient> my_client = RandomClient(ports);
        if (RandomInt() % 1000 < 500) {
          my_client->Put("b", Str(RandomInt()));
        } else {
          my_client->Get("b");
        }
      });
    }
    for (std::thread& thread : threads) {
      thread.join();
    }

    std::string first = clients[0]->Get("b");
    for (int i = 1; i < kServerCount; ++i) {
      ASSERT_EQ(clients[i]->Get("b"), first) << "mismatch at server " << i;
    }
  }

  // Concurrent Append to same key, unreliable.
  const int kClients = 5;
  const int kAppends = 5;
  client.Put("k", "");

  std::vector<std::thread> threads;
  for (int n = 0; n < kClients; ++n) {
    threads.emplace_back([&, n] {
      std::unique_ptr<KvClient> my_client = RandomClient(ports);
      for (int j = 0; j < kAppends; ++j) {
        my_client->Append("k", "x " + Str(n) + " " + Str(j) + " y");
      }
    });
  }
  for (std::thread& thread : threads) {
    thread.join();
  }

  std::string value = client.Get("k");
  CheckAppends(value, std::vector<int>(kClients, kAppends));
  for (int i = 0; i < kServerCount; ++i) {
    ASSERT_EQ(clients[i]->Get("k"), value) << "mismatch at server " << i;
  }

  SleepFor(std::chrono::seconds(1));
}

TEST(KvAcceptanceTest, Hole) {
  const int kServerCount = 5;
  const int kClients = 10;
  SocketDirectory directory;
  Servers servers = MakePartitionedServers(directory, kServerCount);

  for (int iteration = 0; iteration < 5; ++iteration) {
    ASSERT_TRUE(Partition(directory, kServerCount, {{0, 1, 2, 3, 4}}));

    KvClient client2({directory.Port(2)});
    client2.Put("q", "q");

    std::atomic<bool> done = false;
    std::vector<std::thread> threads;
    for (int n = 0; n < kClients; ++n) {
      threads.emplace_back([&, n] {
        std::vector<std::unique_ptr<KvClient>> clients;
        for (int i = 0; i < kServerCount; ++i) {
          clients.push_back(std::make_unique<KvClient>(
              std::vector<std::string>{directory.Port(i)}));
        }
        std::string key = Str(n);
        std::string last;
        clients[0]->Put(key, last);
        while (!done) {
          KvClient& my_client = *clients[RandomInt() % 2];
          if (RandomInt() % 1000 < 500) {
            std::string new_value = Str(RandomInt());
            my_client.Put(key, new_value);
            last = new_value;
          } else {
            EXPECT_EQ(my_client.Get(key), last)
                << n << ": wrong value, key " << key;
          }
        }
      });
    }

    SleepFor(std::chrono::seconds(3));

    ASSERT_TRUE(Partition(directory, kServerCount, {{2, 3, 4}, {0, 1}}));

    // The majority progresses though the minority was mid-agreement.
    Check(client2, "q", "q");
    client2.Put("q", "qq");
    Check(client2, "q", "qq");

    // Restore the network and wait for all clients.
    ASSERT_TRUE(Partition(directory, kServerCount, {{0, 1, 2, 3, 4}}));
    done = true;
    for (std::thread& thread : threads) {
      thread.join();
    }
    ASSERT_FALSE(HasFailure());
    Check(client2, "q", "qq");
  }
}

TEST(KvAcceptanceTest, ManyPartition) {
  const int kServerCount = 5;
  const int kClients = 10;
  SocketDirectory directory;
  Servers servers = MakePartitionedServers(directory, kServerCount);
  for (int i = 0; i < kServerCount; ++i) {
    servers[i]->SetUnreliable(true);
  }
  ASSERT_TRUE(Partition(directory, kServerCount, {{0, 1, 2, 3, 4}}));

  std::atomic<bool> done = false;

  std::thread partitioner([&] {
    while (!done) {
      std::vector<std::vector<int>> groups(3);
      for (int i = 0; i < kServerCount; ++i) {
        groups[RandomInt() % 3].push_back(i);
      }
      Partition(directory, kServerCount, groups);
      SleepFor(std::chrono::milliseconds(RandomInt() % 200));
    }
  });

  std::vector<std::thread> threads;
  for (int n = 0; n < kClients; ++n) {
    threads.emplace_back([&, n] {
      std::unique_ptr<KvClient> my_client =
          RandomClient(directory.Ports(kServerCount));
      std::string key = Str(n);
      std::string last;
      my_client->Put(key, last);
      while (!done) {
        if (RandomInt() % 1000 < 500) {
          std::string new_value = Str(RandomInt());
          my_client->Append(key, new_value);
          last += new_value;
        } else {
          EXPECT_EQ(my_client->Get(key), last)
              << n << ": get wrong value, key " << key;
        }
      }
    });
  }

  SleepFor(std::chrono::seconds(20));
  done = true;
  partitioner.join();
  ASSERT_TRUE(Partition(directory, kServerCount, {{0, 1, 2, 3, 4}}));

  for (std::thread& thread : threads) {
    thread.join();
  }
}

}  // namespace
