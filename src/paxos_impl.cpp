#include <cassert>

#include "paxos.h"

namespace paxos {

void Paxos::Start(int seq, const std::string& value) {
  // TODO: propose `value` for `seq` and drive Paxos agreement.
  assert(false && "Paxos::Start is not implemented");
}

Fate Paxos::Status(int seq, std::string* value) {
  // TODO: report the local Paxos state for `seq`.
  assert(false && "Paxos::Status is not implemented");
  return Fate::kPending;
}

void Paxos::Done(int seq) {
  // TODO: record that this peer no longer needs instances <= `seq`.
  assert(false && "Paxos::Done is not implemented");
}

int Paxos::Max() {
  // TODO: report the highest instance this peer knows of.
  assert(false && "Paxos::Max is not implemented");
  return -1;
}

int Paxos::Min() {
  // TODO: report one more than the highest instance all peers are done with.
  assert(false && "Paxos::Min is not implemented");
  return 0;
}

RpcReply Paxos::HandlePrepare(const RpcRequest& request) {
  // TODO: run Paxos phase one (Prepare) for the requested instance.
  assert(false && "Paxos.Prepare is not implemented");
  return RpcReply();
}

RpcReply Paxos::HandleAccept(const RpcRequest& request) {
  // TODO: run Paxos phase two (Accept) for the requested instance.
  assert(false && "Paxos.Accept is not implemented");
  return RpcReply();
}

RpcReply Paxos::HandleDecide(const RpcRequest& request) {
  // TODO: record the decided value for the requested instance.
  assert(false && "Paxos.Decide is not implemented");
  return RpcReply();
}

}  // namespace paxos
