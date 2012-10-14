
add_definitions(
    -D_WINE
    -D_USE_MATH_DEFINES)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dsound.dll dsound.spec ADD_IMPORTLIB)

add_library(dsound SHARED
    buffer.c
    capture.c
    dsound.c
    dsound_convert.c
    dsound_main.c
    duplex.c
    mixer.c
    primary.c
    propset.c
    regsvr.c
    sound3d.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dsound.def)

set_module_type(dsound win32dll)

target_link_libraries(dsound
    dxguid
    uuid
    wine)

add_importlibs(dsound
    msvcrt
    user32
    advapi32
    ole32
    winmm
    kernel32
    ntdll)

add_cd_file(TARGET dsound DESTINATION reactos/system32 FOR all)

