
add_definitions(-D_DLL -D__USE_CRTIMP)

spec2def(twain_32 ${CMAKE_CURRENT_SOURCE_DIR}/twain_32.spec ${CMAKE_CURRENT_BINARY_DIR}/twain_32.def)

add_library(twain_32 SHARED
    capability.c
    ds_audio.c
    ds_ctrl.c
    ds_image.c
    dsm_ctrl.c
    twain32_main.c
    twain_32.rc)

set_module_type(twain_32 win32dll)

target_link_libraries(twain_32
    ${CMAKE_CURRENT_BINARY_DIR}/twain_32.def
    wine)

add_importlibs(twain_32 msvcrt kernel32 ntdll)
add_dependencies(twain_32 twain_32_def)
add_cab_target(twain_32 1)
