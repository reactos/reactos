
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(fusion ${CMAKE_CURRENT_SOURCE_DIR}/fusion.spec ${CMAKE_CURRENT_BINARY_DIR}/fusion.def)

add_library(fusion SHARED
    asmcache.c
    asmenum.c
    asmname.c
    assembly.c
    fusion.c
    fusion_main.c)

set_module_type(fusion win32dll)

target_link_libraries(fusion
    ${CMAKE_CURRENT_BINARY_DIR}/fusion.def
    wine)

add_importlibs(fusion shlwapi advapi32 dbghelp user32 kernel32 ntdll)
add_dependencies(fusion fusion_def)
add_livecd_target(fusion reactos/system32)
