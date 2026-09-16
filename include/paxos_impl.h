#ifndef PAXOS_INCLUDE_PAXOS_IMPL_H_
#define PAXOS_INCLUDE_PAXOS_IMPL_H_

#include "rpc_dispatch.h"

namespace paxos {

// Registers the `Paxos.Prepare`, `Paxos.Accept`, and `Paxos.Decide`
// handlers on `registry`. Each handler body is a TODO for the real Paxos
// logic; calling one aborts the process until that logic exists.
//
// Params:
//   registry: the dispatch registry to register handlers on.
void RegisterPaxosHandlers(RpcDispatchRegistry* registry);

}  // namespace paxos

#endif  // PAXOS_INCLUDE_PAXOS_IMPL_H_
