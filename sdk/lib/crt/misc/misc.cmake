
list(APPEND LIBCNTPT_MISC_SOURCE
    misc/fltused.c
)

if(ARCH STREQUAL "i386")
    list(APPEND LIBCNTPR_ASM_SOURCE
        misc/i386/readcr4.S
    )
endif()

add_library(getopt misc/getopt.c)
target_compile_definitions(getopt PRIVATE _DLL __USE_CRTIMP)
add_dependencies(getopt psdk)
