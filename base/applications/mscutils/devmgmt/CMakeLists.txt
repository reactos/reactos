
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/dll/devmgr)

add_executable(devmgmt devmgmt.c devmgmt.rc)
set_module_type(devmgmt win32gui UNICODE)
add_importlibs(devmgmt devmgr msvcrt kernel32)
add_cd_file(TARGET devmgmt DESTINATION reactos/system32 FOR all)
