/*
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS GDI32
 * PURPOSE:
 * FILE:             win32ss/gdi/gdi32/misc/heap.c
 * PROGRAMER:
 * REVISION HISTORY:
 * NOTES:
 */

#include <precomp.h>
#include <debug.h>

// global variables in a dll are process-global
HANDLE hProcessHeap = NULL;

PVOID FASTCALL
HEAP_alloc(SIZE_T len)
{
    /* make sure hProcessHeap gets initialized by GdiProcessSetup before we get here */
    assert(hProcessHeap);
    return RtlAllocateHeap ( hProcessHeap, 0, len );
}

NTSTATUS
HEAP_strdupA2W ( LPWSTR* ppszW, LPCSTR lpszA )
{
    ULONG len;
    NTSTATUS Status;

    *ppszW = NULL;
    if ( !lpszA )
        return STATUS_SUCCESS;
    len = lstrlenA(lpszA);

    *ppszW = HEAP_alloc ( (len+1) * sizeof(WCHAR) );
    if ( !*ppszW )
        return STATUS_NO_MEMORY;
    Status = RtlMultiByteToUnicodeN ( *ppszW, len*sizeof(WCHAR), NULL, (PCHAR)lpszA, len );
    (*ppszW)[len] = L'\0';
    return Status;
}

VOID FASTCALL
HEAP_free(LPVOID memory)
{
    /* make sure hProcessHeap gets initialized by GdiProcessSetup before we get here */
    assert(hProcessHeap);

    RtlFreeHeap ( hProcessHeap, 0, memory );
}

LPWSTR FASTCALL
HEAP_strdupA2W_buf(IN LPCSTR lpszA, OUT LPWSTR pszBuff, IN SIZE_T cchBuff)
{
    if (!lpszA)
        return NULL;

    LPWSTR pszW;
    SIZE_T size = strlen(lpszA) + 1;
    if (size < cchBuff)
        pszW = pszBuff;
    else
        pszW = HEAP_alloc(size * sizeof(WCHAR));

    if (!pszW)
        return NULL;

    RtlMultiByteToUnicodeN(pszW, size * sizeof(WCHAR), NULL, lpszA, size);
    return pszW;
}
