
add_library(netmsg MODULE netmsg.rc)
set_module_type(netmsg module UNICODE ENTRYPOINT 0)
add_dependencies(netmsg netmsgmsg)
add_cd_file(TARGET netmsg DESTINATION reactos/system32 FOR all)
