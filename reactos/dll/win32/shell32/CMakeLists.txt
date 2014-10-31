
set_cpp(WITH_RUNTIME)
spec2def(shell32.dll shell32.spec ADD_IMPORTLIB)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D_SHELL32_
    -D_WINE)

include_directories(
    ${REACTOS_SOURCE_DIR}/lib/recyclebin
    ${REACTOS_SOURCE_DIR}/lib/atl
    ${REACTOS_SOURCE_DIR})

list(APPEND SOURCE
    #authors.cpp
    autocomplete.cpp
    brsfolder.cpp
    changenotify.cpp
    classes.cpp
    clipboard.cpp
    control.cpp
    CMenuBand.cpp
    CMenuDeskBar.cpp
    dataobject.cpp
    dde.cpp
    debughlp.cpp
    desktop.cpp
    dialogs.cpp
    dragdrophelper.cpp
    enumidlist.cpp
    extracticon.cpp
    folders.cpp
    iconcache.cpp
    pidl.cpp
    shell32_main.cpp
    shellitem.cpp
    shelllink.cpp
    shellole.cpp
    shellord.cpp
    shellpath.cpp
    shellreg.cpp
    folders/desktop.cpp
    folders/fs.cpp
    folders/mycomp.cpp
    folders/mydocuments.cpp
    folders/printers.cpp
    folders/admintools.cpp
    folders/netplaces.cpp
    folders/fonts.cpp
    folders/cpanel.cpp
    folders/recyclebin.cpp
    droptargets/CexeDropHandler.cpp
    shlexec.cpp
    shlfileop.cpp
    shlfolder.cpp
    shlfsbind.cpp
    shlmenu.cpp
    shlview.cpp
    shpolicy.cpp
    stubs.cpp
    systray.cpp
    fprop.cpp
    drive.cpp
    defcontextmenu.cpp
    openwithmenu.cpp
    newmenu.cpp
    startmenu.cpp
    folder_options.cpp
    filedefext.cpp
    drvdefext.cpp
    precomp.h)

add_library(shell32 SHARED
    ${SOURCE}
    shellstring.c
    vista.c
    shell32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shell32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/shell32.def)

set_module_type(shell32 win32dll UNICODE HOTPATCHABLE)
target_link_libraries(shell32 atlnew wine uuid recyclebin)
add_delay_importlibs(shell32 ole32 version fmifs)
add_importlibs(shell32 advapi32 browseui gdi32 user32 comctl32 comdlg32 shdocvw shlwapi devmgr winspool winmm msvcrt kernel32 ntdll)
add_pch(shell32 precomp.h SOURCE)
add_cd_file(TARGET shell32 DESTINATION reactos/system32 FOR all)
