

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dplay.dll dplay.spec)

add_library(dplay SHARED
    dplay_main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dplay.def)

set_module_type(dplay win32dll UNICODE)

target_link_libraries(dplay
    uuid
    wine)

add_importlibs(dplay
    msvcrt
    user32
    advapi32
    ole32
    winmm
    dinput
    dplayx
    kernel32
    ntdll)

add_dependencies(dplay psdk)

add_cd_file(TARGET dplay DESTINATION reactos/system32 FOR all)

