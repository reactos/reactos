
add_definitions(-D_DLL -D__USE_CRTIMP)

add_library(twain_32 SHARED
    capability.c
    ds_audio.c
    ds_ctrl.c
    ds_image.c
    dsm_ctrl.c
    twain32_main.c
    twain_32.rc
    twain_32.def)

if(NOT MSVC)
    set_source_files_properties(twain_32.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(twain_32 win32dll)

target_link_libraries(twain_32 wine)

add_importlibs(twain_32 msvcrt kernel32 ntdll)

add_cab_target(twain_32 1)
