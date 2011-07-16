
spec2def(twain_32.dll twain_32.spec)

list(APPEND SOURCE
    capability.c
    ds_audio.c
    ds_ctrl.c
    ds_image.c
    dsm_ctrl.c
    twain32_main.c
    twain_32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/twain_32.def)

add_library(twain_32 SHARED ${SOURCE})

set_module_type(twain_32 win32dll)
target_link_libraries(twain_32 wine)
add_importlibs(twain_32 msvcrt kernel32 ntdll)
add_cd_file(TARGET twain_32 DESTINATION reactos/system32 FOR all)
