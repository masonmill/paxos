#ifndef PAXOS_INCLUDE_PEER_CONFIG_H_
#define PAXOS_INCLUDE_PEER_CONFIG_H_

#include <fstream>
#include <string>
#include <vector>

namespace paxos {

// Reads a peer-list file: one Unix socket path per line, line order
// gives peer index.
//
// Params:
//   config_path: filesystem path of the peer-list file.
//   peer_socket_paths: set to the parsed list, in file order.
//
// Returns: true on success, false if the file cannot be read.
inline bool ParsePeerConfig(const std::string& config_path,
                            std::vector<std::string>* peer_socket_paths) {
  std::ifstream config_file(config_path);
  if (!config_file.is_open()) {
    return false;
  }

  peer_socket_paths->clear();
  std::string line;
  while (std::getline(config_file, line)) {
    peer_socket_paths->push_back(line);
  }

  return true;
}

}  // namespace paxos

#endif  // PAXOS_INCLUDE_PEER_CONFIG_H_
