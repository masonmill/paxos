#ifndef PAXOS_INCLUDE_KV_RPCS_H_
#define PAXOS_INCLUDE_KV_RPCS_H_

#include <cstdint>
#include <string>
#include <vector>

#include "rpc_transport.h"

namespace paxos {

// Method names for the two KV client-facing RPCs.
inline constexpr char kKvGetMethod[] = "KV.Get";
inline constexpr char kKvPutAppendMethod[] = "KV.PutAppend";

// A Get request. `client_id` and `op_id` identify retries of one request.
struct GetArgs {
  std::string key;
  std::int64_t client_id = 0;
  std::int64_t op_id = 0;
};

// A Get reply. `value` is empty for a missing key.
struct GetReply {
  bool ok = false;
  std::string value;
};

// A Put or Append request. `client_id` and `op_id` identify retries of one
// request.
struct PutAppendArgs {
  std::string key;
  std::string value;
  bool is_append = false;
  std::int64_t client_id = 0;
  std::int64_t op_id = 0;
};

// A PutAppend reply.
struct PutAppendReply {
  bool ok = false;
};

inline std::vector<std::uint8_t> SerializePayload(const GetArgs& args) {
  PayloadWriter writer;
  writer.Write(args.key);
  writer.Write(args.client_id);
  writer.Write(args.op_id);
  return writer.Take();
}

inline bool DeserializePayload(const std::vector<std::uint8_t>& payload,
                               GetArgs* args) {
  PayloadReader reader(payload);
  return reader.Read(&args->key) && reader.Read(&args->client_id) &&
         reader.Read(&args->op_id) && reader.AtEnd();
}

inline std::vector<std::uint8_t> SerializePayload(const GetReply& reply) {
  PayloadWriter writer;
  writer.Write(reply.ok);
  writer.Write(reply.value);
  return writer.Take();
}

inline bool DeserializePayload(const std::vector<std::uint8_t>& payload,
                               GetReply* reply) {
  PayloadReader reader(payload);
  return reader.Read(&reply->ok) && reader.Read(&reply->value) &&
         reader.AtEnd();
}

inline std::vector<std::uint8_t> SerializePayload(const PutAppendArgs& args) {
  PayloadWriter writer;
  writer.Write(args.key);
  writer.Write(args.value);
  writer.Write(args.is_append);
  writer.Write(args.client_id);
  writer.Write(args.op_id);
  return writer.Take();
}

inline bool DeserializePayload(const std::vector<std::uint8_t>& payload,
                               PutAppendArgs* args) {
  PayloadReader reader(payload);
  return reader.Read(&args->key) && reader.Read(&args->value) &&
         reader.Read(&args->is_append) && reader.Read(&args->client_id) &&
         reader.Read(&args->op_id) && reader.AtEnd();
}

}  // namespace paxos

#endif  // PAXOS_INCLUDE_KV_RPCS_H_
