
set_unicode()

spec2def(telephon.cpl telephon.spec)

add_library(telephon SHARED telephon.c telephon.rc ${CMAKE_CURRENT_BINARY_DIR}/telephon.def)

set_module_type(telephon cpl)

target_link_libraries(telephon uuid)

add_importlibs(telephon
    advapi32
    user32
    comctl32
    ole32
    shell32
    kernel32)

add_cab_target(telephon 1)
