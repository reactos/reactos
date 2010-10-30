
set_unicode()

add_definitions(-DLANGPACK)
include_directories(include)

spec2def(lpk ${CMAKE_CURRENT_SOURCE_DIR}/lpk.spec ${CMAKE_CURRENT_BINARY_DIR}/lpk.def)

add_library(lpk SHARED dllmain.c stub.c lpk.rc)

set_module_type(lpk win32dll)

target_link_libraries(lpk ${CMAKE_CURRENT_BINARY_DIR}/lpk.def)

add_importlibs(lpk user32 usp10 kernel32 ntdll)
add_dependencies(lpk lpk_def)
add_cab_target(lpk 1)
