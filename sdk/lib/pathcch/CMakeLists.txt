
# Down-level static PathCch library, to be used for
# programs that need to run on Windows 7 and below.
add_library(pathcch_static STATIC ${REACTOS_SOURCE_DIR}/dll/win32/kernelbase/wine/path.c)
add_dependencies(pathcch_static xdk)
target_compile_definitions(pathcch_static PRIVATE wcsnicmp=_wcsnicmp STATIC_PATHCCH)

# Windows 8+ compatible libraries, importing from either
# kernelbase.dll or api-ms-win-core-path-l1-1-0.dll
generate_import_lib(pathcch_kernelbase kernelbase.dll pathcch.spec "--version=${DLL_EXPORT_VERSION}" "")
generate_import_lib(pathcch api-ms-win-core-path-l1-1-0.dll pathcch.spec "--version=${DLL_EXPORT_VERSION}" "")
