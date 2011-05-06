
add_library(w32kdll SHARED main.c)

set_entrypoint(w32kdll 0)

target_link_libraries(w32kdll
    ${CMAKE_CURRENT_SOURCE_DIR}/w32kdll_ros.def
    win32ksys)

add_dependencies(w32kdll psdk buildno_header)
add_importlib_target(w32kdll_ros.def)
