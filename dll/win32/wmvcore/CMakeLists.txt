
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(wmvcore.dll wmvcore.spec)

list(APPEND SOURCE
    wmvcore_main.c
    writer.c
    ${CMAKE_CURRENT_BINARY_DIR}/wmvcore_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/wmvcore.def)

add_library(wmvcore SHARED ${SOURCE})
set_module_type(wmvcore win32dll)
target_link_libraries(wmvcore wine)
add_importlibs(wmvcore msvcrt kernel32 ntdll)
add_cd_file(TARGET wmvcore DESTINATION reactos/system32 FOR all)
