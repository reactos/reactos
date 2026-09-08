
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

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
    CSendToZip.cpp
    CZipCreator.cpp
    CZipEnumerator.hpp
    CZipEnumerator.cpp
    CZipExtract.cpp
    CZipFolder.hpp
    CZipFolder.cpp
    CZipPassword.cpp
    Debug.cpp
    zipfldr.spec
    precomp.h
    resource.h)

add_library(zipfldr MODULE
    ${SOURCE}
    zipfldr.rc
    ${CMAKE_CURRENT_BINARY_DIR}/zipfldr.def)
add_pch(zipfldr precomp.h SOURCE)

set_module_type(zipfldr win32dll UNICODE)
target_link_libraries(zipfldr minizip zlib uuid cpprt atl_classes)
set_target_cpp_properties(zipfldr WITH_EXCEPTIONS)
add_importlibs(zipfldr oleaut32 ole32 shlwapi comctl32 shell32 user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET zipfldr DESTINATION reactos/system32 FOR all)
