/*
 * PROJECT:     ReactOS CRT heap support library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of calloc
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <malloc.h>
#include <windef.h>
#include <winbase.h>

void* __cdecl calloc(size_t Count, size_t Size)
{
    size_t bytes = Count * Size;

    if ((Size != 0) && (bytes / Size != Count))
    {
        return NULL;
    }

    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bytes);
}
