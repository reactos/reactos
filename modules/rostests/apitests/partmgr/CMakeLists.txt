
list(APPEND SOURCE
    StorDeviceNumber.c)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(partmgr_apitest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

target_link_libraries(partmgr_apitest wine ${PSEH_LIB})
set_module_type(partmgr_apitest win32cui)
add_importlibs(partmgr_apitest msvcrt kernel32 ntdll)
# TODO: Enable this when we get more than one source file to justify its use
#add_pch(partmgr_apitest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET partmgr_apitest)
