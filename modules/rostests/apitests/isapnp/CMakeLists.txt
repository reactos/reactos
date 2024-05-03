
include_directories(${REACTOS_SOURCE_DIR}/drivers/bus/isapnp)

list(APPEND SOURCE
    empty_card.c
    isabus.c
    res_card.c
    tests.c)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(isapnp_apitest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

set_module_type(isapnp_apitest win32cui)
add_importlibs(isapnp_apitest msvcrt kernel32 ntdll)
add_pch(isapnp_apitest precomp.h "${PCH_SKIP_SOURCE}")

add_rostests_file(TARGET isapnp_apitest)
