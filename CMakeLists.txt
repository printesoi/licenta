CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(licenta)

# Load custom modules
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Find required libraries
FIND_PACKAGE(Jansson REQUIRED)

INCLUDE_DIRECTORIES(
    ${JANSSON_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include)

SET(server_SRCS
    server/main.c
    server/hash.c
    server/os.c
    lib/utils.c
    lib/base64_encode.c
    lib/base64_decode.c
    )

IF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(server_SRCS
        ${server_SRCS}
        server/linux.c)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "BSD" OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET(server_SRCS
        ${server_SRCS}
        server/bsd.c)
ELSE()
    MESSAGE(FATAL_ERROR "Unsupported system ${CMAKE_SYSTEM_NAME}")
ENDIF()


SET(client_SRCS
    client/main.c
    client/client.c
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

TARGET_LINK_LIBRARIES(server
    ${JANSSON_LIBRARIES})

TARGET_LINK_LIBRARIES(client
    ${JANSSON_LIBRARIES})
