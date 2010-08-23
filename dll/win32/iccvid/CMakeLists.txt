
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(iccvid ${CMAKE_CURRENT_SOURCE_DIR}/iccvid.spec ${CMAKE_CURRENT_BINARY_DIR}/iccvid.def)

add_library(iccvid SHARED iccvid.c rsrc.rc)

set_target_properties(iccvid PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(iccvid
    ${CMAKE_CURRENT_BINARY_DIR}/iccvid.def
    wine
    ${REACTOS_SOURCE_DIR}/dll/win32/user32/libuser32.a
    mingw_common)

add_dependencies(iccvid iccvid_def psdk)
