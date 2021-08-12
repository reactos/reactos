
list(APPEND LIBCNTPR_STRING_SOURCE
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
    string/strtoi64.c
    string/strtol.c
    string/strtoul.c
    string/strtoull.c
    string/wcs.c
    string/wcstol.c
    string/wcstoul.c
    string/wctype.c
    string/wtoi64.c
    string/wtoi.c
    string/wtol.c
    string/winesup.c
)

if(ARCH STREQUAL "i386")
    list(APPEND LIBCNTPR_STRING_ASM_SOURCE
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
        string/i386/wcsrchr_asm.s
    )
else()
    list(APPEND LIBCNTPR_STRING_SOURCE
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
        string/wcsrchr.c
    )
endif()

list(APPEND CRT_STRING_SOURCE
    ${LIBCNTPR_STRING_SOURCE}
    string/_mbsnlen.c
    string/_mbstrnlen.c
    string/_splitpath_s.c
    string/_wcslwr_s.c
    string/_wsplitpath_s.c
    string/atof.c
    string/mbstowcs_s.c
    string/strcoll.c
    string/strdup.c
    string/strerror.c
    string/string.c
    string/strncoll.c
    string/strtod.c
    string/strtok.c
    string/strtok_s.c
    string/strtoul.c
    string/strxfrm.c
    string/wcstombs_s.c
    string/wtof.c
)

list(APPEND CRT_STRING_ASM_SOURCE
    ${LIBCNTPR_STRING_ASM_SOURCE}
)

list(APPEND LIBCNTPR_STRING_SOURCE
    string/mbstowcs_nt.c
    string/wcstombs_nt.c
)

# Used by acpi.sys
add_library(strtol
    string/ctype.c
    string/iswctype.c
    string/strtoi64.c
    string/strtol.c
    string/strtoul.c
    string/strtoull.c
    string/wctype.c)
target_compile_definitions(strtol PRIVATE _LIBCNT_)
add_dependencies(strtol psdk)
