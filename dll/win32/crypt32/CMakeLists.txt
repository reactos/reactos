
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -D_WINE
    -D_CRYPT32_)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(crypt32.dll crypt32.spec ADD_IMPORTLIB)

if(MSVC)
    # error C4312: 'type cast': conversion from 'unsigned int' to 'void *' of greater size
    replace_compile_flags("/we4312" " ")
endif()

list(APPEND SOURCE
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
    precomp.h)

add_library(crypt32 MODULE
    ${SOURCE}
    crypt32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/crypt32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/crypt32.def)

set_module_type(crypt32 win32dll)
target_link_libraries(crypt32 wine ${PSEH_LIB} oldnames)
add_delay_importlibs(crypt32 cryptnet)
add_importlibs(crypt32 bcrypt user32 advapi32 advapi32_vista msvcrt kernel32 ntdll)
add_pch(crypt32 precomp.h SOURCE)
add_cd_file(TARGET crypt32 DESTINATION reactos/system32 FOR all)
