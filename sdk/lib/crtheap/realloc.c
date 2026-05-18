/*
 * PROJECT:     ReactOS CRT heap support library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of realloc
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <malloc.h>
#include <windef.h>
#include <winbase.h>

void* __cdecl realloc(void* Block, size_t Size)
{
    if (Block == NULL)
        return HeapAlloc(GetProcessHeap(), 0, Size);

    if (Size != 0)
        return HeapReAlloc(GetProcessHeap(), 0, Block, Size);

    HeapFree(GetProcessHeap(), 0, Block);

    return NULL;
}
