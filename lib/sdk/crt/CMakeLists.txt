include_directories(./include)

add_definitions(-D_CRTBLD)

file(GLOB_RECURSE CRT_SOURCE "*.c")
LIST(REMOVE_ITEM CRT_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/stdio/findgen.c)
LIST(REMOVE_ITEM CRT_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/string/strtold.c)

add_library(crt ${CRT_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h.gch)
set_property(TARGET crt PROPERTY COMPILE_DEFINITIONS __MINGW_IMPORT=extern USE_MSVCRT_PREFIX _MSVCRT_LIB_ _MSVCRT_ _MT)
add_pch(crt ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${CRT_SOURCE})
add_dependencies(crt psdk)

if(ARCH MATCHES i386)
set(LIBCNTPR_SOURCE 
  ${CMAKE_CURRENT_SOURCE_DIR}/except/i386/chkstk_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/except/i386/seh.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/alldiv_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/alldvrm_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/allmul_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/allrem_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/allshl_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/allshr_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/atan_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/aulldiv_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/aulldvrm_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/aullrem_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/aullshr_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/ceil_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/cos_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/fabs_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/floor_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/ftol_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/log_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/pow_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/sin_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/sqrt_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/tan_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/math/i386/ci.c
  ${CMAKE_CURRENT_SOURCE_DIR}/math/abs.c
  ${CMAKE_CURRENT_SOURCE_DIR}/math/div.c
  ${CMAKE_CURRENT_SOURCE_DIR}/math/labs.c
  ${CMAKE_CURRENT_SOURCE_DIR}/math/rand_nt.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mem/i386/memchr_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/mem/i386/memcpy_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/mem/i386/memmove_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/mem/i386/memset_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/mem/memccpy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mem/memcmp.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mem/memicmp.c
  ${CMAKE_CURRENT_SOURCE_DIR}/search/bsearch.c
  ${CMAKE_CURRENT_SOURCE_DIR}/search/lfind.c
  ${CMAKE_CURRENT_SOURCE_DIR}/setjmp/i386/setjmp.s
  ${CMAKE_CURRENT_SOURCE_DIR}/stdlib/qsort.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/strcat_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/strchr_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/strcmp_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/strcpy_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/strlen_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/strncat_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/strncmp_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/strncpy_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/strnlen_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/strrchr_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/wcscat_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/wcschr_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/wcscmp_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/wcscpy_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/wcslen_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/wcsncat_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/wcsncmp_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/wcsncpy_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/wcsnlen_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/i386/wcsrchr_asm.s
  ${CMAKE_CURRENT_SOURCE_DIR}/string/ctype.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/scanf.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/strcspn.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/stricmp.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/strnicmp.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/strlwr.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/strrev.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/strset.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/strstr.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/strupr.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/strpbrk.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/strspn.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/atoi64.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/atoi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/atol.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/itoa.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/itow.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/mbstowcs_nt.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/splitp.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/strtol.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/strtoul.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/strtoull.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/wcs.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/wcstol.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/wcstombs_nt.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/wcstoul.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/wsplitp.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/wtoi64.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/wtoi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/string/wtol.c
  ${CMAKE_CURRENT_SOURCE_DIR}/wstring/wcsicmp.c
  ${CMAKE_CURRENT_SOURCE_DIR}/wstring/wcslwr.c
  ${CMAKE_CURRENT_SOURCE_DIR}/wstring/wcsnicmp.c
  ${CMAKE_CURRENT_SOURCE_DIR}/wstring/wcsupr.c
  ${CMAKE_CURRENT_SOURCE_DIR}/wstring/wcscspn.c
  ${CMAKE_CURRENT_SOURCE_DIR}/wstring/wcsspn.c
  ${CMAKE_CURRENT_SOURCE_DIR}/wstring/wcsstr.c
)
else()
#TBD
endif()

add_library(libcntpr ${LIBCNTPR_SOURCE})
set_property(TARGET libcntpr PROPERTY COMPILE_DEFINITIONS NO_RTL_INLINES _NTSYSTEM_ _NTDLLBUILD_ _LIBCNT_ __CRT__NO_INLINE)
add_dependencies(libcntpr psdk)
