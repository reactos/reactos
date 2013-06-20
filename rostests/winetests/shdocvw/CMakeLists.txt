
add_definitions(-D__ROS_LONG64__)

list(APPEND SOURCE
    shdocvw.c
    shortcut.c
    testlist.c)

add_executable(shdocvw_winetest ${SOURCE})
target_link_libraries(shdocvw_winetest wine uuid)
set_module_type(shdocvw_winetest win32cui)
add_importlibs(shdocvw_winetest gdi32 shell32 ole32 oleaut32 user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET shdocvw_winetest DESTINATION reactos/bin FOR all)

if(NOT MSVC)
    allow_warnings(shdocvw_winetest)
endif()
