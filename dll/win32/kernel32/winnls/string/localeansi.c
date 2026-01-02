/*
 * Locale support
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 David Lee Lambert
 * Copyright 2000 Julio Cesar Gazquez
 * Copyright 2002 Alexandre Julliard for CodeWeavers
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>


/*************************************************************************
 *           FoldStringA    (KERNEL32.@)
 *
 * Map characters in a string.
 *
 * PARAMS
 *  dwFlags [I] Flags controlling chars to map (MAP_ constants from "winnls.h")
 *  src     [I] String to map
 *  srclen  [I] Length of src, or -1 if src is NUL terminated
 *  dst     [O] Destination for mapped string
 *  dstlen  [I] Length of dst, or 0 to find the required length for the mapped string
 *
 * RETURNS
 *  Success: The length of the string written to dst, including the terminating NUL. If
 *           dstlen is 0, the value returned is the same, but nothing is written to dst,
 *           and dst may be NULL.
 *  Failure: 0. Use GetLastError() to determine the cause.
 */
INT WINAPI FoldStringA(DWORD dwFlags, LPCSTR src, INT srclen,
                       LPSTR dst, INT dstlen)
{
    INT ret = 0, srclenW = 0;
    WCHAR *srcW = NULL, *dstW = NULL;

    if (!src || !srclen || dstlen < 0 || (dstlen && !dst) || src == dst)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    srclenW = MultiByteToWideChar(CP_ACP, dwFlags & MAP_COMPOSITE ? MB_COMPOSITE : 0,
                                  src, srclen, NULL, 0);
    srcW = HeapAlloc(GetProcessHeap(), 0, srclenW * sizeof(WCHAR));

    if (!srcW)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto FoldStringA_exit;
    }

    MultiByteToWideChar(CP_ACP, dwFlags & MAP_COMPOSITE ? MB_COMPOSITE : 0,
                        src, srclen, srcW, srclenW);

    dwFlags = (dwFlags & ~MAP_PRECOMPOSED) | MAP_FOLDCZONE;

    ret = FoldStringW(dwFlags, srcW, srclenW, NULL, 0);
    if (ret && dstlen)
    {
        dstW = HeapAlloc(GetProcessHeap(), 0, ret * sizeof(WCHAR));

        if (!dstW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto FoldStringA_exit;
        }

        ret = FoldStringW(dwFlags, srcW, srclenW, dstW, ret);
        if (!WideCharToMultiByte(CP_ACP, 0, dstW, ret, dst, dstlen, NULL, NULL))
        {
            ret = 0;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
    }

    HeapFree(GetProcessHeap(), 0, dstW);

FoldStringA_exit:
    HeapFree(GetProcessHeap(), 0, srcW);
    return ret;
}

/* EOF */
