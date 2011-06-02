
list(APPEND SOURCE
    modeset.c
    vgadata.c
    vga.c
    vbemodes.c
    vbe.c
    vga.rc)

add_library(vga SHARED ${SOURCE})

target_link_libraries(vga libcntpr)

if(MSVC)
set_target_properties(vga PROPERTIES COMPILE_FLAGS "/Gz")
else()
set_target_properties(vga PROPERTIES COMPILE_FLAGS "-mrtd -fno-builtin")
endif()

set_module_type(vga kernelmodedriver)
add_importlibs(vga videoprt)

add_pch(vga vga.h)

add_cd_file(TARGET vga DESTINATION reactos/system32/drivers FOR all)
