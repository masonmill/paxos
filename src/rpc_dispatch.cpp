#include "rpc_dispatch.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>

namespace paxos {

int BindAndListenUnixSocket(const std::string& socket_path) {
  sockaddr_un address{};
  if (socket_path.size() >= sizeof(address.sun_path)) {
    return -1;
  }

  unlink(socket_path.c_str());

  int listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    return -1;
  }

  address.sun_family = AF_UNIX;
  std::strncpy(address.sun_path, socket_path.c_str(),
               sizeof(address.sun_path) - 1);

  if (bind(listen_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) !=
      0) {
    close(listen_fd);
    return -1;
  }

  if (listen(listen_fd, /*backlog=*/16) != 0) {
    close(listen_fd);
    return -1;
  }

  std::fprintf(stderr, "listening on %s\n", socket_path.c_str());
  return listen_fd;
}

void RpcDispatchRegistry::RegisterHandler(const std::string& method_name,
                                          RpcHandler handler) {
  handlers_[method_name] = std::move(handler);
}

RpcReply RpcDispatchRegistry::Dispatch(const RpcRequest& request) const {
  auto handler_entry = handlers_.find(request.method_name);
  if (handler_entry == handlers_.end()) {
    std::fprintf(stderr, "dispatch: unknown method %s\n",
                 request.method_name.c_str());
    RpcReply reply;
    reply.status = RpcStatus::kUnknownMethod;
    return reply;
  }

  std::fprintf(stderr, "dispatch: %s\n", request.method_name.c_str());
  return handler_entry->second(request);
}

}  // namespace paxos
