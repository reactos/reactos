
include_directories(
    ${REACTOS_SOURCE_DIR}/modules/rostests/apitests/include)

list(APPEND SOURCE
    pathcch_compile.c
    pathcch_compile.cpp
    precomp.h)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(pathcch_static_unittest ${SOURCE} ${PCH_SKIP_SOURCE})
add_pch(pathcch_static_unittest precomp.h ${PCH_SKIP_SOURCE})
target_compile_definitions(pathcch_static_unittest PRIVATE STATIC_PATHCCH)
set_module_type(pathcch_static_unittest win32cui)
add_dependencies(pathcch_static_unittest psdk)
target_link_libraries(pathcch_static_unittest pathcch_static)
add_importlibs(pathcch_static_unittest msvcrt kernel32 ntdll)
add_rostests_file(TARGET pathcch_static_unittest)

add_executable(pathcch_dyn1_unittest ${SOURCE} ${PCH_SKIP_SOURCE})
add_pch(pathcch_dyn1_unittest precomp.h ${PCH_SKIP_SOURCE})
set_module_type(pathcch_dyn1_unittest win32cui)
add_dependencies(pathcch_dyn1_unittest psdk)
target_link_libraries(pathcch_dyn1_unittest pathcch_kernelbase)
add_importlibs(pathcch_dyn1_unittest msvcrt kernel32 ntdll)
## add_rostests_file(TARGET pathcch_dyn1_unittest) ## TODO: Requires kernelbase.dll to be compiled

add_executable(pathcch_dyn2_unittest ${SOURCE} ${PCH_SKIP_SOURCE})
add_pch(pathcch_dyn2_unittest precomp.h ${PCH_SKIP_SOURCE})
set_module_type(pathcch_dyn2_unittest win32cui)
add_dependencies(pathcch_dyn2_unittest psdk)
target_link_libraries(pathcch_dyn2_unittest pathcch)
add_importlibs(pathcch_dyn2_unittest msvcrt kernel32 ntdll)
## add_rostests_file(TARGET pathcch_dyn2_unittest) ## TODO: Requires api-ms-win-core-path-l1-1-0.dll
