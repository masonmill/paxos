#include "paxos_impl.h"

#include <cassert>

#include "paxos_rpcs.h"

namespace paxos {

void RegisterPaxosHandlers(RpcDispatchRegistry* registry) {
  registry->RegisterHandler(kPaxosPrepareMethod, [](const RpcRequest&) {
    // TODO: run Paxos phase one (Prepare) for the requested instance.
    assert(false && "Paxos.Prepare is not implemented");
    return RpcReply();
  });

  registry->RegisterHandler(kPaxosAcceptMethod, [](const RpcRequest&) {
    // TODO: run Paxos phase two (Accept) for the requested instance.
    assert(false && "Paxos.Accept is not implemented");
    return RpcReply();
  });

  registry->RegisterHandler(kPaxosDecideMethod, [](const RpcRequest&) {
    // TODO: record the decided value for the requested instance.
    assert(false && "Paxos.Decide is not implemented");
    return RpcReply();
  });
}

}  // namespace paxos
