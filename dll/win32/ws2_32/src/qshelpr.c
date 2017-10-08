/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/qshelpr.c
 * PURPOSE:     Query Set Conversion/Packing Helpers
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

LPSTR
WSAAPI
AnsiDupFromUnicode(IN LPCWSTR UnicodeString)
{
    INT Length = 0;
    BOOL GoOn = TRUE;
    LPSTR DuplicatedString = NULL;
    INT ReturnValue;

    /* Start a loop (which should only run twice) */
    while (GoOn)
    {
        /* Call the conversion function */
        ReturnValue = WideCharToMultiByte(CP_ACP,
                                          0,
                                          UnicodeString,
                                          -1,
                                          DuplicatedString,
                                          Length,
                                          NULL,
                                          NULL);
        if (ReturnValue > Length)
        {
            /* This is the first loop, and we have the real size now */
            Length = ReturnValue;

            /* Allocate buffer for it */
            DuplicatedString = HeapAlloc(WsSockHeap, 0, Length);
            if (!DuplicatedString) GoOn = FALSE;
        }
        else if (ReturnValue > 0)
        {
            /* The second loop was successful and we have the string */
            GoOn = FALSE;
        }
        else
        {
            /* Some weird error happened */
            if (DuplicatedString) HeapFree(WsSockHeap, 0, DuplicatedString);
            DuplicatedString = NULL;
            GoOn = FALSE;
        }
    }

    /* Return the duplicate */
    return DuplicatedString;
}

LPWSTR
WSAAPI
UnicodeDupFromAnsi(IN LPCSTR AnsiString)
{
    INT Length = 0;
    BOOL GoOn = TRUE;
    LPWSTR DuplicatedString = NULL;
    INT ReturnValue;

    /* Start a loop (which should only run twice) */
    while (GoOn)
    {
        /* Call the conversion function */
        ReturnValue = MultiByteToWideChar(CP_ACP,
                                          0,
                                          AnsiString,
                                          -1,
                                          DuplicatedString,
                                          Length);
        if (ReturnValue > Length)
        {
            /* This is the first loop, and we have the real size now */
            Length = ReturnValue;

            /* Allocate buffer for it */
            DuplicatedString = HeapAlloc(WsSockHeap, 0, Length * sizeof(WCHAR));
            if (!DuplicatedString) GoOn = FALSE;
        }
        else if (ReturnValue > 0)
        {
            /* The second loop was successful and we have the string */
            GoOn = FALSE;
        }
        else
        {
            /* Some weird error happened */
            if (DuplicatedString) HeapFree(WsSockHeap, 0, DuplicatedString);
            DuplicatedString = NULL;
            GoOn = FALSE;
        }
    }

    /* Return the duplicate */
    return DuplicatedString;
}

SIZE_T
WSAAPI
ComputeStringSize(IN LPSTR String,
                  IN BOOLEAN IsUnicode)
{
    /* Return the size of the string, in bytes, including null-char */
    return (IsUnicode) ? (wcslen((LPWSTR)String) + 1 ) * sizeof(WCHAR) :
                          strlen(String) + sizeof(CHAR);
}

