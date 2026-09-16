#include "kv_server.h"

#include <cassert>

#include "kv_rpcs.h"

namespace paxos {

void RegisterKvHandlers(RpcDispatchRegistry* registry) {
  registry->RegisterHandler(kKvGetMethod, [](const RpcRequest&) {
    // TODO: look up the requested key in the replicated state machine.
    assert(false && "KV.Get is not implemented");
    return RpcReply();
  });

  registry->RegisterHandler(kKvPutAppendMethod, [](const RpcRequest&) {
    // TODO: apply the requested Put/Append to the replicated state machine.
    assert(false && "KV.PutAppend is not implemented");
    return RpcReply();
  });
}

}  // namespace paxos
