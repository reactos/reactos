
add_library(netevent SHARED netevent.rc)
set_module_type(netevent module UNICODE)
add_dependencies(netevent neteventmsg)
add_cd_file(TARGET netevent DESTINATION reactos/system32 FOR all)
