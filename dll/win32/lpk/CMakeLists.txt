
set_unicode()

add_definitions(-DLANGPACK)
include_directories(include)

add_library(lpk SHARED
    dllmain.c
    stub.c
    lpk.rc
    ${CMAKE_CURRENT_BINARY_DIR}/lpk.def)

set_export_spec(lpk lpk.spec)

add_importlib_target(${CMAKE_CURRENT_BINARY_DIR}/lpk.def)

set_module_type(lpk win32dll)

add_importlibs(lpk user32 usp10 kernel32 ntdll)

add_cab_target(lpk 1)
