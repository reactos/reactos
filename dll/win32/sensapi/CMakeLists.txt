
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

add_library(sensapi SHARED sensapi.c sensapi.def)

if(NOT MSVC)
    set_source_files_properties(sensapi.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(sensapi win32dll)

target_link_libraries(sensapi wine)

add_importlibs(sensapi msvcrt kernel32 ntdll)

add_cab_target(sensapi 1)
