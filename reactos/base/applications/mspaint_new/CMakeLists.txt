project(MSPAINT_NEW)

set_cpp(WITH_RUNTIME)

include_directories(${REACTOS_SOURCE_DIR}/lib/atl)

list(APPEND SOURCE
    dialogs.cpp
    dib.cpp
    drawing.cpp
    history.cpp
    main.cpp
    mouse.cpp
    palette.cpp
    registry.cpp
    scrollbox.cpp
    selection.cpp
    sizebox.cpp
    textedit.cpp
    toolsettings.cpp
    winproc.cpp
    precomp.h)

file(GLOB mspaint_new_rc_deps icons/*.*)
add_rc_deps(rsrc.rc ${mspaint_new_rc_deps})
add_executable(mspaint_new ${SOURCE} rsrc.rc)
set_module_type(mspaint_new win32gui UNICODE)
target_link_libraries(mspaint_new atlnew)
add_importlibs(mspaint_new hhctrl comdlg32 shell32 user32 gdi32 advapi32 comctl32 msvcrt kernel32)
add_pch(mspaint_new precomp.h SOURCE)
add_cd_file(TARGET mspaint_new DESTINATION reactos/system32 FOR all)
