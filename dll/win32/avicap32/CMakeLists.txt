
set_unicode()

spec2def(avicap32 ${CMAKE_CURRENT_SOURCE_DIR}/avicap32.spec ${CMAKE_CURRENT_BINARY_DIR}/avicap32.def)

add_library(avicap32 SHARED avicap32.c avicap32.rc)

set_module_type(avicap32 win32dll)

target_link_libraries(avicap32
    ${CMAKE_CURRENT_BINARY_DIR}/avicap32.def
    wine)

add_importlibs(avicap32 advapi32 user32 version kernel32 ntdll)
add_dependencies(avicap32 avicap32_def)
add_cab_target(avicap32 1)
