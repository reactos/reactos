
add_definitions(-D__WINESRC__ -DWINETEST_USE_DBGSTR_LONGLONG)

if(MSVC)
    # Disable warning C4477 (printf format warnings)
    add_compile_flags("/wd4477")
endif()

add_executable(oleacc_winetest main.c testlist.c)
set_module_type(oleacc_winetest win32cui)
target_link_libraries(oleacc_winetest uuid)
add_importlibs(oleacc_winetest oleacc oleaut32 ole32 user32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET oleacc_winetest)
