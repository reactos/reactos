project(MSPAINT)

add_definitions(-DINITGUID)
set_cpp(WITH_RUNTIME WITH_EXCEPTIONS)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)

list(APPEND SOURCE
    dialogs.cpp
    dib.cpp
    drawing.cpp
    fullscreen.cpp
    history.cpp
    imgarea.cpp
    main.cpp
    miniature.cpp
    mouse.cpp
    palette.cpp
    palettemodel.cpp
    registry.cpp
    scrollbox.cpp
    selection.cpp
    selectionmodel.cpp
    sizebox.cpp
    textedit.cpp
    toolbox.cpp
    toolsettings.cpp
    toolsmodel.cpp
    winproc.cpp
    precomp.h)

file(GLOB mspaint_rc_deps icons/*.*)
add_rc_deps(rsrc.rc ${mspaint_rc_deps})
add_executable(mspaint ${SOURCE} rsrc.rc)
set_module_type(mspaint win32gui UNICODE)
target_link_libraries(mspaint atlnew uuid)
add_importlibs(mspaint hhctrl comdlg32 shell32 user32 gdi32 gdiplus advapi32 comctl32 msvcrt kernel32 rpcrt4 shlwapi)
add_pch(mspaint precomp.h SOURCE)
add_cd_file(TARGET mspaint DESTINATION reactos/system32 FOR all)