SIZE_T
WSAAPI
ComputeQuerySetSize(IN LPWSAQUERYSETA AnsiSet,
                    IN BOOLEAN IsUnicode)
{
    SIZE_T Size = sizeof(WSAQUERYSETA);
    //LPWSAQUERYSETW UnicodeSet = (LPWSAQUERYSETW)AnsiSet;
    DWORD i;

    /* Check for instance name */
    if (AnsiSet->lpszServiceInstanceName)
    {
        /* Add its size */
        Size += ComputeStringSize(AnsiSet->lpszServiceInstanceName, IsUnicode);
    }

    /* Check for Service Class ID */
    if (AnsiSet->lpServiceClassId)
    {
        /* Align the current size and add GUID size */
        Size = (Size + 3) & ~3;
        Size += sizeof(GUID);
    }

    /* Check for version data */
    if (AnsiSet->lpVersion)
    {
        /* Align the current size and add GUID size */
        Size = (Size + 3) & ~3;
        Size += sizeof(WSAVERSION);
    }

    /* Check for comment */
    if (AnsiSet->lpszComment)
    {
        /* Align the current size and add string size */
        Size = (Size + 1) & ~1;
        Size += ComputeStringSize(AnsiSet->lpszComment, IsUnicode);
    }

    /* Check for Provider ID */
    if (AnsiSet->lpNSProviderId)
    {
        /* Align the current size and add GUID size */
        Size = (Size + 3) & ~3;
        Size += sizeof(GUID);
    }

    /* Check for context */
    if (AnsiSet->lpszContext)
    {
        /* Align the current size and add string size */
        Size = (Size + 1) & ~1;
        Size += ComputeStringSize(AnsiSet->lpszContext, IsUnicode);
    }

    /* Check for query string */
    if (AnsiSet->lpszQueryString)
    {
        /* Align the current size and add string size */
        Size = (Size + 1) & ~1;
        Size += ComputeStringSize(AnsiSet->lpszQueryString, IsUnicode);
    }

    /* Check for AF Protocol data */
    if (AnsiSet->lpafpProtocols)
    {
        /* Align the current size and add AFP size */
        Size = (Size + 3) & ~3;
        Size += sizeof(AFPROTOCOLS) * AnsiSet->dwNumberOfProtocols;
    }

    /* Check for CSADDR buffer */
    if (AnsiSet->lpcsaBuffer)
    {
        /* Align the current size */
        Size = (Size + 3) & ~3;

        /* Loop all the addresses in the array */
        for (i = 0; i < AnsiSet->dwNumberOfCsAddrs; i++)
        {
            /* Check for local sockaddr */
            if (AnsiSet->lpcsaBuffer[i].LocalAddr.lpSockaddr)
            {
                /* Align the current size and add the sockaddr's length */
                Size = (Size + 3) & ~3;
                Size += AnsiSet->lpcsaBuffer[i].LocalAddr.iSockaddrLength;
            }
            /* Check for remote sockaddr */
            if (AnsiSet->lpcsaBuffer[i].RemoteAddr.lpSockaddr)
            {
                /* Align the current size and add the sockaddr's length */
                Size = (Size + 3) & ~3;
                Size += AnsiSet->lpcsaBuffer[i].RemoteAddr.iSockaddrLength;
            }

            /* Add the sockaddr size itself */
            Size += sizeof(CSADDR_INFO);
        }
    }

    /* Check for blob data */
    if (AnsiSet->lpBlob)
    {
        /* Align the current size and add blob size */
        Size = (Size + 3) & ~3;
        Size += sizeof(BLOB);

        /* Also add the actual blob data size, if it exists */
        if (AnsiSet->lpBlob) Size += AnsiSet->lpBlob->cbSize;
    }

    /* Return the total size */
    return Size;
}

SIZE_T
WSAAPI
WSAComputeQuerySetSizeA(IN LPWSAQUERYSETA AnsiSet)
{
    /* Call the generic helper */
    return ComputeQuerySetSize(AnsiSet, FALSE);
}

SIZE_T
WSAAPI
WSAComputeQuerySetSizeW(IN LPWSAQUERYSETW UnicodeSet)
{
    /* Call the generic helper */
    return ComputeQuerySetSize((LPWSAQUERYSETA)UnicodeSet, TRUE);
}

PVOID
WSAAPI
WsBufferAllocate(IN PWS_BUFFER Buffer,
                 IN SIZE_T Size,
                 IN DWORD Align)
{
    PVOID NewPosition;

    /* Align the current usage */
    Buffer->BytesUsed = (Buffer->BytesUsed + Align - 1) & ~(Align - 1);

    /* Update our location */
    NewPosition = (PVOID)(Buffer->Position + Buffer->BytesUsed);

    /* Update the usage */
    Buffer->BytesUsed += Size;

    /* Return new location */
    return NewPosition;
}

VOID
WSAAPI
CopyBlobIndirect(IN PWS_BUFFER Buffer,
                 IN OUT LPBLOB RelativeBlob,
                 IN LPBLOB Blob)
{
    /* Make sure we have blob data */
    if ((Blob->pBlobData) && (Blob->cbSize))
    {
        /* Allocate and copy the blob data */
        RelativeBlob->pBlobData = WsBufferAllocate(Buffer,
                                                   Blob->cbSize,
                                                   sizeof(PVOID));
        RtlCopyMemory(RelativeBlob->pBlobData,
                      Blob->pBlobData,
                      Blob->cbSize);
    }
}

