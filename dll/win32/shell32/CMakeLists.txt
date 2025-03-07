PROJECT(SHELL)

add_subdirectory(shelldesktop)
add_subdirectory(shellmenu)
add_subdirectory(shellrecyclebin)

spec2def(shell32.dll shell32.spec ADD_IMPORTLIB)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D_SHELL32_
    -D_WINE)

if(DBG)
    add_definitions(-D_DEBUG=1) # CORE-17505
endif()

list(APPEND SOURCE
    brfolder.cpp
    CActiveDesktop.cpp
    CActiveDesktop.h
    CIDLDataObj.cpp
    CQueryAssociations.cpp
    changenotify.cpp
    debughlp.cpp
    dialogs/dialogs.cpp
    dialogs/drive.cpp
    dialogs/drvdefext.cpp
    dialogs/filedefext.cpp
    dialogs/filetypes.cpp
    dialogs/folder_options.cpp
    dialogs/item_prop.cpp
    dialogs/general.cpp
    dialogs/recycler_prop.cpp
    dialogs/view.cpp
    CDropTargetHelper.cpp
    CEnumIDListBase.cpp
    CExtractIcon.cpp
    folders.cpp
    iconcache.cpp
    propsheet.cpp
    shell32.cpp
    utils.cpp
    CShellItem.cpp
    CShellLink.cpp
    CFolderOptions.cpp
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
    folders/CRegFolder.cpp
    droptargets/CexeDropHandler.cpp
    droptargets/CFSDropTarget.cpp
    droptargets/CRecyclerDropTarget.cpp
    shldataobject.cpp
    shlexec.cpp
    shlfileop.cpp
    shlfolder.cpp
    CFileSysBindData.cpp
    CDefView.cpp
    CDefViewUtil.cpp
    CDefViewDual.cpp
    CDefViewBckgrndMenu.cpp
    stubs.cpp
    systray.cpp
    CUserNotification.cpp
    CDefaultContextMenu.cpp
    COpenWithMenu.cpp
    CNewMenu.cpp
    CSendToMenu.cpp
    CCopyMoveToMenu.cpp
    CCopyAsPathMenu.cpp
    CShellDispatch.cpp
    CFolder.cpp
    CFolderItems.cpp
    CFolderItemVerbs.cpp)

list(APPEND PCH_SKIP_SOURCE
    wine/appbar.c
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
    ${CMAKE_CURRENT_BINARY_DIR}/shell32_stubs.c)

file(GLOB_RECURSE shell32_rc_deps res/*.*)
list(APPEND shell32_rc_deps ${CMAKE_CURRENT_BINARY_DIR}/shell32_shldisp.tlb)
add_rc_deps(shell32.rc ${shell32_rc_deps})

add_library(shell32 MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    shell32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shell32.def)

if(MSVC)
    # error C4311: 'type cast': pointer truncation from 'HANDLE' to 'INT'
    remove_target_compile_option(shell32 "/we4311")
endif()

add_typelib(shell32_shldisp.idl)
set_source_files_properties(shell32.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shell32_shldisp.tlb)

set_module_type(shell32 win32dll UNICODE)
target_link_libraries(shell32 shellmenu shelldesktop wine uuid recyclebin cpprt atl_classes oldnames)
add_delay_importlibs(shell32 powrprof shdocvw devmgr winspool.drv winmm mpr uxtheme ole32 oleaut32 userenv browseui version fmifs netapi32 secur32)
add_importlibs(shell32 advapi32 gdi32 user32 comctl32 comdlg32 shlwapi msvcrt kernel32 ntdll)
add_dependencies(shell32 stdole2) # shell32_shldisp.tlb needs stdole2.tlb
add_pch(shell32 precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET shell32 DESTINATION reactos/system32 FOR all)
