
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/sound
    ${REACTOS_SOURCE_DIR}/drivers/wdm/audio/legacy/wdmaud
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/sound/mmixer
    ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/libsamplerate)

list(APPEND SOURCE
    wdmaud.c
    mixer.c
    mmixer.c
    legacy.c
    wdmaud.h)

add_library(wdmaud.drv MODULE
    ${SOURCE}
    wdmaud.rc)
spec2def(wdmaud.drv wdmaud.drv wdmaud.spec)

set_module_type(wdmaud.drv win32dll UNICODE)
set_target_properties(wdmaud.drv PROPERTIES SUFFIX "")
target_link_libraries(wdmaud.drv mmebuddy libsamplerate mmixer)
add_importlibs(wdmaud.drv user32 winmm advapi32 msvcrt setupapi ksuser kernel32 ntdll)
add_pch(wdmaud.drv wdmaud.h SOURCE)
add_cd_file(TARGET wdmaud.drv DESTINATION reactos/system32 FOR all)