VOID
WSAAPI
CopyAddrInfoArrayIndirect(IN PWS_BUFFER Buffer,
                          IN OUT LPCSADDR_INFO RelativeCsaddr,
                          IN DWORD Addresses,
                          IN LPCSADDR_INFO Csaddr)
{
    DWORD i;

    /* Loop for every address inside */
    for (i = 0; i < Addresses; i++)
    {
        /* Check for a local address */
        if ((Csaddr[i].LocalAddr.lpSockaddr) &&
            (Csaddr[i].LocalAddr.iSockaddrLength))
        {
            /* Allocate and copy the address */
            RelativeCsaddr[i].LocalAddr.lpSockaddr =
                WsBufferAllocate(Buffer,
                                 Csaddr[i].LocalAddr.iSockaddrLength,
                                 sizeof(PVOID));
            RtlCopyMemory(RelativeCsaddr[i].LocalAddr.lpSockaddr,
                          Csaddr[i].LocalAddr.lpSockaddr,
                          Csaddr[i].LocalAddr.iSockaddrLength);
        }
        else
        {
            /* Nothing in this address */
            Csaddr[i].LocalAddr.lpSockaddr = NULL;
            Csaddr[i].LocalAddr.iSockaddrLength = 0;
        }

        /* Check for a remote address */
        if ((Csaddr[i].RemoteAddr.lpSockaddr) &&
            (Csaddr[i].RemoteAddr.iSockaddrLength))
        {
            /* Allocate and copy the address */
            RelativeCsaddr[i].RemoteAddr.lpSockaddr =
                WsBufferAllocate(Buffer,
                                 Csaddr[i].RemoteAddr.iSockaddrLength,
                                 sizeof(PVOID));
            RtlCopyMemory(RelativeCsaddr[i].RemoteAddr.lpSockaddr,
                          Csaddr[i].RemoteAddr.lpSockaddr,
                          Csaddr[i].RemoteAddr.iSockaddrLength);
        }
        else
        {
            /* Nothing in this address */
            Csaddr[i].RemoteAddr.lpSockaddr = NULL;
            Csaddr[i].RemoteAddr.iSockaddrLength = 0;
        }
    }
}

