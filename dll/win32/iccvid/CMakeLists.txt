


add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(iccvid ${CMAKE_CURRENT_SOURCE_DIR}/iccvid.spec ${CMAKE_CURRENT_BINARY_DIR}/iccvid.def)

add_library(iccvid SHARED iccvid.c rsrc.rc)

set_target_properties(iccvid PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(iccvid
    ${CMAKE_CURRENT_BINARY_DIR}/iccvid.def
    wine
    -luser32
    mingw_common)

add_dependencies(iccvid iccvid_def psdk)
