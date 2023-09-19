
add_subdirectory(shlwapi_resource_dll)

# Ensure the resource compiler can find the dll
include_directories($<TARGET_FILE_DIR:shlwapi_resource_dll>)

list(APPEND SOURCE
    PathFindOnPath.c
    PathIsUNC.c
    PathIsUNCServer.c
    PathIsUNCServerShare.c
    PathUnExpandEnvStrings.c
    PathUnExpandEnvStringsForUser.c
    SHAreIconsEqual.c
    SHLoadIndirectString.c
    SHLoadRegUIString.c
    SHPropertyBag.cpp
    StrFormatByteSizeW.c
    testdata.rc
    testlist.c)

# Add a dependency from the resource to the dll
add_rc_deps(testdata.rc ${CMAKE_CURRENT_BINARY_DIR}/shlwapi_resource_dll/shlwapi_resource_dll.dll)

add_executable(shlwapi_apitest ${SOURCE})
set_module_type(shlwapi_apitest win32cui)
target_link_libraries(shlwapi_apitest ${PSEH_LIB} uuid)
add_importlibs(shlwapi_apitest shlwapi oleaut32 ole32 user32 gdi32 advapi32 msvcrt kernel32)
add_dependencies(shlwapi_apitest shlwapi_resource_dll)
add_rostests_file(TARGET shlwapi_apitest)
