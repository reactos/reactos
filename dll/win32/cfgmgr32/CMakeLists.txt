
spec2def(cfgmgr32 ${CMAKE_CURRENT_SOURCE_DIR}/cfgmgr32.spec ${CMAKE_CURRENT_BINARY_DIR}/cfgmgr32.def)

add_library(cfgmgr32 SHARED cfgmgr32.rc)

set_entrypoint(cfgmgr32 0)

target_link_libraries(cfgmgr32 ${CMAKE_CURRENT_BINARY_DIR}/cfgmgr32.def)

add_dependencies(cfgmgr32 cfgmgr32_def psdk buildno_header)
add_livecd_target(cfgmgr32 reactos/system32)
