
add_definitions(-D_NTSYSTEM_)

list(APPEND SOURCE
    arbiter.c)

add_library(arbiter ${SOURCE})
add_dependencies(arbiter bugcodes xdk)
