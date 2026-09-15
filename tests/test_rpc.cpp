#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "rpc_client.h"
#include "rpc_dispatch.h"
#include "rpc_transport.h"

namespace {

TEST(RpcTransportTest, RequestRoundTrip) {
  int fds[2];
  ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

  paxos::RpcRequest sent;
  sent.method_name = "Test.Echo";
  sent.payload = {1, 2, 3};
  ASSERT_TRUE(paxos::SendRequest(fds[0], sent));

  paxos::RpcRequest received;
  ASSERT_TRUE(paxos::ReceiveRequest(fds[1], &received));
  EXPECT_EQ(received.method_name, sent.method_name);
  EXPECT_EQ(received.payload, sent.payload);

  close(fds[0]);
  close(fds[1]);
}

TEST(RpcTransportTest, ReplyRoundTrip) {
  int fds[2];
  ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

  paxos::RpcReply sent;
  sent.status = paxos::RpcStatus::kOk;
  sent.payload = {4, 5};
  ASSERT_TRUE(paxos::SendReply(fds[0], sent));

  paxos::RpcReply received;
  ASSERT_TRUE(paxos::ReceiveReply(fds[1], &received));
  EXPECT_EQ(received.status, sent.status);
  EXPECT_EQ(received.payload, sent.payload);

  close(fds[0]);
  close(fds[1]);
}

TEST(RpcDispatchRegistryTest, DispatchesToKnownMethod) {
  paxos::RpcDispatchRegistry registry;
  registry.RegisterHandler("Test.Echo", [](const paxos::RpcRequest& request) {
    paxos::RpcReply reply;
    reply.status = paxos::RpcStatus::kOk;
    reply.payload = request.payload;
    return reply;
  });

  paxos::RpcRequest request;
  request.method_name = "Test.Echo";
  request.payload = {9, 8};

  paxos::RpcReply reply = registry.Dispatch(request);
  EXPECT_EQ(reply.status, paxos::RpcStatus::kOk);
  EXPECT_EQ(reply.payload, request.payload);
}

TEST(RpcDispatchRegistryTest, RejectsUnknownMethod) {
  paxos::RpcDispatchRegistry registry;

  paxos::RpcRequest request;
  request.method_name = "Test.DoesNotExist";

  paxos::RpcReply reply = registry.Dispatch(request);
  EXPECT_EQ(reply.status, paxos::RpcStatus::kUnknownMethod);
}

TEST(RpcClientTest, CallSucceedsAgainstListenerAndFailsAgainstNothing) {
  const std::string socket_path =
      "/tmp/paxos_test_rpc_transport_and_dispatch.sock";

  paxos::RpcDispatchRegistry registry;
  registry.RegisterHandler("Test.Echo", [](const paxos::RpcRequest& request) {
    paxos::RpcReply reply;
    reply.status = paxos::RpcStatus::kOk;
    reply.payload = request.payload;
    return reply;
  });

  int listen_fd = paxos::BindAndListenUnixSocket(socket_path);
  ASSERT_GE(listen_fd, 0);

  std::thread server_thread(
      [&registry, listen_fd]() { registry.RunAcceptLoop(listen_fd); });
  server_thread.detach();

  std::vector<std::uint8_t> reply_payload;
  EXPECT_TRUE(paxos::Call(socket_path, "Test.Echo", {7, 6, 5}, &reply_payload));
  EXPECT_EQ(reply_payload, (std::vector<std::uint8_t>{7, 6, 5}));

  EXPECT_FALSE(paxos::Call("/tmp/paxos_test_no_such_socket.sock", "Test.Echo",
                           {}, &reply_payload));
}

TEST(RpcClientTest, HandlesConcurrentClients) {
  constexpr int kClientCount = 4;
  const std::string socket_path = "/tmp/paxos_test_rpc_concurrent_clients.sock";

  std::mutex arrival_mutex;
  std::condition_variable all_arrived;
  int arrived_count = 0;

  paxos::RpcDispatchRegistry registry;
  registry.RegisterHandler("Test.WaitForAll", [&](const paxos::RpcRequest&) {
    std::unique_lock<std::mutex> lock(arrival_mutex);
    ++arrived_count;
    all_arrived.notify_all();

    bool all_clients_arrived =
        all_arrived.wait_for(lock, std::chrono::seconds(2),
                             [&] { return arrived_count == kClientCount; });

    paxos::RpcReply reply;
    reply.status = paxos::RpcStatus::kOk;
    reply.payload = {all_clients_arrived ? std::uint8_t{1} : std::uint8_t{0}};
    return reply;
  });

  int listen_fd = paxos::BindAndListenUnixSocket(socket_path);
  ASSERT_GE(listen_fd, 0);

  std::thread server_thread(
      [&registry, listen_fd]() { registry.RunAcceptLoop(listen_fd); });
  server_thread.detach();

  std::vector<std::thread> client_threads;
  std::vector<bool> client_ok(kClientCount, false);
  std::vector<std::vector<std::uint8_t>> client_replies(kClientCount);
  for (int i = 0; i < kClientCount; ++i) {
    client_threads.emplace_back([&, i]() {
      client_ok[i] =
          paxos::Call(socket_path, "Test.WaitForAll", {}, &client_replies[i]);
    });
  }

  for (std::thread& client_thread : client_threads) {
    client_thread.join();
  }

  for (int i = 0; i < kClientCount; ++i) {
    EXPECT_TRUE(client_ok[i]);
    EXPECT_EQ(client_replies[i], (std::vector<std::uint8_t>{1}))
        << "client " << i << " did not see all clients arrive concurrently";
  }
}

}  // namespace
