
add_definitions(
    -D__WINESRC__
    -D_DLL
    -D__USE_CRTIMP
    -D_WINE)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(crypt32 ${CMAKE_CURRENT_SOURCE_DIR}/crypt32.spec ${CMAKE_CURRENT_BINARY_DIR}/crypt32.def)

add_library(crypt32 SHARED
    base64.c
    cert.c
    chain.c
    collectionstore.c
    context.c
    crl.c
    ctl.c
    decode.c
    encode.c
    filestore.c
    main.c
    message.c
    msg.c
    object.c
    oid.c
    proplist.c
    protectdata.c
    provstore.c
    regstore.c
    rootstore.c
    serialize.c
    sip.c
    store.c
    str.c
    crypt32.rc
    version.rc)

set_module_type(crypt32 win32dll)

target_link_libraries(crypt32
    ${CMAKE_CURRENT_BINARY_DIR}/crypt32.def
    wine
    pseh
    oldnames)

add_importlibs(crypt32 user32 advapi32 msvcrt kernel32 ntdll)
add_dependencies(crypt32 crypt32_def)
add_livecd_target(crypt32 reactos/system32)
