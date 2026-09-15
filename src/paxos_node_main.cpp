#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "paxos_node.h"
#include "peer_config.h"
#include "rpc_dispatch.h"

namespace {

// Parses `--config <path> --id <n>` from argv.
//
// Returns: true if both flags were given and `--id`'s value is a valid
//   integer.
bool ParseArgs(int argc, char** argv, std::string* config_path, int* id) {
  bool saw_id = false;

  for (int i = 1; i < argc - 1; ++i) {
    if (std::strcmp(argv[i], "--config") == 0) {
      *config_path = argv[i + 1];
    } else if (std::strcmp(argv[i], "--id") == 0) {
      char* end = nullptr;
      *id = static_cast<int>(std::strtol(argv[i + 1], &end, /*base=*/10));
      saw_id = end != argv[i + 1] && *end == '\0';
    }
  }

  return !config_path->empty() && saw_id;
}

}  // namespace

int main(int argc, char** argv) {
  std::string config_path;
  int id = -1;
  if (!ParseArgs(argc, argv, &config_path, &id)) {
    std::fprintf(stderr, "usage: %s --config <path> --id <n>\n", argv[0]);
    return 1;
  }

  std::vector<std::string> peer_socket_paths;
  if (!paxos::ParsePeerConfig(config_path, &peer_socket_paths)) {
    std::fprintf(stderr, "failed to read config file: %s\n",
                 config_path.c_str());
    return 1;
  }

  if (!paxos::ValidatePeerId(peer_socket_paths.size(), id)) {
    std::fprintf(stderr, "--id %d is out of range for %zu peers\n", id,
                 peer_socket_paths.size());
    return 1;
  }

  const std::string& socket_path = peer_socket_paths[id];
  int listen_fd = paxos::BindAndListenUnixSocket(socket_path);
  if (listen_fd < 0) {
    std::fprintf(stderr, "failed to bind socket: %s\n", socket_path.c_str());
    return 1;
  }

  std::fprintf(stderr, "paxos_node %d listening on %s\n", id,
               socket_path.c_str());

  paxos::RpcDispatchRegistry registry;
  paxos::RegisterPaxosStubHandlers(&registry);
  registry.RunAcceptLoop(listen_fd);

  return 0;
}
