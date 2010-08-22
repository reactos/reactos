
set(CMAKE_RC_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

spec2def(cfgmgr32 ${CMAKE_CURRENT_SOURCE_DIR}/cfgmgr32.spec ${CMAKE_CURRENT_BINARY_DIR}/cfgmgr32.def)

add_library(cfgmgr32 SHARED cfgmgr32.rc)

set_target_properties(cfgmgr32 PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(cfgmgr32 ${CMAKE_CURRENT_BINARY_DIR}/cfgmgr32.def)

add_dependencies(cfgmgr32 cfgmgr32_def psdk)
