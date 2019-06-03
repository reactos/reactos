
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    main.c
    mediadet.c
    samplegrabber.c
    timeline.c
    precomp.h)

add_library(qedit MODULE
    ${SOURCE}
    qedit.rc)
spec2def(qedit qedit.dll qedit.spec)

set_module_type(qedit win32dll)
target_link_libraries(qedit strmbase strmiids uuid wine)
add_importlibs(qedit ole32 oleaut32 msvcrt kernel32 ntdll)
add_pch(qedit precomp.h SOURCE)
add_cd_file(TARGET qedit DESTINATION reactos/system32 FOR all)
