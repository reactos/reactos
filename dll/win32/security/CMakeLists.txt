
add_definitions(-D__SECURITY__)

list(APPEND SOURCE
    security.rc)

add_library(security MODULE ${SOURCE})
spec2def(security security.dll security.spec)

set_module_type(security win32dll ENTRYPOINT 0 UNICODE)
add_importlibs(security secur32)
add_cd_file(TARGET security DESTINATION reactos/system32 FOR all)
