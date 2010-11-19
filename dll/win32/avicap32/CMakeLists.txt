
set_unicode()

add_library(avicap32 SHARED
    avicap32.c
    avicap32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/avicap32.def)

set_module_type(avicap32 win32dll)

set_export_spec(avicap32 avicap32.spec)

add_importlib_target(${CMAKE_CURRENT_BINARY_DIR}/avicap32.def)

target_link_libraries(avicap32 wine)

add_importlibs(avicap32 advapi32 user32 version kernel32 ntdll)

add_cab_target(avicap32 1)
