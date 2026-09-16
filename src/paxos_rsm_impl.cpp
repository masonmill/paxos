#include <cassert>

#include "paxos_rsm.h"

namespace paxos {

void PaxosRSM::AddOp(const std::string& op) {
  // TODO: agree on `op` in the next free instance, applying decided ops.
  assert(false && "PaxosRSM::AddOp is not implemented");
}

}  // namespace paxos