VOID
WSAAPI
CopyQuerySetIndirectA(IN PWS_BUFFER Buffer,
                      IN OUT LPWSAQUERYSETA RelativeSet,
                      IN LPWSAQUERYSETA AnsiSet)
{
    LPSTR AnsiString;

    /* Get the service name */
    AnsiString = AnsiSet->lpszServiceInstanceName;
    if (AnsiString)
    {
        /* One exists, allocate a space in the buffer for it */
        RelativeSet->lpszServiceInstanceName = WsBufferAllocate(Buffer,
                                                                strlen(AnsiString) + 1,
                                                                sizeof(CHAR));
        /* Copy it into the buffer */
        strcpy(RelativeSet->lpszServiceInstanceName, AnsiString);
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpszServiceInstanceName = NULL;
    }

    /* Check for the service class ID */
    if (AnsiSet->lpServiceClassId)
    {
        /* One exists, allocate a space in the buffer for it */
        RelativeSet->lpServiceClassId = WsBufferAllocate(Buffer,
                                                         sizeof(GUID),
                                                         sizeof(PVOID));
        /* Copy it into the buffer */
        *(RelativeSet->lpServiceClassId) = *(AnsiSet->lpServiceClassId);
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpServiceClassId = NULL;
    }

    /* Get the version data */
    if (AnsiSet->lpVersion)
    {
        /* One exists, allocate a space in the buffer for it */
        RelativeSet->lpVersion = WsBufferAllocate(Buffer,
                                                  sizeof(WSAVERSION),
                                                  sizeof(PVOID));
        /* Copy it into the buffer */
        *(RelativeSet->lpVersion) = *(AnsiSet->lpVersion);
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpVersion = NULL;
    }

    /* Get the comment */
    AnsiString = AnsiSet->lpszComment;
    if (AnsiString)
    {
        /* One exists, allocate a space in the buffer for it */
        RelativeSet->lpszComment = WsBufferAllocate(Buffer,
                                                    strlen(AnsiString) + 1,
                                                    sizeof(CHAR));
        /* Copy it into the buffer */
        strcpy(RelativeSet->lpszComment, AnsiString);
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpszComment = NULL;
    }

    /* Get the NS Provider ID */
    if (AnsiSet->lpNSProviderId)
    {
        /* One exists, allocate a space in the buffer for it */
        RelativeSet->lpNSProviderId = WsBufferAllocate(Buffer,
                                                       sizeof(GUID),
                                                       sizeof(PVOID));
        /* Copy it into the buffer */
        *(RelativeSet->lpNSProviderId) = *(AnsiSet->lpNSProviderId);
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpNSProviderId = NULL;
    }

    /* Get the context */
    AnsiString = AnsiSet->lpszContext;
    if (AnsiString)
    {
        /* One exists, allocate a space in the buffer for it */
        RelativeSet->lpszContext = WsBufferAllocate(Buffer,
                                                    strlen(AnsiString) + 1,
                                                    sizeof(CHAR));
        /* Copy it into the buffer */
        strcpy(RelativeSet->lpszContext, AnsiString);
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpszContext = NULL;
    }

    /* Get the query string */
    AnsiString = AnsiSet->lpszQueryString;
    if (AnsiString)
    {
        /* One exists, allocate a space in the buffer for it */
        RelativeSet->lpszQueryString = WsBufferAllocate(Buffer,
                                                        strlen(AnsiString) + 1,
                                                        sizeof(CHAR));
        /* Copy it into the buffer */
        strcpy(RelativeSet->lpszQueryString, AnsiString);
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpszQueryString = NULL;
    }

    /* Check for a protocol structure with non-zero protocols */
    if ((AnsiSet->lpafpProtocols) && (AnsiSet->dwNumberOfProtocols))
    {
        /* One exists, allocate space for it */
        RelativeSet->lpafpProtocols = WsBufferAllocate(Buffer,
                                                       AnsiSet->dwNumberOfProtocols *
                                                       sizeof(AFPROTOCOLS),
                                                       sizeof(PVOID));
        /* Copy it into the buffer */
        RtlCopyMemory(RelativeSet->lpafpProtocols,
                      AnsiSet->lpafpProtocols,
                      AnsiSet->dwNumberOfProtocols * sizeof(AFPROTOCOLS));
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpafpProtocols = NULL;
        RelativeSet->dwNumberOfProtocols = 0;
    }

    /* Check if we have a CSADDR with addresses inside */
    if ((AnsiSet->lpcsaBuffer) && (AnsiSet->dwNumberOfCsAddrs))
    {
        /* Allocate and copy the CSADDR structure itself */
        RelativeSet->lpcsaBuffer = WsBufferAllocate(Buffer,
                                                    AnsiSet->dwNumberOfCsAddrs *
                                                    sizeof(CSADDR_INFO),
                                                    sizeof(PVOID));

        /* Copy it into the buffer */
        RtlCopyMemory(RelativeSet->lpcsaBuffer,
                      AnsiSet->lpcsaBuffer,
                      AnsiSet->dwNumberOfCsAddrs * sizeof(CSADDR_INFO));

        /* Copy the addresses inside the CSADDR */
        CopyAddrInfoArrayIndirect(Buffer,
                                  RelativeSet->lpcsaBuffer,
                                  AnsiSet->dwNumberOfCsAddrs,
                                  AnsiSet->lpcsaBuffer);
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpcsaBuffer = NULL;
        RelativeSet->dwNumberOfCsAddrs = 0;
    }

    /* Check for blob data */
    if (AnsiSet->lpBlob)
    {
        /* Allocate and copy the blob itself */
        RelativeSet->lpBlob = WsBufferAllocate(Buffer,
                                               sizeof(BLOB),
                                               sizeof(PVOID));
        *(RelativeSet->lpBlob) = *(AnsiSet->lpBlob);

        /* Copy the data inside the blob */
        CopyBlobIndirect(Buffer, RelativeSet->lpBlob, AnsiSet->lpBlob);
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpBlob = NULL;
    }
}

