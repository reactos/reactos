
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DUSE_NEW_WINE_REGISTER_RESOURCES)

add_idl_headers(mshtml_nsiface_header nsiface.idl)
add_idl_headers(mshtml_private_iface_header mshtml_private_iface.idl)
spec2def(mshtml.dll mshtml.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    conpoint.c
    dispex.c
    editor.c
    htmlanchor.c
    htmlarea.c
    htmlattr.c
    htmlbody.c
    htmlcurstyle.c
    htmldoc.c
    htmlelem.c
    htmlelemcol.c
    htmlevent.c
    htmlform.c
    htmlframe.c
    htmlgeneric.c
    htmlhead.c
    htmlimg.c
    htmlinput.c
    htmllink.c
    htmllocation.c
    htmlnode.c
    htmlobject.c
    htmlscript.c
    htmlselect.c
    htmlstorage.c
    htmlstyle.c
    htmlstyleelem.c
    htmlstylesheet.c
    htmltable.c
    htmltextarea.c
    htmltextnode.c
    htmlwindow.c
    ifacewrap.c
    loadopts.c
    main.c
    mutation.c
    navigate.c
    npplugin.c
    nsembed.c
    nsevents.c
    nsio.c
    nsservice.c
    olecmd.c
    oleobj.c
    omnavigator.c
    persist.c
    pluginhost.c
    protocol.c
    range.c
    script.c
    secmgr.c
    selection.c
    service.c
    svg.c
    task.c
    view.c
    xmlhttprequest.c)

if(MSVC)
    add_asm_files(mshtml_asm msvc.S)
endif()

list(APPEND PCH_SKIP_SOURCE
    guid.c
    ${CMAKE_CURRENT_BINARY_DIR}/mshtml_stubs.c)

add_library(mshtml MODULE
    ${SOURCE}
    ${mshtml_asm}
    ${PCH_SKIP_SOURCE}
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mshtml.def)

add_idl_reg_scripts(mshtml registry mshtml_classes.idl)
add_idl_reg_scripts(mshtml regtypelib mshtml_private_iface.idl)

list(APPEND mshtml_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/blank.htm
    ${CMAKE_CURRENT_SOURCE_DIR}/mshtml.inf)

set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS "${mshtml_rc_deps}")
set_module_type(mshtml win32dll)
target_link_libraries(mshtml uuid wine strmiids oldnames wine_dll_canunload)
add_delay_importlibs(mshtml wininet)
add_importlibs(mshtml urlmon shlwapi shell32 ole32 oleaut32 inetcomm user32 gdi32 advapi32 msvcrt kernel32_vista kernel32 ntdll)
add_dependencies(mshtml mshtml_nsiface_header mshtml_private_iface_header jscript_jsdisp_header)
add_pch(mshtml mshtml_private.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET mshtml DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(mshtml) # CORE-5743: No CONST_VTABLE
