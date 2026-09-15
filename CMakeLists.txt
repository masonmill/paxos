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
)
target_link_libraries(paxos_node_lib PUBLIC rpc)

add_executable(paxos_node
  src/paxos_node_main.cpp
)
target_link_libraries(paxos_node PRIVATE paxos_node_lib)

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

include(GoogleTest)
gtest_discover_tests(test_rpc)
gtest_discover_tests(test_paxos_node)
