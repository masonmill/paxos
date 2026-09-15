#ifndef PAXOS_INCLUDE_RPC_CLIENT_H_
#define PAXOS_INCLUDE_RPC_CLIENT_H_

#include <cstdint>
#include <string>
#include <vector>

namespace paxos {

// Calls a method on the peer listening at `socket_path`.
//
// Params:
//   socket_path: filesystem path of the peer's Unix domain socket.
//   method_name: the RPC method name to call.
//   request_payload: request payload bytes.
//   reply_payload: set to the reply payload bytes on success.
//
// Returns: true if a reply came back, false on a connect failure, a
//   closed connection, or a bad reply. Does not time out.
bool Call(const std::string& socket_path, const std::string& method_name,
          const std::vector<std::uint8_t>& request_payload,
          std::vector<std::uint8_t>* reply_payload);

}  // namespace paxos

#endif  // PAXOS_INCLUDE_RPC_CLIENT_H_
