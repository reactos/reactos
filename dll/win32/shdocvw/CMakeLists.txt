
add_typelib(shdocvw_v1 shdocvw_v1.idl)
add_dependencies(shdocvw_v1 stdole2)

add_definitions(-D_SHDOCVW_)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()

spec2def(shdocvw.dll shdocvw.spec)

list(APPEND SOURCE
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
    ${CMAKE_CURRENT_BINARY_DIR}/shdocvw_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/shdocvw.def)

add_library(shdocvw SHARED ${SOURCE})
set_module_type(shdocvw win32dll)
target_link_libraries(shdocvw uuid wine)
add_delay_importlibs(shdocvw version ole32 oleaut32)

add_importlibs(shdocvw
    comctl32
    shlwapi
    shell32
    user32
    advapi32
    gdi32
    urlmon
    msvcrt
    kernel32
    ntdll)

add_dependencies(shdocvw shdocvw_v1)
add_pch(shdocvw shdocvw.h)
add_cd_file(TARGET shdocvw DESTINATION reactos/system32 FOR all)
add_importlib_target(shdocvw.spec)

if(NOT MSVC)
    allow_warnings(shdocvw)
endif()
