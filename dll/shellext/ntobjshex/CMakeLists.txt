
project(SHELL)

spec2def(ntobjshex.dll ntobjshex.spec)

file(GLOB_RECURSE ntobjshex_rc_deps resources/*.*)
add_rc_deps(ntobjshex.rc ${ntobjshex_rc_deps})

list(APPEND SOURCE
    ntobjenum.cpp
    ntobjfolder.cpp
    ntobjshex.cpp
    regfolder.cpp
    precomp.h)

add_library(ntobjshex MODULE
    ${SOURCE}
    ntobjshex.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ntobjshex.def)

set_module_type(ntobjshex win32dll UNICODE)
target_link_libraries(ntobjshex uuid wine cpprt atl_classes)

add_importlibs(ntobjshex
    advapi32
    winmm
    ole32
    oleaut32
    shlwapi
    shell32
    comctl32
    msvcrt
    gdi32
    user32
    kernel32
    ntdll)

add_pch(ntobjshex precomp.h SOURCE)
add_cd_file(TARGET ntobjshex DESTINATION reactos/system32 FOR all)
