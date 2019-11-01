
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -D_ATL_VER=_ATL_VER_30)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(atl.dll atl.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    atl.c
    atl30.c
    atl_ax.c
    registrar.c
    ${CMAKE_CURRENT_BINARY_DIR}/atl_stubs.c
    precomp.h)

list(APPEND atl_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/atl.rgs
    ${CMAKE_CURRENT_SOURCE_DIR}/atl_lib_r.rgs
    ${CMAKE_CURRENT_SOURCE_DIR}/atl_lib_t.rgs
    ${CMAKE_CURRENT_BINARY_DIR}/atl_lib.tlb)

set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS "${atl_rc_deps}")

add_library(atl MODULE
    ${SOURCE}
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/atl.def)

add_typelib(atl_lib.idl)
add_dependencies(atl stdole2)
set_module_type(atl win32dll)
target_link_libraries(atl uuid wine)
add_importlibs(atl oleaut32 ole32 user32 gdi32 advapi32 advapi32_vista shlwapi msvcrt kernel32 ntdll)
add_pch(atl precomp.h SOURCE)
add_cd_file(TARGET atl DESTINATION reactos/system32 FOR all)
