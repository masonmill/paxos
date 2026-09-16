#include "paxos_rsm.h"

#include <utility>

namespace paxos {

PaxosRSM::PaxosRSM(Paxos* paxos, ApplyOpCallback apply_op)
    : paxos_(paxos), apply_op_(std::move(apply_op)) {}

}  // namespace paxos
