
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(printui ${CMAKE_CURRENT_SOURCE_DIR}/printui.spec ${CMAKE_CURRENT_BINARY_DIR}/printui.def)

add_library(printui SHARED printui.c printui.rc)

set_target_properties(printui PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(printui
    ${CMAKE_CURRENT_BINARY_DIR}/printui.def
    wine
    -lshell32
    mingw_common)

add_dependencies(printui printui_def psdk)
