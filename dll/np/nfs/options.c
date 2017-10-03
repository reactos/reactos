/* NFSv4.1 client for Windows
 * Copyright © 2012 The Regents of the University of Michigan
 *
 * Olga Kornievskaia <aglo@umich.edu>
 * Casey Bodley <cbodley@umich.edu>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of merchantability
 * or fitness for a particular purpose.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 */

#include <windows.h>
#include "options.h"


DWORD InitializeConnectionInfo(
    IN OUT PCONNECTION_INFO Connection,
    IN PMOUNT_OPTION_BUFFER Options,
    OUT LPWSTR *ConnectionName)
{
    DWORD result = WN_SUCCESS;
    SIZE_T size;

    /* verify that this is a mount options buffer */
    if (Options &&
        Options->Zero == 0 &&
        Options->Secret == MOUNT_OPTION_BUFFER_SECRET)
    {
        Connection->Options = Options;
        size = MAX_CONNECTION_BUFFER_SIZE(Options->Length);
    }
    else
    {
        Connection->Options = NULL;
        size = MAX_CONNECTION_BUFFER_SIZE(0);
    }

    Connection->Buffer = LocalAlloc(LMEM_ZEROINIT, size);
    if (Connection->Buffer)
        *ConnectionName = (LPWSTR)Connection->Buffer->Buffer;
    else
        result = WN_OUT_OF_MEMORY;

    return result;
}

#ifdef __REACTOS__
FORCEINLINE SIZE_T ConnectionBufferSize(
#else
static FORCEINLINE SIZE_T ConnectionBufferSize(
#endif
    IN PCONNECTION_BUFFER Buffer)
{
    return sizeof(USHORT) + sizeof(USHORT) + sizeof(ULONG) +
        Buffer->NameLength + Buffer->EaPadding + Buffer->EaLength;
}

void MarshalConnectionInfo(
    IN OUT PCONNECTION_INFO Connection)
{
    PCONNECTION_BUFFER Buffer = Connection->Buffer;
    LPWSTR ConnectionName = (LPWSTR)Buffer->Buffer;

    Buffer->NameLength = (USHORT)(wcslen(ConnectionName) + 1) * sizeof(WCHAR);

    /* copy the EaBuffer after the end of ConnectionName */
    if (Connection->Options && Connection->Options->Length)
    {
        PBYTE ptr = Buffer->Buffer + Buffer->NameLength;
        /* add padding so EaBuffer starts on a ULONG boundary */
        Buffer->EaPadding = (USHORT)
            (sizeof(ULONG) - (SIZE_T)ptr % sizeof(ULONG)) % sizeof(ULONG);
        Buffer->EaLength = Connection->Options->Length;
        ptr += Buffer->EaPadding;

        RtlCopyMemory(ptr, Connection->Options->Buffer, Buffer->EaLength);
    }

    Connection->BufferSize = (ULONG)ConnectionBufferSize(Buffer);
}

void FreeConnectionInfo(
    IN PCONNECTION_INFO Connection)
{
    if (Connection->Buffer)
    {
        LocalFree(Connection->Buffer);
        Connection->Buffer = NULL;
    }
    Connection->Options = NULL;
    Connection->BufferSize = 0;
}
