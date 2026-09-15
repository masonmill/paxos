#include "peer_config.h"

#include <fstream>
#include <string>

namespace paxos {

bool ParsePeerConfig(const std::string& config_path,
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
