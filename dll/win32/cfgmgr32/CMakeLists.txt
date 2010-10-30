
spec2def(cfgmgr32.dll cfgmgr32.spec)

add_library(cfgmgr32 SHARED cfgmgr32.rc ${CMAKE_CURRENT_BINARY_DIR}/cfgmgr32.def)

set_entrypoint(cfgmgr32 0)

add_dependencies(cfgmgr32 psdk buildno_header)
add_cab_target(cfgmgr32 1)
