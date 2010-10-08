
set_unicode()

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(t2embed ${CMAKE_CURRENT_SOURCE_DIR}/t2embed.spec ${CMAKE_CURRENT_BINARY_DIR}/t2embed.def)

add_library(t2embed SHARED t2embed.c t2embed.rc)

set_module_type(t2embed win32dll)

target_link_libraries(t2embed
    ${CMAKE_CURRENT_BINARY_DIR}/t2embed.def
    wine)

add_importlibs(t2embed gdi32 user32 advapi32 version kernel32 ntdll)

add_dependencies(t2embed t2embed_def)
