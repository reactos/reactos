
project(SHELL)

spec2def(stobject.dll stobject.spec)

file(GLOB_RECURSE stobject_rc_deps resources/*.*)
add_rc_deps(stobject.rc ${stobject_rc_deps})

list(APPEND SOURCE
    csystray.cpp
    stobject.cpp
    hotplug.cpp
    power.cpp
    volume.cpp
    precomp.h)

add_library(stobject MODULE
    ${SOURCE}
    stobject.rc
    ${CMAKE_CURRENT_BINARY_DIR}/stobject.def)

set_module_type(stobject win32dll UNICODE)
target_link_libraries(stobject uuid wine cpprt atl_classes)

add_importlibs(stobject
    setupapi
    advapi32
    winmm
    ole32
    oleaut32
    powrprof
    shlwapi
    shell32
    comctl32
    msvcrt
    gdi32
    user32
    kernel32
    ntdll)

add_pch(stobject precomp.h SOURCE)
add_cd_file(TARGET stobject DESTINATION reactos/system32 FOR all)
