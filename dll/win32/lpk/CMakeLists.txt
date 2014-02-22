

add_definitions(-DLANGPACK)
include_directories(include)

spec2def(lpk.dll lpk.spec)

list(APPEND SOURCE
    dllmain.c
    stub.c
    ros_lpk.h)

add_library(lpk SHARED
    ${SOURCE}
    lpk.rc
    ${CMAKE_CURRENT_BINARY_DIR}/lpk.def)

set_module_type(lpk win32dll UNICODE)


add_importlibs(lpk user32 usp10 msvcrt kernel32 ntdll)
add_pch(lpk ros_lpk.h SOURCE)
add_cd_file(TARGET lpk DESTINATION reactos/system32 FOR all)
