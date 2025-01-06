
list(APPEND SOURCE
    QueryDosVolumePaths.c
    QueryPoints.c
    utils.c)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(mountmgr_apitest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

add_pch(mountmgr_apitest precomp.h "${PCH_SKIP_SOURCE}")
target_link_libraries(mountmgr_apitest wine ${PSEH_LIB})
set_module_type(mountmgr_apitest win32cui)
add_importlibs(mountmgr_apitest msvcrt kernel32 ntdll)
add_rostests_file(TARGET mountmgr_apitest)
