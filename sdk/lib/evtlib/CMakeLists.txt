
## FIXME: Make the library cross-compiling aware (like cmlib or inflib)

add_library(evtlib STATIC evtlib.c)
add_dependencies(evtlib xdk)
target_include_directories(evtlib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
