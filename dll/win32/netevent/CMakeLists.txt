
add_library(netevent MODULE netevent.rc)
set_module_type(netevent module UNICODE ENTRYPOINT 0)
add_dependencies(netevent neteventmsg)
add_cd_file(TARGET netevent DESTINATION reactos/system32 FOR all)
