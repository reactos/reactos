
set_unicode()

add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/lib/atl)

spec2def(browseui ${CMAKE_CURRENT_SOURCE_DIR}/browseui.spec ${CMAKE_CURRENT_BINARY_DIR}/browseui.def)

add_library(browseui SHARED
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
    browseui.rc)

set_module_type(browseui win32dll)

target_link_libraries(browseui
    ${CMAKE_CURRENT_BINARY_DIR}/browseui.def
    atlnew
    uuid
    wine)

add_importlibs(browseui
    msvcrt
    shlwapi
    shell32
    comctl32
    gdi32
    ole32
    oleaut32
    user32
    advapi32
    ntdll)

add_dependencies(browseui browseui_def)
add_livecd_target(browseui reactos/system32)
