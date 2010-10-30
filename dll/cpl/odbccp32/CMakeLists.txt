
set_unicode()

spec2def(odbccp32i.cpl odbccp32.spec)

add_library(odbccp32i SHARED odbccp32.c odbccp32.rc ${CMAKE_CURRENT_BINARY_DIR}/odbccp32.def)

set_module_type(odbccp32i cpl)

add_importlibs(odbccp32i user32 comctl32 kernel32)
add_cab_target(odbccp32i 1)
