cmake_minimum_required(VERSION 3.17)

project(examples)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/sockets-cpp
    .
)

set (getopt_SRC
)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows" )
    set(getopt_SRC getopt.cpp)
endif()

set (clientApp_SRC clientApp.cpp)
add_executable( clientApp ${clientApp_SRC} ${getopt_SRC} )
if (FMT_SUPPORT)
    target_link_libraries( clientApp PUBLIC fmt::fmt )
endif(FMT_SUPPORT)
target_link_libraries( clientApp PUBLIC Threads::Threads )

set (serverApp_SRC serverApp.cpp)
add_executable( serverApp ${serverApp_SRC} ${getopt_SRC} )
if (FMT_SUPPORT)
    target_link_libraries( serverApp PUBLIC fmt::fmt )
endif(FMT_SUPPORT)
target_link_libraries( serverApp PUBLIC Threads::Threads )

set (mcastApp_SRC mcastApp.cpp)
add_executable( mcastApp ${mcastApp_SRC} ${getopt_SRC} )
if (FMT_SUPPORT)
    target_link_libraries( mcastApp PUBLIC fmt::fmt )
endif(FMT_SUPPORT)
target_link_libraries( mcastApp PUBLIC Threads::Threads )

set (unicastApp_SRC unicastApp.cpp)
add_executable( unicastApp ${unicastApp_SRC} ${getopt_SRC} )
if (FMT_SUPPORT)
    target_link_libraries( unicastApp PUBLIC fmt::fmt )
endif(FMT_SUPPORT)
target_link_libraries( unicastApp PUBLIC Threads::Threads )