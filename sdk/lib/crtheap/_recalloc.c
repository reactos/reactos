/*
 * PROJECT:     ReactOS CRT heap support library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _recalloc
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <malloc.h>
#include <windef.h>
#include <winbase.h>

void* __cdecl _recalloc(void* Block, size_t Count, size_t Size)
{
    size_t newSize = Count * Size;

    if ((Size != 0) && (newSize / Size != Count))
    {
        return NULL;
    }

    if (Block == NULL)
    {
        return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, newSize);
    }

    if (newSize == 0)
    {
        HeapFree(GetProcessHeap(), 0, Block);
        return NULL;
    }

    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Block, newSize);
}

#ifdef _M_IX86
const void* const _imp___recalloc = _recalloc;
#else
const void* const __imp__recalloc = _recalloc;
#endif
