
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(samlib ${CMAKE_CURRENT_SOURCE_DIR}/samlib.spec ${CMAKE_CURRENT_BINARY_DIR}/samlib.def)

add_library(samlib SHARED dllmain.c samlib.c samlib.rc)

set_module_type(samlib win32dll)

target_link_libraries(samlib ${CMAKE_CURRENT_BINARY_DIR}/samlib.def)

add_importlibs(samlib advapi32 kernel32 ntdll)
add_dependencies(samlib samlib_def)
add_cab_target(samlib 1)
