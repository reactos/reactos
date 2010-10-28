
ADD_INTERFACE_DEFINITIONS(atl_atliface_header atliface.idl)

add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(atl ${CMAKE_CURRENT_SOURCE_DIR}/atl.spec ${CMAKE_CURRENT_BINARY_DIR}/atl.def)

add_library(atl SHARED
    atl_ax.c
    atl_main.c
    registrar.c
    rsrc.rc)

set_module_type(atl win32dll)

target_link_libraries(atl
    ${CMAKE_CURRENT_BINARY_DIR}/atl.def
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

add_dependencies(atl atl_atliface_header atl_def)
add_livecd_target(atl reactos/system32)
