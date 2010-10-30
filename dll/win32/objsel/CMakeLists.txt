
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(objsel ${CMAKE_CURRENT_SOURCE_DIR}/objsel.spec ${CMAKE_CURRENT_BINARY_DIR}/objsel.def)

add_library(objsel SHARED
    factory.c
    objsel.c
    regsvr.c
    objsel.rc)

set_module_type(objsel win32dll)

target_link_libraries(objsel
    ${CMAKE_CURRENT_BINARY_DIR}/objsel.def
    uuid
    wine)

add_importlibs(objsel
    ole32
    user32
    advapi32
    kernel32
    ntdll)

add_dependencies(objsel objsel_def)
add_cab_target(objsel 1)
