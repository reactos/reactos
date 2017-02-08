
set_cpp()

add_definitions(
    -D__WINESRC__)

if (MSVC)
    add_compile_flags("/EHa-")
endif()

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/lib/atl)

spec2def(browseui.dll browseui.spec ADD_IMPORTLIB)

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
    dllinstall.c
    ${CMAKE_CURRENT_BINARY_DIR}/browseui.def)

add_library(browseui SHARED
    ${SOURCE}
    browseui.rc)

if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #allow_warnings(browseui)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-error")
endif()

set_module_type(browseui win32dll UNICODE)

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

add_pch(browseui precomp.h)
add_cd_file(TARGET browseui DESTINATION reactos/system32 FOR all)

