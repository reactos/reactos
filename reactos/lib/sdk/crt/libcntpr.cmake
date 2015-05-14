
list(APPEND LIBCNTPR_SOURCE
    float/isnan.c
    math/abs.c
    math/div.c
    math/labs.c
    math/rand_nt.c
    mbstring/mbstrlen.c
    mem/memccpy.c
    mem/memcmp.c
    mem/memicmp.c
    misc/fltused.c
    printf/_snprintf.c
    printf/_snwprintf.c
    printf/_vcprintf.c
    printf/_vscwprintf.c
    printf/_vsnprintf.c
    printf/_vsnwprintf.c
    printf/sprintf.c
    printf/streamout.c
    printf/swprintf.c
    printf/vprintf.c
    printf/vsprintf.c
    printf/vswprintf.c
    printf/wstreamout.c
    search/bsearch.c
    search/lfind.c
    stdlib/qsort.c
    string/_splitpath.c
    string/_wsplitpath.c
    string/ctype.c
    string/iswctype.c
    string/is_wctype.c
    string/scanf.c
    string/strcspn.c
    string/stricmp.c
    string/strnicmp.c
    string/strlwr.c
    string/strrev.c
    string/strset.c
    string/strstr.c
    string/strupr.c
    string/strpbrk.c
    string/strspn.c
    string/atoi64.c
    string/atoi.c
    string/atol.c
    string/itoa.c
    string/itow.c
    string/mbstowcs_nt.c
    string/strtol.c
    string/strtoul.c
    string/strtoull.c
    string/wcs.c
    string/wcstol.c
    string/wcstombs_nt.c
    string/wcstoul.c
    string/wctype.c
    string/wtoi64.c
    string/wtoi.c
    string/wtol.c
    string/winesup.c
    wstring/wcsicmp.c
    wstring/wcslwr.c
    wstring/wcsnicmp.c
    wstring/wcsupr.c
    wstring/wcscspn.c
    wstring/wcsspn.c
    wstring/wcsstr.c)

if(ARCH STREQUAL "i386")
    list(APPEND LIBCNTPR_ASM_SOURCE
        except/i386/chkstk_asm.s
        except/i386/seh.s
        except/i386/seh_prolog.s
        setjmp/i386/setjmp.s
        math/i386/alldiv_asm.s
        math/i386/alldvrm_asm.s
        math/i386/allmul_asm.s
        math/i386/allrem_asm.s
        math/i386/allshl_asm.s
        math/i386/allshr_asm.s
        math/i386/atan_asm.s
        math/i386/atan2_asm.s
        math/i386/aulldiv_asm.s
        math/i386/aulldvrm_asm.s
        math/i386/aullrem_asm.s
        math/i386/aullshr_asm.s
        math/i386/ceil_asm.s
        math/i386/cos_asm.s
        math/i386/fabs_asm.s
        math/i386/floor_asm.s
        math/i386/ftol_asm.s
        math/i386/ftol2_asm.s
        math/i386/log_asm.s
        math/i386/log10_asm.s
        math/i386/pow_asm.s
        math/i386/sin_asm.s
        math/i386/sqrt_asm.s
        math/i386/tan_asm.s
        misc/i386/readcr4.S)

    list(APPEND LIBCNTPR_SOURCE
        math/i386/ci.c
        math/i386/cicos.c
        math/i386/cilog.c
        math/i386/cipow.c
        math/i386/cisin.c
        math/i386/cisqrt.c)
    if(NOT MSVC)
        list(APPEND LIBCNTPR_SOURCE except/i386/chkstk_ms.s)
    endif()
elseif(ARCH STREQUAL "amd64")
    list(APPEND LIBCNTPR_ASM_SOURCE
        except/amd64/chkstk_asm.s
        except/amd64/seh.s
        setjmp/amd64/setjmp.s
        math/amd64/atan.S
        math/amd64/atan2.S
        math/amd64/ceil.S
        math/amd64/exp.S
        math/amd64/fabs.S
        math/amd64/floor.S
        math/amd64/floorf.S
        math/amd64/fmod.S
        math/amd64/ldexp.S
        math/amd64/log.S
        math/amd64/log10.S
        math/amd64/pow.S
        math/amd64/sqrt.S
        math/amd64/tan.S)
    list(APPEND LIBCNTPR_SOURCE
        except/amd64/ehandler.c
        math/cos.c
        math/sin.c)
