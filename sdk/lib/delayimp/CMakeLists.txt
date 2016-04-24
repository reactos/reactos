
add_definitions(-DUNICODE -D_UNICODE)
add_library(delayimp delayimp.c)
add_dependencies(delayimp psdk)
add_importlibs(delayimp kernel32)
