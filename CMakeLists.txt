cmake_minimum_required(VERSION 3.20)
project(paxos CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(rpc
  src/rpc_transport.cpp
  src/rpc_dispatch.cpp
  src/rpc_client.cpp
  src/rpc_server.cpp
)
target_include_directories(rpc PUBLIC include)

add_library(paxos_lib
  src/paxos.cpp
  src/paxos_impl.cpp
)
target_link_libraries(paxos_lib PUBLIC rpc)

add_library(paxos_rsm_lib
  src/paxos_rsm.cpp
  src/paxos_rsm_impl.cpp
)
target_link_libraries(paxos_rsm_lib PUBLIC paxos_lib)

add_library(kv_server_lib
  src/kv_server.cpp
  src/kv_server_impl.cpp
)
target_link_libraries(kv_server_lib PUBLIC paxos_rsm_lib)

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

add_library(test_support INTERFACE)
target_include_directories(test_support INTERFACE tests/support)

add_executable(test_kv_acceptance
  tests/test_kv_acceptance.cpp
)
target_link_libraries(test_kv_acceptance PRIVATE
  kv_server_lib kv_client_lib test_support gtest_main
)

add_executable(test_paxos_acceptance
  tests/test_paxos_acceptance.cpp
)
target_link_libraries(test_paxos_acceptance PRIVATE
  paxos_lib test_support gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_kv_acceptance)
gtest_discover_tests(test_paxos_acceptance)
