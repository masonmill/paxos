#include "cluster.h"

#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <thread>

#include "rpc_transport.h"

namespace paxos::testing {

namespace {

constexpr auto kStartupTimeout = std::chrono::seconds(5);

// Per send or receive.
constexpr timeval kCallTimeout = {.tv_sec = 1, .tv_usec = 0};

[[noreturn]] void Fail(const std::string& message) {
  std::fprintf(stderr, "cluster: %s\n", message.c_str());
  std::abort();
}

// Returns: the child's pid, or -1 if `fork` fails.
pid_t StartNode(const std::string& config_path, std::size_t id) {
  const std::string id_string = std::to_string(id);

  pid_t pid = fork();
  if (pid == 0) {
    execl(PAXOS_NODE_PATH, PAXOS_NODE_PATH, "--config", config_path.c_str(),
          "--id", id_string.c_str(), static_cast<char*>(nullptr));
    std::_Exit(127);
  }

  return pid;
}

bool WaitForSocket(const std::string& socket_path) {
  const auto deadline = std::chrono::steady_clock::now() + kStartupTimeout;

  while (std::chrono::steady_clock::now() < deadline) {
    if (std::filesystem::exists(socket_path)) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return false;
}

// Returns: the connected socket, or -1 on failure.
int ConnectWithTimeout(const std::string& socket_path) {
  sockaddr_un address{};
  if (socket_path.size() >= sizeof(address.sun_path)) {
    return -1;
  }

  int connection_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (connection_fd < 0) {
    return -1;
  }

  address.sun_family = AF_UNIX;
  std::strncpy(address.sun_path, socket_path.c_str(),
               sizeof(address.sun_path) - 1);

  if (setsockopt(connection_fd, SOL_SOCKET, SO_RCVTIMEO, &kCallTimeout,
                 sizeof(kCallTimeout)) != 0 ||
      setsockopt(connection_fd, SOL_SOCKET, SO_SNDTIMEO, &kCallTimeout,
                 sizeof(kCallTimeout)) != 0 ||
      connect(connection_fd, reinterpret_cast<sockaddr*>(&address),
              sizeof(address)) != 0) {
    close(connection_fd);
    return -1;
  }

  return connection_fd;
}

}  // namespace

Cluster::Cluster(std::size_t peer_count) {
  // A short root keeps socket paths within `sun_path`.
  char directory_template[] = "/tmp/paxos-XXXXXX";
  if (mkdtemp(directory_template) == nullptr) {
    Fail("mkdtemp failed");
  }
  directory_ = directory_template;
  config_path_ = directory_ + "/peers.conf";

  std::ofstream config_file(config_path_);
  for (std::size_t id = 0; id < peer_count; ++id) {
    socket_paths_.push_back(directory_ + "/peer" + std::to_string(id) +
                            ".sock");
    config_file << socket_paths_.back() << "\n";
  }
  config_file.close();
  if (!config_file) {
    Fail("failed to write " + config_path_);
  }

  for (std::size_t id = 0; id < peer_count; ++id) {
    pid_t pid = StartNode(config_path_, id);
    if (pid < 0) {
      Shutdown();
      Fail("fork failed");
    }
    pids_.push_back(pid);
  }

  for (const std::string& socket_path : socket_paths_) {
    if (!WaitForSocket(socket_path)) {
      Shutdown();
      Fail("timed out waiting for " + socket_path);
    }
  }
}

Cluster::~Cluster() { Shutdown(); }

void Cluster::Shutdown() {
  for (pid_t pid : pids_) {
    kill(pid, SIGKILL);
  }
  for (pid_t pid : pids_) {
    waitpid(pid, nullptr, 0);
  }
  pids_.clear();

  if (!directory_.empty()) {
    std::filesystem::remove_all(directory_);
    directory_.clear();
  }
}

bool Cluster::Call(std::size_t from, std::size_t to,
                   const std::string& method_name,
                   const std::vector<std::uint8_t>& request_payload,
                   std::vector<std::uint8_t>* reply_payload) {
  bool drop_reply = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (blocked_.contains({from, to}) || ShouldDrop(request_drop_rate_)) {
      return false;
    }
    drop_reply = ShouldDrop(reply_drop_rate_);
  }

  int connection_fd = ConnectWithTimeout(socket_paths_[to]);
  if (connection_fd < 0) {
    return false;
  }

  RpcRequest request;
  request.method_name = method_name;
  request.payload = request_payload;

  bool succeeded = false;
  if (SendRequest(connection_fd, request)) {
    RpcReply reply;
    if (ReceiveReply(connection_fd, &reply) && !drop_reply) {
      *reply_payload = reply.payload;
      succeeded = true;
    }
  }

  close(connection_fd);
  return succeeded;
}

void Cluster::Block(std::size_t from, std::size_t to) {
  std::lock_guard<std::mutex> lock(mutex_);
  blocked_.insert({from, to});
}

void Cluster::Unblock(std::size_t from, std::size_t to) {
  std::lock_guard<std::mutex> lock(mutex_);
  blocked_.erase({from, to});
}

void Cluster::Pause(std::size_t id) { kill(pids_[id], SIGSTOP); }

void Cluster::Resume(std::size_t id) { kill(pids_[id], SIGCONT); }

void Cluster::SetUnreliable(double request_drop_rate,
                            double reply_drop_rate) {
  std::lock_guard<std::mutex> lock(mutex_);
  request_drop_rate_ = request_drop_rate;
  reply_drop_rate_ = reply_drop_rate;
}

bool Cluster::ShouldDrop(double rate) {
  return std::bernoulli_distribution(rate)(random_);
}

}  // namespace paxos::testing
