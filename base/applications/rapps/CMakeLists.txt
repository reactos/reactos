project(rapps)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/cryptlib)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)
include_directories(include)

list(APPEND SOURCE
    appview.cpp
    asyncinet.cpp
    available.cpp
    cabinet.cpp
    configparser.cpp
    gui.cpp
    installed.cpp
    integrity.cpp
    loaddlg.cpp
    misc.cpp
    settings.cpp
    settingsdlg.cpp
    unattended.cpp
    winmain.cpp
    include/appview.h
    include/asyncinet.h
    include/available.h
    include/configparser.h
    include/crichedit.h
    include/defines.h
    include/dialogs.h
    include/gui.h
    include/installed.h
    include/misc.h
    include/rapps.h
    include/resource.h
    include/rosui.h
    include/settings.h
    include/unattended.h
    include/winmain.h
)

add_definitions(
    -DUSE_CERT_PINNING)

file(GLOB_RECURSE rapps_rc_deps res/*.*)
add_rc_deps(rapps.rc ${rapps_rc_deps})
add_executable(rapps ${SOURCE} rapps.rc)
set_module_type(rapps win32gui UNICODE)
target_link_libraries(rapps conutils ${PSEH_LIB} uuid wine cpprt atl_classes)
add_importlibs(rapps advapi32 comctl32 gdi32 wininet user32 shell32 shlwapi ole32 setupapi gdiplus msvcrt kernel32 ntdll)
add_pch(rapps include/rapps.h SOURCE)
add_dependencies(rapps rappsmsg)
add_message_headers(ANSI rappsmsg.mc)
add_cd_file(TARGET rapps DESTINATION reactos/system32 FOR all)
