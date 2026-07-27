
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)

add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -D_WIN32_WINNT=0x0600
    -DWINVER=0x0600)

spec2def(dxva2.dll dxva2.spec ADD_IMPORTLIB)

add_library(dxva2 MODULE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/dxva2_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dxva2.def)

set_module_type(dxva2 win32dll)
target_link_libraries(dxva2 wine uuid)
add_importlibs(dxva2 ole32 user32 kernel32_vista msvcrt kernel32 ntdll)
add_dependencies(dxva2 wineheaders psdk)
add_cd_file(TARGET dxva2 DESTINATION reactos/system32 FOR all)
set_wine_module(dxva2)
