
add_library(comsupp comsupp.cpp)
target_link_libraries(comsupp PRIVATE cppstl)
set_target_cpp_properties(comsupp WITH_EXCEPTIONS)

add_dependencies(comsupp psdk)
