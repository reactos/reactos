
include_directories(${SHIMLIB_DIR})

list(APPEND SOURCE
    dispmode.c
    forcedxsetupsuccess.c
    ignoreloadlibrary.c
    versionlie.c
    vmhorizon.c
    main.c
    layer.spec)

add_library(aclayers MODULE ${SOURCE})
spec2def(aclayers aclayers.dll layer.spec)

set_module_type(aclayers win32dll ENTRYPOINT DllMain 12)
target_link_libraries(aclayers shimlib)
add_importlibs(aclayers user32 kernel32 ntdll)
add_cd_file(TARGET aclayers DESTINATION reactos/AppPatch FOR all)
