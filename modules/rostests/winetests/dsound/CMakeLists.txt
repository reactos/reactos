
list(APPEND SOURCE
    capture.c
    ds3d8.c
    ds3d.c
    dsound8.c
    dsound.c
    duplex.c
    propset.c)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(dsound_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

target_link_libraries(dsound_winetest uuid dxguid)
set_module_type(dsound_winetest win32cui)
add_importlibs(dsound_winetest ole32 user32 msvcrt kernel32)
add_pch(dsound_winetest dsound_test.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET dsound_winetest)
