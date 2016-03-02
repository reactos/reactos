
add_executable(chkdsk chkdsk.c chkdsk.rc)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/fmifs)
set_module_type(chkdsk win32cui UNICODE)
target_link_libraries(chkdsk win32err)
add_importlibs(chkdsk fmifs msvcrt kernel32 ntdll)
add_cd_file(TARGET chkdsk DESTINATION reactos/system32 FOR all)
