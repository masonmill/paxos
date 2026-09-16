#include "kv_server.h"

#include "kv_rpcs.h"

namespace paxos {

KvServer::KvServer(std::vector<std::string> servers, int me)
    : paxos_(servers, me, &registry_),
      rsm_(&paxos_, [this](const std::string& op) { ApplyOp(op); }) {
  registry_.RegisterHandler(kKvGetMethod, [this](const RpcRequest& request) {
    return HandleGet(request);
  });
  registry_.RegisterHandler(kKvPutAppendMethod,
                            [this](const RpcRequest& request) {
                              return HandlePutAppend(request);
                            });
  server_ = std::make_unique<RpcServer>(&registry_, servers[me]);
}

KvServer::~KvServer() { Kill(); }

void KvServer::Kill() {
  server_->Kill();
  paxos_.Kill();
}

void KvServer::SetUnreliable(bool unreliable) {
  server_->SetUnreliable(unreliable);
}

}  // namespace paxos
