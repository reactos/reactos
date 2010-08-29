
set(CMAKE_RC_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(shdoclc ${CMAKE_CURRENT_SOURCE_DIR}/shdoclc.spec ${CMAKE_CURRENT_BINARY_DIR}/shdoclc.def)

add_library(shdoclc SHARED rsrc.rc)

set_target_properties(shdoclc PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(shdoclc
    ${CMAKE_CURRENT_BINARY_DIR}/shdoclc.def
    wine
    mingw_common)

add_dependencies(shdoclc shdoclc_def psdk)
