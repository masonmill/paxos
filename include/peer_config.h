#ifndef PAXOS_INCLUDE_PEER_CONFIG_H_
#define PAXOS_INCLUDE_PEER_CONFIG_H_

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
bool ParsePeerConfig(const std::string& config_path,
                     std::vector<std::string>* peer_socket_paths);

}  // namespace paxos

#endif  // PAXOS_INCLUDE_PEER_CONFIG_H_
