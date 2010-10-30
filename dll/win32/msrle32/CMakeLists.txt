
add_definitions(-D__WINESRC__)
add_definitions(-D_DLL -D__USE_CRTIMP)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(msrle32 ${CMAKE_CURRENT_SOURCE_DIR}/msrle32.spec ${CMAKE_CURRENT_BINARY_DIR}/msrle32.def)

add_library(msrle32 SHARED msrle32.c rsrc.rc)

set_module_type(msrle32 win32dll)

target_link_libraries(msrle32
    ${CMAKE_CURRENT_BINARY_DIR}/msrle32.def
    wine)

add_importlibs(msrle32 msvcrt winmm user32 kernel32 ntdll)
add_dependencies(msrle32 msrle32_def)
add_cab_target(msrle32 1)
