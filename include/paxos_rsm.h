#ifndef PAXOS_INCLUDE_PAXOS_RSM_H_
#define PAXOS_INCLUDE_PAXOS_RSM_H_

#include <functional>
#include <string>

#include "paxos.h"

namespace paxos {

// Called with each decided op, in log order, so the application can apply
// it.
using ApplyOpCallback = std::function<void(const std::string& op)>;

// A replicated log of ops agreed on through a Paxos peer. `AddOp` is a TODO
// that aborts the process.
class PaxosRSM {
 public:
  // Params:
  //   paxos: this server's Paxos peer. Not owned.
  //   apply_op: called with each decided op.
  PaxosRSM(Paxos* paxos, ApplyOpCallback apply_op);

  // Adds `op` to the log, applying every earlier decided op first. Returns
  // once `op` is decided.
  //
  // Params:
  //   op: the encoded op to add.
  void AddOp(const std::string& op);

 private:
  [[maybe_unused]] Paxos* paxos_;
  ApplyOpCallback apply_op_;
};

}  // namespace paxos

#endif  // PAXOS_INCLUDE_PAXOS_RSM_H_
