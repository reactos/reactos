
list(APPEND SOURCE
    EnumObjects.cpp
    EnumParentDir.cpp
    IDataObject.cpp)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(zipfldr_apitest
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    zipfldr_apitest.rc)

target_link_libraries(zipfldr_apitest wine uuid cpprt atl_classes)
set_target_cpp_properties(zipfldr_apitest WITH_EXCEPTIONS)
set_module_type(zipfldr_apitest win32cui)
add_importlibs(zipfldr_apitest shlwapi ole32 shell32 user32 msvcrt kernel32 ntdll)
add_pch(zipfldr_apitest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET zipfldr_apitest)
