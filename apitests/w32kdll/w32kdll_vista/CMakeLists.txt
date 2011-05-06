
add_library(w32kdll_vista SHARED main.c w32kdll_vista.S)
set_entrypoint(w32kdll_vista 0)
target_link_libraries(w32kdll_vista ${CMAKE_CURRENT_SOURCE_DIR}/w32kdll_vista.def)
add_dependencies(w32kdll_vista psdk buildno_header)
add_importlib_target(w32kdll_vista.def)
