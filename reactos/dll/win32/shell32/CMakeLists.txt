PROJECT(SHELL)

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
    CMenuBand.cpp
    CMenuDeskBar.cpp
    CIDLDataObj.cpp
    CQueryAssociations.cpp
    dde.cpp
    debughlp.cpp
    CDesktopBrowser.cpp
    dialogs/dialogs.cpp
    dialogs/drive.cpp
    dialogs/drvdefext.cpp
    dialogs/folder_options.cpp
    dialogs/filedefext.cpp
    dialogs/fprop.cpp
    CDropTargetHelper.cpp
    CEnumIDListBase.cpp
    CExtractIcon.cpp
    folders.cpp
    iconcache.cpp
    shell32.cpp
    CShellItem.cpp
    CShellLink.cpp
    folders/CDesktopFolder.cpp
    folders/CFSFolder.cpp
    folders/CDrivesFolder.cpp
    folders/CMyDocsFolder.cpp
    folders/CPrinterFolder.cpp
    folders/CAdminToolsFolder.cpp
    folders/CNetFolder.cpp
    folders/CFontsFolder.cpp
    folders/CControlPanelFolder.cpp
    folders/CRecycleBin.cpp
    droptargets/CexeDropHandler.cpp
    shlexec.cpp
    shlfileop.cpp
    shlfolder.cpp
    CFileSysBindData.cpp
    CDefView.cpp
    stubs.cpp
    systray.cpp
    CDefaultContextMenu.cpp
    COpenWithMenu.cpp
    CNewMenu.cpp
    CStartMenu.cpp
    precomp.h)

add_library(shell32 SHARED
    ${SOURCE}
    wine/brsfolder.c
    wine/changenotify.c
    wine/classes.c
    wine/clipboard.c
    wine/control.c
    wine/pidl.c
    wine/shell32_main.c
    wine/shellole.c
    wine/shellord.c
    wine/shellpath.c
    wine/shellreg.c
    wine/shellstring.c
    wine/shlmenu.c
    wine/shpolicy.c
    vista.c
    shell32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shell32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/shell32.def)

set_module_type(shell32 win32dll UNICODE HOTPATCHABLE)
target_link_libraries(shell32 atlnew wine uuid recyclebin)
add_delay_importlibs(shell32 ole32 userenv version fmifs)
add_importlibs(shell32 advapi32 browseui gdi32 user32 powrprof comctl32 comdlg32 shdocvw shlwapi devmgr winspool winmm msvcrt kernel32 ntdll)
add_pch(shell32 precomp.h SOURCE)
add_cd_file(TARGET shell32 DESTINATION reactos/system32 FOR all)
