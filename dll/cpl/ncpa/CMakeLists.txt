
set_unicode()

spec2def(ncpa ${CMAKE_CURRENT_SOURCE_DIR}/ncpa.spec ${CMAKE_CURRENT_BINARY_DIR}/ncpa.def)

add_library(ncpa SHARED ncpa.c)

set_module_type(ncpa cpl)

target_link_libraries(ncpa ${CMAKE_CURRENT_BINARY_DIR}/ncpa.def)

add_importlibs(ncpa
    msvcrt
    advapi32
    shell32)

add_dependencies(ncpa ncpa_def)
add_cab_target(ncpa 1)
