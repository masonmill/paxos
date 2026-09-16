#ifndef PAXOS_TESTS_SUPPORT_CLUSTER_H_
#define PAXOS_TESTS_SUPPORT_CLUSTER_H_

#include <sys/types.h>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace paxos::testing {

// `paxos_node` processes for one test, killed on destruction. Blocking and
// drops affect only `Call`; pausing also silences node-to-node traffic.
class Cluster {
 public:
  // Starts `peer_count` nodes and waits for their sockets. Aborts if a node
  // fails to start.
  //
  // Params:
  //   peer_count: number of nodes to start.
  explicit Cluster(std::size_t peer_count);

  Cluster(const Cluster&) = delete;
  Cluster& operator=(const Cluster&) = delete;

  ~Cluster();

  // Returns: the number of nodes.
  std::size_t peer_count() const { return socket_paths_.size(); }

  // Returns: the socket path of peer `id`.
  const std::string& socket_path(std::size_t id) const {
    return socket_paths_[id];
  }

  // Returns: the process id of peer `id`.
  pid_t pid(std::size_t id) const { return pids_[id]; }

  // Kills and reaps every node and removes the temp directory. Safe to call
  // more than once.
  void Shutdown();

  // Calls peer `to` on behalf of peer `from`, subject to blocking and drops.
  // Times out, so a call to a paused peer fails.
  //
  // Params:
  //   from: peer the call is made for.
  //   to: peer to call.
  //   method_name: the RPC method name to call.
  //   request_payload: request payload bytes.
  //   reply_payload: set to the reply payload bytes on success.
  //
  // Returns: true if a reply came back, false if the call was blocked,
  //   dropped, timed out, or failed.
  bool Call(std::size_t from, std::size_t to, const std::string& method_name,
            const std::vector<std::uint8_t>& request_payload,
            std::vector<std::uint8_t>* reply_payload);

  // Makes `Call` from `from` to `to` fail without connecting.
  //
  // Params:
  //   from: calling peer.
  //   to: called peer.
  void Block(std::size_t from, std::size_t to);

  // Undoes `Block`.
  //
  // Params:
  //   from: calling peer.
  //   to: called peer.
  void Unblock(std::size_t from, std::size_t to);

  // Stops peer `id`'s process with `SIGSTOP`.
  //
  // Params:
  //   id: peer to pause.
  void Pause(std::size_t id);

  // Continues peer `id`'s process with `SIGCONT`.
  //
  // Params:
  //   id: peer to resume.
  void Resume(std::size_t id);

  // Sets the drop rates for `Call`. A dropped reply is discarded after the
  // peer handled the call.
  //
  // Params:
  //   request_drop_rate: fraction of calls dropped before sending.
  //   reply_drop_rate: fraction of sent calls whose reply is dropped.
  void SetUnreliable(double request_drop_rate, double reply_drop_rate);

 private:
  // Requires `mutex_` held.
  //
  // Returns: true with probability `rate`.
  bool ShouldDrop(double rate);

  std::string directory_;
  std::string config_path_;
  std::vector<std::string> socket_paths_;
  std::vector<pid_t> pids_;

  std::mutex mutex_;
  std::set<std::pair<std::size_t, std::size_t>> blocked_;
  double request_drop_rate_ = 0;
  double reply_drop_rate_ = 0;
  std::mt19937 random_{std::random_device{}()};
};

}  // namespace paxos::testing

#endif  // PAXOS_TESTS_SUPPORT_CLUSTER_H_