VOID
WSAAPI
CopyQuerySetIndirectW(IN PWS_BUFFER Buffer,
                      IN OUT LPWSAQUERYSETW RelativeSet,
                      IN LPWSAQUERYSETW UnicodeSet)
{
    LPWSTR UnicodeString;

    /* Get the service name */
    UnicodeString = UnicodeSet->lpszServiceInstanceName;
    if (UnicodeString)
    {
        /* One exists, allocate a space in the buffer for it */
        RelativeSet->lpszServiceInstanceName = WsBufferAllocate(Buffer,
                                                                (wcslen(UnicodeString) + 1) *
                                                                sizeof(WCHAR),
                                                                sizeof(CHAR));
        /* Copy it into the buffer */
        wcscpy(RelativeSet->lpszServiceInstanceName, UnicodeString);
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpszServiceInstanceName = NULL;
    }

    /* Check for the service class ID */
    if (UnicodeSet->lpServiceClassId)
    {
        /* One exists, allocate a space in the buffer for it */
        RelativeSet->lpServiceClassId = WsBufferAllocate(Buffer,
                                                         sizeof(GUID),
                                                         sizeof(PVOID));
        /* Copy it into the buffer */
        *(RelativeSet->lpServiceClassId) = *(UnicodeSet->lpServiceClassId);
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpServiceClassId = NULL;
    }

    /* Get the version data */
    if (UnicodeSet->lpVersion)
    {
        /* One exists, allocate a space in the buffer for it */
        RelativeSet->lpVersion = WsBufferAllocate(Buffer,
                                                  sizeof(WSAVERSION),
                                                  sizeof(PVOID));
        /* Copy it into the buffer */
        *(RelativeSet->lpVersion) = *(UnicodeSet->lpVersion);
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpVersion = NULL;
    }

    /* Get the comment */
    UnicodeString = UnicodeSet->lpszComment;
    if (UnicodeString)
    {
        /* One exists, allocate a space in the buffer for it */
        RelativeSet->lpszComment = WsBufferAllocate(Buffer,
                                                    (wcslen(UnicodeString) + 1) *
                                                    sizeof(WCHAR),
                                                    sizeof(CHAR));
        /* Copy it into the buffer */
        wcscpy(RelativeSet->lpszComment, UnicodeString);
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpszComment = NULL;
    }

    /* Get the NS Provider ID */
    if (UnicodeSet->lpNSProviderId)
    {
        /* One exists, allocate a space in the buffer for it */
        RelativeSet->lpNSProviderId = WsBufferAllocate(Buffer,
                                                       sizeof(GUID),
                                                       sizeof(PVOID));
        /* Copy it into the buffer */
        *(RelativeSet->lpNSProviderId) = *(UnicodeSet->lpNSProviderId);
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpNSProviderId = NULL;
    }

    /* Get the context */
    UnicodeString = UnicodeSet->lpszContext;
    if (UnicodeString)
    {
        /* One exists, allocate a space in the buffer for it */
        RelativeSet->lpszContext = WsBufferAllocate(Buffer,
                                                    (wcslen(UnicodeString) + 1) *
                                                    sizeof(WCHAR),
                                                    sizeof(CHAR));
        /* Copy it into the buffer */
        wcscpy(RelativeSet->lpszContext, UnicodeString);
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpszContext = NULL;
    }

    /* Get the query string */
    UnicodeString = UnicodeSet->lpszQueryString;
    if (UnicodeString)
    {
        /* One exists, allocate a space in the buffer for it */
        RelativeSet->lpszQueryString = WsBufferAllocate(Buffer,
                                                        (wcslen(UnicodeString) + 1) *
                                                        sizeof(WCHAR),
                                                        sizeof(CHAR));
        /* Copy it into the buffer */
        wcscpy(RelativeSet->lpszQueryString, UnicodeString);
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpszQueryString = NULL;
    }

    /* Check for a protocol structure with non-zero protocols */
    if ((UnicodeSet->lpafpProtocols) && (UnicodeSet->dwNumberOfProtocols))
    {
        /* One exists, allocate space for it */
        RelativeSet->lpafpProtocols = WsBufferAllocate(Buffer,
                                                       UnicodeSet->dwNumberOfProtocols *
                                                       sizeof(AFPROTOCOLS),
                                                       sizeof(PVOID));
        /* Copy it into the buffer */
        RtlCopyMemory(RelativeSet->lpafpProtocols,
                      UnicodeSet->lpafpProtocols,
                      UnicodeSet->dwNumberOfProtocols * sizeof(AFPROTOCOLS));
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpafpProtocols = NULL;
        RelativeSet->dwNumberOfProtocols = 0;
    }

    /* Check if we have a CSADDR with addresses inside */
    if ((UnicodeSet->lpcsaBuffer) && (UnicodeSet->dwNumberOfCsAddrs))
    {
        /* Allocate and copy the CSADDR structure itself */
        RelativeSet->lpcsaBuffer = WsBufferAllocate(Buffer,
                                                    UnicodeSet->dwNumberOfCsAddrs *
                                                    sizeof(CSADDR_INFO),
                                                    sizeof(PVOID));

        /* Copy it into the buffer */
        RtlCopyMemory(RelativeSet->lpcsaBuffer,
                      UnicodeSet->lpcsaBuffer,
                      UnicodeSet->dwNumberOfCsAddrs * sizeof(CSADDR_INFO));

        /* Copy the addresses inside the CSADDR */
        CopyAddrInfoArrayIndirect(Buffer,
                                  RelativeSet->lpcsaBuffer,
                                  UnicodeSet->dwNumberOfCsAddrs,
                                  UnicodeSet->lpcsaBuffer);
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpcsaBuffer = NULL;
        RelativeSet->dwNumberOfCsAddrs = 0;
    }

    /* Check for blob data */
    if (UnicodeSet->lpBlob)
    {
        /* Allocate and copy the blob itself */
        RelativeSet->lpBlob = WsBufferAllocate(Buffer,
                                               sizeof(BLOB),
                                               sizeof(PVOID));
        *(RelativeSet->lpBlob) = *(UnicodeSet->lpBlob);

        /* Copy the data inside the blob */
        CopyBlobIndirect(Buffer, RelativeSet->lpBlob, UnicodeSet->lpBlob);
    }
    else
    {
        /* Nothing in the buffer */
        RelativeSet->lpBlob = NULL;
    }
}

