
list(APPEND SOURCE
    wine2ros.c
)
add_library(wine2ros STATIC ${SOURCE})
add_dependencies(wine2ros psdk)
target_include_directories(wine2ros INTERFACE ${REACTOS_SOURCE_DIR}/sdk/lib/wine2ros)
