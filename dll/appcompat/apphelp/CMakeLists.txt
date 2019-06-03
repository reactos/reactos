
include_directories(${SHIMLIB_DIR})

remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x601 -DWINVER=0x601)
#add_definitions(-DSDBAPI_DEBUG_ALLOC)

list(APPEND SOURCE
    apphelp.c
    dbgheap.c
    hsdb.c
    layer.c
    sdbapi.c
    sdbfileattr.c
    sdbread.c
    sdbstringtable.c
    sdbwrite.c
    shimeng.c
    apphelp.spec)

list(APPEND HEADERS
    apphelp.h
    sdbpapi.h
    sdbstringtable.h
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/appcompat/sdbtagid.h
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/appcompat/sdbtypes.h
    sdbwrite.h
    shimeng.h)

add_library(apphelp MODULE
    ${SOURCE}
    ${HEADERS}
    apphelp.rc)
spec2def(apphelp apphelp.dll apphelp.spec ADD_IMPORTLIB)

set_module_type(apphelp win32dll UNICODE ENTRYPOINT DllMain 12)
add_delay_importlibs(apphelp version)
add_importlibs(apphelp kernel32 ntdll)
add_cd_file(TARGET apphelp DESTINATION reactos/system32 FOR all)
