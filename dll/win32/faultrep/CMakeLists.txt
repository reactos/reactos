
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(faultrep ${CMAKE_CURRENT_SOURCE_DIR}/faultrep.spec ${CMAKE_CURRENT_BINARY_DIR}/faultrep.def)

add_library(faultrep SHARED faultrep.c)

set_target_properties(faultrep PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(faultrep
    ${CMAKE_CURRENT_BINARY_DIR}/faultrep.def
    wine
    mingw_common
    ${REACTOS_SOURCE_DIR}/dll/win32/advapi32/libadvapi32.a)

add_dependencies(faultrep faultrep_def psdk)
