#include "rpc_server.h"

#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <random>

namespace paxos {

namespace {

constexpr int kAcceptPollMilliseconds = 100;

// Returns: true with probability `rate`.
bool ShouldDrop(double rate) {
  thread_local std::mt19937 random{std::random_device{}()};
  return std::bernoulli_distribution(rate)(random);
}

}  // namespace

RpcServer::RpcServer(const RpcDispatchRegistry* registry,
                     const std::string& socket_path)
    : registry_(registry) {
  int listen_fd = BindAndListenUnixSocket(socket_path);
  if (listen_fd < 0) {
    std::fprintf(stderr, "rpc_server: failed to bind %s\n",
                 socket_path.c_str());
    return;
  }
  accept_thread_ = std::thread(&RpcServer::RunAcceptLoop, this, listen_fd);
}

RpcServer::~RpcServer() { Kill(); }

void RpcServer::Kill() {
  dead_ = true;
  if (accept_thread_.joinable()) {
    accept_thread_.join();
  }

  std::unique_lock<std::mutex> lock(connections_mutex_);
  connections_done_.wait(lock, [this] { return active_connections_ == 0; });
}

void RpcServer::SetUnreliable(bool unreliable) { unreliable_ = unreliable; }

void RpcServer::RunAcceptLoop(int listen_fd) {
  pollfd listen_poll = {.fd = listen_fd, .events = POLLIN, .revents = 0};

  while (!dead_) {
    // Wakes periodically to notice `Kill`.
    if (poll(&listen_poll, 1, kAcceptPollMilliseconds) <= 0) {
      continue;
    }
    int connection_fd = accept(listen_fd, nullptr, nullptr);
    if (connection_fd < 0) {
      continue;
    }
    if (dead_) {
      close(connection_fd);
      break;
    }

    if (unreliable_ && ShouldDrop(0.1)) {
      close(connection_fd);
      continue;
    }
    bool drop_reply = unreliable_ && ShouldDrop(0.2);
    ++rpc_count_;
    {
      std::lock_guard<std::mutex> lock(connections_mutex_);
      ++active_connections_;
    }

    std::thread([this, connection_fd, drop_reply] {
      RpcRequest request;
      if (ReceiveRequest(connection_fd, &request)) {
        RpcReply reply = registry_->Dispatch(request);
        if (!drop_reply) {
          SendReply(connection_fd, reply);
        }
      }
      close(connection_fd);

      std::lock_guard<std::mutex> lock(connections_mutex_);
      --active_connections_;
      connections_done_.notify_all();
    }).detach();
  }

  close(listen_fd);
}

}  // namespace paxos
