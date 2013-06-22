
set_cpp(WITH_EXCEPTIONS)

add_library(comsupp comsupp.cpp)
add_dependencies(comsupp psdk)
