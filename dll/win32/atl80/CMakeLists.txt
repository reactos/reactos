
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -D_ATL_VER=_ATL_VER_80)

spec2def(atl80.dll atl80.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    ${REACTOS_SOURCE_DIR}/dll/win32/atl/atl.c
    atl80.c
    ${REACTOS_SOURCE_DIR}/dll/win32/atl/atl_ax.c
    ${REACTOS_SOURCE_DIR}/dll/win32/atl/registrar.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/atl80_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/atl80.def)

add_library(atl80 SHARED ${SOURCE})
set_module_type(atl80 win32dll)
target_link_libraries(atl80 uuid wine)
add_importlibs(atl80 oleaut32 user32 ole32 gdi32 advapi32 advapi32_vista shlwapi msvcrt kernel32 ntdll)
add_pch(atl80 precomp.h SOURCE)
add_cd_file(TARGET atl80 DESTINATION reactos/system32 FOR all)
