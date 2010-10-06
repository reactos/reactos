
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(softpub ${CMAKE_CURRENT_SOURCE_DIR}/softpub.spec ${CMAKE_CURRENT_BINARY_DIR}/softpub.def)

add_library(softpub SHARED softpub.rc)

set_entrypoint(softpub 0)

target_link_libraries(softpub
    ${CMAKE_CURRENT_BINARY_DIR}/softpub.def
    wine)

add_importlibs(softpub wintrust)

add_dependencies(softpub softpub_def)
