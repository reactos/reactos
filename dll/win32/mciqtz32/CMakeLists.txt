
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mciqtz32.dll mciqtz32.spec)

list(APPEND SOURCE
    mciqtz.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mciqtz32.def)

add_library(mciqtz32 SHARED ${SOURCE})

set_module_type(mciqtz32 win32dll)

target_link_libraries(mciqtz32 wine strmiids)
add_importlibs(mciqtz32 winmm oleaut32 ole32 user32 gdi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET mciqtz32 DESTINATION reactos/system32 FOR all)
