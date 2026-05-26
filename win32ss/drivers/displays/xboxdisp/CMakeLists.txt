
list(APPEND SOURCE
    enable.c
    screen.c
    surface.c
    palette.c
    pointer.c
    accel.c
    dd.c
    xboxdisp.h)

add_library(xboxdisp MODULE
    ${SOURCE}
    xboxdisp.rc)

set_module_type(xboxdisp kerneldll ENTRYPOINT DrvEnableDriver 12)
add_pch(xboxdisp xboxdisp.h SOURCE)
target_link_libraries(xboxdisp libcntpr)
add_importlibs(xboxdisp win32k)
add_dependencies(xboxdisp psdk)

if(SARCH STREQUAL "xbox")
    add_cd_file(TARGET xboxdisp DESTINATION reactos/system32 FOR all)
endif()
