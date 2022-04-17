
list(APPEND CRT_WINE_SOURCE
    wine/cpp.c
    wine/except.c
    wine/heap.c
    wine/undname.c
)

if(ARCH STREQUAL "i386")
    list(APPEND CRT_WINE_SOURCE
        wine/except_i386.c
    )
elseif(ARCH STREQUAL "amd64")
    list(APPEND CRT_WINE_SOURCE
        wine/except_x86_64.c
    )
elseif(ARCH STREQUAL "arm")
    list(APPEND CRT_WINE_SOURCE
        wine/except_arm.c
    )
elseif(ARCH STREQUAL "arm64")
    list(APPEND CRT_WINE_SOURCE
        wine/except_arm64.c
    )
endif()

# includes for wine code
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

#set_source_files_properties(${CRT_WINE_SOURCE} PROPERTIES INCLUDE_DIRECTORIES)
