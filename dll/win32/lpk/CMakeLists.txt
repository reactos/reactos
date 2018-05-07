
add_definitions(-DLANGPACK)
include_directories(include)
spec2def(lpk.dll lpk.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    bidi.c
    dllmain.c
    stub.c
    lpk.c
    ros_lpk.h)

add_library(lpk SHARED
    ${SOURCE}
    lpk.rc
    ${CMAKE_CURRENT_BINARY_DIR}/lpk.def)

set_module_type(lpk win32dll UNICODE)

target_link_libraries(lpk
    wine)

add_importlibs(lpk usp10 user32 gdi32 msvcrt kernel32 ntdll)
add_pch(lpk ros_lpk.h SOURCE)
add_cd_file(TARGET lpk DESTINATION reactos/system32 FOR all)
