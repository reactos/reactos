
add_definitions(
    -D_ATL_NO_EXCEPTIONS)

remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x601 -DWINVER=0x601)

set_cpp(WITH_RUNTIME)

spec2def(fontext.dll fontext.spec)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)

list(APPEND SOURCE
    CDataObject.cpp
    CEnumFonts.cpp
    CFontCache.cpp
    CFontCache.hpp
    CFontExt.cpp
    CFontExt.hpp
    CFontMenu.cpp
    fontext.cpp
    fontpidl.cpp
    fontpidl.hpp
    precomp.h
    resource.h)

add_library(fontext MODULE
    ${SOURCE}
    fontext.rc
    fontext.spec
    ${CMAKE_CURRENT_BINARY_DIR}/fontext.def)

set_module_type(fontext win32dll UNICODE)
target_link_libraries(fontext uuid wine)
add_delay_importlibs(fontext ole32 oleaut32 shlwapi gdi32)
add_importlibs(fontext shell32 advapi32 user32 msvcrt kernel32 ntdll)
add_pch(fontext precomp.h SOURCE)
add_cd_file(TARGET fontext DESTINATION reactos/system32 FOR all)
