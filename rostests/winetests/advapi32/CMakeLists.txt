
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    cred.c
    crypt.c
    crypt_lmhash.c
    crypt_md4.c
    crypt_md5.c
    crypt_sha.c
    eventlog.c
    lsa.c
    registry.c
    security.c
    service.c
    testlist.c)

add_executable(advapi32_winetest ${SOURCE})

target_link_libraries(advapi32_winetest uuid)

set_module_type(advapi32_winetest win32cui)
add_importlibs(advapi32_winetest advapi32 ole32 msvcrt kernel32 ntdll)
add_cd_file(TARGET advapi32_winetest DESTINATION reactos/bin FOR all)
