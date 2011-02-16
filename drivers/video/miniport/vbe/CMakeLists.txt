
add_library(vbemp SHARED
    edid.c
    vbemp.c
    vbemp.rc)

set_module_type(vbemp kernelmodedriver)
add_importlibs(vbemp videoprt)

target_link_libraries(vbemp libcntpr)

add_cab_target(vbemp 2)
