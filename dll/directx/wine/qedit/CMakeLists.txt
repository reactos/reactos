
spec2def(qedit.dll qedit.spec)
add_definitions(-Dwcsdup=_wcsdup)
list(APPEND SOURCE
    guids.c
    main.c
    mediadet.c
    nullrenderer.c
    samplegrabber.c
    timeline.c)

add_library(qedit MODULE
    ${SOURCE}
    qedit.rc
    ${CMAKE_CURRENT_BINARY_DIR}/qedit.def)

set_module_type(qedit win32dll)
target_link_libraries(qedit strmbase strmiids uuid wine)
add_importlibs(qedit ole32 oleaut32 msvcrt user32 kernel32_vista kernel32 ntdll)
add_dependencies(qedit dxsdk)
#add_pch(qedit precomp.h SOURCE)
add_cd_file(TARGET qedit DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(qedit) # CORE-5743: No ARRAY_SIZE and CONST_VTABLE
