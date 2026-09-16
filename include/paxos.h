#ifndef PAXOS_INCLUDE_PAXOS_H_
#define PAXOS_INCLUDE_PAXOS_H_

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "rpc_dispatch.h"
#include "rpc_server.h"

namespace paxos {

// The state of one Paxos instance, as seen by `Paxos::Status`.
enum class Fate {
  kDecided,
  kPending,
  kForgotten,
};

// One Paxos peer, agreeing with the others on a sequence of values. Its
// operations and RPC handlers are TODOs that abort the process.
class Paxos {
 public:
  // Creates peer `me`, serving the Paxos RPCs on `registry` if given, or
  // otherwise on its own socket at `peers[me]`.
  //
  // Params:
  //   peers: socket paths of all peers, including this one.
  //   me: this peer's index in `peers`.
  //   registry: the dispatch registry to register handlers on, or nullptr.
  //     Not owned.
  Paxos(std::vector<std::string> peers, int me, RpcDispatchRegistry* registry);

  Paxos(const Paxos&) = delete;
  Paxos& operator=(const Paxos&) = delete;

  ~Paxos();

  // Starts agreement on `value` for instance `seq`, and returns without
  // waiting for a decision.
  //
  // Params:
  //   seq: the instance to agree on.
  //   value: the value this peer proposes.
  void Start(int seq, const std::string& value);

  // Reports this peer's view of instance `seq`.
  //
  // Params:
  //   seq: the instance to check.
  //   value: set to the decided value, if decided.
  //
  // Returns: the instance's fate.
  Fate Status(int seq, std::string* value);

  // Tells Paxos that this peer no longer needs instances <= `seq`.
  //
  // Params:
  //   seq: the highest instance this peer is done with.
  void Done(int seq);

  // Returns: the highest instance this peer knows of, or -1 if none.
  int Max();

  // Returns: one more than the highest instance every peer is done with.
  //   Instances below it are forgotten.
  int Min();

  // Stops serving on this peer's own socket and waits for in-flight RPCs.
  // Safe to call more than once.
  void Kill();

  // Sets whether this peer's own socket drops some requests and replies.
  // No-op on a shared registry. For testing.
  //
  // Params:
  //   unreliable: true to start dropping, false to stop.
  void SetUnreliable(bool unreliable);

  // Returns: the number of RPCs served on this peer's own socket, or 0 on a
  //   shared registry. For testing.
  int rpc_count() const { return server_ ? server_->rpc_count() : 0; }

 private:
  void RegisterHandlers(RpcDispatchRegistry* registry);

  RpcReply HandlePrepare(const RpcRequest& request);
  RpcReply HandleAccept(const RpcRequest& request);
  RpcReply HandleDecide(const RpcRequest& request);

  std::vector<std::string> peers_;
  int me_;

  std::atomic<bool> dead_ = false;
  RpcDispatchRegistry own_registry_;
  std::unique_ptr<RpcServer> server_;
};

}  // namespace paxos

#endif  // PAXOS_INCLUDE_PAXOS_H_
