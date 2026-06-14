/*
 * PROJECT:     ReactOS CRT heap support library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of free
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <malloc.h>
#include <windef.h>
#include <winbase.h>

void __cdecl free(void* Block)
{
    HeapFree(GetProcessHeap(), 0, Block);
}
