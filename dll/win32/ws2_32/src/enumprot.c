/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/enumprot.c
 * PURPOSE:     Protocol Enumeration
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 *              Pierre Schweitzer
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

BOOL
WSAAPI
CheckProtocolMatch(IN LPINT ProtocolSet,
                   IN LPWSAPROTOCOL_INFOW ProtocolInfo)
{
    BOOL Return = FALSE;
    DWORD i = 0;
    INT ProtocolId;

    /* Make sure we have a set */
    if (ProtocolSet)
    {
        /* Get the first ID */
        ProtocolId = ProtocolSet[i];

        /* Loop the list */
        while (ProtocolId != 0 && ProtocolInfo->iProtocol != 0)
        {
            /* Check if it's within ranges */
            if ((ProtocolId >= ProtocolInfo->iProtocol) &&
                (ProtocolId <= (ProtocolInfo->iProtocol +
                                ProtocolInfo->iProtocolMaxOffset)))
            {
                /* Found it */
                Return = TRUE;
                break;
            }

            /* Move on */
            i++;
            ProtocolId = ProtocolSet[i];
        }
    }
    else
    {
        /* Assume match */
        Return = TRUE;
    }

    /* Return result */
    return Return;
}

VOID
WSAAPI
ProtocolInfoFromContext(IN LPWSAPROTOCOL_INFOW ProtocolInfo,
                        IN PPROTOCOL_ENUM_CONTEXT Context)
{
    /* Check if we'll have space */
    if ((Context->BufferUsed + sizeof(*ProtocolInfo)) <=
        (Context->BufferLength))
    {
        /* Copy the data */
        RtlMoveMemory((PVOID)((ULONG_PTR)Context->ProtocolBuffer +
                              Context->BufferUsed),
                      ProtocolInfo,
                      sizeof(*ProtocolInfo));

        /* Increase the count */
        Context->Count++;
    }
}

BOOL
WSAAPI
ProtocolEnumerationProc(PVOID EnumContext,
                        PTCATALOG_ENTRY Entry)
{
    PPROTOCOL_ENUM_CONTEXT Context = (PPROTOCOL_ENUM_CONTEXT)EnumContext;
    LPWSAPROTOCOL_INFOW ProtocolInfo = &Entry->ProtocolInfo;

    /* Check if this protocol matches */
    if (CheckProtocolMatch(Context->Protocols, ProtocolInfo))
    {
        /* Copy the information */
        ProtocolInfoFromContext(ProtocolInfo, Context);
        Context->BufferUsed += sizeof(*ProtocolInfo);
    }

    /* Continue enumeration */
    return TRUE;
}

BOOL
WSAAPI
ProviderEnumerationProc(PVOID EnumContext,
                        PNSCATALOG_ENTRY Entry)
{
    INT PathLen;
    PPROVIDER_ENUM_CONTEXT Context = (PPROVIDER_ENUM_CONTEXT)EnumContext;

    /* Check if this provider matches */
    if (IsEqualGUID(&Entry->ProviderId, &Context->ProviderId))
    {
        /* Get the information about the provider */
        PathLen = wcslen(Entry->DllPath) + 1;
        Context->FoundPathLen = PathLen;
        Context->Found = 1;

        /* If we have enough room, copy path */
        if (PathLen <= Context->ProviderDllPathLen)
        {
            wcscpy(Context->ProviderDllPath, Entry->DllPath);
        }

        /* Stop enumeration */
        return FALSE;
    }
    else
    {
        /* Continue enumeration */
        return TRUE;
    }

}

PTCATALOG
WSAAPI
OpenInitializedCatalog(VOID)
{
    PTCATALOG Catalog;
    HKEY WsKey;

    /* Allocate the catalog */
    Catalog = WsTcAllocate();
    if (Catalog)
    {
        /* Open the WS Key */
        WsKey = WsOpenRegistryRoot();

        /* Initialize the catalog */
        WsTcInitializeFromRegistry(Catalog, WsKey, NULL);

        /* Close the key */
        RegCloseKey(WsKey);
    }

    /* Return it */
    return Catalog;
}

