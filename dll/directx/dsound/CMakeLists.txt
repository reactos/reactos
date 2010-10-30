
add_definitions(-D_WINE)
add_definitions(-D_DLL -D__USE_CRTIMP)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dsound ${CMAKE_CURRENT_SOURCE_DIR}/dsound.spec ${CMAKE_CURRENT_BINARY_DIR}/dsound.def)

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
    version.rc)

set_module_type(dsound win32dll)

target_link_libraries(dsound
    ${CMAKE_CURRENT_BINARY_DIR}/dsound.def
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

add_dependencies(dsound dsound_def)
add_cab_target(dsound 1)
