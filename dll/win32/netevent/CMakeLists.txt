
include_directories(${REACTOS_BINARY_DIR}/include/reactos)
add_library(netevent SHARED netevt.rc)
set_module_type(netevent win32dll UNICODE ENTRYPOINT 0)
add_dependencies(netevent neteventmsg)
add_cd_file(TARGET netevent DESTINATION reactos/system32 FOR all)
