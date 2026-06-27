
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D_ATL_VER=_ATL_VER_30)

spec2def(atl.dll atl.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    atl.c
    atl30.c
    atl_ax.c
    registrar.c
    ${CMAKE_CURRENT_BINARY_DIR}/atl_stubs.c
    precomp.h)

add_library(atl MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/atl.def)

add_dependencies(atl stdole2)
set_module_type(atl win32dll)
add_idl_reg_scripts(atl registry atl_classes.idl)
add_idl_reg_scripts(atl regtypelib atl_lib.idl)
target_link_libraries(atl uuid wine wine_dll_register wine_dll_canunload oldnames)
add_importlibs(atl oleaut32 ole32 user32 gdi32 advapi32 advapi32_vista shlwapi msvcrt kernel32 ntdll)
add_pch(atl precomp.h SOURCE)
add_cd_file(TARGET atl DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(atl) # CORE-5743: No ARRAY_SIZE macro
