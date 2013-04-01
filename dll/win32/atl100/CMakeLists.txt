
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(atl100.dll atl100.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    atl.c
    atl_ax.c
    registrar.c
    ${CMAKE_CURRENT_BINARY_DIR}/atl100_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/atl100.def)

add_library(atl100 SHARED ${SOURCE})
set_module_type(atl100 win32dll)
target_link_libraries(atl100 uuid wine)

add_importlibs(atl100 ole32 oleaut32 user32 gdi32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET atl100 DESTINATION reactos/system32 FOR all)
