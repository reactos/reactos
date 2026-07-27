# Source list mirrors dlls/amstream/Makefile.in upstream; keep them in sync on resync.

# ddrawstream.c uses CONDITION_VARIABLE, which the PSDK gates behind
# _WIN32_WINNT >= 0x600. The backing exports come from kernel32_vista below.
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)

add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -D_WIN32_WINNT=0x0600
    -DWINVER=0x0600)

spec2def(amstream.dll amstream.spec)

list(APPEND SOURCE
    audiodata.c
    audiostream.c
    ddrawstream.c
    filter.c
    main.c
    multimedia.c)

add_library(amstream MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/amstream.def)

# amstream_classes.idl carries "#pragma makedep register": it is the COM
# registration data for DllRegisterServer, not a header. Must come after
# add_library(): the helper appends to the existing target.
add_idl_reg_scripts(amstream registry amstream_classes.idl)

set_module_type(amstream win32dll)
# wine_dll_register/wine_dll_canunload supply the DllRegisterServer,
# DllUnregisterServer and DllCanUnloadNow exports declared in amstream.spec;
# upstream Wine generates those from its own build glue.
target_link_libraries(amstream wine wine_dll_register wine_dll_canunload strmbase strmiids uuid)
add_importlibs(amstream ole32 advapi32 ddraw user32 kernel32_vista msvcrt kernel32 ntdll)
add_dependencies(amstream wineheaders)
add_cd_file(TARGET amstream DESTINATION reactos/system32 FOR all)
set_wine_module(amstream)
