
set_unicode()

add_definitions(-DLANGPACK)
include_directories(include)

spec2def(lpk.dll lpk.spec)

list(APPEND SOURCE
    dllmain.c
    stub.c
    lpk.rc
    ${CMAKE_CURRENT_BINARY_DIR}/lpk.def)

add_library(lpk SHARED ${SOURCE})

set_module_type(lpk win32dll)

add_importlib_target(lpk.spec)
add_importlibs(lpk user32 usp10 msvcrt kernel32 ntdll)
add_cd_file(TARGET lpk DESTINATION reactos/system32 FOR all)
