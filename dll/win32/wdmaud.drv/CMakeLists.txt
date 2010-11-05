
set_unicode()

add_definitions(-DNDEBUG=1)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/libs/sound
    ${REACTOS_SOURCE_DIR}/drivers/wdm/audio/legacy/wdmaud
    ${REACTOS_SOURCE_DIR}/lib/drivers/sound/mmixer
    ${REACTOS_SOURCE_DIR}/lib/3rdparty/libsamplerate)

add_library(wdmaud.drv SHARED
    wdmaud.c
    mixer.c
    mmixer.c
    legacy.c
    wdmaud.rc
    wdmaud.def)

if(NOT MSVC)
    set_source_files_properties(wdmaud.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(wdmaud.drv win32dll)

set_target_properties(wdmaud.drv PROPERTIES SUFFIX "")

target_link_libraries(wdmaud.drv
    mmebuddy
    libsamplerate
    mmixer)

add_importlibs(wdmaud.drv user32 winmm advapi32 msvcrt setupapi ksuser kernel32 ntdll)

add_cab_target(wdmaud.drv 1)