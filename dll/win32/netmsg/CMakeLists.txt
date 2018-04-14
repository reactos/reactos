
add_library(netmsg SHARED netmsg.rc)
set_module_type(netmsg module UNICODE)
add_dependencies(netmsg netmsgmsg)
add_cd_file(TARGET netmsg DESTINATION reactos/system32 FOR all)
