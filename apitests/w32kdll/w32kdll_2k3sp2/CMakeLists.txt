
add_library(w32kdll_2k3sp2 SHARED main.c w32kdll_2k3sp2.S)
set_entrypoint(w32kdll_2k3sp2 0)
target_link_libraries(w32kdll_2k3sp2 ${CMAKE_CURRENT_SOURCE_DIR}/w32kdll_2k3sp2.def)
add_dependencies(w32kdll_2k3sp2 psdk buildno_header)
add_importlib_target(w32kdll_2k3sp2.def)
