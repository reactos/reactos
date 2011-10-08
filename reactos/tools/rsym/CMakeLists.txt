
if(ARCH MATCHES i386)
    add_executable(rsym rsym_common.c rsym.c)
elseif(ARCH MATCHES amd64)
    add_executable(rsym rsym_common.c rsym64.c)
endif()
add_executable(raddr2line rsym_common.c raddr2line.c)
