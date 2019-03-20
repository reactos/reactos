
list(APPEND SOURCE
    enable.c
    debug.c
    palette.c
    screen.c)

add_library(vgaddi MODULE
    ${SOURCE}
    vga_new.rc)

set_module_type(vgaddi kerneldll ENTRYPOINT DrvEnableDriver 12)

add_importlibs(vgaddi win32k)
target_link_libraries(vgaddi libcntpr)
add_dependencies(vgaddi psdk)
add_cd_file(TARGET vgaddi DESTINATION reactos/system32 FOR all)
