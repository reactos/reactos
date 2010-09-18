


add_definitions(-DUNICODE -D_UNICODE)

spec2def(avicap32 ${CMAKE_CURRENT_SOURCE_DIR}/avicap32.spec ${CMAKE_CURRENT_BINARY_DIR}/avicap32.def)

add_library(avicap32 SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/avicap32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/avicap32.rc)

set_target_properties(avicap32 PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(avicap32
    ${CMAKE_CURRENT_BINARY_DIR}/avicap32.def
    -ladvapi32
    -luser32
    wine
    mingw_common
    -lversion)

add_dependencies(avicap32 avicap32_def psdk)
