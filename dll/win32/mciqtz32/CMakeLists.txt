


add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mciqtz32 ${CMAKE_CURRENT_SOURCE_DIR}/mciqtz32.spec ${CMAKE_CURRENT_BINARY_DIR}/mciqtz32.def)

add_library(mciqtz32 SHARED mciqtz.c version.rc)

set_target_properties(mciqtz32 PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(mciqtz32
    ${CMAKE_CURRENT_BINARY_DIR}/mciqtz32.def
    wine
    -lwinmm
    -loleaut32
    -lole32
    strmiids
    mingw_common
    -luser32
    -lgdi32)

add_dependencies(mciqtz32 mciqtz32_def psdk)
