
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(shdoclc ${CMAKE_CURRENT_SOURCE_DIR}/shdoclc.spec ${CMAKE_CURRENT_BINARY_DIR}/shdoclc.def)

add_library(shdoclc SHARED rsrc.rc)

set_entrypoint(shdoclc 0)

target_link_libraries(shdoclc
    ${CMAKE_CURRENT_BINARY_DIR}/shdoclc.def
    wine)

add_importlibs(shdoclc kernel32 ntdll)
add_dependencies(shdoclc shdoclc_def psdk buildno_header)
