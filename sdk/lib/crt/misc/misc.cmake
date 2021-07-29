
list(APPEND LIBCNTPT_MISC_SOURCE
    misc/fltused.c
)

if(ARCH STREQUAL "i386")
    list(APPEND LIBCNTPR_ASM_SOURCE
        misc/i386/readcr4.S
    )
endif()

list(APPEND CRT_MISC_SOURCE
    ${LIBCNTPT_MISC_SOURCE}
    misc/__crt_MessageBoxA.c
    misc/amsg.c
    misc/assert.c
    misc/crt_init.c
    misc/environ.c
    misc/getargs.c
    misc/i10output.c
    misc/initterm.c
    misc/lock.c
    misc/purecall.c
    misc/stubs.c
    misc/tls.c
)

add_library(getopt misc/getopt.c)
target_compile_definitions(getopt PRIVATE _DLL __USE_CRTIMP)
add_dependencies(getopt psdk)
