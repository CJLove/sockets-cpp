cmake_minimum_required(VERSION 3.1)

project(sockets-cpp)

include_directories( 
    ../include 
    .
)

if (BUILD_COVERAGE)
    add_definitions( --coverage )
endif (BUILD_COVERAGE)

set (sockets-cpp_SRC
    TcpClient.cpp
    TcpServer.cpp
    UdpMcast.cpp
)

if(BUILD_SHARED_LIBS)
    add_library( sockets-cpp SHARED ${sockets-cpp_SRC} )
endif(BUILD_SHARED_LIBS)

if(BUILD_STATIC_LIBS)
    add_library( sockets-cpp ${sockets-cpp_SRC} )
endif(BUILD_STATIC_LIBS)

target_link_libraries( sockets-cpp pthread rt )

if (BUILD_COVERAGE)
    target_link_libraries( sockets-cpp gcov )
endif (BUILD_COVERAGE)

if (BUILD_SHARED_LIBS)
    install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)
endif(BUILD_SHARED_LIBS)
