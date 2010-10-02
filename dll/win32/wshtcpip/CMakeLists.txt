
set_unicode()

include_directories(${REACTOS_SOURCE_DIR}/lib/tdilib)

spec2def(wshtcpip ${CMAKE_CURRENT_SOURCE_DIR}/wshtcpip.spec ${CMAKE_CURRENT_BINARY_DIR}/wshtcpip.def)

add_library(wshtcpip SHARED wshtcpip.c wshtcpip.rc)

set_target_properties(wshtcpip PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(wshtcpip
    ${CMAKE_CURRENT_BINARY_DIR}/wshtcpip.def
    tdilib)

add_importlibs(wshtcpip kernel32 ws2_32 kernel32 ntdll)
add_dependencies(wshtcpip wshtcpip_def psdk)
