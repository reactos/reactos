
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -DCOM_NO_WINDOWS_H
    -Dstrcasecmp=_stricmp
    -Dstrncasecmp=_strnicmp
)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_idl_headers(mshtml_nsiface_header nsiface.idl)
spec2def(mshtml.dll mshtml.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    conpoint.c
    dispex.c
    editor.c
    hlink.c
    htmlanchor.c
    htmlarea.c
    htmlattr.c
    htmlbody.c
    htmlcomment.c
    htmlcurstyle.c
    htmldoc.c
    htmlelem.c
    htmlelemcol.c
    htmlembed.c
    htmlevent.c
    htmlform.c
    htmlframe.c
    htmlframebase.c
    htmlgeneric.c
    htmlhead.c
    htmliframe.c
    htmlimg.c
    htmlinput.c
    htmllink.c
    htmllocation.c
    htmlmeta.c
    htmlnode.c
    htmlobject.c
    htmloption.c
    htmlscreen.c
    htmlscript.c
    htmlselect.c
    htmlstorage.c
    htmlstyle.c
    htmlstyle2.c
    htmlstyle3.c
    htmlstyleelem.c
    htmlstylesheet.c
    htmltable.c
    htmltablecell.c
    htmltablerow.c
    htmltextarea.c
    htmltextcont.c
    htmltextnode.c
    htmlwindow.c
    ifacewrap.c
    ipwindow.c
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
    olewnd.c
    omnavigator.c
    persist.c
    pluginhost.c
    propbag.c
    protocol.c
    script.c
    secmgr.c
    selection.c
    service.c
    task.c
    txtrange.c
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

list(APPEND mshtml_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/blank.htm
    ${CMAKE_CURRENT_SOURCE_DIR}/mshtml.inf
    ${CMAKE_CURRENT_SOURCE_DIR}/mshtml_classes.rgs)

set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS "${mshtml_rc_deps}")
set_module_type(mshtml win32dll)
target_link_libraries(mshtml uuid wine strmiids oldnames)
add_delay_importlibs(mshtml wininet)
add_importlibs(mshtml urlmon shlwapi shell32 ole32 oleaut32 user32 gdi32 advapi32 msvcrt kernel32 ntdll)
add_dependencies(mshtml mshtml_nsiface_header)
add_pch(mshtml mshtml_private.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET mshtml DESTINATION reactos/system32 FOR all)
