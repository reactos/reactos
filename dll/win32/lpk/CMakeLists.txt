
set_unicode()

add_definitions(-DLANGPACK)
include_directories(include)

spec2def(lpk.dll lpk.spec)

add_library(lpk SHARED dllmain.c stub.c lpk.rc ${CMAKE_CURRENT_BINARY_DIR}/lpk.def)

set_module_type(lpk win32dll)

add_importlibs(lpk user32 usp10 kernel32 ntdll)

add_cab_target(lpk 1)
