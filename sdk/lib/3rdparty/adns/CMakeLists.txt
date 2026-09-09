
include_directories(
    src
    adns_win32)

add_definitions(
    -D_DLL -D__USE_CRTIMP
    -DADNS_JGAA_WIN32
    -D_CRT_NO_POSIX_ERROR_CODES)

list(APPEND SOURCE
    adns_win32/adns_unix_calls.c
    src/check.c
    src/event.c
    src/general.c
    src/parse.c
    src/poll.c
    src/query.c
    src/reply.c
    src/setup.c
    src/transmit.c
    src/types.c
    src/internal.h)

add_library(adns STATIC ${SOURCE})
add_pch(adns src/internal.h SOURCE)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(adns PRIVATE -Wno-unused-but-set-variable)
endif()

add_dependencies(adns psdk)
target_include_directories(adns
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/adns_win32)
