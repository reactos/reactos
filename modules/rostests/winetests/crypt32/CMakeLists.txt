
add_definitions(-DUSE_WINE_TODOS)

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(
        /wd4101
        /wd5287
        /wd4477
)
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
       CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
        -Wno-format
        -Wno-unused-variable
)
endif()

list(APPEND SOURCE
    base64.c
    cert.c
    chain.c
    crl.c
    ctl.c
    encode.c
    main.c
    message.c
    msg.c
    object.c
    oid.c
    protectdata.c
    sip.c
    store.c
    str.c)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(crypt32_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

set_module_type(crypt32_winetest win32cui)
add_importlibs(crypt32_winetest crypt32 advapi32 user32 shlwapi shell32 msvcrt kernel32)
add_pch(crypt32_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET crypt32_winetest)
