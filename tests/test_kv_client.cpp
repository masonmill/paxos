#include "gtest/gtest.h"
#include "kv_rpcs.h"
#include "kv_server.h"
#include "paxos.h"
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

TEST(KvServerTest, HandlersAbortWhenInvoked) {
  paxos::RpcDispatchRegistry registry;
  paxos::RegisterKvHandlers(&registry);

  paxos::RpcRequest get_request;
  get_request.method_name = paxos::kKvGetMethod;
  EXPECT_DEATH(registry.Dispatch(get_request), "");

  paxos::RpcRequest put_append_request;
  put_append_request.method_name = paxos::kKvPutAppendMethod;
  EXPECT_DEATH(registry.Dispatch(put_append_request), "");
}

TEST(KvServerTest, CoexistsWithPaxosHandlersOnOneRegistry) {
  paxos::RpcDispatchRegistry registry;
  paxos::Paxos peer({"/tmp/paxos_test_peer0.sock"}, /*me=*/0, &registry);
  paxos::RegisterKvHandlers(&registry);

  paxos::RpcRequest prepare_request;
  prepare_request.method_name = paxos::kPaxosPrepareMethod;
  EXPECT_DEATH(registry.Dispatch(prepare_request), "");

  paxos::RpcRequest get_request;
  get_request.method_name = paxos::kKvGetMethod;
  EXPECT_DEATH(registry.Dispatch(get_request), "");
}

}  // namespace
