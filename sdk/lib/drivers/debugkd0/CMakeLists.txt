
add_definitions(-D_NTSYSTEM_)

list(APPEND SOURCE
    debugkd0.c)

add_library(debugkd0 ${SOURCE})
add_dependencies(debugkd0 xdk)
target_link_libraries(debugkd0 cportlib)
