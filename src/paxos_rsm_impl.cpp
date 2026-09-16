#include <cassert>

#include "paxos_rsm.h"

namespace paxos {

void PaxosRSM::Start(int instance_number, std::uint64_t value) {
  // TODO: propose `value` for `instance_number` and drive Paxos agreement.
  assert(false && "PaxosRSM::Start is not implemented");
}

PaxosInstanceState PaxosRSM::Status(int instance_number,
                                     std::uint64_t* value) {
  // TODO: report the local Paxos state for `instance_number`.
  assert(false && "PaxosRSM::Status is not implemented");
  return PaxosInstanceState::kPending;
}

void PaxosRSM::Done(int instance_number) {
  // TODO: tell Paxos this node no longer needs instances <= `instance_number`.
  assert(false && "PaxosRSM::Done is not implemented");
}

int PaxosRSM::Max() {
  // TODO: report the highest instance number this node has started.
  assert(false && "PaxosRSM::Max is not implemented");
  return -1;
}

}  // namespace paxos
