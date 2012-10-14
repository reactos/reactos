
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()

generate_idl_iids(qmgr_local.idl)

spec2def(qmgr.dll qmgr.spec)

list(APPEND SOURCE
    enum_files.c
    enum_jobs.c
    factory.c
    file.c
    job.c
    qmgr.c
    qmgr_main.c
    service.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/qmgr_local_i.c
    ${CMAKE_CURRENT_BINARY_DIR}/qmgr.def)

add_library(qmgr SHARED ${SOURCE})

add_idl_headers(qmgr_idlheader qmgr_local.idl)

set_module_type(qmgr win32dll)
target_link_libraries(qmgr uuid wine)
add_importlibs(qmgr ole32 wininet urlmon advapi32 msvcrt kernel32 ntdll)
add_pch(qmgr qmgr.h)
add_cd_file(TARGET qmgr DESTINATION reactos/system32 FOR all)
add_dependencies(qmgr qmgr_idlheader)
