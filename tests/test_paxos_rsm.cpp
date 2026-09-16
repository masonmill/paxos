#include <cstdint>

#include "gtest/gtest.h"
#include "paxos_rsm.h"
#include "rpc_dispatch.h"

namespace {

TEST(PaxosRsmTest, MethodsAbortWhenCalled) {
  paxos::RpcDispatchRegistry registry;
  paxos::PaxosRSM rsm(&registry);

  EXPECT_DEATH(rsm.Start(/*instance_number=*/0, /*value=*/42), "");

  std::uint64_t decided_value = 0;
  EXPECT_DEATH(rsm.Status(/*instance_number=*/0, &decided_value), "");

  EXPECT_DEATH(rsm.Done(/*instance_number=*/0), "");

  EXPECT_DEATH(rsm.Max(), "");
}

TEST(PaxosRsmTest, RegistersApplyOpCallbackWithoutInvokingIt) {
  paxos::RpcDispatchRegistry registry;
  paxos::PaxosRSM rsm(&registry);

  bool callback_invoked = false;
  rsm.RegisterApplyOpCallback(
      [&callback_invoked](int, std::uint64_t) { callback_invoked = true; });

  EXPECT_FALSE(callback_invoked);
}

}  // namespace
