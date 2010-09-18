


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(spoolss ${CMAKE_CURRENT_SOURCE_DIR}/spoolss.spec ${CMAKE_CURRENT_BINARY_DIR}/spoolss.def)

add_library(spoolss SHARED spoolss_main.c router.c)

set_target_properties(spoolss PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(spoolss
    ${CMAKE_CURRENT_BINARY_DIR}/spoolss.def
    wine
    -lwinspool
    mingw_common)

add_dependencies(spoolss spoolss_def psdk)
