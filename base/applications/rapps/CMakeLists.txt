project(rapps)

set_cpp(WITH_RUNTIME)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/cryptlib)
include_directories(include)

list(APPEND SOURCE
    aboutdlg.cpp
    available.cpp
    cabinet.cpp
    gui.cpp
    installed.cpp
    integrity.cpp
    loaddlg.cpp
    misc.cpp
    settingsdlg.cpp
    winmain.cpp
    unattended.cpp
    include/rapps.h
    include/available.h
    include/gui.h
    include/dialogs.h
    include/installed.h
    include/crichedit.h
    include/defines.h
    include/misc.h
    include/resource.h
    include/rosui.h
    include/winmain.h
    include/unattended.h
)

add_definitions(
    -DUSE_CERT_PINNING
    -D_ATL_NO_EXCEPTIONS)

file(GLOB_RECURSE rapps_rc_deps res/*.*)
add_rc_deps(rapps.rc ${rapps_rc_deps})
add_executable(rapps ${SOURCE} rapps.rc)
set_module_type(rapps win32gui UNICODE)
target_link_libraries(rapps uuid wine)
add_importlibs(rapps advapi32 comctl32 gdi32 wininet user32 shell32 shlwapi ole32 setupapi msvcrt kernel32 ntdll)
add_pch(rapps include/rapps.h SOURCE)
add_dependencies(rapps rappsmsg)
add_message_headers(ANSI rappsmsg.mc)
add_cd_file(TARGET rapps DESTINATION reactos/system32 FOR all)
