
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(hhctrl.ocx hhctrl.ocx.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    chm.c
    content.c
    help.c
    hhctrl.c
    index.c
    search.c
    stream.c
    webbrowser.c
    hhctrl.h)

add_library(hhctrl SHARED
    ${SOURCE}
    guid.c
    hhctrl.rc
    ${CMAKE_CURRENT_BINARY_DIR}/hhctrl.def)

add_typelib(hhctrl_tlb.idl)

list(APPEND hhctrl_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/hhctrl_tlb.rgs
    ${CMAKE_CURRENT_BINARY_DIR}/hhctrl_tlb.tlb)

set_source_files_properties(hhctrl.rc PROPERTIES OBJECT_DEPENDS "${hhctrl_rc_deps}")
set_module_type(hhctrl win32ocx)
target_link_libraries(hhctrl uuid wine)
add_importlibs(hhctrl advapi32 comctl32 shlwapi ole32 oleaut32 user32 gdi32 msvcrt kernel32 ntdll)
add_dependencies(hhctrl stdole2 wineheaders)
add_pch(hhctrl hhctrl.h SOURCE)
add_cd_file(TARGET hhctrl DESTINATION reactos/system32 FOR all)
