#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "paxos.h"
#include "socket_directory.h"

namespace {

using paxos::Fate;
using paxos::Paxos;
using paxos::testing::Partition;
using paxos::testing::SocketDirectory;
using Peers = std::vector<std::unique_ptr<Paxos>>;

std::mt19937& Random() {
  static std::mt19937 random{std::random_device{}()};
  return random;
}

int RandomInt() { return static_cast<int>(Random()() % 1000000000); }

std::string RandomString(std::size_t length) {
  static constexpr char kLetters[] = "abcdefghijklmnopqrstuvwxyz";
  std::string result;
  for (std::size_t i = 0; i < length; ++i) {
    result += kLetters[Random()() % (sizeof(kLetters) - 1)];
  }
  return result;
}

void SleepFor(std::chrono::milliseconds duration) {
  std::this_thread::sleep_for(duration);
}

// Returns: peers sharing `directory`'s ports, with `peer_count` slots.
Peers MakePeers(const SocketDirectory& directory, int peer_count) {
  std::vector<std::string> ports = directory.Ports(peer_count);
  Peers peers;
  for (int i = 0; i < peer_count; ++i) {
    peers.push_back(std::make_unique<Paxos>(ports, i, nullptr));
  }
  return peers;
}

// Returns: peers that reach each other only through partition links.
Peers MakePartitionedPeers(const SocketDirectory& directory, int peer_count) {
  Peers peers;
  for (int i = 0; i < peer_count; ++i) {
    peers.push_back(std::make_unique<Paxos>(
        directory.PartitionedPorts(peer_count, i), i, nullptr));
  }
  return peers;
}

// Returns: how many peers decided `seq`. Fails the test if two peers
// decided differently, or on a value outside a non-empty `wanted_values`.
int NumDecided(const Peers& peers, int seq,
               const std::vector<std::string>& wanted_values = {}) {
  int count = 0;
  std::string decided_value;
  for (std::size_t i = 0; i < peers.size(); ++i) {
    if (peers[i] == nullptr) {
      continue;
    }

    std::string value;
    if (peers[i]->Status(seq, &value) != Fate::kDecided) {
      continue;
    }
    if (count > 0 && value != decided_value) {
      ADD_FAILURE() << "decided value " << value << " for seq=" << seq
                    << " at peer " << i << " does not match " << decided_value;
    }
    if (!wanted_values.empty() &&
        std::find(wanted_values.begin(), wanted_values.end(), value) ==
            wanted_values.end()) {
      ADD_FAILURE() << "decided value " << value << " for seq=" << seq
                    << " at peer " << i << " is not an expected value";
    }
    ++count;
    decided_value = value;
  }
  return count;
}

// Returns: true once at least `wanted` peers decided `seq`, false if that
// does not happen in time.
bool WaitN(const Peers& peers, int seq, int wanted,
           const std::vector<std::string>& wanted_values = {}) {
  auto delay = std::chrono::milliseconds(10);
  for (int iteration = 0; iteration < 30; ++iteration) {
    if (NumDecided(peers, seq, wanted_values) >= wanted) {
      break;
    }
    SleepFor(delay);
    if (delay < std::chrono::seconds(1)) {
      delay *= 2;
    }
  }
  return NumDecided(peers, seq, wanted_values) >= wanted;
}

bool WaitMajority(const Peers& peers, int seq,
                  const std::vector<std::string>& wanted_values = {}) {
  return WaitN(peers, seq, static_cast<int>(peers.size()) / 2 + 1,
               wanted_values);
}

// Returns: true if at most `max` peers decided `seq` after a pause.
bool CheckMax(const Peers& peers, int seq, int max) {
  SleepFor(std::chrono::seconds(3));
  return NumDecided(peers, seq) <= max;
}

std::string Str(int value) { return std::to_string(value); }

TEST(PaxosAcceptanceTest, Basic) {
  const int kPeerCount = 3;
  SocketDirectory directory;
  Peers peers = MakePeers(directory, kPeerCount);

  // Single proposer.
  peers[0]->Start(0, "hello");
  ASSERT_TRUE(WaitN(peers, 0, kPeerCount, {"hello"}));

  // Many proposers, same value.
  for (int i = 0; i < kPeerCount; ++i) {
    peers[i]->Start(1, "77");
  }
  ASSERT_TRUE(WaitN(peers, 1, kPeerCount, {"77"}));

  // Many proposers, different values.
  peers[0]->Start(2, "100");
  peers[1]->Start(2, "101");
  peers[2]->Start(2, "102");
  ASSERT_TRUE(WaitN(peers, 2, kPeerCount, {"100", "101", "102"}));

  // Out-of-order instances.
  peers[0]->Start(7, "700");
  peers[0]->Start(6, "600");
  peers[1]->Start(5, "500");
  ASSERT_TRUE(WaitN(peers, 7, kPeerCount, {"700"}));
  peers[0]->Start(4, "400");
  peers[1]->Start(3, "300");
  ASSERT_TRUE(WaitN(peers, 6, kPeerCount, {"600"}));
  ASSERT_TRUE(WaitN(peers, 5, kPeerCount, {"500"}));
  ASSERT_TRUE(WaitN(peers, 4, kPeerCount, {"400"}));
  ASSERT_TRUE(WaitN(peers, 3, kPeerCount, {"300"}));

  EXPECT_EQ(peers[0]->Max(), 7);
}

TEST(PaxosAcceptanceTest, Deaf) {
  const int kPeerCount = 5;
  SocketDirectory directory;
  Peers peers = MakePeers(directory, kPeerCount);

  peers[0]->Start(0, "hello");
  ASSERT_TRUE(WaitN(peers, 0, kPeerCount, {"hello"}));

  std::filesystem::remove(directory.Port(0));
  std::filesystem::remove(directory.Port(kPeerCount - 1));

  peers[1]->Start(1, "goodbye");
  ASSERT_TRUE(WaitMajority(peers, 1, {"goodbye"}));
  SleepFor(std::chrono::seconds(1));
  ASSERT_EQ(NumDecided(peers, 1, {"goodbye"}), kPeerCount - 2)
      << "a deaf peer heard about a decision";

  peers[0]->Start(1, "xxx");
  ASSERT_TRUE(WaitN(peers, 1, kPeerCount - 1, {"goodbye"}));
  SleepFor(std::chrono::seconds(1));
  ASSERT_EQ(NumDecided(peers, 1, {"goodbye"}), kPeerCount - 1)
      << "a deaf peer heard about a decision";

  peers[kPeerCount - 1]->Start(1, "yyy");
  ASSERT_TRUE(WaitN(peers, 1, kPeerCount, {"goodbye"}));
}

TEST(PaxosAcceptanceTest, Forget) {
  const int kPeerCount = 6;
  SocketDirectory directory;
  Peers peers = MakePeers(directory, kPeerCount);

  for (int i = 0; i < kPeerCount; ++i) {
    ASSERT_LE(peers[i]->Min(), 0) << "wrong initial Min()";
  }

  peers[0]->Start(0, "00");
  peers[1]->Start(1, "11");
  peers[2]->Start(2, "22");
  peers[0]->Start(6, "66");
  peers[1]->Start(7, "77");

  ASSERT_TRUE(WaitN(peers, 0, kPeerCount, {"00"}));
  for (int i = 0; i < kPeerCount; ++i) {
    ASSERT_EQ(peers[i]->Min(), 0);
  }

  ASSERT_TRUE(WaitN(peers, 1, kPeerCount, {"11"}));
  for (int i = 0; i < kPeerCount; ++i) {
    ASSERT_EQ(peers[i]->Min(), 0);
  }

  for (int i = 0; i < kPeerCount; ++i) {
    peers[i]->Done(0);
  }
  for (int i = 1; i < kPeerCount; ++i) {
    peers[i]->Done(1);
  }
  for (int i = 0; i < kPeerCount; ++i) {
    peers[i]->Start(8 + i, "xx");
  }

  bool all_ok = false;
  for (int iteration = 0; iteration < 12 && !all_ok; ++iteration) {
    all_ok = true;
    for (int i = 0; i < kPeerCount; ++i) {
      if (peers[i]->Min() != 1) {
        all_ok = false;
      }
    }
    if (!all_ok) {
      SleepFor(std::chrono::seconds(1));
    }
  }
  EXPECT_TRUE(all_ok) << "Min() did not advance after Done()";
}

TEST(PaxosAcceptanceTest, ManyForget) {
  const int kPeerCount = 3;
  const int kMaxSeq = 20;
  SocketDirectory directory;
  Peers peers = MakePeers(directory, kPeerCount);
  for (int i = 0; i < kPeerCount; ++i) {
    peers[i]->SetUnreliable(true);
  }

  std::thread starter([&] {
    std::vector<int> seqs;
    for (int seq = 0; seq < kMaxSeq; ++seq) {
      seqs.push_back(seq);
    }
    std::shuffle(seqs.begin(), seqs.end(), Random());
    for (int seq : seqs) {
      peers[RandomInt() % kPeerCount]->Start(seq, Str(RandomInt()));
    }
  });

  std::atomic<bool> done = false;
  std::thread forgetter([&] {
    while (!done) {
      int seq = RandomInt() % kMaxSeq;
      Paxos& peer = *peers[RandomInt() % kPeerCount];
      std::string value;
      if (seq >= peer.Min() && peer.Status(seq, &value) == Fate::kDecided) {
        peer.Done(seq);
      }
      std::this_thread::yield();
    }
  });

  SleepFor(std::chrono::seconds(5));
  done = true;
  starter.join();
  forgetter.join();
  for (int i = 0; i < kPeerCount; ++i) {
    peers[i]->SetUnreliable(false);
  }
  SleepFor(std::chrono::seconds(2));

  for (int seq = 0; seq < kMaxSeq; ++seq) {
    for (int i = 0; i < kPeerCount; ++i) {
      std::string value;
      if (seq >= peers[i]->Min()) {
        peers[i]->Status(seq, &value);
      }
    }
  }
}

TEST(PaxosAcceptanceTest, ForgetMem) {
  const int kPeerCount = 3;
  SocketDirectory directory;
  Peers peers = MakePeers(directory, kPeerCount);

  peers[0]->Start(0, "x");
  ASSERT_TRUE(WaitN(peers, 0, kPeerCount, {"x"}));

  for (int seq = 1; seq <= 10; ++seq) {
    peers[0]->Start(seq, RandomString(1000));
    ASSERT_TRUE(WaitN(peers, seq, kPeerCount));
  }

  for (int i = 0; i < kPeerCount; ++i) {
    peers[i]->Done(10);
  }
  for (int i = 0; i < kPeerCount; ++i) {
    peers[i]->Start(11 + i, "z");
  }
  SleepFor(std::chrono::seconds(3));
  for (int i = 0; i < kPeerCount; ++i) {
    ASSERT_EQ(peers[i]->Min(), 11);
  }

  std::vector<std::string> again;
  for (int seq = 0; seq < kPeerCount; ++seq) {
    again.push_back(RandomString(20));
    for (int i = 0; i < kPeerCount; ++i) {
      std::string value;
      ASSERT_EQ(peers[i]->Status(seq, &value), Fate::kForgotten)
          << "seq " << seq << " < Min() but not Forgotten";
      peers[i]->Start(seq, again[seq]);
    }
  }
  SleepFor(std::chrono::seconds(1));
  for (int seq = 0; seq < kPeerCount; ++seq) {
    for (int i = 0; i < kPeerCount; ++i) {
      std::string value;
      ASSERT_EQ(peers[i]->Status(seq, &value), Fate::kForgotten)
          << "seq " << seq << " < Min() but not Forgotten";
      ASSERT_NE(value, again[seq]);
    }
  }
}

TEST(PaxosAcceptanceTest, RpcCount) {
  const int kPeerCount = 3;
  SocketDirectory directory;
  Peers peers = MakePeers(directory, kPeerCount);

  const int kSerialInstances = 5;
  int seq = 0;
  for (int i = 0; i < kSerialInstances; ++i) {
    peers[0]->Start(seq, "x");
    ASSERT_TRUE(WaitN(peers, seq, kPeerCount, {"x"}));
    ++seq;
  }

  SleepFor(std::chrono::seconds(2));

  int serial_total = 0;
  for (int j = 0; j < kPeerCount; ++j) {
    serial_total += peers[j]->rpc_count();
  }

  // Per agreement: a prepare, an accept, and a decide to each peer.
  EXPECT_LE(serial_total, kSerialInstances * kPeerCount * kPeerCount)
      << "too many RPCs for serial Start()s";

  const int kConcurrentInstances = 5;
  for (int i = 0; i < kConcurrentInstances; ++i) {
    std::vector<std::thread> starters;
    for (int j = 0; j < kPeerCount; ++j) {
      starters.emplace_back(
          [&, seq, i, j] { peers[j]->Start(seq, Str(j + i * 10)); });
    }
    for (std::thread& starter : starters) {
      starter.join();
    }
    ASSERT_TRUE(WaitN(peers, seq, kPeerCount));
    ++seq;
  }

  SleepFor(std::chrono::seconds(2));

  int concurrent_total = -serial_total;
  for (int j = 0; j < kPeerCount; ++j) {
    concurrent_total += peers[j]->rpc_count();
  }

  // Worst case per agreement: the third proposer retries twice.
  EXPECT_LE(concurrent_total, kConcurrentInstances * kPeerCount * 15)
      << "too many RPCs for concurrent Start()s";
}

TEST(PaxosAcceptanceTest, Many) {
  const int kPeerCount = 3;
  const int kInstances = 50;
  SocketDirectory directory;
  Peers peers = MakePeers(directory, kPeerCount);
  for (int i = 0; i < kPeerCount; ++i) {
    peers[i]->Start(0, "0");
  }

  for (int seq = 1; seq < kInstances; ++seq) {
    // Limits active instances, and so open file descriptors.
    while (seq >= 5 && NumDecided(peers, seq - 5) < kPeerCount) {
      SleepFor(std::chrono::milliseconds(20));
    }
    for (int i = 0; i < kPeerCount; ++i) {
      peers[i]->Start(seq, Str(seq * 10 + i));
    }
  }

  bool all_decided = false;
  while (!all_decided) {
    all_decided = true;
    for (int seq = 1; seq < kInstances; ++seq) {
      if (NumDecided(peers, seq) < kPeerCount) {
        all_decided = false;
      }
    }
    SleepFor(std::chrono::milliseconds(100));
  }
}

TEST(PaxosAcceptanceTest, Old) {
  const int kPeerCount = 5;
  SocketDirectory directory;
  std::vector<std::string> ports = directory.Ports(kPeerCount);
  Peers peers(kPeerCount);

  peers[1] = std::make_unique<Paxos>(ports, 1, nullptr);
  peers[2] = std::make_unique<Paxos>(ports, 2, nullptr);
  peers[3] = std::make_unique<Paxos>(ports, 3, nullptr);
  peers[1]->Start(1, "111");

  ASSERT_TRUE(WaitMajority(peers, 1, {"111"}));

  peers[0] = std::make_unique<Paxos>(ports, 0, nullptr);
  peers[0]->Start(1, "222");

  ASSERT_TRUE(WaitN(peers, 1, 4, {"111"}));
}

TEST(PaxosAcceptanceTest, ManyUnreliable) {
  const int kPeerCount = 3;
  const int kInstances = 50;
  SocketDirectory directory;
  Peers peers = MakePeers(directory, kPeerCount);
  for (int i = 0; i < kPeerCount; ++i) {
    peers[i]->SetUnreliable(true);
    peers[i]->Start(0, "0");
  }

  for (int seq = 1; seq < kInstances; ++seq) {
    // Limits active instances, and so open file descriptors.
    while (seq >= 3 && NumDecided(peers, seq - 3) < kPeerCount) {
      SleepFor(std::chrono::milliseconds(20));
    }
    for (int i = 0; i < kPeerCount; ++i) {
      peers[i]->Start(seq, Str(seq * 10 + i));
    }
  }

  bool all_decided = false;
  while (!all_decided) {
    all_decided = true;
    for (int seq = 1; seq < kInstances; ++seq) {
      if (NumDecided(peers, seq) < kPeerCount) {
        all_decided = false;
      }
    }
    SleepFor(std::chrono::milliseconds(100));
  }
}

TEST(PaxosAcceptanceTest, Partition) {
  const int kPeerCount = 5;
  SocketDirectory directory;
  Peers peers = MakePartitionedPeers(directory, kPeerCount);
  int seq = 0;

  // No decision if partitioned.
  ASSERT_TRUE(Partition(directory, kPeerCount, {{0, 2}, {1, 3}, {4}}));
  peers[1]->Start(seq, "111");
  ASSERT_TRUE(CheckMax(peers, seq, 0)) << "too many decided";

  // Decision in majority partition.
  ASSERT_TRUE(Partition(directory, kPeerCount, {{0}, {1, 2, 3}, {4}}));
  SleepFor(std::chrono::seconds(2));
  ASSERT_TRUE(WaitMajority(peers, seq, {"111"}));

  // All agree after full heal.
  peers[0]->Start(seq, "1000");
  peers[4]->Start(seq, "1004");
  ASSERT_TRUE(Partition(directory, kPeerCount, {{0, 1, 2, 3, 4}}));
  ASSERT_TRUE(WaitN(peers, seq, kPeerCount, {"111"}));

  // One peer switches partitions.
  for (int iteration = 0; iteration < 20; ++iteration) {
    ++seq;

    ASSERT_TRUE(Partition(directory, kPeerCount, {{0, 1, 2}, {3, 4}}));
    peers[0]->Start(seq, Str(seq * 10));
    peers[3]->Start(seq, Str(seq * 10 + 1));
    ASSERT_TRUE(WaitMajority(peers, seq, {Str(seq * 10)}));
    ASSERT_LE(NumDecided(peers, seq, {Str(seq * 10)}), 3)
        << "too many decided";

    ASSERT_TRUE(Partition(directory, kPeerCount, {{0, 1}, {2, 3, 4}}));
    ASSERT_TRUE(WaitN(peers, seq, kPeerCount, {Str(seq * 10)}));
  }

  // One peer switches partitions, unreliable.
  for (int iteration = 0; iteration < 20; ++iteration) {
    ++seq;
    const std::vector<std::string> values = {Str(seq * 10), Str(seq * 10 + 1),
                                             Str(seq * 10 + 2)};

    for (int i = 0; i < kPeerCount; ++i) {
      peers[i]->SetUnreliable(true);
    }

    ASSERT_TRUE(Partition(directory, kPeerCount, {{0, 1, 2}, {3, 4}}));
    for (int i = 0; i < kPeerCount; ++i) {
      peers[i]->Start(seq, Str(seq * 10 + i));
    }
    ASSERT_TRUE(WaitN(peers, seq, 3, values));
    ASSERT_LE(NumDecided(peers, seq, values), 3) << "too many decided";

    ASSERT_TRUE(Partition(directory, kPeerCount, {{0, 1}, {2, 3, 4}}));

    for (int i = 0; i < kPeerCount; ++i) {
      peers[i]->SetUnreliable(false);
    }

    ASSERT_TRUE(WaitN(peers, seq, kPeerCount, values));
  }
}

TEST(PaxosAcceptanceTest, Lots) {
  const int kPeerCount = 5;
  SocketDirectory directory;
  Peers peers = MakePartitionedPeers(directory, kPeerCount);
  for (int i = 0; i < kPeerCount; ++i) {
    peers[i]->SetUnreliable(true);
  }

  std::atomic<bool> done = false;
  std::atomic<int> seq = 0;

  std::thread partitioner([&] {
    while (!done) {
      std::vector<std::vector<int>> groups(3);
      for (int i = 0; i < kPeerCount; ++i) {
        groups[RandomInt() % 3].push_back(i);
      }
      Partition(directory, kPeerCount, groups);
      SleepFor(std::chrono::milliseconds(RandomInt() % 200));
    }
  });

  std::thread starter([&] {
    while (!done) {
      int num_decided = 0;
      int current_seq = seq;
      for (int i = 0; i < current_seq; ++i) {
        if (NumDecided(peers, i) == kPeerCount) {
          ++num_decided;
        }
      }
      if (current_seq - num_decided < 10) {
        for (int i = 0; i < kPeerCount; ++i) {
          peers[i]->Start(current_seq, Str(RandomInt() % 10));
        }
        ++seq;
      }
      SleepFor(std::chrono::milliseconds(RandomInt() % 300));
    }
  });

  std::thread checker([&] {
    while (!done) {
      for (int i = 0; i < seq; ++i) {
        NumDecided(peers, i);
      }
      SleepFor(std::chrono::milliseconds(RandomInt() % 300));
    }
  });

  SleepFor(std::chrono::seconds(20));
  done = true;
  partitioner.join();
  starter.join();
  checker.join();

  for (int i = 0; i < kPeerCount; ++i) {
    peers[i]->SetUnreliable(false);
  }
  ASSERT_TRUE(Partition(directory, kPeerCount, {{0, 1, 2, 3, 4}}));
  SleepFor(std::chrono::seconds(5));

  for (int i = 0; i < seq; ++i) {
    ASSERT_TRUE(WaitMajority(peers, i));
  }
}

}  // namespace
