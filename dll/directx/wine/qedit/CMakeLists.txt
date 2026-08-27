
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -DUSE_NEW_WINE_REGISTER_RESOURCES)

spec2def(qedit.dll qedit.spec)

list(APPEND SOURCE
    main.c
    mediadet.c
    nullrenderer.c
    samplegrabber.c
    timeline.c)

add_library(qedit MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/qedit.def)

add_idl_reg_scripts(qedit registry qedit_classes.idl)

set_module_type(qedit win32dll)
target_link_libraries(qedit wine wine_dll_canunload strmbase strmiids uuid oldnames)
add_importlibs(qedit ole32 oleaut32 advapi32 user32 kernel32_vista msvcrt kernel32 ntdll)
add_dependencies(qedit wineheaders)
add_cd_file(TARGET qedit DESTINATION reactos/system32 FOR all)
set_wine_module(qedit)
