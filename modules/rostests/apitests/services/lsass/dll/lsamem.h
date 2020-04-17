/*
 * PROJECT:     ReactOS lsass_apitest
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     memory functions
 * COPYRIGHT:   Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 */

#ifndef _LSAMEM_H_
#define _LSAMEM_H_

LPVOID HeapAllocTag(
    _In_ HANDLE hHeap,
    _In_ DWORD  dwFlags,
    _In_ SIZE_T dwBytes,
    _In_ char* tag3);
BOOL HeapFreeTag(
    _In_ HANDLE hHeap,
    _In_ DWORD  dwFlags,
    _In_ LPVOID lpMem,
    _In_ char* tag3);
BOOL HeapIsTag(
    _In_ LPVOID lpMem,
    _In_ char* tag3);

#endif
