#include <cstdint>

#include "gtest/gtest.h"
#include "paxos_rsm.h"
#include "rpc_dispatch.h"

namespace {

TEST(PaxosRsmTest, StubMethodsDoNotCrash) {
  paxos::RpcDispatchRegistry registry;
  paxos::PaxosRSM rsm(&registry);

  rsm.Start(/*instance_number=*/0, /*value=*/42);

  std::uint64_t decided_value = 0;
  rsm.Status(/*instance_number=*/0, &decided_value);

  rsm.Done(/*instance_number=*/0);

  rsm.Max();
}

TEST(PaxosRsmTest, RegistersApplyOpCallbackWithoutInvokingIt) {
  paxos::RpcDispatchRegistry registry;
  paxos::PaxosRSM rsm(&registry);

  bool callback_invoked = false;
  rsm.RegisterApplyOpCallback(
      [&callback_invoked](int, std::uint64_t) { callback_invoked = true; });

  rsm.Start(/*instance_number=*/0, /*value=*/42);
  rsm.Done(/*instance_number=*/0);

  EXPECT_FALSE(callback_invoked);
}

}  // namespace
