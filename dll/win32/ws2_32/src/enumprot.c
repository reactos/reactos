/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        enumprot.c
 * PURPOSE:     Protocol Enumeration
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/
#include "ws2_32.h"

//#define NDEBUG
#include <debug.h>

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

BOOL
WSAAPI
CheckProtocolMatch(IN LPINT ProtocolSet,
                   IN LPWSAPROTOCOL_INFOW ProtocolInfo)
{
    BOOL Return = FALSE;
    DWORD i = 0;
    INT ProtocolId = 0;

    /* Make sure we have a set */
    if (ProtocolSet)
    {
        /* Get the first ID */
        ProtocolId = ProtocolSet[i];

        /* Loop the list */
        while (ProtocolId != 0)
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

PTCATALOG
WSAAPI
OpenInitializedCatalog(VOID)
{
    INT ErrorCode;
    PTCATALOG Catalog;
    HKEY WsKey;

    /* Allocate the catalog */
    Catalog = WsTcAllocate();
    if (Catalog)
    {
        /* Open the WS Key */
        WsKey = WsOpenRegistryRoot();

        /* Initialize the catalog */
        ErrorCode = WsTcInitializeFromRegistry(Catalog, WsKey, NULL);

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
 * @unimplemented
 */
INT
WSAAPI
WSAEnumProtocolsA(IN LPINT lpiProtocols,
                  OUT LPWSAPROTOCOL_INFOA lpProtocolBuffer,
                  IN OUT LPDWORD lpdwBufferLength)
{
    DPRINT("WSAEnumProtocolsA: %p\n", lpiProtocols);
    UNIMPLEMENTED;
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
INT
WSAAPI
WSAEnumProtocolsW(IN LPINT lpiProtocols,
                  OUT LPWSAPROTOCOL_INFOW lpProtocolBuffer,
                  IN OUT  LPDWORD lpdwBufferLength)
{
    DPRINT("WSAEnumProtocolsW: %p\n", lpiProtocols);
    UNIMPLEMENTED;
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
INT
WSPAPI
WPUGetProviderPath(IN LPGUID lpProviderId,
                   OUT LPWSTR lpszProviderDllPath,
                   IN OUT LPINT lpProviderDllPathLen,
                   OUT LPINT lpErrno)
{
    DPRINT("WPUGetProviderPath: %p\n", lpProviderId);
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
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
 * @unimplemented
 */
INT
WSPAPI
WSCGetProviderPath(IN LPGUID lpProviderId,
                   OUT LPWSTR lpszProviderDllPath,
                   IN OUT LPINT lpProviderDllPathLen,
                   OUT LPINT lpErrno)
{
    DPRINT("WSCGetProviderPath: %p\n", lpProviderId);
    UNIMPLEMENTED;
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}
