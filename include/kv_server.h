#ifndef PAXOS_INCLUDE_KV_SERVER_H_
#define PAXOS_INCLUDE_KV_SERVER_H_

#include "rpc_dispatch.h"

namespace paxos {

// Registers the `KV.Get` and `KV.PutAppend` handlers on `registry`. Each
// handler body is a TODO for the real KV logic; calling one aborts the
// process until that logic exists.
//
// Params:
//   registry: the dispatch registry to register handlers on.
void RegisterKvHandlers(RpcDispatchRegistry* registry);

}  // namespace paxos

#endif  // PAXOS_INCLUDE_KV_SERVER_H_
