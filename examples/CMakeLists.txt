cmake_minimum_required(VERSION 3.1)

project(examples)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/sockets-cpp
    .
)

set (clientApp_SRC clientApp.cpp)
add_executable( clientApp ${clientApp_SRC} )
target_link_libraries( clientApp sockets-cpp pthread )

set (serverApp_SRC serverApp.cpp)
add_executable( serverApp ${serverApp_SRC} )
target_link_libraries( serverApp sockets-cpp pthread )

set (mcastApp_SRC mcastApp.cpp)
add_executable( mcastApp ${mcastApp_SRC} )
target_link_libraries( mcastApp sockets-cpp pthread )

set (unicastApp_SRC unicastApp.cpp)
add_executable( unicastApp ${unicastApp_SRC} )
target_link_libraries( unicastApp sockets-cpp pthread )