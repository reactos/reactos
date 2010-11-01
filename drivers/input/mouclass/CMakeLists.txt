


add_library(mouclass SHARED
    misc.c
    mouclass.c
    mouclass.rc)

set_module_type(mouclass kernelmodedriver)

target_link_libraries(mouclass pseh)

add_importlibs(mouclass ntoskrnl hal)

add_cab_target(mouclass 2)