
add_library(crtheap STATIC
    _msize.c
    _recalloc.c
    calloc.c
    free.c
    malloc.c
    realloc.c
)

target_compile_definitions(crtheap PRIVATE
    _CRTBLD
)

add_dependencies(crtheap xdk)
