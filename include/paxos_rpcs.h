#ifndef PAXOS_INCLUDE_PAXOS_RPCS_H_
#define PAXOS_INCLUDE_PAXOS_RPCS_H_

#include <cstdint>

namespace paxos {

// Method names for the three Paxos peer RPCs.
inline constexpr char kPaxosPrepareMethod[] = "Paxos.Prepare";
inline constexpr char kPaxosAcceptMethod[] = "Paxos.Accept";
inline constexpr char kPaxosDecideMethod[] = "Paxos.Decide";

// A Prepare request for one Paxos instance.
struct PrepareArgs {
  std::int64_t instance_number = 0;
  std::uint64_t proposal_number = 0;
};

// A Prepare reply. `accepted_value` is a placeholder for a real value
// type, set by the caller's own Paxos logic.
struct PrepareReply {
  bool ok = false;
  std::uint64_t accepted_proposal_number = 0;
  std::uint64_t accepted_value = 0;
};

// An Accept request for one Paxos instance. `value` is a placeholder for
// a real value type.
struct AcceptArgs {
  std::int64_t instance_number = 0;
  std::uint64_t proposal_number = 0;
  std::uint64_t value = 0;
};

// An Accept reply.
struct AcceptReply {
  bool ok = false;
};

// A Decide request for one Paxos instance. `value` is a placeholder for
// a real value type.
struct DecideArgs {
  std::int64_t instance_number = 0;
  std::uint64_t value = 0;
};

// A Decide reply.
struct DecideReply {
  bool ok = false;
};

}  // namespace paxos

#endif  // PAXOS_INCLUDE_PAXOS_RPCS_H_
