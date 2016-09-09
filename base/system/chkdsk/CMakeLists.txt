
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/fmifs)

add_executable(chkdsk chkdsk.c chkdsk.rc)
set_module_type(chkdsk win32cui UNICODE)
add_importlibs(chkdsk fmifs msvcrt kernel32 ntdll)
add_cd_file(TARGET chkdsk DESTINATION reactos/system32 FOR all)
