
spec2def(user32_vista.dll user32_vista.spec ADD_IMPORTLIB)

include_directories(
    include
    ${REACTOS_SOURCE_DIR}/win32ss/include)

list(APPEND SOURCE
    dpi.c)

add_library(user32_vista_static
    ${SOURCE})
target_link_libraries(user32_vista_static win32ksys)
add_dependencies(user32_vista_static psdk)

add_library(user32_vista MODULE
    ${SOURCE}
    user32_vista.rc
    ${CMAKE_CURRENT_BINARY_DIR}/user32_vista.def)

set_module_type(user32_vista win32dll UNICODE ENTRYPOINT 0)
add_importlibs(user32_vista user32 gdi32 ntdll)
target_link_libraries(user32_vista user32_vista_static win32ksys)
add_dependencies(user32_vista psdk)
add_cd_file(TARGET user32_vista DESTINATION reactos/system32 FOR all)
