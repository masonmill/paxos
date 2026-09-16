#include "cluster.h"

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <thread>

namespace paxos::testing {

namespace {

// How long to wait for a started node to bind its socket.
constexpr auto kStartupTimeout = std::chrono::seconds(5);

// Prints `message` and aborts. The cluster cannot be used after a
// failed setup step, so there is nothing to recover.
[[noreturn]] void Fail(const std::string& message) {
  std::fprintf(stderr, "cluster: %s\n", message.c_str());
  std::abort();
}

// Starts `paxos_node --config <config_path> --id <id>`.
//
// Returns: the child's process id, or -1 if `fork` fails.
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

// Waits until a file exists at `socket_path` or the startup timeout
// passes.
//
// Returns: true if the file appeared in time.
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

}  // namespace

Cluster::Cluster(std::size_t peer_count) {
  // Socket paths must fit in `sockaddr_un::sun_path`, so use a short
  // root under /tmp rather than the system temp directory.
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
  // A node that already crashed is still a zombie until reaped, so
  // `kill` succeeds on it and `waitpid` reaps it either way.
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

}  // namespace paxos::testing
