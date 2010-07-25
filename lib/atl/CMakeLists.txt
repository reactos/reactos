
file(GLOB_RECURSE SOURCE "*.cpp")

add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-DROS_Headers)

add_library(atl ${SOURCE})