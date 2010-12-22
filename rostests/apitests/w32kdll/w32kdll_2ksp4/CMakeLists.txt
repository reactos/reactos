
add_library(w32kdll_2ksp4 SHARED main.c w32kdll_2ksp4.S)
set_entrypoint(w32kdll_2ksp4 0)
target_link_libraries(w32kdll_2ksp4 ${CMAKE_CURRENT_SOURCE_DIR}/w32kdll_2ksp4.def)
add_dependencies(w32kdll_2ksp4 psdk buildno_header)
add_importlib_target(w32kdll_2ksp4.def)
