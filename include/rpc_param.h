#ifndef PAXOS_INCLUDE_RPC_PARAM_H_
#define PAXOS_INCLUDE_RPC_PARAM_H_

#include <cstddef>
#include <cstdint>

namespace paxos {

// Maximum length, in bytes, of a method name in a request.
inline constexpr std::size_t kMaxMethodNameLength = 64;

// Maximum length, in bytes, of a payload in a request or reply.
inline constexpr std::size_t kMaxPayloadSize = 4096;

// Status codes carried in a reply.
enum class RpcStatus : std::uint32_t {
  kOk = 0,
  kUnknownMethod = 1,
};

}  // namespace paxos

#endif  // PAXOS_INCLUDE_RPC_PARAM_H_
