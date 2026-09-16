#ifndef PAXOS_INCLUDE_RPC_SERVER_H_
#define PAXOS_INCLUDE_RPC_SERVER_H_

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include "rpc_dispatch.h"

namespace paxos {

// Serves a registry's handlers on a Unix socket until killed, each
// connection on its own thread.
class RpcServer {
 public:
  // Binds `socket_path` and starts serving.
  //
  // Params:
  //   registry: the handlers to serve. Not owned; must outlive the server.
  //   socket_path: filesystem path to listen on.
  RpcServer(const RpcDispatchRegistry* registry,
            const std::string& socket_path);

  RpcServer(const RpcServer&) = delete;
  RpcServer& operator=(const RpcServer&) = delete;

  ~RpcServer();

  // Stops serving and waits for in-flight RPCs. Safe to call more than once.
  void Kill();

  // Sets whether the server drops some requests and replies. For testing.
  //
  // Params:
  //   unreliable: true to start dropping, false to stop.
  void SetUnreliable(bool unreliable);

  // Returns: the number of RPCs served. For testing.
  int rpc_count() const { return rpc_count_; }

 private:
  void RunAcceptLoop(int listen_fd);

  const RpcDispatchRegistry* registry_;
  std::atomic<bool> dead_ = false;
  std::atomic<bool> unreliable_ = false;
  std::atomic<int> rpc_count_ = 0;
  std::thread accept_thread_;

  std::mutex connections_mutex_;
  std::condition_variable connections_done_;
  int active_connections_ = 0;
};

}  // namespace paxos

#endif  // PAXOS_INCLUDE_RPC_SERVER_H_
