
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    base64.c
    cert.c
    chain.c
    crl.c
    ctl.c
    encode.c
    main.c
    message.c
    msg.c
    object.c
    oid.c
    protectdata.c
    sip.c
    store.c
    str.c
    testlist.c)

add_executable(crypt32_winetest ${SOURCE})
target_link_libraries(crypt32_winetest wine)
set_module_type(crypt32_winetest win32cui)
add_importlibs(crypt32_winetest crypt32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET crypt32_winetest DESTINATION reactos/bin FOR all)
