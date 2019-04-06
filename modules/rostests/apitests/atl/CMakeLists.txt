
add_definitions(-DINITGUID -DWINETEST_USE_DBGSTR_LONGLONG)
set_cpp(WITH_RUNTIME WITH_EXCEPTIONS)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)

list(APPEND SOURCE
    atltypes.cpp
    CAtlFileMapping.cpp
    CComBSTR.cpp
    CComHeapPtr.cpp
    CComObject.cpp
    CComQIPtr.cpp
    CImage.cpp
    CRegKey.cpp
    CSimpleArray.cpp
    CSimpleMap.cpp
    CString.cpp
    precomp.h)

add_executable(atl_apitest
    ${SOURCE}
    CComVariant.cpp
    testlist.c
    atl_apitest.rc)

target_link_libraries(atl_apitest wine uuid)
set_module_type(atl_apitest win32cui)
add_importlibs(atl_apitest rpcrt4 ole32 oleaut32 msimg32 gdi32 advapi32 user32 msvcrt kernel32 ntdll)
add_pch(atl_apitest precomp.h SOURCE)
add_rostests_file(TARGET atl_apitest)
