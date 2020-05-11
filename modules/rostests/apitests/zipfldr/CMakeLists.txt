
add_definitions(-DINITGUID -DWINETEST_USE_DBGSTR_LONGLONG)
set_cpp(WITH_RUNTIME WITH_EXCEPTIONS)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)

list(APPEND SOURCE
    EnumObjects.cpp
    EnumParentDir.cpp
    IDataObject.cpp
    precomp.h
    resource.h)

add_executable(zipfldr_apitest
    ${SOURCE}
    testlist.c
    zipfldr_apitest.rc)

target_link_libraries(zipfldr_apitest wine uuid)
set_module_type(zipfldr_apitest win32cui)
add_importlibs(zipfldr_apitest shlwapi ole32 shell32 user32 msvcrt kernel32 ntdll)
add_pch(zipfldr_apitest precomp.h SOURCE)
add_rostests_file(TARGET zipfldr_apitest)
