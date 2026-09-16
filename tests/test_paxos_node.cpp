#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "paxos_impl.h"
#include "paxos_node.h"
#include "paxos_rpcs.h"
#include "peer_config.h"
#include "rpc_dispatch.h"
#include "rpc_transport.h"

namespace {

TEST(PeerConfigTest, ParsesOnePathPerLine) {
  const std::string config_path = "/tmp/paxos_test_peer_config.conf";
  std::ofstream config_file(config_path);
  config_file << "/tmp/peer0.sock\n/tmp/peer1.sock\n/tmp/peer2.sock\n";
  config_file.close();

  std::vector<std::string> peer_socket_paths;
  ASSERT_TRUE(paxos::ParsePeerConfig(config_path, &peer_socket_paths));
  EXPECT_EQ(peer_socket_paths,
            (std::vector<std::string>{"/tmp/peer0.sock", "/tmp/peer1.sock",
                                      "/tmp/peer2.sock"}));

  std::remove(config_path.c_str());
}

TEST(PeerConfigTest, ReturnsFalseForMissingFile) {
  std::vector<std::string> peer_socket_paths;
  EXPECT_FALSE(paxos::ParsePeerConfig("/tmp/paxos_test_no_such_config.conf",
                                      &peer_socket_paths));
}

TEST(PaxosRpcsTest, SerializesAndDeserializesRoundTrip) {
  paxos::PrepareArgs sent;
  sent.instance_number = 7;
  sent.proposal_number = 42;

  std::vector<std::uint8_t> payload = paxos::SerializePayload(sent);

  paxos::PrepareArgs received;
  ASSERT_TRUE(paxos::DeserializePayload(payload, &received));
  EXPECT_EQ(received.instance_number, sent.instance_number);
  EXPECT_EQ(received.proposal_number, sent.proposal_number);
}

TEST(PaxosNodeTest, ValidatesPeerIdRange) {
  EXPECT_TRUE(paxos::ValidatePeerId(/*peer_count=*/3, /*id=*/0));
  EXPECT_TRUE(paxos::ValidatePeerId(/*peer_count=*/3, /*id=*/2));
  EXPECT_FALSE(paxos::ValidatePeerId(/*peer_count=*/3, /*id=*/3));
  EXPECT_FALSE(paxos::ValidatePeerId(/*peer_count=*/3, /*id=*/-1));
}

TEST(PaxosImplTest, HandlersAbortWhenInvoked) {
  paxos::RpcDispatchRegistry registry;
  paxos::RegisterPaxosHandlers(&registry);

  paxos::RpcRequest prepare_request;
  prepare_request.method_name = paxos::kPaxosPrepareMethod;
  EXPECT_DEATH(registry.Dispatch(prepare_request), "");

  paxos::RpcRequest accept_request;
  accept_request.method_name = paxos::kPaxosAcceptMethod;
  EXPECT_DEATH(registry.Dispatch(accept_request), "");

  paxos::RpcRequest decide_request;
  decide_request.method_name = paxos::kPaxosDecideMethod;
  EXPECT_DEATH(registry.Dispatch(decide_request), "");
}

}  // namespace
