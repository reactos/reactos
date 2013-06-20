
add_definitions(-D__ROS_LONG64__)

list(APPEND SOURCE
    asmcache.c
    asmenum.c
    asmname.c
    fusion.c
    testlist.c)

add_executable(fusion_winetest ${SOURCE})
target_link_libraries(fusion_winetest wine)
set_module_type(fusion_winetest win32cui)
add_importlibs(fusion_winetest user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET fusion_winetest DESTINATION reactos/bin FOR all)

if(NOT MSVC)
    allow_warnings(fusion_winetest)
endif()
