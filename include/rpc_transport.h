#ifndef PAXOS_INCLUDE_RPC_TRANSPORT_H_
#define PAXOS_INCLUDE_RPC_TRANSPORT_H_

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace paxos {

// Maximum length, in bytes, of a method name in a request.
inline constexpr std::size_t kMaxMethodNameLength = 64;

// Maximum length, in bytes, of a payload in a request or reply.
inline constexpr std::size_t kMaxPayloadSize = 4 * 1024 * 1024;

// Status codes carried in a reply.
enum class RpcStatus : std::uint32_t {
  kOk = 0,
  kUnknownMethod = 1,
};

// A request read from or written to a message stream.
struct RpcRequest {
  std::string method_name;
  std::vector<std::uint8_t> payload;
};

// A reply read from or written to a message stream.
struct RpcReply {
  RpcStatus status = RpcStatus::kOk;
  std::vector<std::uint8_t> payload;
};

// Sends a request on `fd`.
//
// Params:
//   fd: an open, connected socket.
//   request: method name and payload to send.
//
// Returns: true on success, false if the write fails.
bool SendRequest(int fd, const RpcRequest& request);

// Reads one request from `fd`.
//
// Params:
//   fd: an open, connected socket.
//   request: set to the method name and payload read.
//
// Returns: true on success, false if the peer closed the connection or
//   sent a bad message.
bool ReceiveRequest(int fd, RpcRequest* request);

// Sends a reply on `fd`.
//
// Params:
//   fd: an open, connected socket.
//   reply: status and payload to send.
//
// Returns: true on success, false if the write fails.
bool SendReply(int fd, const RpcReply& reply);

// Reads one reply from `fd`.
//
// Params:
//   fd: an open, connected socket.
//   reply: set to the status and payload read.
//
// Returns: true on success, false if the peer closed the connection or
//   sent a bad message.
bool ReceiveReply(int fd, RpcReply* reply);

// Copies `value`'s bytes into a payload. `T` must have a fixed size and
// no pointers, so a byte copy carries its full value.
//
// Params:
//   value: the struct to encode.
//
// Returns: the payload bytes.
template <typename T>
std::vector<std::uint8_t> SerializePayload(const T& value) {
  static_assert(std::is_trivially_copyable_v<T>,
                "T must have a fixed size and no pointers.");
  const auto* bytes = reinterpret_cast<const std::uint8_t*>(&value);
  return std::vector<std::uint8_t>(bytes, bytes + sizeof(T));
}

// Copies a payload's bytes into a struct. `T` must have a fixed size and
// no pointers, so a byte copy carries its full value.
//
// Params:
//   payload: bytes produced by `SerializePayload`.
//   value: set to the decoded struct.
//
// Returns: true on success, false if `payload`'s size does not match
//   `sizeof(T)`.
template <typename T>
bool DeserializePayload(const std::vector<std::uint8_t>& payload, T* value) {
  static_assert(std::is_trivially_copyable_v<T>,
                "T must have a fixed size and no pointers.");
  if (payload.size() != sizeof(T)) {
    return false;
  }
  std::memcpy(value, payload.data(), sizeof(T));
  return true;
}

// Builds a payload from a sequence of fixed-size values and strings.
class PayloadWriter {
 public:
  template <typename T>
  void Write(const T& value) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "T must have a fixed size and no pointers.");
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(&value);
    payload_.insert(payload_.end(), bytes, bytes + sizeof(T));
  }

  // Writes `value` with a length prefix.
  void Write(const std::string& value) {
    Write(static_cast<std::uint64_t>(value.size()));
    payload_.insert(payload_.end(), value.begin(), value.end());
  }

  // Returns: the payload built so far, leaving the writer empty.
  std::vector<std::uint8_t> Take() { return std::move(payload_); }

 private:
  std::vector<std::uint8_t> payload_;
};

// Reads values from a payload in the order a `PayloadWriter` wrote them.
class PayloadReader {
 public:
  // Params:
  //   payload: the bytes to read. Not owned; must outlive the reader.
  explicit PayloadReader(const std::vector<std::uint8_t>& payload)
      : payload_(payload) {}

  // Returns: true on success, false if the payload is too short.
  template <typename T>
  bool Read(T* value) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "T must have a fixed size and no pointers.");
    if (payload_.size() - offset_ < sizeof(T)) {
      return false;
    }
    std::memcpy(value, payload_.data() + offset_, sizeof(T));
    offset_ += sizeof(T);
    return true;
  }

  // Returns: true on success, false if the payload is too short.
  bool Read(std::string* value) {
    std::uint64_t size = 0;
    if (!Read(&size) || payload_.size() - offset_ < size) {
      return false;
    }
    value->assign(payload_.begin() + offset_,
                  payload_.begin() + offset_ + size);
    offset_ += size;
    return true;
  }

  // Returns: true if every byte has been read.
  bool AtEnd() const { return offset_ == payload_.size(); }

 private:
  const std::vector<std::uint8_t>& payload_;
  std::size_t offset_ = 0;
};

}  // namespace paxos

#endif  // PAXOS_INCLUDE_RPC_TRANSPORT_H_
