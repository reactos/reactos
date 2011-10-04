set_cpp()

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D_SHELL32_
    -DCOM_NO_WINDOWS_H
    -D_WINE)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/lib/recyclebin
    ${REACTOS_SOURCE_DIR}/lib/atl
    ${REACTOS_SOURCE_DIR})

spec2def(shell32.dll shell32.spec)

list(APPEND SOURCE
    authors.cpp
    autocomplete.cpp
    brsfolder.cpp
    changenotify.cpp
    classes.cpp
    clipboard.cpp
    control.cpp
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
    shellstring.cpp
    shfldr_desktop.cpp
    shfldr_fs.cpp
    shfldr_mycomp.cpp
    shfldr_mydocuments.cpp
    shfldr_printers.cpp
    shfldr_admintools.cpp
    shfldr_netplaces.cpp
    shfldr_fonts.cpp
    shfldr_cpanel.cpp
    shfldr_recyclebin.cpp
    shlexec.cpp
    shlfileop.cpp
    shlfolder.cpp
    shlfsbind.cpp
    shlmenu.cpp
    shlview.cpp
    shpolicy.cpp
    shv_def_cmenu.cpp
    startmenu.cpp
    stubs.cpp
    ros-systray.cpp
    fprop.cpp
    drive.cpp
    she_ocmenu.cpp
    shv_item_new.cpp
    folder_options.cpp
    shell32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shell32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/shell32.def)

add_library(shell32 SHARED ${SOURCE})
allow_warnings(shell32)
set_module_type(shell32 win32dll UNICODE)

target_link_libraries(shell32
    atlnew
    wine
    uuid
    recyclebin)

add_delay_importlibs(shell32 ole32 version)

add_importlibs(shell32
    advapi32
    gdi32
    user32
    comctl32
    comdlg32
    shlwapi
    devmgr
    winspool
    winmm
    msvcrt
    kernel32
    ntdll)

add_pch(shell32 precomp.h)

add_cd_file(TARGET shell32 DESTINATION reactos/system32 FOR all)
add_importlib_target(shell32.spec)
