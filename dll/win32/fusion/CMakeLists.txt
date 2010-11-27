
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(fusion.dll fusion.spec)

add_library(fusion SHARED
    asmcache.c
    asmenum.c
    asmname.c
    assembly.c
    fusion.c
    fusion_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/fusion.def)

set_module_type(fusion win32dll)

target_link_libraries(fusion wine)

add_importlibs(fusion shlwapi advapi32 dbghelp user32 msvcrt kernel32 ntdll)

add_cab_target(fusion 1)
