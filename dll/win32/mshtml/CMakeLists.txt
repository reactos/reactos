
ADD_INTERFACE_DEFINITIONS(mshtml_nsiface_header nsiface.idl)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -D_DLL
    -D__USE_CRTIMP)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mshtml ${CMAKE_CURRENT_SOURCE_DIR}/mshtml.spec ${CMAKE_CURRENT_BINARY_DIR}/mshtml.def)

add_library(mshtml SHARED
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
    rsrc.rc)

set_module_type(mshtml win32dll)

target_link_libraries(mshtml
    ${CMAKE_CURRENT_BINARY_DIR}/mshtml.def
    uuid
    wine
    strmiids
    oldnames)

add_importlibs(mshtml
#    msvcrt
    urlmon
    shlwapi
    ole32
    oleaut32
    user32
    gdi32
    advapi32
    wininet
    kernel32
    ntdll)

add_dependencies(mshtml mshtml_nsiface_header mshtml_def)
add_cab_target(mshtml 1)
