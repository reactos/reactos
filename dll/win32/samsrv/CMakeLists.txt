
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-DUNICODE -D_UNICODE)

spec2def(samsrv ${CMAKE_CURRENT_SOURCE_DIR}/samsrv.spec ${CMAKE_CURRENT_BINARY_DIR}/samsrv.def)

add_library(samsrv SHARED samsrv.c samsrv.rc)

set_target_properties(samsrv PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(samsrv
    ${CMAKE_CURRENT_BINARY_DIR}/samsrv.def
    mingw_common)

add_dependencies(samsrv samsrv_def psdk)
