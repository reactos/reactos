
add_definitions(-DUNICODE -D_UNICODE)
add_library(scrnsave scrnsave.c)
add_dependencies(scrnsave psdk)
