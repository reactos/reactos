
add_definitions(
    -D_WINE
    -D_USE_MATH_DEFINES
    -D__WINESRC__)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(dsound.dll dsound.spec ADD_IMPORTLIB)

add_library(dsound MODULE
    buffer.c
    capture.c
    dsound.c
    dsound_convert.c
    dsound_main.c
    duplex.c
    mixer.c
    primary.c
    propset.c
    sound3d.c
    guid.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dsound.def)

set_module_type(dsound win32dll)
target_link_libraries(dsound dxguid uuid wine)
add_importlibs(dsound winmm advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET dsound DESTINATION reactos/system32 FOR all)
