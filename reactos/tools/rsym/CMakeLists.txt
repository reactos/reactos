add_library(rsym_common rsym_common.c)

if(ARCH STREQUAL "i386")
    add_executable(rsym rsym.c)
elseif(ARCH STREQUAL "amd64")
    add_executable(rsym rsym64.c)
endif()
target_link_libraries(rsym rsym_common)
add_executable(raddr2line rsym_common.c raddr2line.c)
