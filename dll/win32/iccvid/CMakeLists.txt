
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(iccvid SHARED iccvid.c rsrc.rc iccvid.def)

if(NOT MSVC)
    set_source_files_properties(iccvid.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(iccvid win32dll)

target_link_libraries(iccvid wine)

add_importlibs(iccvid user32 msvcrt kernel32 ntdll)

add_cab_target(iccvid 1)
