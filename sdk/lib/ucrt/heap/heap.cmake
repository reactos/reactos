
list(APPEND UCRT_HEAP_SOURCES
    heap/align.cpp
    heap/calloc.cpp
    heap/calloc_base.cpp
    heap/expand.cpp
    heap/free.cpp
    heap/free_base.cpp
    heap/heapchk.cpp
    heap/heapmin.cpp
    heap/heapwalk.cpp
    heap/heap_handle.cpp
    heap/malloc.cpp
    heap/malloc_base.cpp
    heap/msize.cpp
    heap/new_handler.cpp
    heap/new_mode.cpp
    heap/realloc.cpp
    heap/realloc_base.cpp
    heap/recalloc.cpp
)

if(DBG)
    list(APPEND UCRT_HEAP_SOURCES
        heap/debug_heap.cpp
        heap/debug_heap_hook.cpp
    )
endif()
