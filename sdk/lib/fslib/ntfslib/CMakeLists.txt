
list(APPEND SOURCE
    ntfslib.c)

add_library(ntfslib ${SOURCE})
target_compile_definitions(ntfslib PRIVATE _USRDLL)
add_dependencies(ntfslib psdk)
