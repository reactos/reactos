
set_unicode()

include_directories(${REACTOS_SOURCE_DIR}/lib/tdilib)

spec2def(wshtcpip.dll wshtcpip.spec)

add_library(wshtcpip SHARED wshtcpip.c wshtcpip.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wshtcpip.def)

set_entrypoint(wshtcpip 0)

target_link_libraries(wshtcpip tdilib)

add_importlibs(wshtcpip ws2_32 kernel32 ntdll)
add_dependencies(wshtcpip psdk buildno_header)

add_livecd_target(wshtcpip reactos/system32)