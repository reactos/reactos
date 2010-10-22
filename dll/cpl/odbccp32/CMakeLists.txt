
set_unicode()

spec2def(odbccp32i ${CMAKE_CURRENT_SOURCE_DIR}/odbccp32.spec ${CMAKE_CURRENT_BINARY_DIR}/odbccp32.def)

add_library(odbccp32i SHARED odbccp32.c odbccp32.rc)

set_module_type(odbccp32i cpl)

target_link_libraries(odbccp32i ${CMAKE_CURRENT_BINARY_DIR}/odbccp32.def)

add_importlibs(odbccp32i user32 comctl32 kernel32)
add_dependencies(odbccp32i odbccp32i_def)
add_livecd_target(odbccp32i reactos/system32)
