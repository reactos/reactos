
set_unicode()

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(t2embed.dll t2embed.spec)

add_library(t2embed SHARED t2embed.c t2embed.rc ${CMAKE_CURRENT_BINARY_DIR}/t2embed.def)

set_module_type(t2embed win32dll)

target_link_libraries(t2embed wine)

add_importlibs(t2embed gdi32 user32 advapi32 version kernel32 ntdll)


add_cab_target(t2embed 1)
