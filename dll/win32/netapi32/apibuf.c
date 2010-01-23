/*
 * Copyright 2002 Andriy Palamarchuk
 *
 * Net API buffer calls
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "lmcons.h"
#include "lmapibuf.h"
#include "lmerr.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/************************************************************
 *                NetApiBufferAllocate  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetApiBufferAllocate(DWORD ByteCount, LPVOID* Buffer)
{
    TRACE("(%d, %p)\n", ByteCount, Buffer);

    if (Buffer == NULL) return ERROR_INVALID_PARAMETER;
    *Buffer = HeapAlloc(GetProcessHeap(), 0, ByteCount);
    if (*Buffer)
        return NERR_Success;
    else
        return GetLastError();
}

/************************************************************
 *                NetApiBufferFree  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetApiBufferFree(LPVOID Buffer)
{
    TRACE("(%p)\n", Buffer);
    HeapFree(GetProcessHeap(), 0, Buffer);
    return NERR_Success;
}

/************************************************************
 *                NetApiBufferReallocate  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetApiBufferReallocate(LPVOID OldBuffer, DWORD NewByteCount,
                                             LPVOID* NewBuffer)
{
    TRACE("(%p, %d, %p)\n", OldBuffer, NewByteCount, NewBuffer);
    if (NewByteCount) 
    {
        if (OldBuffer)
            *NewBuffer = HeapReAlloc(GetProcessHeap(), 0, OldBuffer, NewByteCount);
        else
            *NewBuffer = HeapAlloc(GetProcessHeap(), 0, NewByteCount);
	return *NewBuffer ? NERR_Success : GetLastError();
    } 
    else 
    {
	if (!HeapFree(GetProcessHeap(), 0, OldBuffer)) return GetLastError();
	*NewBuffer = 0;
	return NERR_Success;
    }
}

/************************************************************
 *                NetApiBufferSize  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetApiBufferSize(LPVOID Buffer, LPDWORD ByteCount)
{
    DWORD dw;

    TRACE("(%p, %p)\n", Buffer, ByteCount);
    if (Buffer == NULL)
        return ERROR_INVALID_PARAMETER;
    dw = HeapSize(GetProcessHeap(), 0, Buffer);
    TRACE("size: %d\n", dw);
    if (dw != 0xFFFFFFFF)
        *ByteCount = dw;
    else
        *ByteCount = 0;

    return NERR_Success;
}
