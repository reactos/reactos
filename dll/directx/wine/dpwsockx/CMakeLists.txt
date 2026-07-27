
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

spec2def(dpwsockx.dll dpwsockx.spec)

list(APPEND SOURCE
    dpwsockx_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/dpwsockx_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dpwsockx.def)

add_library(dpwsockx MODULE ${SOURCE} version.rc)
set_module_type(dpwsockx win32dll)
target_link_libraries(dpwsockx wine dxguid uuid oldnames)
add_importlibs(dpwsockx ws2_32 kernel32_vista ucrtbase msvcrt kernel32 ntdll)
add_dependencies(dpwsockx wineheaders)
add_cd_file(TARGET dpwsockx DESTINATION reactos/system32 FOR all)
set_wine_module(dpwsockx)
