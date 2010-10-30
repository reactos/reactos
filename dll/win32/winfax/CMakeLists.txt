
spec2def(winfax.dll winfax.spec)

add_library(winfax SHARED winfax.c winfax.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winfax.def)

set_entrypoint(winfax 0)



add_importlibs(winfax kernel32 ntdll)
add_dependencies(winfax psdk)
