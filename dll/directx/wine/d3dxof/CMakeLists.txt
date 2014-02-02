
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(d3dxof.dll d3dxof.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    d3dxof.c
    main.c
    mszip.c
    parsing.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3dxof.def)

add_library(d3dxof SHARED ${SOURCE} version.rc)
set_module_type(d3dxof win32dll)
target_link_libraries(d3dxof dxguid uuid wine)
add_importlibs(d3dxof msvcrt kernel32 ntdll)
add_cd_file(TARGET d3dxof DESTINATION reactos/system32 FOR all)
