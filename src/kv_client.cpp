#include "kv_client.h"

#include "rpc_client.h"
#include "rpc_transport.h"

namespace paxos {

KvClient::KvClient(std::vector<std::string> server_socket_paths)
    : server_socket_paths_(std::move(server_socket_paths)) {}

namespace {

// Sends `request_payload` to the servers round-robin, starting at
// `*next_server_index`, until one returns a reply that decodes as a
// `Reply`.
template <typename Reply>
Reply CallServersRoundRobinUntilSuccess(
    const std::vector<std::string>& server_socket_paths,
    std::size_t* next_server_index, const std::string& method_name,
    const std::vector<std::uint8_t>& request_payload) {
  while (true) {
    const std::string& server_socket_path =
        server_socket_paths[*next_server_index];
    *next_server_index = (*next_server_index + 1) % server_socket_paths.size();

    std::vector<std::uint8_t> reply_payload;
    Reply reply;
    if (Call(server_socket_path, method_name, request_payload,
             &reply_payload) &&
        DeserializePayload(reply_payload, &reply)) {
      return reply;
    }
  }
}

}  // namespace

GetReply KvClient::Get(const GetArgs& args) {
  return CallServersRoundRobinUntilSuccess<GetReply>(
      server_socket_paths_, &next_server_index_, kKvGetMethod,
      SerializePayload(args));
}

PutAppendReply KvClient::PutAppend(const PutAppendArgs& args) {
  return CallServersRoundRobinUntilSuccess<PutAppendReply>(
      server_socket_paths_, &next_server_index_, kKvPutAppendMethod,
      SerializePayload(args));
}

}  // namespace paxos
