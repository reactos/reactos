
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-D__WINESRC__)
add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-D_WINE)

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

set_target_properties(crypt32 PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(crypt32
    ${CMAKE_CURRENT_BINARY_DIR}/crypt32.def
    wine
    pseh
    mingw_common
    -luser32
    -ladvapi32
    -lmsvcrt)

add_dependencies(crypt32 crypt32_def psdk)