INT
WSAAPI
WSABuildQuerySetBufferA(IN LPWSAQUERYSETA AnsiSet,
                        IN SIZE_T BufferSize,
                        OUT LPWSAQUERYSETA RelativeSet)
{
    INT ErrorCode = ERROR_SUCCESS;
    SIZE_T SetSize;
    WS_BUFFER Buffer;
    LPWSAQUERYSETA NewSet;

    /* Find out how big the set really is */
    SetSize = WSAComputeQuerySetSizeA(AnsiSet);
    if (SetSize <= BufferSize)
    {
        /* Configure the buffer */
        Buffer.Position = (ULONG_PTR)RelativeSet;
        Buffer.MaxSize = SetSize;
        Buffer.BytesUsed = 0;

        /* Copy the set itself into the buffer */
        NewSet = WsBufferAllocate(&Buffer, sizeof(*AnsiSet), sizeof(PVOID));
        *NewSet = *AnsiSet;

        /* Now copy the data inside */
        CopyQuerySetIndirectA(&Buffer, NewSet, AnsiSet);
    }
    else
    {
        /* We failed */
        ErrorCode = SOCKET_ERROR;
    }

    /* Return to caller */
    return ErrorCode;
}

INT
WSAAPI
WSABuildQuerySetBufferW(IN LPWSAQUERYSETW UnicodeSet,
                        IN SIZE_T BufferSize,
                        OUT LPWSAQUERYSETW RelativeSet)
{
    INT ErrorCode = ERROR_SUCCESS;
    SIZE_T SetSize;
    WS_BUFFER Buffer;
    LPWSAQUERYSETW NewSet;

    /* Find out how big the set really is */
    SetSize = WSAComputeQuerySetSizeW(UnicodeSet);
    if (SetSize <= BufferSize)
    {
        /* Configure the buffer */
        Buffer.Position = (ULONG_PTR)RelativeSet;
        Buffer.MaxSize = SetSize;
        Buffer.BytesUsed = 0;

        /* Copy the set itself into the buffer */
        NewSet = WsBufferAllocate(&Buffer, sizeof(*UnicodeSet), sizeof(PVOID));
        *NewSet = *UnicodeSet;

        /* Now copy the data inside */
        CopyQuerySetIndirectW(&Buffer, NewSet, UnicodeSet);
    }
    else
    {
        /* We failed */
        ErrorCode = SOCKET_ERROR;
    }

    /* Return to caller */
    return ErrorCode;
}

