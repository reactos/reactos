
set_unicode()
include_directories(${REACTOS_BINARY_DIR}/include/reactos)
add_library(netevent SHARED netevt.rc)
set_entrypoint(netevent 0)
set_image_base(netevent ${baseaddress_netevent}) 
add_dependencies(netevent neteventmsg)
add_cd_file(TARGET netevent DESTINATION reactos/system32 FOR all)
