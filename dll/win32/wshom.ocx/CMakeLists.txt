
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(wshom.ocx wshom.ocx.spec)
add_idl_headers(wshom_idlheader wshom.idl)

list(APPEND SOURCE
    network.c
    shell.c
    wshom_main.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(wshom MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    wshom.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wshom.def)

list(APPEND wshom_rc_deps ${CMAKE_CURRENT_SOURCE_DIR}/wshom.rgs)

set_source_files_properties(wshom.rc PROPERTIES OBJECT_DEPENDS "${wshom_rc_deps}")
set_module_type(wshom win32ocx)
add_idl_reg_scripts(wshom regtypelib wshom.idl)
target_link_libraries(wshom uuid wine wine_dll_register wine_dll_canunload oldnames)
add_importlibs(wshom oleaut32 ole32 scrrun shell32 advapi32 advapi32_vista user32 msvcrt kernel32 ntdll)
add_dependencies(wshom stdole2 wshom_idlheader)
add_pch(wshom precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET wshom DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(wshom) # CORE-5743: No ARRAY_SIZE and CONST_VTABLE
