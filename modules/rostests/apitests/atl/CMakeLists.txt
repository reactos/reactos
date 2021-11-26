
add_definitions(-DINITGUID -DWINETEST_USE_DBGSTR_LONGLONG)

list(APPEND SOURCE
    atltypes.cpp
    CAtlFileMapping.cpp
    CAtlArray.cpp
    CAtlList.cpp
    CComBSTR.cpp
    CComHeapPtr.cpp
    CComObject.cpp
    CComQIPtr.cpp
    CComVariant.cpp
    CHeapPtrList.cpp
    CImage.cpp
    CPath.cpp
    CPath.inl
    CRegKey.cpp
    CSimpleArray.cpp
    CSimpleMap.cpp
    CString.cpp
    CString.inl
    SubclassWindow.cpp)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(atl_apitest
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    atl_apitest.rc)

target_link_libraries(atl_apitest wine uuid cpprt atl_classes)
set_target_cpp_properties(atl_apitest WITH_EXCEPTIONS)
set_module_type(atl_apitest win32cui)
add_importlibs(atl_apitest shlwapi rpcrt4 ole32 oleaut32 msimg32 gdi32 advapi32 user32 msvcrt kernel32 ntdll)
add_pch(atl_apitest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET atl_apitest)
