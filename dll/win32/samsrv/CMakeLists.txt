


add_definitions(-DUNICODE -D_UNICODE)

spec2def(samsrv ${CMAKE_CURRENT_SOURCE_DIR}/samsrv.spec ${CMAKE_CURRENT_BINARY_DIR}/samsrv.def)

add_library(samsrv SHARED samsrv.c samsrv.rc)

set_target_properties(samsrv PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(samsrv
    ${CMAKE_CURRENT_BINARY_DIR}/samsrv.def
    mingw_common)

add_dependencies(samsrv samsrv_def psdk)
