#include "paxos_rsm.h"

#include <utility>

namespace paxos {

PaxosRSM::PaxosRSM(RpcDispatchRegistry* dispatch_registry)
    : dispatch_registry_(dispatch_registry) {}

void PaxosRSM::RegisterApplyOpCallback(ApplyOpCallback callback) {
  apply_op_callback_ = std::move(callback);
}

}  // namespace paxos
