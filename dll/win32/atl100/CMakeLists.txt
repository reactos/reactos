
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -D_ATL_VER=_ATL_VER_100)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    ${REACTOS_SOURCE_DIR}/dll/win32/atl/atl.c
    ${REACTOS_SOURCE_DIR}/dll/win32/atl/atl_ax.c
    ${REACTOS_SOURCE_DIR}/dll/win32/atl/registrar.c
    precomp.h)

add_library(atl100 MODULE ${SOURCE})
spec2def(atl100 atl100.dll atl100.spec ADD_IMPORTLIB)

set_module_type(atl100 win32dll)
target_link_libraries(atl100 uuid wine)
add_importlibs(atl100 ole32 oleaut32 user32 gdi32 advapi32 advapi32_vista shlwapi msvcrt kernel32 ntdll)
add_pch(atl100 precomp.h SOURCE)
add_cd_file(TARGET atl100 DESTINATION reactos/system32 FOR all)
