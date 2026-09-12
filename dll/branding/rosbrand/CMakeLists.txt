
add_library(rosbrand MODULE
    rosbrand.rc)

set_entrypoint(rosbrand 0)
add_cd_file(TARGET rosbrand DESTINATION reactos/system32 FOR all)
