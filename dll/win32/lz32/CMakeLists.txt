
set(CMAKE_RC_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(lz32 ${CMAKE_CURRENT_SOURCE_DIR}/lz32.spec ${CMAKE_CURRENT_BINARY_DIR}/lz32.def)

add_library(lz32 SHARED version.rc)

set_target_properties(lz32 PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(lz32
    ${CMAKE_CURRENT_BINARY_DIR}/lz32.def
    wine
    mingw_common)

add_dependencies(lz32 lz32_def psdk)