/*
 * @implemented
 */
INT
WSPAPI
WSCEnumProtocols(IN LPINT lpiProtocols,
                 OUT LPWSAPROTOCOL_INFOW lpProtocolBuffer,
                 IN OUT LPDWORD lpdwBufferLength,
                 OUT LPINT lpErrno)
{
    INT Status;
    PTCATALOG Catalog;
    PROTOCOL_ENUM_CONTEXT Context;
    DPRINT("WSCEnumProtocols: %p\n", lpiProtocols);

    /* Create a catalog object from the current one */
    Catalog = OpenInitializedCatalog();
    if (!Catalog)
    {
        /* Fail if we couldn't */
        *lpErrno = WSAENOBUFS;
        return SOCKET_ERROR;
    }

    /* Setup the context */
    Context.Protocols = lpiProtocols;
    Context.ProtocolBuffer = lpProtocolBuffer;
    Context.BufferLength = lpProtocolBuffer ? *lpdwBufferLength : 0;
    Context.BufferUsed = 0;
    Context.Count = 0;
    Context.ErrorCode = ERROR_SUCCESS;

    /* Enumerate the catalog */
    WsTcEnumerateCatalogItems(Catalog, ProtocolEnumerationProc, &Context);

    /* Get status */
    Status = Context.Count;

    /* Check the error code */
    if (Context.ErrorCode == ERROR_SUCCESS)
    {
        /* Check if enough space was available */
        if (Context.BufferLength < Context.BufferUsed)
        {
            /* Fail and tell them how much we need */
            *lpdwBufferLength = Context.BufferUsed;
            *lpErrno = WSAENOBUFS;
            Status = SOCKET_ERROR;
        }
    }
    else
    {
        /* Failure, normalize error */
        Status = SOCKET_ERROR;
        *lpErrno = Context.ErrorCode;
    }

    /* Delete the catalog object */
    WsTcDelete(Catalog);

    /* Return */
    return Status;
}

/*
 * @implemented
 */
INT
WSAAPI
WSAEnumProtocolsA(IN LPINT lpiProtocols,
                  OUT LPWSAPROTOCOL_INFOA lpProtocolBuffer,
                  IN OUT LPDWORD lpdwBufferLength)
{
    INT error, i, count;
    LPWSAPROTOCOL_INFOW protocolInfoW;
    DWORD size;
    DPRINT("WSAEnumProtocolsA: %p %p %p\n", lpiProtocols, lpProtocolBuffer, lpdwBufferLength);
    if (!lpdwBufferLength)
    {
        SetLastError(WSAENOBUFS);
        return SOCKET_ERROR;
    }
    count = WSCEnumProtocols(lpiProtocols, NULL, &size, &error);
    if (!lpProtocolBuffer || *lpdwBufferLength < (size/sizeof(WSAPROTOCOL_INFOW))*sizeof(WSAPROTOCOL_INFOA))
    {
        *lpdwBufferLength = (size/sizeof(WSAPROTOCOL_INFOW))*sizeof(WSAPROTOCOL_INFOA);
        SetLastError(WSAENOBUFS);
        return SOCKET_ERROR;
    }
    protocolInfoW = HeapAlloc(WsSockHeap, 0, size);
    count = WSCEnumProtocols(lpiProtocols, protocolInfoW, &size, &error);
    if (SOCKET_ERROR == count)
    {
        HeapFree(WsSockHeap, 0, protocolInfoW);
        SetLastError(error);
        return SOCKET_ERROR;
    }
    *lpdwBufferLength = 0;
    for (i = 0; i < count; i++)
    {
        /* Copy the data */
        RtlMoveMemory(&lpProtocolBuffer[i],
                      &protocolInfoW[i],
                      sizeof(lpProtocolBuffer[0])-sizeof(lpProtocolBuffer[0].szProtocol));
        wcstombs(lpProtocolBuffer[i].szProtocol, protocolInfoW[i].szProtocol, sizeof(lpProtocolBuffer[0].szProtocol));
        *lpdwBufferLength += sizeof(WSAPROTOCOL_INFOA);
    }
    HeapFree(WsSockHeap, 0, protocolInfoW);
    return i;
}

