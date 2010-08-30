
list(APPEND SOURCE framebased.c)

if(ARCH MATCHES i386)
list(APPEND SOURCE
    i386/framebased.S
    i386/framebased-gcchack.c
    i386/framebased-gcchack-asm.S)
elseif(ARCH MATCHES amd64)
list(APPEND SOURCE amd64/framebased.S)
elseif(ARCH MATCHES powerpc)
list(APPEND SOURCE powerpc/framebased.S)
endif(ARCH MATCHES i386)

add_library(pseh ${SOURCE})
add_dependencies(pseh psdk)

