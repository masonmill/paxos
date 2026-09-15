#include "rpc_transport.h"

#include <arpa/inet.h>
#include <unistd.h>

#include <csignal>
#include <cstdint>

namespace paxos {

namespace {

// A write to a peer that already closed its end raises SIGPIPE by
// default, which kills the process. Ignore it once, process-wide, so a
// dead peer surfaces as a normal write failure instead.
struct SigpipeIgnorer {
  SigpipeIgnorer() { std::signal(SIGPIPE, SIG_IGN); }
};
const SigpipeIgnorer kSigpipeIgnorer;

// Writes exactly `length` bytes from `data` to `fd`.
//
// Returns: true on success, false if the socket closes before all bytes
//   are written.
bool WriteExactly(int fd, const void* data, std::size_t length) {
  const auto* bytes = static_cast<const std::uint8_t*>(data);
  std::size_t written = 0;

  while (written < length) {
    ssize_t result = write(fd, bytes + written, length - written);
    if (result <= 0) {
      return false;
    }
    written += static_cast<std::size_t>(result);
  }

  return true;
}

// Reads exactly `length` bytes from `fd` into `data`.
//
// Returns: true on success, false if the socket closes before all bytes
//   are read.
bool ReadExactly(int fd, void* data, std::size_t length) {
  auto* bytes = static_cast<std::uint8_t*>(data);
  std::size_t bytes_read = 0;

  while (bytes_read < length) {
    ssize_t result = read(fd, bytes + bytes_read, length - bytes_read);
    if (result <= 0) {
      return false;
    }
    bytes_read += static_cast<std::size_t>(result);
  }

  return true;
}

// Sends `length` bytes from `data` to `fd`, preceded by a length prefix.
//
// Returns: true on success, false if the write fails.
bool WriteLengthPrefixedBytes(int fd, const void* data, std::size_t length) {
  std::uint32_t length_prefix = htonl(static_cast<std::uint32_t>(length));
  if (!WriteExactly(fd, &length_prefix, sizeof(length_prefix))) {
    return false;
  }
  return WriteExactly(fd, data, length);
}

// Reads a length-prefixed byte string from `fd` into `out`.
//
// Returns: true on success, false on a closed connection or a length
//   prefix over `max_length`.
bool ReadLengthPrefixedBytes(int fd, std::size_t max_length,
                             std::vector<std::uint8_t>* out) {
  std::uint32_t length_prefix = 0;
  if (!ReadExactly(fd, &length_prefix, sizeof(length_prefix))) {
    return false;
  }

  std::size_t length = ntohl(length_prefix);
  if (length > max_length) {
    return false;
  }

  out->resize(length);
  if (length == 0) {
    return true;
  }
  return ReadExactly(fd, out->data(), length);
}

}  // namespace

bool SendRequest(int fd, const RpcRequest& request) {
  if (request.method_name.size() > kMaxMethodNameLength ||
      request.payload.size() > kMaxPayloadSize) {
    return false;
  }

  if (!WriteLengthPrefixedBytes(fd, request.method_name.data(),
                                request.method_name.size())) {
    return false;
  }

  return WriteLengthPrefixedBytes(fd, request.payload.data(),
                                  request.payload.size());
}

bool ReceiveRequest(int fd, RpcRequest* request) {
  std::vector<std::uint8_t> method_name_bytes;
  if (!ReadLengthPrefixedBytes(fd, kMaxMethodNameLength, &method_name_bytes)) {
    return false;
  }

  request->method_name.assign(method_name_bytes.begin(),
                              method_name_bytes.end());

  return ReadLengthPrefixedBytes(fd, kMaxPayloadSize, &request->payload);
}

bool SendReply(int fd, const RpcReply& reply) {
  if (reply.payload.size() > kMaxPayloadSize) {
    return false;
  }

  std::uint32_t status = static_cast<std::uint32_t>(reply.status);
  std::uint32_t status_network_order = htonl(status);
  if (!WriteExactly(fd, &status_network_order, sizeof(status_network_order))) {
    return false;
  }

  return WriteLengthPrefixedBytes(fd, reply.payload.data(),
                                  reply.payload.size());
}

bool ReceiveReply(int fd, RpcReply* reply) {
  std::uint32_t status_network_order = 0;
  if (!ReadExactly(fd, &status_network_order, sizeof(status_network_order))) {
    return false;
  }
  reply->status = static_cast<RpcStatus>(ntohl(status_network_order));

  return ReadLengthPrefixedBytes(fd, kMaxPayloadSize, &reply->payload);
}

}  // namespace paxos
