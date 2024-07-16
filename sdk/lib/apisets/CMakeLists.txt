
add_library(apisets
    apisets.c
    apisets.h
    apisetsp.h

    # Generated file:
    apisets.table.c
)

target_include_directories(apisets INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(apisets PRIVATE _NTSYSTEM_)
add_dependencies(apisets psdk)
