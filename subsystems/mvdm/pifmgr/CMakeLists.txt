
file(GLOB pifmgr_rc_deps "res/*.*" "res_alt/*.*")
add_rc_deps(pifmgr.rc ${pifmgr_rc_deps})
add_library(pifmgr MODULE pifmgr.rc)
set_module_type(pifmgr module UNICODE)
add_cd_file(TARGET pifmgr DESTINATION reactos/system32 FOR all)
