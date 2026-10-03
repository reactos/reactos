
spec2def(ieframe.dll ieframe.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    classinfo.c
    client.c
    dochost.c
    events.c
    frame.c
    ie.c
    ieframe_main.c
    iehtmlwnd.c
    iexplore.c
    inetfolder.c
    intshcut.c
    navigate.c
    oleobject.c
    persist.c
    shellbrowser.c
    shelluihelper.c
    urlhist.c
    view.c
    webbrowser.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/ieframe_stubs.c)

add_library(ieframe MODULE
    ${SOURCE}
    ieframe.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ieframe.def)

list(APPEND ieframe_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/ieframe.rgs
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/ietoolbar.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/pages.ico)

set_source_files_properties(ieframe.rc PROPERTIES OBJECT_DEPENDS "${ieframe_rc_deps}")
add_dependencies(ieframe stdole2)
set_module_type(ieframe win32dll)
add_idl_reg_scripts(ieframe registry ieframe_v1.idl)
add_idl_reg_scripts(ieframe regtypelib ieframe_v1.idl)
target_link_libraries(ieframe uuid wine wine_dll_register wine_dll_canunload oldnames)
add_importlibs(ieframe urlmon shell32 comctl32 shlwapi oleaut32 ole32 user32 gdi32 advapi32 msvcrt kernel32 ntdll)
add_pch(ieframe precomp.h SOURCE)
add_cd_file(TARGET ieframe DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(ieframe) # CORE-5743: No CONST_VTABLE
