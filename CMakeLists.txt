cmake_minimum_required(VERSION 3.20)
project(paxos CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(rpc
  src/rpc_transport.cpp
  src/rpc_dispatch.cpp
  src/rpc_client.cpp
)
target_include_directories(rpc PUBLIC include)

add_library(paxos_node_lib
  src/peer_config.cpp
  src/paxos_node.cpp
  src/paxos_impl.cpp
  src/kv_server_impl.cpp
)
target_link_libraries(paxos_node_lib PUBLIC rpc)

add_executable(paxos_node
  src/paxos_node_main.cpp
)
target_link_libraries(paxos_node PRIVATE paxos_node_lib)

add_library(paxos_rsm_lib
  src/paxos_rsm.cpp
  src/paxos_rsm_impl.cpp
)
target_link_libraries(paxos_rsm_lib PUBLIC rpc)

add_library(kv_client_lib
  src/kv_client.cpp
)
target_link_libraries(kv_client_lib PUBLIC rpc)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(test_rpc
  tests/test_rpc.cpp
)
target_link_libraries(test_rpc PRIVATE rpc gtest_main)

add_executable(test_paxos_node
  tests/test_paxos_node.cpp
)
target_link_libraries(test_paxos_node PRIVATE paxos_node_lib gtest_main)

add_executable(test_paxos_rsm
  tests/test_paxos_rsm.cpp
)
target_link_libraries(test_paxos_rsm PRIVATE paxos_rsm_lib gtest_main)

add_executable(test_kv_client
  tests/test_kv_client.cpp
)
target_link_libraries(test_kv_client PRIVATE paxos_node_lib kv_client_lib gtest_main)

include(GoogleTest)
gtest_discover_tests(test_rpc)
gtest_discover_tests(test_paxos_node)
gtest_discover_tests(test_paxos_rsm)
gtest_discover_tests(test_kv_client)
