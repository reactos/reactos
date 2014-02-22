
list(APPEND SOURCE
    edid.c
    vbemp.c
    vbemp.h)

add_library(vbemp SHARED ${SOURCE} vbemp.rc)
set_module_type(vbemp kernelmodedriver)
add_importlibs(vbemp videoprt)
add_pch(vbemp vbemp.h SOURCE)
target_link_libraries(vbemp libcntpr)
add_cd_file(TARGET vbemp DESTINATION reactos/system32/drivers FOR all)
