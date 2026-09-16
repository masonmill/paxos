#include <thread>

#include "gtest/gtest.h"
#include "kv_client.h"
#include "kv_rpcs.h"
#include "paxos_node.h"
#include "paxos_rpcs.h"
#include "rpc_dispatch.h"
#include "rpc_transport.h"

namespace {

TEST(KvRpcsTest, SerializesAndDeserializesRoundTrip) {
  paxos::GetArgs sent_get;
  sent_get.key = 7;

  std::vector<std::uint8_t> get_payload = paxos::SerializePayload(sent_get);
  paxos::GetArgs received_get;
  ASSERT_TRUE(paxos::DeserializePayload(get_payload, &received_get));
  EXPECT_EQ(received_get.key, sent_get.key);

  paxos::PutAppendArgs sent_put_append;
  sent_put_append.key = 7;
  sent_put_append.value = 42;
  sent_put_append.is_append = true;

  std::vector<std::uint8_t> put_append_payload =
      paxos::SerializePayload(sent_put_append);
  paxos::PutAppendArgs received_put_append;
  ASSERT_TRUE(
      paxos::DeserializePayload(put_append_payload, &received_put_append));
  EXPECT_EQ(received_put_append.key, sent_put_append.key);
  EXPECT_EQ(received_put_append.value, sent_put_append.value);
  EXPECT_EQ(received_put_append.is_append, sent_put_append.is_append);
}

TEST(KvServerTest, StubHandlersReturnOkStatus) {
  paxos::RpcDispatchRegistry registry;
  paxos::RegisterKvStubHandlers(&registry);

  paxos::RpcRequest get_request;
  get_request.method_name = paxos::kKvGetMethod;
  paxos::RpcReply get_reply = registry.Dispatch(get_request);
  EXPECT_EQ(get_reply.status, paxos::RpcStatus::kOk);
  paxos::GetReply get_result;
  EXPECT_TRUE(paxos::DeserializePayload(get_reply.payload, &get_result));

  paxos::RpcRequest put_append_request;
  put_append_request.method_name = paxos::kKvPutAppendMethod;
  paxos::RpcReply put_append_reply = registry.Dispatch(put_append_request);
  EXPECT_EQ(put_append_reply.status, paxos::RpcStatus::kOk);
  paxos::PutAppendReply put_append_result;
  EXPECT_TRUE(
      paxos::DeserializePayload(put_append_reply.payload, &put_append_result));
}

TEST(KvServerTest, CoexistsWithPaxosHandlersOnOneRegistry) {
  paxos::RpcDispatchRegistry registry;
  paxos::RegisterPaxosStubHandlers(&registry);
  paxos::RegisterKvStubHandlers(&registry);

  paxos::RpcRequest prepare_request;
  prepare_request.method_name = paxos::kPaxosPrepareMethod;
  EXPECT_EQ(registry.Dispatch(prepare_request).status, paxos::RpcStatus::kOk);

  paxos::RpcRequest get_request;
  get_request.method_name = paxos::kKvGetMethod;
  EXPECT_EQ(registry.Dispatch(get_request).status, paxos::RpcStatus::kOk);
}

TEST(KvClientTest, RetriesUntilASuccessfulServer) {
  paxos::RpcDispatchRegistry registry;
  paxos::RegisterKvStubHandlers(&registry);

  const std::string live_socket_path = "/tmp/paxos_test_kv_client_live.sock";
  int listen_fd = paxos::BindAndListenUnixSocket(live_socket_path);
  ASSERT_GE(listen_fd, 0);

  std::thread accept_thread(
      [&registry, listen_fd] { registry.RunAcceptLoop(listen_fd); });
  accept_thread.detach();

  paxos::KvClient client(
      {"/tmp/paxos_test_kv_client_unreachable.sock", live_socket_path});

  // `Get` retries forever on failure, so reaching this line at all proves
  // it got past the unreachable server and reached the live one.
  client.Get(paxos::GetArgs());
  SUCCEED();
}

}  // namespace
