
add_definitions(-D__SECURITY__)
spec2def(security.dll security.spec)

list(APPEND SOURCE
    security.rc
    ${CMAKE_CURRENT_BINARY_DIR}/security.def)

add_library(security MODULE ${SOURCE})
set_module_type(security win32dll ENTRYPOINT 0 UNICODE)
add_importlibs(security secur32)
add_cd_file(TARGET security DESTINATION reactos/system32 FOR all)
