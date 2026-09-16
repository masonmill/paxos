#include "paxos.h"

#include <utility>

#include "paxos_rpcs.h"

namespace paxos {

Paxos::Paxos(std::vector<std::string> peers, int me,
             RpcDispatchRegistry* registry)
    : peers_(std::move(peers)), me_(me) {
  if (registry != nullptr) {
    RegisterHandlers(registry);
    return;
  }

  RegisterHandlers(&own_registry_);
  server_ = std::make_unique<RpcServer>(&own_registry_, peers_[me_]);
}

Paxos::~Paxos() { Kill(); }

void Paxos::Kill() {
  dead_ = true;
  if (server_) {
    server_->Kill();
  }
}

void Paxos::SetUnreliable(bool unreliable) {
  if (server_) {
    server_->SetUnreliable(unreliable);
  }
}

void Paxos::RegisterHandlers(RpcDispatchRegistry* registry) {
  registry->RegisterHandler(kPaxosPrepareMethod,
                            [this](const RpcRequest& request) {
                              return HandlePrepare(request);
                            });
  registry->RegisterHandler(kPaxosAcceptMethod,
                            [this](const RpcRequest& request) {
                              return HandleAccept(request);
                            });
  registry->RegisterHandler(kPaxosDecideMethod,
                            [this](const RpcRequest& request) {
                              return HandleDecide(request);
                            });
}

}  // namespace paxos
