
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(fusion.dll fusion.spec)

list(APPEND COMMON_SOURCE
    asmcache.c
    asmenum.c
    asmname.c
    assembly.c
    fusion.c
    fusionpriv.h
    ${CMAKE_CURRENT_BINARY_DIR}/fusion_stubs.c)

add_library(fusion_common STATIC ${COMMON_SOURCE})
add_dependencies(fusion_common psdk)

add_library(fusion SHARED
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/fusion.def)

set_module_type(fusion win32dll)
target_link_libraries(fusion fusion_common uuid wine)
add_importlibs(fusion advapi32 dbghelp shlwapi user32 msvcrt kernel32 ntdll)
add_pch(fusion_common fusionpriv.h COMMON_SOURCE)
add_cd_file(TARGET fusion DESTINATION reactos/Microsoft.NET/Framework/v1.0.3705 FOR all)
add_cd_file(TARGET fusion DESTINATION reactos/system32 FOR all)

add_subdirectory(fusion_1_1)
add_subdirectory(fusion_2_0)
