
add_library(vbemp SHARED
    edid.c
    vbemp.c
    vbemp.rc)

set_module_type(vbemp kernelmodedriver)
add_importlibs(vbemp videoprt)
add_pch(vbemp vbemp.h)
target_link_libraries(vbemp libcntpr)
add_cd_file(TARGET vbemp DESTINATION reactos/system32/drivers FOR all)
