
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers)

list(APPEND SOURCE
    AfdHelpers.c
    send.c
    windowsize.c)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(afd_apitest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

target_link_libraries(afd_apitest wine)
set_module_type(afd_apitest win32cui)
add_importlibs(afd_apitest ws2_32 msvcrt kernel32 ntdll)
add_pch(afd_apitest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET afd_apitest)
