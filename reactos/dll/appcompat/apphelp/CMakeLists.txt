
include_directories(${SHIMLIB_DIR})

remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

spec2def(apphelp.dll apphelp.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    apphelp.c
    hsdb.c
    layer.c
    sdbapi.c
    sdbfileattr.c
    sdbread.c
    sdbstringtable.c
    sdbwrite.c
    apphelp.spec
    apphelp.h
    ${CMAKE_CURRENT_BINARY_DIR}/apphelp_stubs.c)

add_library(apphelp SHARED
    ${SOURCE}
    apphelp.rc
    ${CMAKE_CURRENT_BINARY_DIR}/apphelp.def)

set_module_type(apphelp win32dll)
target_link_libraries(apphelp wine)
add_delay_importlibs(apphelp version)
add_importlibs(apphelp msvcrt kernel32 ntdll)
add_cd_file(TARGET apphelp DESTINATION reactos/system32 FOR all)
