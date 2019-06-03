
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    mspatcha_main.c
    lzx.c
    lzx.h)

add_library(mspatcha MODULE ${SOURCE} version.rc)
spec2def(mspatcha mspatcha.dll mspatcha.spec ADD_IMPORTLIB)

set_module_type(mspatcha win32dll)
target_link_libraries(mspatcha wine)
add_importlibs(mspatcha msvcrt kernel32 ntdll)
add_cd_file(TARGET mspatcha DESTINATION reactos/system32 FOR all)
