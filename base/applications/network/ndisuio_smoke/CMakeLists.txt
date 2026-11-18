
include_directories(
    BEFORE include
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers/ndisuio)

add_executable(ndisuio_smoke ndisuio_smoke.c)
set_module_type(ndisuio_smoke win32cui UNICODE)
add_importlibs(ndisuio_smoke msvcrt kernel32 ntdll)
add_cd_file(TARGET ndisuio_smoke DESTINATION VerOS/system32 FOR all)
