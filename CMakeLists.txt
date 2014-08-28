CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(licenta)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

SET(server_SRCS
    server/main.c
    )

SET(client_SRCS
    client/main.c
    )

ADD_EXECUTABLE(server ${server_SRCS})
ADD_EXECUTABLE(client ${client_SRCS})
