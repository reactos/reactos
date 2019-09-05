
list(APPEND SOURCE
    QueryPoints.c
    precomp.h)

add_executable(mountmgr_apitest ${SOURCE} testlist.c)
target_link_libraries(mountmgr_apitest wine ${PSEH_LIB})
set_module_type(mountmgr_apitest win32cui)
add_importlibs(mountmgr_apitest msvcrt kernel32 ntdll)
add_pch(mountmgr_apitest precomp.h SOURCE)
add_rostests_file(TARGET mountmgr_apitest)
