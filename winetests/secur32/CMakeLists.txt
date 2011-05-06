
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    main.c
    ntlm.c
    schannel.c
    secur32.c
    testlist.c)

add_executable(secur32_winetest ${SOURCE})
target_link_libraries(secur32_winetest wine)
set_module_type(secur32_winetest win32cui)
add_importlibs(secur32_winetest advapi32 msvcrt kernel32 ntdll)
add_cab_target(secur32_winetest 7)
