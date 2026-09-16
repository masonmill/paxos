#ifndef PAXOS_TESTS_SUPPORT_SOCKET_DIRECTORY_H_
#define PAXOS_TESTS_SUPPORT_SOCKET_DIRECTORY_H_

#include <stdlib.h>

#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "gtest/gtest.h"

namespace paxos::testing {

// A temp directory for one test's sockets, removed on destruction.
class SocketDirectory {
 public:
  SocketDirectory() {
    // A short root keeps socket paths within `sun_path`.
    char directory_template[] = "/tmp/paxos-XXXXXX";
    if (mkdtemp(directory_template) != nullptr) {
      path_ = directory_template;
    }
  }

  SocketDirectory(const SocketDirectory&) = delete;
  SocketDirectory& operator=(const SocketDirectory&) = delete;

  ~SocketDirectory() { std::filesystem::remove_all(path_); }

  // Returns: the socket path peer `id` listens on.
  std::string Port(int id) const { return path_ + "/px-" + std::to_string(id); }

  // Returns: the socket path peer `from` uses to reach peer `to`.
  std::string Link(int from, int to) const {
    return path_ + "/px-" + std::to_string(from) + "-" + std::to_string(to);
  }

  // Returns: the listening ports of `peer_count` peers.
  std::vector<std::string> Ports(int peer_count) const {
    std::vector<std::string> ports;
    for (int i = 0; i < peer_count; ++i) {
      ports.push_back(Port(i));
    }
    return ports;
  }

  // Returns: peer `me`'s view of the peers, reaching others through links.
  std::vector<std::string> PartitionedPorts(int peer_count, int me) const {
    std::vector<std::string> ports;
    for (int j = 0; j < peer_count; ++j) {
      ports.push_back(j == me ? Port(me) : Link(me, j));
    }
    return ports;
  }

 private:
  std::string path_;
};

// Links each group's peers to each other, removing all other links.
//
// Returns: false if a link fails.
inline bool Partition(const SocketDirectory& directory, int peer_count,
                      const std::vector<std::vector<int>>& groups) {
  for (int i = 0; i < peer_count; ++i) {
    for (int j = 0; j < peer_count; ++j) {
      std::filesystem::remove(directory.Link(i, j));
    }
  }

  for (const std::vector<int>& group : groups) {
    for (int i : group) {
      for (int j : group) {
        std::error_code error;
        std::filesystem::create_hard_link(directory.Port(j),
                                          directory.Link(i, j), error);
        if (error) {
          ADD_FAILURE() << "link " << directory.Link(i, j) << ": "
                        << error.message();
          return false;
        }
      }
    }
  }
  return true;
}

}  // namespace paxos::testing

#endif  // PAXOS_TESTS_SUPPORT_SOCKET_DIRECTORY_H_
