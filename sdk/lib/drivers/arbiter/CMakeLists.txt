
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

add_library(arbiter_hal STATIC ${SOURCE})
target_compile_definitions(arbiter_hal PRIVATE _NTHAL_ _NTHALDLL_)
target_include_directories(arbiter_hal INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(arbiter_hal bugcodes xdk)
