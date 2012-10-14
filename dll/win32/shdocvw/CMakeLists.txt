
add_typelib(shdocvw_v1.idl)

add_definitions(-D_SHDOCVW_)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(shdocvw.dll shdocvw.spec ADD_IMPORTLIB)

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
    ${CMAKE_CURRENT_BINARY_DIR}/shdocvw_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/shdocvw.def)

set_source_files_properties(shdocvw.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shdocvw_v1.tlb)

add_library(shdocvw SHARED
    ${SOURCE}
    shdocvw.rc)

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

# shdocvw_v1.tlb needs stdole2.tlb
add_dependencies(shdocvw stdole2)

add_pch(shdocvw shdocvw.h)
add_cd_file(TARGET shdocvw DESTINATION reactos/system32 FOR all)

if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #allow_warnings(shdocvw)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-error")
endif()
