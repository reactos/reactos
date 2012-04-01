
include_directories(include)

add_definitions(-D_CRTBLD)

if(ARCH MATCHES i386)
    list(APPEND CHKSTK_SOURCE except/i386/chkstk_asm.s)
    if(NOT MSVC)
        list(APPEND CHKSTK_SOURCE except/i386/chkstk_ms.s)
    endif()
elseif(ARCH MATCHES amd64)
    list(APPEND CHKSTK_SOURCE except/amd64/chkstk_asm.s)
    if(NOT MSVC)
        list(APPEND CHKSTK_SOURCE except/amd64/chkstk_ms.s)
    endif()
elseif(ARCH MATCHES powerpc)
    list(APPEND CHKSTK_SOURCE except/powerpc/chkstk_asm.s)
endif()

add_library(chkstk ${CHKSTK_SOURCE})
add_dependencies(chkstk asm)

include(crt.cmake)
include(libcntpr.cmake)
include(msvcrtex.cmake)
include(oldnames.cmake)

add_library(user32_wsprintf
    printf/streamout.c
    printf/wstreamout.c
    printf/wsprintfA.c
    printf/wsprintfW.c
    printf/wvsprintfA.c
    printf/wvsprintfW.c
    printf/wvsnprintfA.c
    printf/wvsnprintfW.c
    string/mbstowcs_nt.c
    string/wcstombs_nt.c)

add_dependencies(user32_wsprintf psdk)
add_target_compile_definitions(user32_wsprintf _USER32_WSPRINTF)

add_library(getopt misc/getopt.c)
add_target_compile_definitions(getopt _DLL __USE_CRTIMP)
add_dependencies(getopt psdk)
