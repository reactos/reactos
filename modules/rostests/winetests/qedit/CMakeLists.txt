
add_definitions(-DUSE_WINE_TODOS -DCONST_VTABLE= -DWINETEST_WINE_DBG_STUBS)
add_executable(qedit_winetest
    mediadet.c
    nullrenderer.c
    samplegrabber.c
    timeline.c
    testlist.c
    qedit.rc)
target_link_libraries(qedit_winetest strmbase strmiids uuid)
set_module_type(qedit_winetest win32cui)
add_importlibs(qedit_winetest oleaut32 ole32 user32 gdi32 advapi32 kernel32_vista msvcrt kernel32 ntdll)
add_rostests_file(TARGET qedit_winetest)
