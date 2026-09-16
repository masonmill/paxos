#include <cassert>

#include "kv_server.h"

namespace paxos {

RpcReply KvServer::HandleGet(const RpcRequest& request) {
  // TODO: add the Get to the log and reply with the key's value.
  assert(false && "KV.Get is not implemented");
  return RpcReply();
}

RpcReply KvServer::HandlePutAppend(const RpcRequest& request) {
  // TODO: add the Put/Append to the log, ignoring duplicate requests.
  assert(false && "KV.PutAppend is not implemented");
  return RpcReply();
}

void KvServer::ApplyOp(const std::string& op) {
  // TODO: decode `op` and apply it to the store.
  assert(false && "KvServer::ApplyOp is not implemented");
}

}  // namespace paxos
