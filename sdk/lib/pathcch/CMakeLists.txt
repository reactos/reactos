
# Down-level static PathCch library, to be used for
# programs that need to run on Windows 7 and below.
add_library(pathcch_static STATIC ${REACTOS_SOURCE_DIR}/dll/win32/kernelbase/wine/path.c)
add_dependencies(pathcch_static xdk)
target_compile_definitions(pathcch_static PRIVATE wcsnicmp=_wcsnicmp STATIC_PATHCCH)

# Windows 8+ compatible libraries, importing from either
# kernelbase.dll or api-ms-win-core-path-l1-1-0.dll
generate_import_lib(pathcch_kernelbase kernelbase.dll pathcch.spec "--version=${DLL_EXPORT_VERSION}" "")
generate_import_lib(pathcch api-ms-win-core-path-l1-1-0.dll pathcch.spec "--version=${DLL_EXPORT_VERSION}" "")


add_executable(pathcch_test_compile test_compile.c test_compile.cpp)
target_compile_definitions(pathcch_test_compile PRIVATE STATIC_PATHCCH)
set_module_type(pathcch_test_compile win32cui UNICODE)
add_dependencies(pathcch_test_compile psdk)
target_link_libraries(pathcch_test_compile pathcch_static)
add_importlibs(pathcch_test_compile msvcrt kernel32 ntdll)
add_cd_file(TARGET pathcch_test_compile DESTINATION reactos/bin FOR all)


add_executable(pathcch_test_dyn1_compile test_compile.c test_compile.cpp)
set_module_type(pathcch_test_dyn1_compile win32cui UNICODE)
add_dependencies(pathcch_test_dyn1_compile psdk)
target_link_libraries(pathcch_test_dyn1_compile pathcch_kernelbase)
add_importlibs(pathcch_test_dyn1_compile msvcrt kernel32 ntdll)
add_cd_file(TARGET pathcch_test_dyn1_compile DESTINATION reactos/bin FOR all)


add_executable(pathcch_test_dyn2_compile test_compile.c test_compile.cpp)
set_module_type(pathcch_test_dyn2_compile win32cui UNICODE)
add_dependencies(pathcch_test_dyn2_compile psdk)
target_link_libraries(pathcch_test_dyn2_compile pathcch)
add_importlibs(pathcch_test_dyn2_compile msvcrt kernel32 ntdll)
add_cd_file(TARGET pathcch_test_dyn2_compile DESTINATION reactos/bin FOR all)
