
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_idl_headers(atl_atliface_header atliface.idl)
generate_idl_iids(atliface.idl)

set_rc_compiler()
spec2def(atl.dll atl.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    atl_ax.c
    atl_main.c
    registrar.c
    stubs.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/atliface_i.c
    ${CMAKE_CURRENT_BINARY_DIR}/atl.def)

add_library(atl SHARED ${SOURCE})

set_module_type(atl win32dll)

target_link_libraries(atl uuid wine)

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
add_cd_file(TARGET atl DESTINATION reactos/system32 FOR all)
