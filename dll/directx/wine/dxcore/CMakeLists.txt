
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

spec2def(dxcore.dll dxcore.spec ADD_IMPORTLIB)

add_library(dxcore MODULE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/dxcore.def)

set_module_type(dxcore win32dll)
target_link_libraries(dxcore wine)
add_importlibs(dxcore msvcrt kernel32 ntdll)
add_dependencies(dxcore wineheaders psdk)
add_cd_file(TARGET dxcore DESTINATION reactos/system32 FOR all)
set_wine_module(dxcore)
