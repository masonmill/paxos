#!/usr/bin/env bash
#
# Starts or stops a local N-node paxos_node cluster for manual testing.
#
# Usage:
#   scripts/run_cluster.sh start [--nodes N] [--scratch-dir DIR]
#   scripts/run_cluster.sh stop  [--scratch-dir DIR]
#
# Manual walkthrough:
#   1. Build the project: cmake --build build
#   2. Start a cluster:   scripts/run_cluster.sh start --nodes 3
#      Each node prints its startup line (id and socket path) to stderr.
#   3. Smoke-test it: write a short program (or reuse tests/test_rpc.cpp's
#      pattern) that calls Paxos.Prepare/Accept/Decide or KV.Get/PutAppend
#      against a printed socket path; the node aborts, since those handlers
#      are TODOs until their real logic is implemented.
#   4. Tear it down:      scripts/run_cluster.sh stop
#      No paxos_node processes should remain (check with `pgrep paxos_node`).
#   5. Start again immediately; it should succeed even though the previous
#      run's socket files are still on disk (paxos_node removes stale ones).

set -euo pipefail

node_count=3
scratch_dir="/tmp/paxos_cluster"
action="${1:-}"
shift || true

while [[ $# -gt 0 ]]; do
  case "$1" in
    --nodes)
      node_count="$2"
      shift 2
      ;;
    --scratch-dir)
      scratch_dir="$2"
      shift 2
      ;;
    *)
      echo "unknown argument: $1" >&2
      exit 1
      ;;
  esac
done

config_path="${scratch_dir}/peers.conf"
pid_file="${scratch_dir}/pids"
paxos_node_binary="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/build/paxos_node"

start_cluster() {
  mkdir -p "${scratch_dir}"

  : > "${config_path}"
  for ((peer_id = 0; peer_id < node_count; ++peer_id)); do
    echo "${scratch_dir}/peer${peer_id}.sock" >> "${config_path}"
  done

  : > "${pid_file}"
  for ((peer_id = 0; peer_id < node_count; ++peer_id)); do
    "${paxos_node_binary}" --config "${config_path}" --id "${peer_id}" &
    echo "$!" >> "${pid_file}"
  done

  echo "started ${node_count} nodes; config: ${config_path}, pids: ${pid_file}"
}

stop_cluster() {
  if [[ ! -f "${pid_file}" ]]; then
    echo "no pid file at ${pid_file}; nothing to stop"
    return
  fi

  while read -r pid; do
    kill "${pid}" 2>/dev/null || true
  done < "${pid_file}"

  rm -f "${pid_file}"
  echo "stopped cluster"
}

case "${action}" in
  start)
    start_cluster
    ;;
  stop)
    stop_cluster
    ;;
  *)
    echo "usage: $0 {start|stop} [--nodes N] [--scratch-dir DIR]" >&2
    exit 1
    ;;
esac
