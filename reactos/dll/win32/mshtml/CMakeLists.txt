
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_idl_Headers(mshtml_nsiface_header nsiface.idl)
spec2def(mshtml.dll mshtml.spec)

list(APPEND SOURCE
    conpoint.c
    dispex.c
    editor.c
    hlink.c
    htmlanchor.c
    htmlbody.c
    htmlcomment.c
    htmlcurstyle.c
    htmldoc.c
    htmldoc3.c
    htmldoc5.c
    htmlelem.c
    htmlelem2.c
    htmlelem3.c
    htmlelemcol.c
    htmlevent.c
    htmlform.c
    htmlframe.c
    htmlframebase.c
    htmlgeneric.c
    htmliframe.c
    htmlimg.c
    htmlinput.c
    htmllocation.c
    htmlnode.c
    htmloption.c
    htmlscreen.c
    htmlscript.c
    htmlselect.c
    htmlstyle.c
    htmlstyle2.c
    htmlstyle3.c
    htmlstylesheet.c
    htmltable.c
    htmltablerow.c
    htmltextarea.c
    htmltextcont.c
    htmltextnode.c
    htmlwindow.c
    install.c
    loadopts.c
    main.c
    mutation.c
    navigate.c
    nsembed.c
    nsevents.c
    nsio.c
    nsservice.c
    olecmd.c
    oleobj.c
    olewnd.c
    omnavigator.c
    persist.c
    protocol.c
    script.c
    secmgr.c
    selection.c
    service.c
    task.c
    txtrange.c
    view.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mshtml_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mshtml.def)

add_library(mshtml SHARED ${SOURCE})
set_module_type(mshtml win32dll)
target_link_libraries(mshtml uuid wine strmiids oldnames)
add_delay_importlibs(mshtml wininet)

add_importlibs(mshtml
    urlmon
    shlwapi
    ole32
    oleaut32
    user32
    gdi32
    advapi32
    msvcrt
    kernel32
    ntdll)

add_dependencies(mshtml mshtml_nsiface_header)
add_cd_file(TARGET mshtml DESTINATION reactos/system32 FOR all)
