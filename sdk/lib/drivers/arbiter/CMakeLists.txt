
add_definitions(-D_NTSYSTEM_)

list(APPEND SOURCE
    arbiter.c
    entry.c
    handler.c
    ordering.c
    range.c
    transaction.c)

add_library(arbiter ${SOURCE})
add_dependencies(arbiter bugcodes xdk)
