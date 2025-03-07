
list(APPEND UCRTSUPP_SOURCE
    ${CRT_FLOAT_SOURCE}
    ${CRT_MATH_SOURCE}
    misc/amsg.c
    misc/purecall.c
    misc/tls.c
    wine/cpp.c
    wine/except.c
    wine/undname.c
)

list(APPEND UCRTSUPP_ASM_SOURCE
    ${CRT_FLOAT_ASM_SOURCE}
    ${CRT_MATH_ASM_SOURCE}
    ${CRT_SETJMP_ASM_SOURCE}
    ${CRT_WINE_ASM_SOURCE}
)

if(ARCH STREQUAL "i386")
    list(APPEND UCRTSUPP_SOURCE
        except/i386/CxxHandleV8Frame.c
        wine/except_i386.c
    )
    list(APPEND UCRTSUPP_ASM_SOURCE
        except/i386/__CxxFrameHandler3.s
        except/i386/chkesp.s
        wine/rosglue_i386.s
    )
    if(MSVC)
        list(APPEND UCRTSUPP_ASM_SOURCE
            except/i386/cpp.s
            except/i386/prolog.s
        )
    endif()
elseif(ARCH STREQUAL "amd64")
    list(APPEND UCRTSUPP_SOURCE
        wine/except_x86_64.c
    )
    if(MSVC)
        list(APPEND UCRTSUPP_ASM_SOURCE
            except/amd64/cpp.s
        )
    endif()
elseif(ARCH STREQUAL "arm")
    list(APPEND UCRTSUPP_SOURCE
        wine/except_arm.c
    )
    if(MSVC)
        list(APPEND UCRTSUPP_ASM_SOURCE
            except/arm/cpp.s
        )
    endif()
elseif(ARCH STREQUAL "arm64")
    list(APPEND UCRTSUPP_SOURCE
        wine/except_arm64.c
    )
endif()

add_asm_files(ucrtsupp_asm ${UCRTSUPP_ASM_SOURCE})

add_library(ucrtsupport ${UCRTSUPP_SOURCE} ${ucrtsupp_asm})
target_link_libraries(ucrtsupport chkstk ${PSEH_LIB})
target_compile_definitions(ucrtsupport PRIVATE
    __UCRTSUPPORT__
    CRTDLL
    _MSVCRT_LIB_
    _MSVCRT_
    _MT
    USE_MSVCRT_PREFIX
    __MINGW_IMPORT=extern
    __fma3_lib_init=__acrt_initialize_fma3
    set_terminate=_wine_set_terminate
    terminate=_wine_terminate
    _get_terminate=_wine_get_terminate
    unexpected=_wine_unexpected
    __pxcptinfoptrs=_wine__pxcptinfoptrs
)
#add_pch(crt precomp.h)
add_dependencies(ucrtsupport psdk asm)
