
set_unicode()

spec2def(samsrv ${CMAKE_CURRENT_SOURCE_DIR}/samsrv.spec ${CMAKE_CURRENT_BINARY_DIR}/samsrv.def)

add_library(samsrv SHARED samsrv.c samsrv.rc)

set_entrypoint(samsrv 0)

target_link_libraries(samsrv ${CMAKE_CURRENT_BINARY_DIR}/samsrv.def)

add_importlibs(samsrv kernel32 ntdll)
add_dependencies(samsrv samsrv_def psdk buildno_header)
