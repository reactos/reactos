
include_directories(${REACTOS_SOURCE_DIR}/lib/tdilib)

spec2def(wshtcpip ${CMAKE_CURRENT_SOURCE_DIR}/wshtcpip.spec ${CMAKE_CURRENT_BINARY_DIR}/wshtcpip.def)

add_library(wshtcpip SHARED wshtcpip.c wshtcpip.rc)

set_module_type(wshtcpip win32dll)

target_link_libraries(wshtcpip
    ${CMAKE_CURRENT_BINARY_DIR}/wshtcpip.def
    tdilib)

add_importlibs(wshtcpip ntdll ws2_32)
add_dependencies(wshtcpip wshtcpip_def psdk)
