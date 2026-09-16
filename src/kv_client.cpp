#include "kv_client.h"

#include <utility>

#include "kv_rpcs.h"
#include "rpc_client.h"

namespace paxos {

namespace {

// Calls random servers until one returns a reply that decodes as `Reply`.
template <typename Reply>
void CallUntilReply(const std::vector<std::string>& server_socket_paths,
                    std::mt19937_64& random, const std::string& method_name,
                    const std::vector<std::uint8_t>& request_payload,
                    Reply* reply) {
  while (true) {
    const std::string& server_socket_path =
        server_socket_paths[random() % server_socket_paths.size()];
    std::vector<std::uint8_t> reply_payload;
    if (Call(server_socket_path, method_name, request_payload,
             &reply_payload) &&
        DeserializePayload(reply_payload, reply)) {
      return;
    }
  }
}

}  // namespace

KvClient::KvClient(std::vector<std::string> server_socket_paths)
    : server_socket_paths_(std::move(server_socket_paths)),
      client_id_(static_cast<std::int64_t>(random_())) {}

std::string KvClient::Get(const std::string& key) {
  GetArgs args;
  args.key = key;
  args.client_id = client_id_;
  args.op_id = ++last_op_id_;

  GetReply reply;
  CallUntilReply(server_socket_paths_, random_, kKvGetMethod,
                 SerializePayload(args), &reply);
  return reply.ok ? reply.value : "";
}

void KvClient::Put(const std::string& key, const std::string& value) {
  PutAppend(key, value, /*is_append=*/false);
}

void KvClient::Append(const std::string& key, const std::string& value) {
  PutAppend(key, value, /*is_append=*/true);
}

void KvClient::PutAppend(const std::string& key, const std::string& value,
                         bool is_append) {
  PutAppendArgs args;
  args.key = key;
  args.value = value;
  args.is_append = is_append;
  args.client_id = client_id_;
  args.op_id = ++last_op_id_;

  PutAppendReply reply;
  CallUntilReply(server_socket_paths_, random_, kKvPutAppendMethod,
                 SerializePayload(args), &reply);
}

}  // namespace paxos
