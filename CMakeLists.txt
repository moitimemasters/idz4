cmake_minimum_required(VERSION 3.10)
project(UDPCommunication)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -DDEBUG_LOGS")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")


include_directories(include)

add_library(udpcommunication
    src/connection_manager.cpp
    src/handshake_manager.cpp
    src/message_parser.cpp
    src/socket_manager.cpp
    src/udp_client.cpp
    src/udp_server.cpp
)

set_target_properties(udpcommunication PROPERTIES LINKER_LANGUAGE CXX)


add_subdirectory(4-5)
add_subdirectory(6-7)
add_subdirectory(8)
