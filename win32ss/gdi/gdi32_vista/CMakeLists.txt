
include_directories(
    include
    ${REACTOS_SOURCE_DIR}/win32ss/include)

spec2def(gdi32_vista.dll gdi32_vista.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    dc.c)

add_library(gdi32_vista MODULE
    ${SOURCE}
    gdi32_vista.rc
    ${CMAKE_CURRENT_BINARY_DIR}/gdi32_vista.def)

set_module_type(gdi32_vista
    win32dll
    ENTRYPOINT 0
    UNICODE)

target_link_libraries(gdi32_vista win32ksys)

#add_importlibs(gdi32_vista user32 advapi32 kernel32 ntdll)
#add_pch(gdi32_vista include/precomp.h SOURCE)
add_dependencies(gdi32_vista psdk)
add_cd_file(TARGET gdi32_vista DESTINATION reactos/system32 FOR all)
