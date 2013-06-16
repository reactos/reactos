
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(atl.dll atl.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    atl_main.c
    registrar.c
    ${CMAKE_CURRENT_BINARY_DIR}/atl_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/atl.def)

add_library(atl SHARED ${SOURCE} rsrc.rc)
set_module_type(atl win32dll)
target_link_libraries(atl uuid wine)
add_importlibs(atl atl100 oleaut32 ole32 user32 msvcrt kernel32 ntdll)
add_dependencies(atl atl_atliface_header)
add_cd_file(TARGET atl DESTINATION reactos/system32 FOR all)
