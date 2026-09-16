#ifndef PAXOS_INCLUDE_KV_CLIENT_H_
#define PAXOS_INCLUDE_KV_CLIENT_H_

#include <cstddef>
#include <string>
#include <vector>

#include "kv_rpcs.h"

namespace paxos {

// A client for the KV service. Retries the servers round-robin until one
// replies.
class KvClient {
 public:
  // Params:
  //   server_socket_paths: sockets of every KV server to try. Must not
  //     be empty.
  explicit KvClient(std::vector<std::string> server_socket_paths);

  // Calls `KV.Get`, retrying the servers round-robin until one replies.
  //
  // Params:
  //   args: the request.
  //
  // Returns: the reply from whichever server answered.
  GetReply Get(const GetArgs& args);

  // Calls `KV.PutAppend`, retrying the servers round-robin until one
  // replies.
  //
  // Params:
  //   args: the request.
  //
  // Returns: the reply from whichever server answered.
  PutAppendReply PutAppend(const PutAppendArgs& args);

 private:
  std::vector<std::string> server_socket_paths_;
  std::size_t next_server_index_ = 0;
};

}  // namespace paxos

#endif  // PAXOS_INCLUDE_KV_CLIENT_H_
