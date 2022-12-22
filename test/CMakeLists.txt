cmake_minimum_required(VERSION 3.1)

project (socketTests)

enable_testing()

# Disable clang-tidy checks for unit test code
set(CMAKE_CXX_CLANG_TIDY "")

add_definitions( -DGTEST_LINKED_AS_SHARED_LIBRARY )

if (BUILD_COVERAGE)
    add_definitions( --coverage )
endif (BUILD_COVERAGE)

include_directories( 
    ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include/sockets-cpp
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/test
)

set ( socketTests_SRC
    main.cpp
    test_AddrLookup.cpp
    test_UdpSocket.cpp
    test_TcpClient.cpp
    test_TcpServer.cpp
)

add_executable ( socketTests ${socketTests_SRC} )
 

target_link_libraries( socketTests fmt::fmt  gtest gmock Threads::Threads )


if (BUILD_COVERAGE)
    target_link_libraries( socketTests gcov )
endif (BUILD_COVERAGE)

add_test (socketTests socketTests )
