cmake_minimum_required(VERSION 3.20)
project(Vision_IA_X32_NX VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VISIONIA_NATIVE_OPTIMIZATION "Enable -march=native for local builds" OFF)

if(MSVC)
  add_compile_options($<$<CONFIG:Release>:/O2>)
else()
  add_compile_options($<$<CONFIG:Release>:-O3> -fno-strict-aliasing)
  if(VISIONIA_NATIVE_OPTIMIZATION)
    add_compile_options($<$<CONFIG:Release>:-march=native>)
  endif()
endif()

find_package(OpenCV REQUIRED COMPONENTS core video imgproc)
find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(GTest REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/x32nx.proto)

add_library(x32nx_core STATIC
  src/rs255223.c
  src/fec_rs.cpp
  src/rvq_vocoder.cpp
  src/multiplexer.cpp
  src/crypto_chacha.cpp
  src/quality_control.cpp
  ${PROTO_SRCS}
  ${PROTO_HDRS}
)

target_include_directories(x32nx_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}
  ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(x32nx_core PUBLIC
  ${OpenCV_LIBS}
  protobuf::libprotobuf
  OpenSSL::Crypto
  Threads::Threads
)

add_executable(vision_ia_node src/main.cpp)
target_link_libraries(vision_ia_node PRIVATE x32nx_core)

enable_testing()

add_executable(run_fec_tests tests/test_fec.cpp)
target_link_libraries(run_fec_tests PRIVATE x32nx_core GTest::gtest GTest::gtest_main)
add_test(NAME ReedSolomonQA COMMAND run_fec_tests)

add_executable(run_component_tests tests/test_components.cpp)
target_link_libraries(run_component_tests PRIVATE x32nx_core GTest::gtest GTest::gtest_main)
add_test(NAME ComponentsQA COMMAND run_component_tests)

install(TARGETS vision_ia_node run_fec_tests run_component_tests RUNTIME DESTINATION bin)
