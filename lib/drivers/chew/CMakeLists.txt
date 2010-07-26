
add_definitions(-D_NTOSKRNL_)
add_library(chew workqueue.c)
add_dependencies(chew build_resources)