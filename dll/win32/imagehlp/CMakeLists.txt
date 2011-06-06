
add_definitions(
    -D_DLL -D__USE_CRTIMP
    -D_IMAGEHLP_SOURCE_)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(imagehlp.dll imagehlp.spec)

list(APPEND SOURCE
    access.c
    imagehlp_main.c
    integrity.c
    modify.c
    imagehlp.rc
    ${CMAKE_CURRENT_BINARY_DIR}/imagehlp_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/imagehlp.def)

add_library(imagehlp SHARED 
    ${CMAKE_CURRENT_BINARY_DIR}/imagehlp_precomp.h.gch
    ${SOURCE})

set_module_type(imagehlp win32dll)

target_link_libraries(imagehlp wine)

add_importlibs(imagehlp dbghelp msvcrt kernel32 ntdll)
add_importlib_target(imagehlp.spec)

add_pch(imagehlp ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

add_cd_file(TARGET imagehlp DESTINATION reactos/system32 FOR all)
