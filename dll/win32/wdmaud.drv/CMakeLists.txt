
add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-DNDEBUG=1)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/sound)
include_directories(${REACTOS_SOURCE_DIR}/drivers/wdm/audio/legacy/wdmaud)
include_directories(${REACTOS_SOURCE_DIR}/lib/drivers/sound/mmixer)
include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/libsamplerate)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/wdmaud.drv.def
    COMMAND native-winebuild -o ${CMAKE_CURRENT_BINARY_DIR}/wdmaud.drv.def --def -E ${CMAKE_CURRENT_SOURCE_DIR}/wdmaud.spec --filename wdmaud.drv
    DEPENDS native-winebuild)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/wdmaud.drv.def PROPERTIES GENERATED TRUE)
add_custom_target(wdmaud.drv_def ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/wdmaud.drv.def)

add_library(wdmaud.drv SHARED
    wdmaud.c
    mixer.c
    mmixer.c
    legacy.c
    wdmaud.rc)

set_module_type(wdmaud.drv win32dll)
set_target_properties(wdmaud.drv PROPERTIES SUFFIX "")

target_link_libraries(wdmaud.drv
    ${CMAKE_CURRENT_BINARY_DIR}/wdmaud.drv.def
    mmebuddy
    libsamplerate
    mmixer)

add_importlibs(wdmaud.drv ntdll user32 winmm advapi32 msvcrt setupapi ksuser)
add_dependencies(wdmaud.drv wdmaud.drv_def psdk)
