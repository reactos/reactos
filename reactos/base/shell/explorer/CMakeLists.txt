PROJECT(SHELL)

set_cpp(WITH_RUNTIME)

include_directories(${REACTOS_SOURCE_DIR}/lib/atl)

list(APPEND SOURCE
    desktop.cpp
    explorer.cpp
    rshell.cpp
    settings.cpp
    shellservice.cpp
    startctxmnu.cpp
    startmnu.cpp
    startmnusite.cpp
    startmnucust.cpp
    startup.cpp
    taskband.cpp
    taskswnd.cpp
    tbsite.cpp
    trayntfy.cpp
    trayprop.cpp
    traywnd.cpp
    util.cpp
    precomp.h)

file(GLOB explorer_rc_deps res/*.*)
add_rc_deps(explorer.rc ${explorer_rc_deps})
add_executable(explorer ${SOURCE} explorer.rc)
target_link_libraries(explorer uuid atlnew wine)
set_module_type(explorer win32gui UNICODE)
add_importlibs(explorer advapi32 gdi32 user32 comctl32 ole32 oleaut32 shell32 browseui shlwapi shdocvw version uxtheme msvcrt kernel32 ntdll)
add_pch(explorer precomp.h SOURCE)
add_cd_file(TARGET explorer DESTINATION reactos FOR all)
