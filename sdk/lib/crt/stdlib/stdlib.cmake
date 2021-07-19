
list(APPEND LIBCNTPR_STDLIB_SOURCE
    stdlib/qsort.c
)

list(APPEND CRT_STDLIB_SOURCE
    ${LIBCNTPR_STDLIB_SOURCE}
    stdlib/_exit.c
    stdlib/_set_abort_behavior.c
    stdlib/abort.c
    stdlib/atexit.c
    stdlib/ecvt.c
    stdlib/errno.c
    stdlib/fcvt.c
    stdlib/fcvtbuf.c
    stdlib/fullpath.c
    stdlib/gcvt.c
    stdlib/getenv.c
    stdlib/makepath.c
    stdlib/makepath_s.c
    stdlib/mbtowc.c
    stdlib/mbstowcs.c
    stdlib/obsol.c
    stdlib/putenv.c
    stdlib/rot.c
    stdlib/senv.c
    stdlib/swab.c
    stdlib/wfulpath.c
    stdlib/wputenv.c
    stdlib/wsenv.c
    stdlib/wmakpath.c
    stdlib/wmakpath_s.c
)

if(USE_CLANG_CL)
    # clang-cl is missing pragma function support
    # https://bugs.llvm.org/show_bug.cgi?id=35116
    set_property(SOURCE stdlib/rot.c APPEND_STRING PROPERTY COMPILE_FLAGS " /fallback")

    if(ARCH STREQUAL "i386")
        list(APPEND CRT_STDLIB_ASM_SOURCE
            stdlib/clang-alias.s
        )
    elseif(ARCH STREQUAL "amd64")
        list(APPEND CRT_STDLIB_ASM_SOURCE
            stdlib/clang-alias.s
        )
    endif()
endif()
