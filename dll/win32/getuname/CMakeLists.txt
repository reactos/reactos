
set_unicode()

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(getuname ${CMAKE_CURRENT_SOURCE_DIR}/getuname.spec ${CMAKE_CURRENT_BINARY_DIR}/getuname.def)

add_library(getuname SHARED getuname.c getuname.rc)

set_module_type(getuname win32dll)

target_link_libraries(getuname ${CMAKE_CURRENT_BINARY_DIR}/getuname.def)

add_importlibs(getuname kernel32 ntdll)
add_dependencies(getuname getuname_def)
add_cab_target(getuname 1)
