
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/fmifs)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(chkdsk chkdsk.c chkdsk.rc)
set_module_type(chkdsk win32cui UNICODE)
target_compile_definitions(chkdsk PRIVATE _CRT_NON_CONFORMING_SWPRINTFS)
target_link_libraries(chkdsk conutils ${PSEH_LIB})
add_importlibs(chkdsk fmifs msvcrt kernel32 ntdll)
add_cd_file(TARGET chkdsk DESTINATION reactos/system32 FOR all)
