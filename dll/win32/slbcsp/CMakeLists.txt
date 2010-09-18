


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(slbcsp ${CMAKE_CURRENT_SOURCE_DIR}/slbcsp.spec ${CMAKE_CURRENT_BINARY_DIR}/slbcsp.def)

add_library(slbcsp SHARED main.c)

set_target_properties(slbcsp PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(slbcsp
    ${CMAKE_CURRENT_BINARY_DIR}/slbcsp.def
    wine
    mingw_common)

add_dependencies(slbcsp slbcsp_def psdk)
