
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND COMMON_SOURCE
    asmcache.c
    asmenum.c
    asmname.c
    assembly.c
    fusion.c
    precomp.h)

add_library(fusion_common STATIC ${COMMON_SOURCE})
add_dependencies(fusion_common psdk)

add_library(fusion MODULE
    version.rc)
spec2def(fusion fusion.dll fusion.spec)

set_module_type(fusion win32dll)
target_link_libraries(fusion fusion_common uuid wine)
add_importlibs(fusion advapi32 dbghelp shlwapi user32 msvcrt kernel32 ntdll)
add_pch(fusion_common precomp.h COMMON_SOURCE)
add_cd_file(TARGET fusion DESTINATION reactos/Microsoft.NET/Framework/v1.0.3705 FOR all)
add_cd_file(TARGET fusion DESTINATION reactos/system32 FOR all)

add_subdirectory(fusion_1_1)
add_subdirectory(fusion_2_0)
