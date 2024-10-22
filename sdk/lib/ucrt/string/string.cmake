
list(APPEND UCRT_STRING_SOURCES
    string/memcpy_s.cpp
    string/memicmp.cpp
    string/strcat_s.cpp
    string/strcoll.cpp
    string/strcpy_s.cpp
    string/strdup.cpp
    string/stricmp.cpp
    string/stricoll.cpp
    string/strlwr.cpp
    string/strncat_s.cpp
    string/strncnt.cpp
    string/strncoll.cpp
    string/strncpy_s.cpp
    string/strnicmp.cpp
    string/strnicol.cpp
    string/strnlen.cpp
    string/strnset_s.cpp
    string/strset_s.cpp
    string/strtok.cpp
    string/strtok_s.cpp
    string/strupr.cpp
    string/strxfrm.cpp
    string/wcscat.cpp
    string/wcscat_s.cpp
    string/wcscmp.cpp
    string/wcscoll.cpp
    string/wcscpy.cpp
    string/wcscpy_s.cpp
    string/wcscspn.cpp
    string/wcsdup.cpp
    string/wcsicmp.cpp
    string/wcsicoll.cpp
    string/wcslwr.cpp
    string/wcsncat.cpp
    string/wcsncat_s.cpp
    string/wcsncmp.cpp
    string/wcsncnt.cpp
    string/wcsncoll.cpp
    string/wcsncpy.cpp
    string/wcsncpy_s.cpp
    string/wcsnicmp.cpp
    string/wcsnicol.cpp
    string/wcsnset.cpp
    string/wcsnset_s.cpp
    string/wcspbrk.cpp
    string/wcsrev.cpp
    string/wcsset.cpp
    string/wcsset_s.cpp
    string/wcsspn.cpp
    string/wcstok.cpp
    string/wcstok_s.cpp
    string/wcsupr.cpp
    string/wcsxfrm.cpp
    string/wmemcpy_s.cpp
    string/wmemmove_s.cpp
)

if(${ARCH} STREQUAL "i386")
    list(APPEND UCRT_STRING_ASM_SOURCES
        string/i386/_memicmp.s
        string/i386/_strnicm.s
        string/i386/memccpy.s
        string/i386/strcat.s
        string/i386/strcmp.s
        string/i386/strcspn.s
        string/i386/strlen.s
        string/i386/strncat.s
        string/i386/strncmp.s
        string/i386/strncpy.s
        string/i386/strnset.s
        string/i386/strpbrk.s
        string/i386/strrev.s
        string/i386/strset.s
        string/i386/strspn.s
    )
elseif(${ARCH} STREQUAL "amd64")
    list(APPEND UCRT_STRING_ASM_SOURCES
        string/amd64/strcat.s
        string/amd64/strcmp.s
        string/amd64/strlen.s
        string/amd64/strncat.s
        string/amd64/strncmp.s
        string/amd64/strncpy.s
    )
    list(APPEND UCRT_STRING_SOURCES
        string/amd64/strcspn.c
        string/amd64/strpbrk.c
        string/amd64/strspn.c
        string/memccpy.c
        string/strnset.c
        string/strrev.c
        string/strset.c
    )
else()
    if(${ARCH} STREQUAL "arm64")
        list(APPEND UCRT_STRING_ASM_SOURCES
            string/arm64/strlen.s
            string/arm64/wcslen.s
    )
    else()
        list(APPEND UCRT_STRING_SOURCES
            string/arm/strlen.c
        )
    endif()
    list(APPEND UCRT_STRING_SOURCES
        string/memccpy.c
        string/strcat.c
        string/strcmp.c
        string/strcspn.c
        string/strncat.c
        string/strncmp.c
        string/strncpy.c
        string/strnset.c
        string/strpbrk.c
        string/strrev.c
        string/strset.c
        string/strspn.c
    )
endif()

add_asm_files(UCRT_STRING_ASM ${UCRT_STRING_ASM_SOURCES})
list(APPEND UCRT_STRING_SOURCES ${UCRT_STRING_ASM})
