
add_definitions(-D__WINESRC__)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(wintrust.dll wintrust.spec ADD_IMPORTLIB)

add_definitions(-Dwcsdup=_wcsdup)

list(APPEND SOURCE
    asn.c
    crypt.c
    register.c
    softpub.c
    wintrust_main.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/wintrust_stubs.c)

add_library(wintrust MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wintrust.def)

if(MSVC)
    # Disable warning C4090: 'function': different 'const' qualifiers
    target_compile_options(wintrust PRIVATE /wd4090)
endif()

set_module_type(wintrust win32dll)
target_link_libraries(wintrust wine ${PSEH_LIB})
# FIXME: imagehlp should be delay-imported. See CORE-6504
#add_delay_importlibs(wintrust cryptui imagehlp)
add_delay_importlibs(wintrust cryptui)
add_importlibs(wintrust imagehlp crypt32 user32 advapi32 msvcrt kernel32 ntdll)
add_pch(wintrust precomp.h SOURCE)
add_cd_file(TARGET wintrust DESTINATION reactos/system32 FOR all)
