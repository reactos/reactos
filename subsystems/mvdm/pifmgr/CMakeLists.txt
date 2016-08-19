
file(GLOB pifmgr_rc_deps res/*.*)
add_rc_deps(pifmgr.rc ${pifmgr_rc_deps})
add_library(pifmgr SHARED pifmgr.rc)
set_module_type(pifmgr win32dll UNICODE ENTRYPOINT 0)
add_cd_file(TARGET pifmgr DESTINATION reactos/system32 FOR all)
