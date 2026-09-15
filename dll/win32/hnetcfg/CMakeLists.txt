
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(hnetcfg.dll hnetcfg.spec)

list(APPEND SOURCE
    apps.c
    hnetcfg.c
    manager.c
    policy.c
    port.c
    profile.c
    service.c
    precomp.h)

add_library(hnetcfg MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/hnetcfg.def)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(hnetcfg PRIVATE -Wno-old-style-declaration)
endif()

set_module_type(hnetcfg win32dll)
add_idl_reg_scripts(hnetcfg regtypelib hnetcfg_tlb.idl)
add_idl_reg_scripts(hnetcfg registry hnetcfg.idl)
add_dependencies(hnetcfg stdole2)
target_link_libraries(hnetcfg wine wine_dll_register wine_dll_canunload uuid oldnames msvcrtex)
add_importlibs(hnetcfg ole32 oleaut32 winhttp ws2_32 shcore advapi32 mpr xmllite msvcrt kernel32 kernel32_vista ntdll)
add_pch(hnetcfg precomp.h SOURCE)
add_cd_file(TARGET hnetcfg DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(hnetcfg) # CORE-5743: No ARRAY_SIZE and CONST_VTABLE
