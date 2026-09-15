#include "rpc_client.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>

#include "rpc_transport.h"

namespace paxos {

bool Call(const std::string& socket_path, const std::string& method_name,
          const std::vector<std::uint8_t>& request_payload,
          std::vector<std::uint8_t>* reply_payload) {
  sockaddr_un address{};
  if (socket_path.size() >= sizeof(address.sun_path)) {
    return false;
  }

  int connection_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (connection_fd < 0) {
    return false;
  }

  address.sun_family = AF_UNIX;
  std::strncpy(address.sun_path, socket_path.c_str(),
               sizeof(address.sun_path) - 1);

  if (connect(connection_fd, reinterpret_cast<sockaddr*>(&address),
              sizeof(address)) != 0) {
    close(connection_fd);
    std::fprintf(stderr, "call: %s %s failed: connect refused\n",
                 socket_path.c_str(), method_name.c_str());
    return false;
  }

  RpcRequest request;
  request.method_name = method_name;
  request.payload = request_payload;

  bool succeeded = false;
  if (SendRequest(connection_fd, request)) {
    RpcReply reply;
    if (ReceiveReply(connection_fd, &reply)) {
      *reply_payload = reply.payload;
      succeeded = true;
    }
  }

  close(connection_fd);

  std::fprintf(stderr, "call: %s %s %s\n", socket_path.c_str(),
               method_name.c_str(), succeeded ? "succeeded" : "failed");
  return succeeded;
}

}  // namespace paxos
