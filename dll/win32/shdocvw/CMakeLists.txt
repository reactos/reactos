
ADD_TYPELIB(shdocvw_v1 shdocvw_v1.idl)
add_dependencies(shdocvw_v1 stdole2)

add_definitions(-D_SHDOCVW_)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(shdocvw.dll shdocvw.spec)

add_library(shdocvw SHARED
    classinfo.c
    client.c
    dochost.c
    events.c
    factory.c
    frame.c
    ie.c
    iexplore.c
    intshcut.c
    navigate.c
    oleobject.c
    persist.c
    shdocvw_main.c
    shlinstobj.c
    taskbarlist.c
    urlhist.c
    view.c
    webbrowser.c
    shdocvw.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shdocvw.def)

set_module_type(shdocvw win32dll)

target_link_libraries(shdocvw
    uuid
    wine)

add_importlibs(shdocvw
    ole32
    oleaut32
    shlwapi
    shell32
    user32
    advapi32
    gdi32
    version
    urlmon
    kernel32
    ntdll)

add_dependencies(shdocvw shdocvw_v1 shdocvw_def)
add_cab_target(shdocvw 1)