INT
WSAAPI
MapAnsiQuerySetToUnicode(IN LPWSAQUERYSETA AnsiSet,
                         IN OUT PSIZE_T SetSize,
                         OUT LPWSAQUERYSETW UnicodeSet)
{
    INT ErrorCode = ERROR_SUCCESS;
    SIZE_T AnsiSize, UnicodeSize;
    LPWSAQUERYSETA AnsiCopy = NULL;
    LPWSAQUERYSETW UnicodeCopy;
    LPWSTR ServiceCopy = NULL, CommentCopy = NULL;
    LPWSTR ContextCopy = NULL, QueryCopy = NULL;

    /* Calculate the size of the Ansi version and allocate space for a copy */
    AnsiSize = WSAComputeQuerySetSizeA(AnsiSet);
    AnsiCopy = HeapAlloc(WsSockHeap, 0, AnsiSize);
    if (!AnsiCopy)
    {
        /* Fail, couldn't allocate memory */
        ErrorCode = WSA_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    /* Build the relative buffer version */
    ErrorCode = WSABuildQuerySetBufferA(AnsiSet, AnsiSize, AnsiCopy);
    if (ErrorCode != ERROR_SUCCESS) goto Exit;

    /* Re-use the ANSI version since the fields match */
    UnicodeCopy = (LPWSAQUERYSETW)AnsiCopy;

    /* Check if we have a service instance name */
    if (AnsiCopy->lpszServiceInstanceName)
    {
        /* Duplicate it into unicode form */
        ServiceCopy = UnicodeDupFromAnsi(AnsiCopy->lpszServiceInstanceName);
        if (!ServiceCopy)
        {
            /* Fail */
            ErrorCode = WSA_NOT_ENOUGH_MEMORY;
            goto Exit;
        }

        /* Set the new string pointer */
        UnicodeCopy->lpszServiceInstanceName = ServiceCopy;
    }

    /* Check if we have a service instance name */
    if (AnsiCopy->lpszContext)
    {
        /* Duplicate it into unicode form */
        ContextCopy = UnicodeDupFromAnsi(AnsiCopy->lpszContext);
        if (!ContextCopy)
        {
            /* Fail */
            ErrorCode = WSA_NOT_ENOUGH_MEMORY;
            goto Exit;
        }

        /* Set the new string pointer */
        UnicodeCopy->lpszContext = ContextCopy;
    }

    /* Check if we have a service instance name */
    if (AnsiCopy->lpszComment)
    {
        /* Duplicate it into unicode form */
        CommentCopy = UnicodeDupFromAnsi(AnsiCopy->lpszComment);
        if (!CommentCopy)
        {
            /* Fail */
            ErrorCode = WSA_NOT_ENOUGH_MEMORY;
            goto Exit;
        }

        /* Set the new string pointer */
        UnicodeCopy->lpszComment = CommentCopy;
    }

    /* Check if we have a query name */
    if (AnsiCopy->lpszQueryString)
    {
        /* Duplicate it into unicode form */
        QueryCopy = UnicodeDupFromAnsi(AnsiCopy->lpszQueryString);
        if (!QueryCopy)
        {
            /* Fail */
            ErrorCode = WSA_NOT_ENOUGH_MEMORY;
            goto Exit;
        }

        /* Set the new string pointer */
        UnicodeCopy->lpszQueryString = QueryCopy;
    }

    /* Now that we have the absolute unicode buffer, calculate its size */
    UnicodeSize = WSAComputeQuerySetSizeW(UnicodeCopy);
    if (UnicodeSize > *SetSize)
    {
        /* The buffer wasn't large enough; return how much we need */
        *SetSize = UnicodeSize;
        ErrorCode = WSAEFAULT;
        goto Exit;
    }

    /* Build the relative unicode buffer */
    ErrorCode = WSABuildQuerySetBufferW(UnicodeCopy, *SetSize, UnicodeSet);

Exit:
    /* Free the Ansi copy if we had one */
    if (AnsiCopy) HeapFree(WsSockHeap, 0, AnsiCopy);

    /* Free all the strings */
    if (ServiceCopy) HeapFree(WsSockHeap, 0, ServiceCopy);
    if (CommentCopy) HeapFree(WsSockHeap, 0, CommentCopy);
    if (ContextCopy) HeapFree(WsSockHeap, 0, ContextCopy);
    if (QueryCopy) HeapFree(WsSockHeap, 0, QueryCopy);

    /* Return error code */
    return ErrorCode;
}

INT
WSAAPI
MapUnicodeQuerySetToAnsi(IN LPWSAQUERYSETW UnicodeSet,
                         IN OUT PSIZE_T SetSize,
                         OUT LPWSAQUERYSETA AnsiSet)
{
    INT ErrorCode = ERROR_SUCCESS;
    SIZE_T UnicodeSize, AnsiSize;
    LPWSAQUERYSETW UnicodeCopy = NULL;
    LPWSAQUERYSETA AnsiCopy;
    LPSTR ServiceCopy = NULL, CommentCopy = NULL;
    LPSTR ContextCopy = NULL, QueryCopy = NULL;

    /* Calculate the size of the Ansi version and allocate space for a copy */
    UnicodeSize = WSAComputeQuerySetSizeW(UnicodeSet);
    UnicodeCopy = HeapAlloc(WsSockHeap, 0, UnicodeSize);
    if (!UnicodeCopy)
    {
        /* Fail, couldn't allocate memory */
        ErrorCode = WSA_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    /* Build the relative buffer version */
    ErrorCode = WSABuildQuerySetBufferW(UnicodeSet, UnicodeSize, UnicodeCopy);
    if (ErrorCode != ERROR_SUCCESS) goto Exit;

    /* Re-use the Unicode version since the fields match */
    AnsiCopy = (LPWSAQUERYSETA)UnicodeCopy;

    /* Check if we have a service instance name */
    if (UnicodeCopy->lpszServiceInstanceName)
    {
        /* Duplicate it into unicode form */
        ServiceCopy = AnsiDupFromUnicode(UnicodeCopy->lpszServiceInstanceName);
        if (!ServiceCopy)
        {
            /* Fail */
            ErrorCode = WSA_NOT_ENOUGH_MEMORY;
            goto Exit;
        }

        /* Set the new string pointer */
        AnsiCopy->lpszServiceInstanceName = ServiceCopy;
    }

    /* Check if we have a service instance name */
    if (UnicodeCopy->lpszContext)
    {
        /* Duplicate it into unicode form */
        ContextCopy = AnsiDupFromUnicode(UnicodeCopy->lpszContext);
        if (!ContextCopy)
        {
            /* Fail */
            ErrorCode = WSA_NOT_ENOUGH_MEMORY;
            goto Exit;
        }

        /* Set the new string pointer */
        AnsiCopy->lpszContext = ContextCopy;
    }

    /* Check if we have a service instance name */
    if (UnicodeCopy->lpszComment)
    {
        /* Duplicate it into unicode form */
        CommentCopy = AnsiDupFromUnicode(UnicodeCopy->lpszComment);
        if (!CommentCopy)
        {
            /* Fail */
            ErrorCode = WSA_NOT_ENOUGH_MEMORY;
            goto Exit;
        }

        /* Set the new string pointer */
        AnsiCopy->lpszComment = CommentCopy;
    }

    /* Check if we have a query name */
    if (UnicodeCopy->lpszQueryString)
    {
        /* Duplicate it into unicode form */
        QueryCopy = AnsiDupFromUnicode(UnicodeCopy->lpszQueryString);
        if (!QueryCopy)
        {
            /* Fail */
            ErrorCode = WSA_NOT_ENOUGH_MEMORY;
            goto Exit;
        }

        /* Set the new string pointer */
        AnsiCopy->lpszQueryString = QueryCopy;
    }

    /* Now that we have the absolute unicode buffer, calculate its size */
    AnsiSize = WSAComputeQuerySetSizeA(AnsiCopy);
    if (AnsiSize > *SetSize)
    {
        /* The buffer wasn't large enough; return how much we need */
        *SetSize = AnsiSize;
        ErrorCode = WSAEFAULT;
        goto Exit;
    }

    /* Build the relative unicode buffer */
    ErrorCode = WSABuildQuerySetBufferA(AnsiCopy, *SetSize, AnsiSet);

Exit:
    /* Free the Ansi copy if we had one */
    if (UnicodeCopy) HeapFree(WsSockHeap, 0, UnicodeCopy);

    /* Free all the strings */
    if (ServiceCopy) HeapFree(WsSockHeap, 0, ServiceCopy);
    if (CommentCopy) HeapFree(WsSockHeap, 0, CommentCopy);
    if (ContextCopy) HeapFree(WsSockHeap, 0, ContextCopy);
    if (QueryCopy) HeapFree(WsSockHeap, 0, QueryCopy);

    /* Return error code */
    return ErrorCode;
}

INT
WSAAPI
CopyQuerySetW(IN LPWSAQUERYSETW UnicodeSet,
              OUT LPWSAQUERYSETW *UnicodeCopy)
{
    SIZE_T SetSize;

    /* Get the size */
    SetSize = WSAComputeQuerySetSizeW(UnicodeSet);

    /* Allocate memory for copy */
    *UnicodeCopy = HeapAlloc(WsSockHeap, 0, SetSize);
    if (!(*UnicodeCopy)) return WSA_NOT_ENOUGH_MEMORY;

    /* Build a copy and return */
    return WSABuildQuerySetBufferW(UnicodeSet, SetSize, *UnicodeCopy);
}
