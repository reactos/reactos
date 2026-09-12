
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

add_library(arbiter_hal ${SOURCE})
target_compile_definitions(arbiter_hal PRIVATE _NTHAL_ _NTHALDLL_)
target_include_directories(arbiter_hal PRIVATE ${SOURCE})
add_dependencies(arbiter_hal bugcodes xdk)
