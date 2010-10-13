
set_unicode()

spec2def(telephon ${CMAKE_CURRENT_SOURCE_DIR}/telephon.spec ${CMAKE_CURRENT_BINARY_DIR}/telephon.def)

add_library(telephon SHARED telephon.c telephon.rc)

set_module_type(telephon cpl)

target_link_libraries(telephon ${CMAKE_CURRENT_BINARY_DIR}/telephon.def uuid)

add_importlibs(telephon
    advapi32
    user32
    comctl32
    ole32
    shell32
    kernel32)

add_dependencies(telephon telephon_def)
