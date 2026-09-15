#ifndef PAXOS_INCLUDE_RPC_TRANSPORT_H_
#define PAXOS_INCLUDE_RPC_TRANSPORT_H_

#include <cstdint>
#include <string>
#include <vector>

#include "rpc_param.h"

namespace paxos {

// A request read from or written to a message stream.
struct RpcRequest {
  std::string method_name;
  std::vector<std::uint8_t> payload;
};

// A reply read from or written to a message stream.
struct RpcReply {
  RpcStatus status = RpcStatus::kOk;
  std::vector<std::uint8_t> payload;
};

// Sends a request on `fd`.
//
// Params:
//   fd: an open, connected socket.
//   request: method name and payload to send.
//
// Returns: true on success, false if the write fails.
bool SendRequest(int fd, const RpcRequest& request);

// Reads one request from `fd`.
//
// Params:
//   fd: an open, connected socket.
//   request: set to the method name and payload read.
//
// Returns: true on success, false if the peer closed the connection or
//   sent a bad message.
bool ReceiveRequest(int fd, RpcRequest* request);

// Sends a reply on `fd`.
//
// Params:
//   fd: an open, connected socket.
//   reply: status and payload to send.
//
// Returns: true on success, false if the write fails.
bool SendReply(int fd, const RpcReply& reply);

// Reads one reply from `fd`.
//
// Params:
//   fd: an open, connected socket.
//   reply: set to the status and payload read.
//
// Returns: true on success, false if the peer closed the connection or
//   sent a bad message.
bool ReceiveReply(int fd, RpcReply* reply);

}  // namespace paxos

#endif  // PAXOS_INCLUDE_RPC_TRANSPORT_H_
