
remove_definitions(-D__ROS_LONG64__)
add_definitions(-DUSE_WINE_TODOS -DWINETEST_USE_DBGSTR_LONGLONG -D__WINESRC__)

list(APPEND SOURCE
    shcore.c
    testlist.c)

add_executable(shcore_winetest ${SOURCE})
target_link_libraries(shcore_winetest uuid)
set_module_type(shcore_winetest win32cui)
add_importlibs(shcore_winetest shell32 ole32 advapi32 msvcrt kernel32)

if(MSVC)
    add_importlibs(shcore_winetest ntdll)
endif()

add_rostests_file(TARGET shcore_winetest)
