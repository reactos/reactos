
set_cpp(WITH_EXCEPTIONS WITH_STL)
add_library(comsupp comsupp.cpp)
add_dependencies(comsupp psdk)
