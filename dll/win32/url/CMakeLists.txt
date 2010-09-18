


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(url ${CMAKE_CURRENT_SOURCE_DIR}/url.spec ${CMAKE_CURRENT_BINARY_DIR}/url.def)

add_library(url SHARED url_main.c)

set_target_properties(url PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(url
    ${CMAKE_CURRENT_BINARY_DIR}/url.def
    wine
    -lshell32
    -lshlwapi
    mingw_common)

add_dependencies(url url_def psdk)
