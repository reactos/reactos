
list(APPEND SOURCE
    shcore.c
)

add_executable(shcore_winetest ${SOURCE})
set_module_type(shcore_winetest win32cui)
target_compile_definitions(shcore_winetest PRIVATE CONST_VTABLE STANDALONE WINETEST_USE_DBGSTR_LONGLONG)
target_link_libraries(shcore_winetest uuid)
add_importlibs(shcore_winetest advapi32 ws2_32 crypt32 secur32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET shcore_winetest)
