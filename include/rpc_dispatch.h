#ifndef PAXOS_INCLUDE_RPC_DISPATCH_H_
#define PAXOS_INCLUDE_RPC_DISPATCH_H_

#include <functional>
#include <string>
#include <unordered_map>

#include "rpc_transport.h"

namespace paxos {

// Removes any stale file at `socket_path`, then binds and listens a Unix
// domain socket there.
//
// Params:
//   socket_path: filesystem path for the new socket.
//
// Returns: the listening socket, or -1 on failure.
int BindAndListenUnixSocket(const std::string& socket_path);

// A handler for one RPC method name.
using RpcHandler = std::function<RpcReply(const RpcRequest&)>;

// Maps method-name strings to handlers, and dispatches requests to them.
class RpcDispatchRegistry {
 public:
  // Registers `handler` under `method_name`, replacing any earlier
  // handler for that name. `handler` may run on several threads at once.
  void RegisterHandler(const std::string& method_name, RpcHandler handler);

  // Looks up the handler for `request.method_name` and invokes it.
  //
  // Returns: the handler's reply, or a reply with status
  //   `RpcStatus::kUnknownMethod` if no handler is registered for the
  //   name.
  RpcReply Dispatch(const RpcRequest& request) const;

  // Accepts connections on `listen_fd` until the process stops, and
  // serves each one on its own thread.
  //
  // Params:
  //   listen_fd: a bound, listening socket.
  void RunAcceptLoop(int listen_fd) const;

 private:
  std::unordered_map<std::string, RpcHandler> handlers_;
};

}  // namespace paxos

#endif  // PAXOS_INCLUDE_RPC_DISPATCH_H_
