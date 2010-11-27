
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(objsel SHARED
    factory.c
    objsel.c
    regsvr.c
    objsel.rc
    objsel.def)

if(NOT MSVC)
    set_source_files_properties(objsel.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(objsel win32dll)

target_link_libraries(objsel
    uuid
    wine)

add_importlibs(objsel
    ole32
    user32
    advapi32
    msvcrt
    kernel32
    ntdll)

add_cab_target(objsel 1)
