#ifndef PAXOS_TESTS_SUPPORT_CLUSTER_H_
#define PAXOS_TESTS_SUPPORT_CLUSTER_H_

#include <sys/types.h>

#include <cstddef>
#include <string>
#include <vector>

namespace paxos::testing {

// A cluster of `paxos_node` processes started for one test. Each cluster
// gets its own temp directory holding `peers.conf` and every peer's
// socket, so clusters in separate tests never collide. Destroying the
// cluster kills and reaps every process it started.
class Cluster {
 public:
  // Starts `peer_count` nodes and waits for each one's socket to appear.
  // Aborts the test process if a node cannot be started.
  //
  // Params:
  //   peer_count: number of nodes to start.
  explicit Cluster(std::size_t peer_count);

  Cluster(const Cluster&) = delete;
  Cluster& operator=(const Cluster&) = delete;

  ~Cluster();

  // Returns: the number of nodes in the cluster.
  std::size_t peer_count() const { return socket_paths_.size(); }

  // Returns: the socket path of peer `id`.
  const std::string& socket_path(std::size_t id) const {
    return socket_paths_[id];
  }

  // Returns: the process id of peer `id`.
  pid_t pid(std::size_t id) const { return pids_[id]; }

  // Kills and reaps every node, then removes the cluster's temp
  // directory. Safe to call more than once.
  void Shutdown();

 private:
  std::string directory_;
  std::string config_path_;
  std::vector<std::string> socket_paths_;
  std::vector<pid_t> pids_;
};

}  // namespace paxos::testing

#endif  // PAXOS_TESTS_SUPPORT_CLUSTER_H_
