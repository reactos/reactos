
list(APPEND LIBCNTPR_STRING_SOURCE
    string/_splitpath.c
    string/_stricmp_nt.c
    string/_strlwr_nt.c
    string/_strnicmp_nt.c
    string/_strupr_nt.c
    string/_wsplitpath.c
    string/atoi.c
    string/atoi64.c
    string/atol.c
    string/ctype.c
    #string/is_wctype.c
    string/iswctype_nt.c
    string/itoa.c
    string/itow.c
    string/mbstowcs_nt.c
    string/scanf.c
    string/strcspn.c
    string/strpbrk.c
    string/strrev.c
    string/strset.c
    string/strspn.c
    string/strstr.c
    string/strtoi64.c
    string/strtol.c
    string/strtoul.c
    string/strtoull.c
    string/tolower_nt.c
    string/toupper_nt_mb.c
    string/towupper_nt.c
    string/wcs.c
    string/wcstol.c
    string/wcstombs_nt.c
    string/wcstoul.c
    string/wctype.c
    string/wtoi.c
    string/wtoi64.c
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
