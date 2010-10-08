
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(resutils ${CMAKE_CURRENT_SOURCE_DIR}/resutils.spec ${CMAKE_CURRENT_BINARY_DIR}/resutils.def)

add_library(resutils SHARED resutils.c)

set_module_type(resutils win32dll)

target_link_libraries(resutils
    ${CMAKE_CURRENT_BINARY_DIR}/resutils.def
    wine)

add_importlibs(resutils kernel32 ntdll)
add_dependencies(resutils resutils_def)
