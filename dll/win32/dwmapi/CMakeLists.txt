


add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dwmapi ${CMAKE_CURRENT_SOURCE_DIR}/dwmapi.spec ${CMAKE_CURRENT_BINARY_DIR}/dwmapi.def)

add_library(dwmapi SHARED dwmapi_main.c version.rc)

set_target_properties(dwmapi PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(dwmapi
    ${CMAKE_CURRENT_BINARY_DIR}/dwmapi.def
    wine
    mingw_common)

add_dependencies(dwmapi dwmapi_def psdk)
