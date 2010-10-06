
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/winsock)

spec2def(rasadhlp ${CMAKE_CURRENT_SOURCE_DIR}/rasadhlp.spec ${CMAKE_CURRENT_BINARY_DIR}/rasadhlp.def)

add_library(rasadhlp SHARED
    autodial.c
    init.c
    winsock.c)

set_module_type(rasadhlp win32dll)

target_link_libraries(rasadhlp
    ${CMAKE_CURRENT_BINARY_DIR}/rasadhlp.def)

add_importlibs(rasadhlp
    ws2_32
    kernel32
    ntdll)

add_dependencies(rasadhlp rasadhlp_def)
