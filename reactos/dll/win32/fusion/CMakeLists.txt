
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()

spec2def(fusion.dll fusion.spec)

list(APPEND SOURCE
    asmcache.c
    asmenum.c
    asmname.c
    assembly.c
    fusion.c
    fusion_main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/fusion_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/fusion.def)

add_library(fusion SHARED ${SOURCE})

set_module_type(fusion win32dll)
target_link_libraries(fusion wine uuid)
add_importlibs(fusion shlwapi advapi32 dbghelp user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET fusion DESTINATION reactos/Microsoft.NET/Framework/v1.0.3705 FOR all)
add_cd_file(TARGET fusion DESTINATION reactos/Microsoft.NET/Framework/v1.1.4322 FOR all)
add_cd_file(TARGET fusion DESTINATION reactos/Microsoft.NET/Framework/v2.0.50727 FOR all)
