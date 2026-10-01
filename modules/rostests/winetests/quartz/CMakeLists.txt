
# __WINESRC__ switches unknwn.h to the inline COBJMACROS wrappers, which the
# tests rely on when calling e.g. IPin_AddRef() on a void * out parameter.
add_definitions(-DWINETEST_USE_DBGSTR_LONGLONG -DCONST_VTABLE= -D__WINESRC__ -DWINETEST_WINE_DBG_STUBS -Dwcsdup=_wcsdup)

list(APPEND SOURCE
    acmwrapper.c
    avidec.c
    avisplit.c
    dsoundrender.c
    filesource.c
    filtergraph.c
    filtermapper.c
    memallocator.c
    mpegaudio.c
    mpeglayer3.c
    mpegsplit.c
    mpegvideo.c
    passthrough.c
    systemclock.c
    videorenderer.c
    vmr7.c
    vmr9.c
    waveparser.c
    testlist.c)

add_idl_headers(quartz_test_idlheader fil_data.idl)
add_executable(quartz_winetest ${SOURCE} rsrc.rc)
target_link_libraries(quartz_winetest strmbase strmiids uuid dxguid)
add_dependencies(quartz_winetest dxsdk quartz_test_idlheader)
set_module_type(quartz_winetest win32cui)
add_importlibs(quartz_winetest
    ole32 oleaut32 advapi32 user32 gdi32 msvfw32 winmm
    d3d9 ddraw dsound msdmo kernel32_vista msvcrt kernel32 ntdll)

if(MSVC)
    add_importlibs(quartz_winetest ntdll)
endif()

add_rostests_file(TARGET quartz_winetest)
