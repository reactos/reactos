
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(fusion.dll fusion.spec)

list(APPEND SOURCE
    asmcache.c
    asmenum.c
    asmname.c
    assembly.c
    fusion.c
    fusionpriv.h
    ${CMAKE_CURRENT_BINARY_DIR}/fusion_stubs.c)

add_library(fusion SHARED
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/fusion.def)

set_module_type(fusion win32dll)
target_link_libraries(fusion uuid wine)
add_importlibs(fusion advapi32 dbghelp shlwapi user32 msvcrt kernel32 ntdll)
add_pch(fusion fusionpriv.h SOURCE)
add_cd_file(TARGET fusion DESTINATION reactos/Microsoft.NET/Framework/v1.0.3705 FOR all)
add_cd_file(TARGET fusion DESTINATION reactos/Microsoft.NET/Framework/v1.1.4322 FOR all)
add_cd_file(TARGET fusion DESTINATION reactos/Microsoft.NET/Framework/v2.0.50727 FOR all)
add_cd_file(TARGET fusion DESTINATION reactos/system32 FOR all)
