
if(ARCH STREQUAL "i386")
    list(APPEND LIBCNTPR_EXCEPT_ASM_SOURCE
        except/i386/chkstk_asm.s
    )
    list(APPEND CRT_EXCEPT_ASM_SOURCE
        except/i386/__CxxFrameHandler3.s
        except/i386/chkesp.s
        except/i386/prolog.s
    )
    list(APPEND CRT_EXCEPT_SOURCE
        except/i386/CxxHandleV8Frame.c
    )
    if(MSVC)
        list(APPEND CRT_EXCEPT_ASM_SOURCE
            except/i386/cpp.s
            except/i386/cpp_alias.s)
    endif()
elseif(ARCH STREQUAL "amd64")
    list(APPEND LIBCNTPR_EXCEPT_SOURCE
        except/amd64/ehandler.c
    )
    list(APPEND LIBCNTPR_EXCEPT_ASM_SOURCE
        except/amd64/chkstk_ms.s
        except/amd64/seh.s
    )
    list(APPEND CRT_EXCEPT_ASM_SOURCE
        except/amd64/seh.s
    )
    if(MSVC)
        list(APPEND CRT_EXCEPT_ASM_SOURCE
            except/amd64/cpp.s
            except/amd64/cpp_alias.s)
    endif()
elseif(ARCH STREQUAL "arm")
    list(APPEND LIBCNTPR_EXCEPT_SOURCE
        except/arm/ehandler.c
    )
    list(APPEND LIBCNTPR_EXCEPT_ASM_SOURCE
        except/arm/__jump_unwind.s
        except/arm/_abnormal_termination.s
        except/arm/_except_handler2.s
        except/arm/_except_handler3.s
        except/arm/_global_unwind2.s
        except/arm/_local_unwind2.s
        except/arm/chkstk_asm.s
    )
    list(APPEND CRT_EXCEPT_ASM_SOURCE
        except/arm/_abnormal_termination.s
        except/arm/_except_handler2.s
        except/arm/_except_handler3.s
        except/arm/_global_unwind2.s
        except/arm/_local_unwind2.s
        except/arm/chkstk_asm.s
    )
    if(MSVC)
        list(APPEND CRT_EXCEPT_ASM_SOURCE
            except/arm/cpp.s
            except/arm/cpp_alias.s)
    endif()
endif()

list(APPEND CRT_EXCEPT_SOURCE
    ${LIBCNTPR_EXCEPT_SOURCE}
    except/stack.c
)

if(ARCH STREQUAL "i386")
    list(APPEND CHKSTK_ASM_SOURCE except/i386/chkstk_asm.s)
elseif(ARCH STREQUAL "amd64")
    list(APPEND CHKSTK_ASM_SOURCE except/amd64/chkstk_ms.s)
elseif(ARCH STREQUAL "arm")
    list(APPEND CHKSTK_ASM_SOURCE except/arm/chkstk_asm.s)
elseif(ARCH STREQUAL "arm64")
    list(APPEND CHKSTK_ASM_SOURCE except/arm64/chkstk_asm.s)
endif()

add_asm_files(chkstk_lib_asm ${CHKSTK_ASM_SOURCE})
add_library(chkstk ${CHKSTK_SOURCE} ${chkstk_lib_asm})
set_target_properties(chkstk PROPERTIES LINKER_LANGUAGE "C")
add_dependencies(chkstk asm)
