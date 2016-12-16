
include_directories(${SHIMLIB_DIR})

spec2def(aclayers.dll layer.spec)

list(APPEND SOURCE
    dispmode.c
    versionlie.c
    main.c
    layer.spec)

add_library(aclayers SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/aclayers.def)

set_module_type(aclayers win32dll)
target_link_libraries(aclayers shimlib)
add_importlibs(aclayers user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET aclayers DESTINATION reactos/AppPatch FOR all)
