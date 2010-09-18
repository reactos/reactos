


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)
add_definitions(-D_WINE)

spec2def(msvidc32 ${CMAKE_CURRENT_SOURCE_DIR}/msvidc32.spec ${CMAKE_CURRENT_BINARY_DIR}/msvidc32.def)

add_library(msvidc32 SHARED msvideo1.c rsrc.rc)

set_target_properties(msvidc32 PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(msvidc32
    ${CMAKE_CURRENT_BINARY_DIR}/msvidc32.def
    wine
    -luser32
    mingw_common)

add_dependencies(msvidc32 msvidc32_def psdk)
