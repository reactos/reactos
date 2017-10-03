
spec2def(hal.dll ../hal.spec ADD_IMPORTLIB)

add_definitions(
    -D_NTHALDLL_
    -D_NTHAL_)

include_directories(
    include
    ${REACTOS_SOURCE_DIR}/ntoskrnl/include)

list(APPEND SOURCES
    omap3/halinit_up.c
    omap3/halup.rc)

add_library(hal SHARED
    ${SOURCES})

set_module_type(hal kerneldll ENTRYPOINT 0)
