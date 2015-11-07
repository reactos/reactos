
add_definitions(-DLE)
spec2def(ws2_32_new.dll ws2_32.spec)

include_directories(
    inc
    ${REACTOS_SOURCE_DIR}/include/reactos/winsock)

list(APPEND SOURCE
    src/addrconv.c
    src/addrinfo.c
    src/async.c
    src/bhook.c
    src/dcatalog.c
    src/dcatitem.c
    src/dllmain.c
    src/dprocess.c
    src/dprovide.c
    src/dsocket.c
    src/dthread.c
    src/dupsock.c
    src/enumprot.c
    src/event.c
    src/getproto.c
    src/getxbyxx.c
    src/ioctl.c
    src/nscatalo.c
    src/nscatent.c
    src/nspinstl.c
    src/nsprovid.c
    src/nsquery.c
    src/qos.c
    src/qshelpr.c
    src/rasdial.c
    src/recv.c
    src/rnr.c
    #src/scihlpr.c
    src/select.c
    src/send.c
    src/sockctrl.c
    src/socklife.c
    src/spinstal.c
    src/sputil.c
    src/startup.c
    src/wsautil.c
    inc/ws2_32.h)

add_library(ws2_32_new SHARED
    ${SOURCE}
    ws2_32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ws2_32_new.def)

set_module_type(ws2_32_new win32dll)
target_link_libraries(ws2_32_new ${PSEH_LIB})
add_importlibs(ws2_32_new user32 advapi32 dnsapi ws2help msvcrt kernel32 ntdll)
add_pch(ws2_32_new inc/ws2_32.h SOURCE)
add_cd_file(TARGET ws2_32_new DESTINATION reactos/system32 FOR all)
