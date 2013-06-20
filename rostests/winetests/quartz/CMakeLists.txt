
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

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

add_executable(quartz_winetest ${SOURCE})
target_link_libraries(quartz_winetest wine uuid)
set_module_type(quartz_winetest win32cui)
add_importlibs(quartz_winetest ole32 oleaut32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET quartz_winetest DESTINATION reactos/bin FOR all)
