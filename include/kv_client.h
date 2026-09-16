#ifndef PAXOS_INCLUDE_KV_CLIENT_H_
#define PAXOS_INCLUDE_KV_CLIENT_H_

#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace paxos {

// A client for the KV service. Retries random servers until one replies.
// Not thread-safe.
class KvClient {
 public:
  // Params:
  //   server_socket_paths: sockets of every KV server to try. Must not
  //     be empty.
  explicit KvClient(std::vector<std::string> server_socket_paths);

  // Returns: the current value for `key`, or "" if it does not exist.
  std::string Get(const std::string& key);

  // Sets `key` to `value`.
  void Put(const std::string& key, const std::string& value);

  // Appends `value` to `key`'s current value.
  void Append(const std::string& key, const std::string& value);

 private:
  void PutAppend(const std::string& key, const std::string& value,
                 bool is_append);

  std::vector<std::string> server_socket_paths_;
  std::mt19937_64 random_{std::random_device{}()};
  std::int64_t client_id_;
  std::int64_t last_op_id_ = -1;
};

}  // namespace paxos

#endif  // PAXOS_INCLUDE_KV_CLIENT_H_
