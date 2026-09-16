#ifndef PAXOS_INCLUDE_PAXOS_NODE_H_
#define PAXOS_INCLUDE_PAXOS_NODE_H_

#include <cstddef>

namespace paxos {

// Checks whether `id` is a valid peer index for a config with
// `peer_count` entries.
//
// Params:
//   peer_count: number of peers in the parsed config.
//   id: the `--id` value to check.
//
// Returns: true if `id` is in range.
bool ValidatePeerId(std::size_t peer_count, int id);

}  // namespace paxos

#endif  // PAXOS_INCLUDE_PAXOS_NODE_H_
