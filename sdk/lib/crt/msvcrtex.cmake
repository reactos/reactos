
include_directories(include/internal/mingw-w64)

list(APPEND MSVCRTEX_SOURCE
    ${CRT_STARTUP_SOURCE}
    math/sincos.c
    misc/fltused.c
    misc/isblank.c
    misc/iswblank.c
    misc/ofmt_stub.c
    stdio/acrt_iob_func.c)

if(DLL_EXPORT_VERSION LESS 0x600)
    list(APPEND MSVCRTEX_SOURCE
        misc/dbgrpt.cpp
        stdlib/_invalid_parameter.c
        stdlib/rand_s.c
        wstring/mbrtowc.c
        wstring/wcrtomb.c
    )
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Clang performs some optimizations requiring those funtions
    list(APPEND MSVCRTEX_SOURCE
        math/round.c
        math/roundf.c
        math/exp2.c
        math/exp2f.c
        )
endif()

if(ARCH STREQUAL "i386")
    # Clang wants __aulldiv for its optimizations
    list(APPEND MSVCRTEX_ASM_SOURCE
        except/i386/chkstk_asm.s
        except/i386/chkstk_ms.s
        math/i386/alldiv_asm.s
        math/i386/aulldiv_asm.s
        )
    if (CMAKE_C_COMPILER_ID STREQUAL "Clang" AND NOT MSVC)
        list(APPEND MSVCRTEX_ASM_SOURCE
            math/i386/ceilf.S
            math/i386/floorf.S)
        list(APPEND MSVCRTEX_SOURCE
            math/i386/sqrtf.c)
    endif()
    if(MSVC AND DLL_EXPORT_VERSION LESS 0x600)
        list(APPEND MSVCRTEX_ASM_SOURCE
            except/i386/__CxxFrameHandler3.s
            math/i386/ftoul2_legacy_asm.s)
        list(APPEND MSVCRTEX_SOURCE
            except/i386/CxxHandleV8Frame.c)
    endif()
elseif(ARCH STREQUAL "amd64")
    list(APPEND MSVCRTEX_ASM_SOURCE
        except/amd64/chkstk_ms.s)
    if(MSVC AND DLL_EXPORT_VERSION LESS 0x600)
        list(APPEND MSVCRTEX_ASM_SOURCE
            except/amd64/__CxxFrameHandler3.s
        )
    endif()
elseif(ARCH STREQUAL "arm")
    list(APPEND MSVCRTEX_SOURCE
        math/arm/__rt_sdiv.c
        math/arm/__rt_sdiv64_worker.c
        math/arm/__rt_udiv.c
        math/arm/__rt_udiv64_worker.c
        math/arm/__rt_div_worker.h
        math/arm/__dtoi64.c
        math/arm/__dtou64.c
        math/arm/__stoi64.c
        math/arm/__stou64.c
        math/arm/__fto64.h
        math/arm/__i64tod.c
        math/arm/__u64tod.c
        math/arm/__i64tos.c
        math/arm/__u64tos.c
        math/arm/__64tof.h
    )
    list(APPEND MSVCRTEX_ASM_SOURCE
        except/arm/chkstk_asm.s
        math/arm/__rt_sdiv64.s
        math/arm/__rt_srsh.s
        math/arm/__rt_udiv64.s
    )
endif()

set_source_files_properties(${MSVCRTEX_ASM_SOURCE} PROPERTIES COMPILE_DEFINITIONS "_DLL;_MSVCRTEX_")
add_asm_files(msvcrtex_asm ${MSVCRTEX_ASM_SOURCE})

add_library(msvcrtex OBJECT ${MSVCRTEX_SOURCE} ${msvcrtex_asm})
target_compile_definitions(msvcrtex PRIVATE _DLL _MSVCRTEX_)

if(MSVC AND (ARCH STREQUAL "i386"))
    # user32.dll needs this as a stand-alone object file
    add_asm_files(ftol2_asm math/i386/ftol2_asm.s)
    add_library(ftol2_sse OBJECT ${ftol2_asm})
    target_compile_definitions(ftol2_sse PRIVATE $<TARGET_PROPERTY:msvcrtex,COMPILE_DEFINITIONS>)
    set_target_properties(ftol2_sse PROPERTIES LINKER_LANGUAGE C)
endif()


if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(msvcrtex PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-main>)
    if(LTCG)
        target_compile_options(msvcrtex PRIVATE -fno-lto)
    endif()
endif()

set_source_files_properties(startup/crtdll.c PROPERTIES COMPILE_DEFINITIONS CRTDLL)
set_source_files_properties(startup/crtexe.c
                            startup/wcrtexe.c PROPERTIES COMPILE_DEFINITIONS _M_CEE_PURE)

if(NOT MSVC)
    target_link_libraries(msvcrtex oldnames)
endif()

add_dependencies(msvcrtex psdk asm)