elseif(ARCH STREQUAL "arm")
    list(APPEND LIBCNTPR_SOURCE
        except/arm/chkstk_asm.s
        except/arm/__jump_unwind.s
        math/arm/__rt_sdiv.c
        math/arm/__rt_sdiv64_worker.c
        math/arm/__rt_udiv.c
        math/arm/__rt_udiv64_worker.c
    )
    list(APPEND LIBCNTPR_ASM_SOURCE
        except/arm/_abnormal_termination.s
        except/arm/_except_handler2.s
        except/arm/_except_handler3.s
        except/arm/_global_unwind2.s
        except/arm/_local_unwind2.s
        except/arm/chkstk_asm.s
        except/arm/ehandler.c
        float/arm/_clearfp.s
        float/arm/_controlfp.s
        float/arm/_fpreset.s
        float/arm/_statusfp.s
        math/arm/atan.s
        math/arm/atan2.s
        math/arm/ceil.s
        math/arm/exp.s
        math/arm/fabs.s
        math/arm/fmod.s
        math/arm/floor.s
        math/arm/ldexp.s
        math/arm/log.s
        math/arm/log10.s
        math/arm/pow.s
        math/arm/sqrt.s
        math/arm/tan.s
        math/arm/__dtoi64.s
        math/arm/__dtou64.s
        math/arm/__i64tod.s
        math/arm/__i64tos.s
        math/arm/__stoi64.s
        math/arm/__stou64.s
        math/arm/__u64tod.s
        math/arm/__u64tos.s
        math/arm/__rt_sdiv64.s
        math/arm/__rt_srsh.s
        math/arm/__rt_udiv64.s
        setjmp/arm/setjmp.s
    )
endif()

if(ARCH STREQUAL "i386")
    list(APPEND LIBCNTPR_ASM_SOURCE
        mem/i386/memchr_asm.s
        mem/i386/memmove_asm.s
        mem/i386/memset_asm.s
        string/i386/strcat_asm.s
        string/i386/strchr_asm.s
        string/i386/strcmp_asm.s
        string/i386/strcpy_asm.s
        string/i386/strlen_asm.s
        string/i386/strncat_asm.s
        string/i386/strncmp_asm.s
        string/i386/strncpy_asm.s
        string/i386/strnlen_asm.s
        string/i386/strrchr_asm.s
        string/i386/wcscat_asm.s
        string/i386/wcschr_asm.s
        string/i386/wcscmp_asm.s
        string/i386/wcscpy_asm.s
        string/i386/wcslen_asm.s
        string/i386/wcsncat_asm.s
        string/i386/wcsncmp_asm.s
        string/i386/wcsncpy_asm.s
        string/i386/wcsnlen_asm.s
        string/i386/wcsrchr_asm.s)
else()
    list(APPEND LIBCNTPR_SOURCE
        math/cos.c
        math/sin.c
        mem/memchr.c
        mem/memcpy.c
        mem/memmove.c
        mem/memset.c
        string/strcat.c
        string/strchr.c
        string/strcmp.c
        string/strcpy.c
        string/strlen.c
        string/strncat.c
        string/strncmp.c
        string/strncpy.c
        string/strnlen.c
        string/strrchr.c
        string/wcscat.c
        string/wcschr.c
        string/wcscmp.c
        string/wcscpy.c
        string/wcslen.c
        string/wcsncat.c
        string/wcsncmp.c
        string/wcsncpy.c
        string/wcsnlen.c
        string/wcsrchr.c)
endif()

set_source_files_properties(${LIBCNTPR_ASM_SOURCE} PROPERTIES COMPILE_DEFINITIONS "NO_RTL_INLINES;_NTSYSTEM_;_NTDLLBUILD_;_LIBCNT_;__CRT__NO_INLINE;CRTDLL")
add_asm_files(libcntpr_asm ${LIBCNTPR_ASM_SOURCE})

add_library(libcntpr ${LIBCNTPR_SOURCE} ${libcntpr_asm})
add_target_compile_definitions(libcntpr
    NO_RTL_INLINES
    _NTSYSTEM_
    _NTDLLBUILD_
    _LIBCNT_
    __CRT__NO_INLINE
    CRTDLL)
add_dependencies(libcntpr psdk asm)
