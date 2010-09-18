


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(samlib ${CMAKE_CURRENT_SOURCE_DIR}/samlib.spec ${CMAKE_CURRENT_BINARY_DIR}/samlib.def)

add_library(samlib SHARED dllmain.c samlib.c samlib.rc)

set_target_properties(samlib PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(samlib
    ${CMAKE_CURRENT_BINARY_DIR}/samlib.def
    -ladvapi32
    mingw_common)

add_dependencies(samlib samlib_def psdk)
