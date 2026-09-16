#ifndef PAXOS_INCLUDE_KV_RPCS_H_
#define PAXOS_INCLUDE_KV_RPCS_H_

#include <cstdint>

namespace paxos {

// Method names for the two KV client-facing RPCs.
inline constexpr char kKvGetMethod[] = "KV.Get";
inline constexpr char kKvPutAppendMethod[] = "KV.PutAppend";

// A Get request. `key` is a placeholder for a real key type.
struct GetArgs {
  std::uint64_t key = 0;
};

// A Get reply. `value` is a placeholder for a real value type.
struct GetReply {
  bool ok = false;
  std::uint64_t value = 0;
};

// A PutAppend request. `key` and `value` are placeholders for real key
// and value types.
struct PutAppendArgs {
  std::uint64_t key = 0;
  std::uint64_t value = 0;
  bool is_append = false;
};

// A PutAppend reply.
struct PutAppendReply {
  bool ok = false;
};

}  // namespace paxos

#endif  // PAXOS_INCLUDE_KV_RPCS_H_
