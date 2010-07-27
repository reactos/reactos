file(GLOB_RECURSE SOURCE "*.cpp")

add_library(cardlib ${SOURCE})
add_dependencies(cardlib psdk)