
spec2def(winfax ${CMAKE_CURRENT_SOURCE_DIR}/winfax.spec ${CMAKE_CURRENT_BINARY_DIR}/winfax.def)

add_library(winfax SHARED winfax.c winfax.rc)

set_entrypoint(winfax 0)

target_link_libraries(winfax ${CMAKE_CURRENT_BINARY_DIR}/winfax.def)

add_importlibs(winfax kernel32 ntdll)
add_dependencies(winfax winfax_def psdk)
