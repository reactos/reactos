
spec2def(storport.sys storport.spec ADD_IMPORTLIB)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(
    -D_WIN32_WINNT=0x602
    -DNTDDI_VERSION=0x06020000) # NTDDI_WIN8

list(APPEND SOURCE
    fdo.c
    ioctl.c
    miniport.c
    misc.c
    pdo.c
    srb.c
    srbex.c
    storport.c
    stubs.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(storport MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    storport.rc
    ${CMAKE_CURRENT_BINARY_DIR}/storport.def)
add_pch(storport precomp.h "${PCH_SKIP_SOURCE}")

set_module_type(storport kernelmodedriver)
target_link_libraries(storport sptilib ${PSEH_LIB})
add_importlibs(storport ntoskrnl hal)
add_cd_file(TARGET storport DESTINATION reactos/system32/drivers NO_CAB FOR all)
