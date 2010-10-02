
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mciqtz32 ${CMAKE_CURRENT_SOURCE_DIR}/mciqtz32.spec ${CMAKE_CURRENT_BINARY_DIR}/mciqtz32.def)

add_library(mciqtz32 SHARED mciqtz.c version.rc)

set_module_type(mciqtz32 win32dll)

target_link_libraries(mciqtz32
    ${CMAKE_CURRENT_BINARY_DIR}/mciqtz32.def
    wine
    strmiids)

add_importlibs(mciqtz32 winmm oleaut32 ole32 user32 gdi32 kernel32 ntdll)
add_dependencies(mciqtz32 mciqtz32_def)
