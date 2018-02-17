#pragma once

static inline void* __WINE_ALLOC_SIZE(1) heap_alloc(SIZE_T size)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}

static inline void* __WINE_ALLOC_SIZE(1) heap_alloc_zero(SIZE_T size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

static inline void* __WINE_ALLOC_SIZE(2) heap_realloc(LPVOID mem, SIZE_T size)
{
    return HeapReAlloc(GetProcessHeap(), 0, mem, size);
}

static inline BOOL heap_free(LPVOID mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}
