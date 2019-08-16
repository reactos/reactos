
spec2def(dsound.dll dsound.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    capture.c
    capturebuffer.c
    classfactory.c
    devicelist.c
    directsound.c
    dsound.c
    enum.c
    misc.c
    notify.c
    primary.c
    property.c
    regsvr.c
    secondary.c
    stubs.c
    precomp.h
    resource.h)

add_library(dsound MODULE
    ${SOURCE}
    version.rc)

set_module_type(dsound win32dll)
target_link_libraries(dsound dxguid uuid)
add_importlibs(dsound winmm ole32 advapi32 setupapi ksuser user32 msvcrt kernel32 ntdll)
add_pch(dsound precomp.h SOURCE)
add_cd_file(TARGET dsound DESTINATION reactos/system32 FOR all)
