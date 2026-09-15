#ifndef PAXOS_INCLUDE_PAXOS_RSM_H_
#define PAXOS_INCLUDE_PAXOS_RSM_H_

#include <cstdint>
#include <functional>

#include "rpc_dispatch.h"

namespace paxos {

// The state of one Paxos instance, as seen by `PaxosRSM::Status`.
enum class PaxosInstanceState {
  kDecided,
  kPending,
  kForgotten,
};

// Called once an instance's value is decided, so the application can
// apply it. `PaxosRSM` invokes this; this stub type never calls it.
using ApplyOpCallback = std::function<void(int instance_number,
                                           std::uint64_t value)>;

// The direct-call boundary between an RSM layer and the same-process Paxos
// layer. All methods are stubs with no real Paxos logic.
class PaxosRSM {
 public:
  // Wraps the same-process Paxos handle `dispatch_registry` exposes.
  //
  // Params:
  //   dispatch_registry: the node's dispatch registry. Not owned.
  explicit PaxosRSM(RpcDispatchRegistry* dispatch_registry);

  // Stores `callback`, to run once a value is decided. `PaxosRSM` invokes
  // it; this stub type never does.
  //
  // Params:
  //   callback: the function to call on decision.
  void RegisterApplyOpCallback(ApplyOpCallback callback);

  // Starts agreement on `value` for `instance_number`.
  //
  // Params:
  //   instance_number: the Paxos instance to agree on.
  //   value: the value this node proposes.
  void Start(int instance_number, std::uint64_t value);

  // Reports whether `instance_number` is decided.
  //
  // Params:
  //   instance_number: the Paxos instance to check.
  //   value: set to the decided value, if decided.
  //
  // Returns: the instance's state.
  PaxosInstanceState Status(int instance_number, std::uint64_t* value);

  // Tells Paxos that `instance_number` and all earlier instances are no
  // longer needed by this node.
  //
  // Params:
  //   instance_number: the highest instance number this node is done with.
  void Done(int instance_number);

  // Returns: the highest instance number this node has started, or -1 if
  //   none.
  int Max();

 private:
  [[maybe_unused]] RpcDispatchRegistry* dispatch_registry_;
  ApplyOpCallback apply_op_callback_;
};

}  // namespace paxos

#endif  // PAXOS_INCLUDE_PAXOS_RSM_H_
