#include "paxos_node.h"

namespace paxos {

bool ValidatePeerId(std::size_t peer_count, int id) {
  return id >= 0 && static_cast<std::size_t>(id) < peer_count;
}

}  // namespace paxos
