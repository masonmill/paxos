#ifndef PAXOS_INCLUDE_KV_SERVER_H_
#define PAXOS_INCLUDE_KV_SERVER_H_

#include <memory>
#include <string>
#include <vector>

#include "paxos.h"
#include "paxos_rsm.h"
#include "rpc_dispatch.h"
#include "rpc_server.h"

namespace paxos {

// One KV server, replicating its store through Paxos. Its RPC handlers and
// op application are TODOs that abort the process.
class KvServer {
 public:
  // Creates server `me`, serving the KV and Paxos RPCs on `servers[me]`.
  //
  // Params:
  //   servers: socket paths of all servers, including this one.
  //   me: this server's index in `servers`.
  KvServer(std::vector<std::string> servers, int me);

  KvServer(const KvServer&) = delete;
  KvServer& operator=(const KvServer&) = delete;

  ~KvServer();

  // Stops serving and waits for in-flight RPCs. Safe to call more than once.
  void Kill();

  // Sets whether the server drops some requests and replies. For testing.
  //
  // Params:
  //   unreliable: true to start dropping, false to stop.
  void SetUnreliable(bool unreliable);

  // Returns: this server's Paxos peer. For testing.
  Paxos* paxos() { return &paxos_; }

 private:
  RpcReply HandleGet(const RpcRequest& request);
  RpcReply HandlePutAppend(const RpcRequest& request);

  // Applies a decided op to the store.
  void ApplyOp(const std::string& op);

  RpcDispatchRegistry registry_;
  Paxos paxos_;
  PaxosRSM rsm_;
  std::unique_ptr<RpcServer> server_;
};

}  // namespace paxos

#endif  // PAXOS_INCLUDE_KV_SERVER_H_
