
add_definitions(
    -D__WINESRC__
    -D_WINE
    -D_CRYPT32_)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(crypt32.dll crypt32.spec ADD_IMPORTLIB)

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
    ${CMAKE_CURRENT_BINARY_DIR}/crypt32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/crypt32.def)

add_library(crypt32 SHARED ${SOURCE} crypt32.rc)

if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #allow_warnings(crypt32)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-error")
endif()

set_module_type(crypt32 win32dll)
target_link_libraries(crypt32 wine ${PSEH_LIB} oldnames)
add_importlibs(crypt32 user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET crypt32 DESTINATION reactos/system32 FOR all)
