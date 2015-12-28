
include_directories(${REACTOS_SOURCE_DIR}/win32ss/include)

add_library(gditools
    gditools.c)

add_dependencies(gditools xdk)