/*
 * @implemented
 */
INT
WSAAPI
WSAEnumProtocolsW(IN LPINT lpiProtocols,
                  OUT LPWSAPROTOCOL_INFOW lpProtocolBuffer,
                  IN OUT  LPDWORD lpdwBufferLength)
{
    INT error, count;
    DPRINT("WSAEnumProtocolsW: %p %p %p\n", lpiProtocols, lpProtocolBuffer, lpdwBufferLength);
    count = WSCEnumProtocols(lpiProtocols, lpProtocolBuffer, lpdwBufferLength, &error);
    if (SOCKET_ERROR == count)
    {
        SetLastError(error);
        return SOCKET_ERROR;
    }
    return count;
}


/*
 * @implemented
 */
INT
WSPAPI
WPUGetProviderPath(IN LPGUID lpProviderId,
                   OUT LPWSTR lpszProviderDllPath,
                   IN OUT LPINT lpProviderDllPathLen,
                   OUT LPINT lpErrno)
{
    return WSCGetProviderPath(lpProviderId, lpszProviderDllPath, lpProviderDllPathLen, lpErrno);
}

/*
 * @implemented
 */
INT
WSAAPI
WSAProviderConfigChange(IN OUT LPHANDLE lpNotificationHandle,
                        IN LPWSAOVERLAPPED lpOverlapped,
                        IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    DPRINT("WSAProviderConfigChange: %p\n", lpNotificationHandle);
    UNIMPLEMENTED;
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/*
 * @implemented
 */
INT
WSPAPI
WSCGetProviderPath(IN LPGUID lpProviderId,
                   OUT LPWSTR lpszProviderDllPath,
                   IN OUT LPINT lpProviderDllPathLen,
                   OUT LPINT lpErrno)
{
    PWSTHREAD Thread;
    PWSPROCESS Process;
    PNSCATALOG Catalog;
    INT ErrorCode, PathLen;
    PROVIDER_ENUM_CONTEXT Context;

    DPRINT("WSCGetProviderPath: %p %p %p %p\n", lpProviderId, lpszProviderDllPath, lpProviderDllPathLen, lpErrno);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* FIXME: if WSANOTINITIALISED, we should init
         * and perform the search!
         */

        /* Leave now */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Get the catalog */
    Catalog = WsProcGetNsCatalog(Process);

    _SEH2_TRY
    {
        /* Setup the context */
        Context.ProviderId = *lpProviderId;
        Context.ProviderDllPath = lpszProviderDllPath;
        Context.ProviderDllPathLen = *lpProviderDllPathLen;
        Context.FoundPathLen = 0;
        Context.Found = 0;
        Context.ErrorCode = ERROR_SUCCESS;

        ErrorCode = ERROR_SUCCESS;

        /* Enumerate the catalog */
        WsNcEnumerateCatalogItems(Catalog, ProviderEnumerationProc, &Context);

        /* Check the error code */
        if (Context.ErrorCode == ERROR_SUCCESS)
        {
            /* Check if provider was found */
            if (Context.Found)
            {
                PathLen = Context.FoundPathLen;

                /* Check whether buffer is too small
                 * If it isn't, return length without null char
                 * (see ProviderEnumerationProc)
                 */
                if (Context.FoundPathLen <= *lpProviderDllPathLen)
                {
                    PathLen = Context.FoundPathLen - 1;
                }
                else
                {
                    ErrorCode = WSAEFAULT;
                }

                /* Set returned/required length */
                *lpProviderDllPathLen = PathLen;
            }
            else
            {
                ErrorCode = WSAEINVAL;
            }
        }
        else
        {
            ErrorCode = Context.ErrorCode;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ErrorCode = WSAEFAULT;
    }
    _SEH2_END;

    /* Do we have to return failure? */
    if (ErrorCode != ERROR_SUCCESS)
    {
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    return 0;
}
