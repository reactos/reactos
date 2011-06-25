
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D_SHELL32_
    -DCOM_NO_WINDOWS_H
    -D_WINE)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/lib/recyclebin
    ${REACTOS_SOURCE_DIR})

spec2def(shell32.dll shell32.spec ADD_IMPORTLIB GENERATE_STUBS)

list(APPEND SOURCE
    authors.c
    autocomplete.c
    brsfolder.c
    changenotify.c
    classes.c
    clipboard.c
    control.c
    dataobject.c
    dde.c
    debughlp.c
    desktop.c
    dialogs.c
    dragdrophelper.c
    enumidlist.c
    extracticon.c
    folders.c
    iconcache.c
    pidl.c
    regsvr.c
    shell32_main.c
    shellitem.c
    shelllink.c
    shellole.c
    shellord.c
    shellpath.c
    shellreg.c
    shellstring.c
    shfldr_desktop.c
    shfldr_fs.c
    shfldr_mycomp.c
    shfldr_mydocuments.c
    shfldr_printers.c
    shfldr_admintools.c
    shfldr_netplaces.c
    shfldr_fonts.c
    shfldr_cpanel.c
    shfldr_recyclebin.c
    shlexec.c
    shlfileop.c
    shlfolder.c
    shlfsbind.c
    shlmenu.c
    shlview.c
    shpolicy.c
    shv_def_cmenu.c
    startmenu.c
    stubs.c
    ros-systray.c
    fprop.c
    drive.c
    she_ocmenu.c
    shv_item_new.c
    folder_options.c
    shell32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shell32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/shell32.def)

add_library(shell32 SHARED ${SOURCE})

set_module_type(shell32 win32dll)

target_link_libraries(shell32
    wine
    uuid
    recyclebin)

add_importlibs(shell32
    advapi32
    gdi32
    user32
    comctl32
    comdlg32
    shlwapi
    ole32
    version
    devmgr
    winspool
    winmm
    kernel32
    msvcrt
    ntdll)

add_pch(shell32 precomp.h)

add_cd_file(TARGET shell32 DESTINATION reactos/system32 FOR all)
add_importlib_target(shell32.spec)
