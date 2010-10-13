
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdmac ${CMAKE_CURRENT_SOURCE_DIR}/kbdmac.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdmac.def)

add_library(kbdmac SHARED kbdmac.c kbdmac.rc)

set_entrypoint(kbdmac 0)

target_link_libraries(kbdmac ${CMAKE_CURRENT_BINARY_DIR}/kbdmac.def)

add_dependencies(kbdmac kbdmac_def psdk buildno_header)
