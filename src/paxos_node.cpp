#include "paxos_node.h"

#include <cstdio>

#include "paxos_rpcs.h"

namespace paxos {

bool ValidatePeerId(std::size_t peer_count, int id) {
  return id >= 0 && static_cast<std::size_t>(id) < peer_count;
}

void RegisterPaxosStubHandlers(RpcDispatchRegistry* registry) {
  registry->RegisterHandler(kPaxosPrepareMethod, [](const RpcRequest&) {
    std::fprintf(stderr, "%s: not implemented\n", kPaxosPrepareMethod);
    RpcReply reply;
    reply.payload = SerializePayload(PrepareReply());
    return reply;
  });

  registry->RegisterHandler(kPaxosAcceptMethod, [](const RpcRequest&) {
    std::fprintf(stderr, "%s: not implemented\n", kPaxosAcceptMethod);
    RpcReply reply;
    reply.payload = SerializePayload(AcceptReply());
    return reply;
  });

  registry->RegisterHandler(kPaxosDecideMethod, [](const RpcRequest&) {
    std::fprintf(stderr, "%s: not implemented\n", kPaxosDecideMethod);
    RpcReply reply;
    reply.payload = SerializePayload(DecideReply());
    return reply;
  });
}

}  // namespace paxos
