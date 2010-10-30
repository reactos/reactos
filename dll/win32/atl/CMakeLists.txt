
ADD_INTERFACE_DEFINITIONS(atl_atliface_header atliface.idl)

add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(atl.dll atl.spec)

add_library(atl SHARED
    atl_ax.c
    atl_main.c
    registrar.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/atl.def)

set_module_type(atl win32dll)

target_link_libraries(atl
    uuid
    wine)

add_importlibs(atl
    msvcrt
    ole32
    oleaut32
    user32
    advapi32
    gdi32
    kernel32
    ntdll)

add_dependencies(atl atl_atliface_header)
add_cab_target(atl 1)
