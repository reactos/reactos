
add_definitions(-D_NTSYSTEM_)

list(APPEND SOURCE
    arbiter.c
    entry.c
    handler.c
    ordering.c
    range.c
    transaction.c)

add_library(arbiter STATIC ${SOURCE})
add_dependencies(arbiter bugcodes xdk)
target_include_directories(arbiter INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
