
list(APPEND SOURCE
    avisplitter.c
    dsoundrender.c
    filtergraph.c
    filtermapper.c
    memallocator.c
    misc.c
    referenceclock.c
    videorenderer.c
    testlist.c)

add_idl_headers(quartz_test_idlheader fil_data.idl)
add_executable(quartz_winetest ${SOURCE})
target_link_libraries(quartz_winetest uuid)
add_dependencies(quartz_winetest quartz_test_idlheader)
set_module_type(quartz_winetest win32cui)
add_importlibs(quartz_winetest ole32 oleaut32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET quartz_winetest DESTINATION reactos/bin FOR all)
