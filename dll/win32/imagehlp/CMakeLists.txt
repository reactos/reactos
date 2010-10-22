
add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-D_IMAGEHLP_SOURCE_)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(imagehlp ${CMAKE_CURRENT_SOURCE_DIR}/imagehlp.spec ${CMAKE_CURRENT_BINARY_DIR}/imagehlp.def)

list(APPEND SOURCE
    access.c
    imagehlp_main.c
    integrity.c
    modify.c
    imagehlp.rc)

add_library(imagehlp SHARED 
    ${CMAKE_CURRENT_BINARY_DIR}/imagehlp_precomp.h.gch
    ${SOURCE})

set_module_type(imagehlp win32dll)

target_link_libraries(imagehlp
    ${CMAKE_CURRENT_BINARY_DIR}/imagehlp.def
    wine)

add_importlibs(imagehlp dbghelp kernel32 ntdll)
add_pch(imagehlp ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(imagehlp imagehlp_def)
add_livecd_target(imagehlp reactos/system32)
