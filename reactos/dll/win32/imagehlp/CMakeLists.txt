
add_definitions(-D_IMAGEHLP_SOURCE_)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(imagehlp.dll imagehlp.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    access.c
    imagehlp_main.c
    integrity.c
    modify.c
    precomp.h)

add_library(imagehlp SHARED
    ${SOURCE}
    imagehlp.rc
    ${CMAKE_CURRENT_BINARY_DIR}/imagehlp_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/imagehlp.def)

set_module_type(imagehlp win32dll)
target_link_libraries(imagehlp wine)
add_importlibs(imagehlp dbghelp msvcrt kernel32 ntdll)
add_pch(imagehlp precomp.h SOURCE)
add_cd_file(TARGET imagehlp DESTINATION reactos/system32 FOR all)
