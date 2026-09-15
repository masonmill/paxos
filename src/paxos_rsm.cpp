#include "paxos_rsm.h"

#include <utility>

namespace paxos {

PaxosRSM::PaxosRSM(RpcDispatchRegistry* dispatch_registry)
    : dispatch_registry_(dispatch_registry) {}

void PaxosRSM::RegisterApplyOpCallback(ApplyOpCallback callback) {
  apply_op_callback_ = std::move(callback);
}

void PaxosRSM::Start(int, std::uint64_t) {}

PaxosInstanceState PaxosRSM::Status(int, std::uint64_t*) {
  return PaxosInstanceState::kPending;
}

void PaxosRSM::Done(int) {}

int PaxosRSM::Max() { return -1; }

}  // namespace paxos
