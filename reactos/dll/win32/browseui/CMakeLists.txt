
set_unicode()
set_cpp()

add_definitions(
    -D__WINESRC__
    -DROS_Headers)

if (MSVC)
    add_compiler_flags(/EHa-)
endif()

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/lib/atl)

set_rc_compiler()

spec2def(browseui.dll browseui.spec)

list(APPEND SOURCE
    aclmulti.cpp
    addressband.cpp
    addresseditbox.cpp
    bandproxy.cpp
    bandsite.cpp
    bandsitemenu.cpp
    basebar.cpp
    basebarsite.cpp
    brandband.cpp
    browseui.cpp
    browseuiord.cpp
    commonbrowser.cpp
    globalfoldersettings.cpp
    internettoolbar.cpp
    regtreeoptions.cpp
    shellbrowser.cpp
    toolsband.cpp
    travellog.cpp
    utility.cpp
    browseui.rc
    dllinstall.c
    ${CMAKE_CURRENT_BINARY_DIR}/browseui.def)

add_library(browseui SHARED ${SOURCE})
set_module_type(browseui win32dll)

target_link_libraries(browseui
    atlnew
    uuid
    wine)

add_importlibs(browseui
    shlwapi
    shell32
    comctl32
    gdi32
    ole32
    oleaut32
    user32
    advapi32
    msvcrt
    kernel32
    ntdll)

add_cd_file(TARGET browseui DESTINATION reactos/system32 FOR all)
