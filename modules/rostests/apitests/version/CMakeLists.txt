
list(APPEND SOURCE
    VerQueryValue.c
    precomp.h)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(version_apitest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

target_link_libraries(version_apitest wine uuid ${PSEH_LIB})
set_module_type(version_apitest win32cui)
add_importlibs(version_apitest version msvcrt kernel32 ntdll)
add_pch(version_apitest precomp.h "${PCH_SKIP_SOURCE}")

add_rostests_file(TARGET version_apitest)
