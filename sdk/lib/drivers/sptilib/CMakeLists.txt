
add_library(sptilib STATIC sptilib.c)
target_link_libraries(sptilib PRIVATE ${PSEH_LIB})
add_dependencies(sptilib xdk)
target_include_directories(sptilib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
