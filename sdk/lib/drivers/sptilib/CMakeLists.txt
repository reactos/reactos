
list(APPEND SOURCE
    sptilib.c)

add_library(sptilib ${SOURCE})
target_link_libraries(sptilib PRIVATE ${PSEH_LIB})
add_dependencies(sptilib xdk)
