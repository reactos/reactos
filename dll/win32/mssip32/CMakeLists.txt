


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(mssip32 ${CMAKE_CURRENT_SOURCE_DIR}/mssip32.spec ${CMAKE_CURRENT_BINARY_DIR}/mssip32.def)

add_library(mssip32 SHARED main.c)

set_target_properties(mssip32 PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(mssip32
    ${CMAKE_CURRENT_BINARY_DIR}/mssip32.def
    wine
    mingw_common)

add_dependencies(mssip32 mssip32_def psdk)
