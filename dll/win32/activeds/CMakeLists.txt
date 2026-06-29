
spec2def(activeds.dll activeds.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    activeds_main.c
    pathname.c
    stubs.c
    precomp.h)

add_library(activeds MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/activeds.def)

if(MSVC)
    target_compile_options(activeds PRIVATE /wd4267) # conversion from 'size_t' to 'int', possible loss of data
endif()

set_module_type(activeds win32dll)
add_idl_reg_scripts(activeds registry activeds.idl)
target_link_libraries(activeds wine wine_dll_register wine_dll_canunload uuid oldnames)
add_importlibs(activeds ole32 oleaut32 advapi32 msvcrt kernel32 ntdll)
add_pch(activeds precomp.h SOURCE)
add_cd_file(TARGET activeds DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(activeds) # CORE-5743: No ARRAY_SIZE macro
