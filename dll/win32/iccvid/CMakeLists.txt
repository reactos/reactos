
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(iccvid ${CMAKE_CURRENT_SOURCE_DIR}/iccvid.spec ${CMAKE_CURRENT_BINARY_DIR}/iccvid.def)

add_library(iccvid SHARED iccvid.c rsrc.rc)

set_module_type(iccvid win32dll)

target_link_libraries(iccvid
    ${CMAKE_CURRENT_BINARY_DIR}/iccvid.def
    wine)

add_importlibs(iccvid user32 kernel32 ntdll)
add_dependencies(iccvid iccvid_def)
add_cab_target(iccvid 1)
