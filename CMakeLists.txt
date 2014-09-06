CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(licenta)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

SET(server_SRCS
    server/main.c
    lib/utils.c
    lib/base64_encode.c
    lib/base64_decode.c
    )

SET(client_SRCS
    client/main.c
    lib/utils.c
    lib/parser.c
    lib/base64_encode.c
    lib/base64_decode.c
    )

FOREACH(FL IN LISTS server_SRCS client_SRCS)
    GET_FILENAME_COMPONENT(FC ${FL} NAME)
    SET_SOURCE_FILES_PROPERTIES(${FL} PROPERTIES COMPILE_DEFINITIONS "__FILENAME__=\"${FC}\"")
ENDFOREACH()

ADD_EXECUTABLE(server ${server_SRCS})
ADD_EXECUTABLE(client ${client_SRCS})
