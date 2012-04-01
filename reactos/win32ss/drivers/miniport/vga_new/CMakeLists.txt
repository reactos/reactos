
list(APPEND SOURCE
    modeset.c
    vgadata.c
    vga.c
    vbemodes.c
    vbe.c)

add_library(vga SHARED
    ${SOURCE}
    vga.rc)

target_link_libraries(vga libcntpr)

# FIXME: http://www.cmake.org/Bug/view.php?id=12998
if(MSVC)
    #add_target_compile_flags(vga "/Gz")
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "/Gz")
else()
    #add_target_compile_flags(vga "-mrtd -fno-builtin")
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-mrtd -fno-builtin")
endif()

set_module_type(vga kernelmodedriver)
add_importlibs(vga videoprt)

add_pch(vga vga.h)

add_cd_file(TARGET vga DESTINATION reactos/system32/drivers FOR all)
