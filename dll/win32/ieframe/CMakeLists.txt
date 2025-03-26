
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__ -D__ROS_LONG64__)
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

add_typelib(ieframe_v1.idl)

list(APPEND ieframe_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/ieframe.rgs
    ${CMAKE_CURRENT_SOURCE_DIR}/ieframe_v1.rgs
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/ietoolbar.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/pages.ico
    ${CMAKE_CURRENT_BINARY_DIR}/ieframe_v1.tlb)

set_source_files_properties(ieframe.rc PROPERTIES OBJECT_DEPENDS "${ieframe_rc_deps}")
add_dependencies(ieframe stdole2)
set_module_type(ieframe win32dll)
target_link_libraries(ieframe uuid wine)
add_importlibs(ieframe urlmon shell32 comctl32 shlwapi oleaut32 ole32 user32 gdi32 advapi32 msvcrt kernel32 ntdll)
add_pch(ieframe precomp.h SOURCE)
add_cd_file(TARGET ieframe DESTINATION reactos/system32 FOR all)
