
include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/subsys
    ${REACTOS_SOURCE_DIR}/win32ss/include)

spec2def(winsrv.dll winsrv.spec)

list(APPEND SOURCE
    harderror.c
    init.c
    register.c
    shutdown.c
    winsrv.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winsrv.def)

add_library(winsrv SHARED ${SOURCE})

target_link_libraries(winsrv win32ksys ${PSEH_LIB}) # win32ksys because of NtUser...()

set_module_type(winsrv win32dll UNICODE)

add_importlibs(winsrv msvcrt kernel32 ntdll csrsrv)
add_delay_importlibs(winsrv user32 gdi32 advapi32)

add_dependencies(winsrv bugcodes)
add_cd_file(TARGET winsrv DESTINATION reactos/system32 FOR all)
