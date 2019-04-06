
set_cpp(WITH_RUNTIME WITH_EXCEPTIONS)
if(NOT MSVC)
    # HACK: this should be enabled globally!
    add_compile_flags_language("-std=c++11" "CXX")
endif()

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D_ATL_NO_EXCEPTIONS)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib
    ${REACTOS_SOURCE_DIR}/sdk/lib/atl
    ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/zlib/contrib)

spec2def(zipfldr.dll zipfldr.spec ADD_IMPORTLIB)


list(APPEND SOURCE
    zipfldr.cpp
    zippidl.cpp
    zippidl.hpp
    IZip.hpp
    CConfirmReplace.cpp
    CExplorerCommand.cpp
    CEnumZipContents.cpp
    CFolderViewCB.cpp
    CZipEnumerator.hpp
    CZipExtract.cpp
    CZipFolder.hpp
    CZipPassword.cpp
    Debug.cpp
    zipfldr.spec
    precomp.h
    resource.h)

add_library(zipfldr MODULE
    ${SOURCE}
    ${ZLIB_SOURCE}
    zipfldr.rc
    ${CMAKE_CURRENT_BINARY_DIR}/zipfldr.def)


set_module_type(zipfldr win32dll UNICODE)
target_link_libraries(zipfldr minizip zlib uuid)
add_importlibs(zipfldr oleaut32 ole32 shlwapi comctl32 shell32 user32 advapi32 msvcrt kernel32 ntdll)
add_pch(zipfldr precomp.h SOURCE)
add_cd_file(TARGET zipfldr DESTINATION reactos/system32 FOR all)

