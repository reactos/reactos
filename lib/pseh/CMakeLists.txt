
if(NOT MSVC)

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
    endif()

    add_library(pseh ${SOURCE})
    allow_warnings(pseh)
    target_link_libraries(pseh chkstk)
    add_dependencies(pseh psdk)

else()

    if(ARCH MATCHES i386)
        list(APPEND SOURCE
            dummy.c
            i386/seh.s
            i386/seh_prolog.s)
    elseif(ARCH MATCHES amd64)
        list(APPEND SOURCE
            dummy.c
            amd64/seh.s
            amd64/seh_prolog.s)
    endif()

    add_library(pseh ${SOURCE})
    add_dependencies(pseh asm)

endif()
