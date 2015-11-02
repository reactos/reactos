/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/getxbyxx.c
 * PURPOSE:     Get X by Y Functions for Name Resolution.
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

//#define NDEBUG
#include <debug.h>

/* DATA **********************************************************************/

AFPROTOCOLS afp[2] = {{AF_INET, IPPROTO_UDP}, {AF_INET, IPPROTO_TCP}};
                     
/* FUNCTIONS *****************************************************************/

VOID
WSAAPI
FixList(PCHAR **List, 
        ULONG_PTR Base)
{
    /* Make sure it's valid */
    if(*List)
    {
        PCHAR *Addr;

        /* Get the right base */
        Addr = *List = (PCHAR*)(((ULONG_PTR)*List + Base));

        /* Loop the pointers */
        while(*Addr)
        {
            /* Rebase them too */
            *Addr = (PCHAR)(((ULONG_PTR)*Addr + Base));
            Addr++;
        }
    }
}

VOID
WSAAPI
UnpackServEnt(PSERVENT Servent)
{
    ULONG_PTR ServentPtr = (ULONG_PTR)Servent;

    /* Convert all the List Offsets to Pointers */
    FixList(&Servent->s_aliases, ServentPtr);

    /* Convert the Name and Protocol Offesets to Pointers */
    Servent->s_name = (PCHAR)(Servent->s_name + ServentPtr);
    Servent->s_proto = (PCHAR)(Servent->s_proto + ServentPtr);
}

VOID
WSAAPI
UnpackHostEnt(PHOSTENT Hostent)
{
    ULONG_PTR HostentPtr = (ULONG_PTR)Hostent;

    /* Convert the Name Offset to a Pointer */
    if(Hostent->h_name) Hostent->h_name = (PCHAR)(Hostent->h_name + HostentPtr);

    /* Convert all the List Offsets to Pointers */
    FixList(&Hostent->h_aliases, HostentPtr);
    FixList(&Hostent->h_addr_list, HostentPtr);
}

VOID
WSAAPI
Local_Ip4AddresstoString(IN PCHAR AddressBuffer,
                         IN PCHAR Address)
{
    /* Convert the address into IPv4 format */
    sprintf(AddressBuffer, "%u.%u.%u.%u",
            ((unsigned)Address[0] & 0xff),
            ((unsigned)Address[1] & 0xff),
            ((unsigned)Address[2] & 0xff),
            ((unsigned)Address[3] & 0xff));
}

VOID
WSAAPI
Local_Ip6AddresstoString(IN PCHAR AddressBuffer,
                         IN PCHAR Address)
{
    DWORD i;

    /* Convert the address into IPv6 format */
    for (i = 0; i < 8; i++)
    {
        sprintf(AddressBuffer, "%x:",
                ((unsigned)Address[0] & 0xff));
    }
}

LPBLOB
WSAAPI
getxyDataEnt(IN OUT PCHAR *Results,
             IN DWORD Length,
             IN LPSTR Name,
             IN LPCGUID Type,
             IN LPSTR *NewName)
{
    PWSAQUERYSETA WsaQuery = (PWSAQUERYSETA)*Results;
    INT ErrorCode;
    HANDLE RnRHandle;
    LPBLOB Blob = NULL;
    PVOID NewResults;

    /* Assume empty return name */
    if (NewName) *NewName = NULL;

    /* Set up the Winsock Service Query */
    RtlZeroMemory(WsaQuery, sizeof(*WsaQuery));
    WsaQuery->dwSize = sizeof(*WsaQuery);
    WsaQuery->lpszServiceInstanceName = Name;
    WsaQuery->lpServiceClassId = (LPGUID)Type;
    WsaQuery->dwNameSpace = NS_ALL;
    WsaQuery->dwNumberOfProtocols = 2;
    WsaQuery->lpafpProtocols = &afp[0];

    /* Send the Query Request to find a Service */
    ErrorCode = WSALookupServiceBeginA(WsaQuery,
                                       LUP_RETURN_BLOB | LUP_RETURN_NAME,
                                       &RnRHandle);

    if(ErrorCode == ERROR_SUCCESS) 
    {
        while (TRUE)
        {
            /* Service was found, send the real query */
            ErrorCode = WSALookupServiceNextA(RnRHandle,
                                              0,
                                              &Length,
                                              WsaQuery);

            /* Return the information requested */
            if(ErrorCode == ERROR_SUCCESS) 
            {
                /* Get the Blob and check if we have one */
                Blob = WsaQuery->lpBlob;
                if(Blob) 
                {
                    /* Did they want the name back? */
                    if(NewName) *NewName = WsaQuery->lpszServiceInstanceName;
                } 
                else 
                {
                    /* Check if this was a Hostname lookup */
                    if (Type == &HostnameGuid)
                    {
                        /* Return the name anyways */
                        if(NewName) *NewName = WsaQuery->lpszServiceInstanceName;
                    }
                    else
                    {
                        /* We don't have a blob, sorry */
                        ErrorCode = WSANO_DATA;
                    }
                }
            } 
            else 
            {
                /* WSALookupServiceEnd will set its own error, so save ours */
                ErrorCode = GetLastError();

                /* Check if we failed because of missing buffer space */
                if ((ErrorCode == WSAEFAULT) && (Length > RNR_BUFFER_SIZE))
                {
                    /* Allocate a new buffer */
                    NewResults = HeapAlloc(WsSockHeap, 0, Length);
                    if (NewResults)
                    {
                        /* Tell the caller his new buffer */
                        *Results = NewResults;

                        /* Update the WSA Query's location */
                        WsaQuery = (PWSAQUERYSETA)NewResults;

                        /* Loop again */
                        continue;
                    }
                    else
                    {
                        /* No memory to allocate the new buffer */
                        ErrorCode = WSA_NOT_ENOUGH_MEMORY;
                    }
                }
            }
        
            /* Finish the Query Request */
            WSALookupServiceEnd(RnRHandle);

            /* Now set the Last Error */
            if(ErrorCode != ERROR_SUCCESS) SetLastError(ErrorCode);

            /* Leave the loop */
            break;
        }
    }

    /* Return the blob */
    return Blob;
}

/*
 * @implemented
 */
PHOSTENT
WSAAPI
gethostbyname(IN const char FAR * name)
{
    PHOSTENT Hostent;
    LPBLOB Blob;
    INT ErrorCode;
    CHAR ResultsBuffer[RNR_BUFFER_SIZE];
    PCHAR Results = ResultsBuffer;
    CHAR szLocalName[200];
    PCHAR pszName;
    PWSPROCESS Process;
    PWSTHREAD Thread;
    DPRINT("gethostbyname: %s\n", name);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Leave now */
        SetLastError(ErrorCode);
        return NULL;
    }

    /* Check if no name was given */
    if(!name || !*name) 
    {
        /* This means we should do a local lookup first */
        if(gethostname(szLocalName, 200) != NO_ERROR) return(NULL);
        pszName = szLocalName;
    } 
    else 
    {
        /* Use the name tha twas given to us */
        pszName = (PCHAR)name;
    }

    /* Get the Hostname in a Blob Structure */
    Blob = getxyDataEnt(&Results,
                        RNR_BUFFER_SIZE,
                        pszName,
                        &HostAddrByNameGuid,
                        0);
    
    /* Check if we didn't get a blob, or if we got an empty name */
    if (!(Blob) && (!(name) || !(*name)))
    {
        /* Try a new query */
        Blob = getxyDataEnt(&Results,
                            RNR_BUFFER_SIZE,
                            NULL,
                            &HostAddrByNameGuid,
                            0);
    }

    /* Check if we got a blob */
    if(Blob) 
    {
        /* Copy the blob to our buffer and convert it */
        Hostent = WsThreadBlobToHostent(Thread, Blob);

        /* Unpack the hostent */
        if(Hostent) UnpackHostEnt(Hostent);
    } 
    else 
    {
        /* We failed, so zero it out */
        Hostent = 0;

        /* Normalize the error message */
        if(GetLastError() == WSASERVICE_NOT_FOUND) 
        {
            SetLastError(WSAHOST_NOT_FOUND);
        }
    }

    /* Check if we received a newly allocated buffer; free it. */
    if (Results != ResultsBuffer) HeapFree(WsSockHeap, 0, Results);

    /* Notify RAS Auto-dial helper */
    if (Hostent) WSNoteSuccessfulHostentLookup(name, *Hostent->h_addr);

    /* Return the hostent */
    return Hostent;
}

/*
 * @implemented
 */
PHOSTENT
WSAAPI
gethostbyaddr(IN const char FAR * addr,
              IN int len,
              IN int type)
{
    CHAR AddressBuffer[100];
    PHOSTENT Hostent;
    LPBLOB Blob;
    CHAR ResultsBuffer[RNR_BUFFER_SIZE];
    PCHAR Results = ResultsBuffer;
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode;
    DPRINT("gethostbyaddr: %s\n", addr);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Leave now */
        SetLastError(ErrorCode);
        return NULL;
    }

    /* Check for valid address pointer */
    if (!addr)
    {
        /* Fail */
        SetLastError(WSAEINVAL);
        return NULL;
    }

    /* Check which type it is */
    if (type == AF_INET)
    {
        /* Use IPV4 Address to String */
        Local_Ip4AddresstoString(AddressBuffer, (PCHAR)addr);
    }
    else if (type == AF_INET6)
    {
        /* Use IPV6 Address to String */
        Local_Ip6AddresstoString(AddressBuffer, (PCHAR)addr);
    }
    else
    {
        /* Invalid address type; fail */
        SetLastError(WSAEINVAL);
        return NULL;
    }

    /* Get the Hostname in a Blob Structure */
    Blob = getxyDataEnt(&Results,
                        RNR_BUFFER_SIZE,
                        AddressBuffer,
                        &AddressGuid,
                        0);
    
    /* Check if we got a blob */
    if(Blob) 
    {
        /* Copy the blob to our buffer and convert it */
        Hostent = WsThreadBlobToHostent(Thread, Blob);

        /* Unpack the hostent */
        if(Hostent) UnpackHostEnt(Hostent);
    } 
    else 
    {
        /* We failed, so zero it out */
        Hostent = 0;

        /* Normalize the error message */
        if(GetLastError() == WSASERVICE_NOT_FOUND) 
        {
            SetLastError(WSAHOST_NOT_FOUND);
        }
    }

    /* Check if we received a newly allocated buffer; free it. */
    if (Results != ResultsBuffer) HeapFree(WsSockHeap, 0, Results);

    /* Return the hostent */
    return Hostent;
}

/*
 * @implemented
 */
INT
WSAAPI
gethostname(OUT char FAR * name,
            IN int namelen)
{
    PCHAR Name;
    CHAR ResultsBuffer[RNR_BUFFER_SIZE];
    PCHAR Results = ResultsBuffer;
    DPRINT("gethostname: %p\n", name);

    /* Get the Hostname in a String */
    if(getxyDataEnt(&Results, RNR_BUFFER_SIZE, NULL, &HostnameGuid, &Name))
    {
        /* Copy it */
        strcpy((LPSTR)name, Name);
    }

    /* Check if we received a newly allocated buffer; free it. */
    if (Results != ResultsBuffer) HeapFree(WsSockHeap, 0, Results);

    /* Return success */
    return ERROR_SUCCESS;
}

/*
 * @implemented
 */
PSERVENT
WSAAPI
getservbyport(IN int port,
              IN const char FAR * proto)
{
    PSERVENT Servent;
    LPBLOB Blob;
    CHAR ResultsBuffer[RNR_BUFFER_SIZE];
    PCHAR Results = ResultsBuffer;
    PCHAR PortName;
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode;
    DPRINT("getservbyport: %s\n", proto);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Leave now */
        SetLastError(ErrorCode);
        return NULL;
    }

    /* No protocol specifed */
    if(!proto) proto = "";

    /* Allocate memory for the port name */
    PortName = HeapAlloc(WsSockHeap, 0, strlen(proto) + 1 + 1 + 5);
    if (!PortName)
    {
        /* Fail */
        SetLastError(WSA_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    /* Put it into the right syntax */
    sprintf(PortName, "%d/%s", (port & 0xffff), proto);

    /* Get the Service in a Blob */
    Blob = getxyDataEnt(&Results, RNR_BUFFER_SIZE, PortName, &IANAGuid, 0);

    /* Free the string we sent */
    HeapFree(WsSockHeap, 0, PortName);

    /* Check if we got a blob */
    if(Blob) 
    {
        /* Copy the blob to our buffer and convert it */
        Servent = WsThreadBlobToServent(Thread, Blob);

        /* Unpack the hostent */
        if(Servent) UnpackServEnt(Servent);
    } 
    else 
    {
        /* We failed, so zero it out */
        Servent = 0;

        /* Normalize the error message */
        if(GetLastError() == WSATYPE_NOT_FOUND) SetLastError(WSANO_DATA);
    }

    /* Check if we received a newly allocated buffer; free it. */
    if (Results != ResultsBuffer) HeapFree(WsSockHeap, 0, Results);

    /* Return the hostent */
    return Servent;
}

/*
 * @implemented
 */
PSERVENT
WSAAPI
getservbyname(IN const char FAR * name,
              IN const char FAR * proto)
{
    PSERVENT Servent;
    LPBLOB Blob;
    CHAR ResultsBuffer[RNR_BUFFER_SIZE];
    PCHAR Results = ResultsBuffer;
    PCHAR PortName;
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode;
    DPRINT("getservbyname: %s\n", name);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Leave now */
        SetLastError(ErrorCode);
        return NULL;
    }

    /* No protocol specifed */
    if(!proto) proto = "";

    /* Allocate buffer for it */
    PortName = HeapAlloc(WsSockHeap, 0, strlen(proto) + 1 + strlen(name) + 1);
    if (!PortName)
    {
        /* Fail */
        SetLastError(WSA_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    /* Put it into the right syntax */
    sprintf(PortName, "%s/%s", name, proto);

    /* Get the Service in a Blob */
    Blob = getxyDataEnt(&Results, RNR_BUFFER_SIZE, PortName, &IANAGuid, 0);

    /* Free the string we sent */
    HeapFree(WsSockHeap, 0, PortName);

    /* Check if we got a blob */
    if(Blob) 
    {
        /* Copy the blob to our buffer and convert it */
        Servent = WsThreadBlobToServent(Thread, Blob);

        /* Unpack the hostent */
        if(Servent) UnpackServEnt(Servent);
    } 
    else 
    {
        /* We failed, so zero it out */
        Servent = 0;

        /* Normalize the error message */
        if(GetLastError() == WSATYPE_NOT_FOUND) SetLastError(WSANO_DATA);
    }

    /* Check if we received a newly allocated buffer; free it. */
    if (Results != ResultsBuffer) HeapFree(WsSockHeap, 0, Results);

    /* Return the hostent */
    return Servent;
}

/*
 * @implemented
 */
HANDLE
WSAAPI
WSAAsyncGetHostByAddr(IN HWND hWnd,
                      IN UINT wMsg,
                      IN CONST CHAR FAR *Address, 
                      IN INT Length,
                      IN INT Type, 
                      OUT CHAR FAR *Buffer, 
                      IN INT BufferLength)
{
    HANDLE TaskHandle;
    PWSPROCESS Process;
    PWSTHREAD Thread;
    PVOID AddressCopy;
    PWSASYNCBLOCK AsyncBlock;
    INT ErrorCode;
    DPRINT("WSAAsyncGetHostByAddr: %lx, %lx, %s\n", hWnd, wMsg, Address);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Leave now */
        SetLastError(ErrorCode);
        return NULL;
    }

    /* Initialize the Async Thread */
    if (!WsAsyncCheckAndInitThread())
    {
        /* Fail */
        SetLastError(WSAENOBUFS);
        return NULL;
    }

    /* Allocate an async block */
    if (!(AsyncBlock = WsAsyncAllocateBlock(Length)))
    {
        /* Fail */
        SetLastError(WSAENOBUFS);
        return NULL;
    }

    /* Make a copy of the address */
    AddressCopy = AsyncBlock + 1;
    RtlMoveMemory(AddressCopy, Address, Length);

    /* Initialize the Async Block */
    AsyncBlock->Operation = WsAsyncGetHostByAddr;
    AsyncBlock->GetHost.hWnd = hWnd;
    AsyncBlock->GetHost.wMsg = wMsg;
    AsyncBlock->GetHost.ByWhat = AddressCopy;
    AsyncBlock->GetHost.Length = Length;
    AsyncBlock->GetHost.Type = Type;
    AsyncBlock->GetHost.Buffer = Buffer;
    AsyncBlock->GetHost.BufferLength = BufferLength;

    /* Save the task handle and queue the request */
    TaskHandle = AsyncBlock->TaskHandle;
    WsAsyncQueueRequest(AsyncBlock);

    /* Return the task handle */
    return TaskHandle;
}

/*
 * @implemented
 */
HANDLE
WSAAPI
WSAAsyncGetHostByName(IN HWND hWnd, 
                      IN UINT wMsg,  
                      IN CONST CHAR FAR *Name, 
                      OUT CHAR FAR *Buffer, 
                      IN INT BufferLength)
{
    HANDLE TaskHandle;
    PWSPROCESS Process;
    PWSTHREAD Thread;
    PWSASYNCBLOCK AsyncBlock;
    INT ErrorCode;
    PVOID NameCopy;
    DPRINT("WSAAsyncGetProtoByNumber: %lx, %lx, %s\n", hWnd, wMsg, Name);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Leave now */
        SetLastError(ErrorCode);
        return NULL;
    }

    /* Initialize the Async Thread */
    if (!WsAsyncCheckAndInitThread())
    {
        /* Fail */
        SetLastError(WSAENOBUFS);
        return NULL;
    }

    /* Allocate an async block */
    if (!(AsyncBlock = WsAsyncAllocateBlock(strlen(Name) + sizeof(CHAR))))
    {
        /* Fail */
        SetLastError(WSAENOBUFS);
        return NULL;
    }

    /* Make a copy of the address */
    NameCopy = AsyncBlock + 1;
    strcpy(NameCopy, Name);

    /* Initialize the Async Block */
    AsyncBlock->Operation = WsAsyncGetHostByName;
    AsyncBlock->GetHost.hWnd = hWnd;
    AsyncBlock->GetHost.wMsg = wMsg;
    AsyncBlock->GetHost.ByWhat = NameCopy;
    AsyncBlock->GetHost.Buffer = Buffer;
    AsyncBlock->GetHost.BufferLength = BufferLength;

    /* Save the task handle and queue the request */
    TaskHandle = AsyncBlock->TaskHandle;
    WsAsyncQueueRequest(AsyncBlock);

    /* Return the task handle */
    return TaskHandle;
}

/*
 * @implemented
 */
HANDLE
WSAAPI
WSAAsyncGetProtoByName(IN HWND hWnd,
                       IN UINT wMsg,
                       IN CONST CHAR FAR *Name,
                       OUT CHAR FAR *Buffer,
                       IN INT BufferLength)
{
    HANDLE TaskHandle;
    PWSPROCESS Process;
    PWSTHREAD Thread;
    PWSASYNCBLOCK AsyncBlock;
    INT ErrorCode;
    PVOID NameCopy;
    DPRINT("WSAAsyncGetProtoByName: %lx, %lx, %s\n", hWnd, wMsg, Name);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Leave now */
        SetLastError(ErrorCode);
        return NULL;
    }

    /* Initialize the Async Thread */
    if (!WsAsyncCheckAndInitThread())
    {
        /* Fail */
        SetLastError(WSAENOBUFS);
        return NULL;
    }

    /* Allocate an async block */
    if (!(AsyncBlock = WsAsyncAllocateBlock(strlen(Name) + sizeof(CHAR))))
    {
        /* Fail */
        SetLastError(WSAENOBUFS);
        return NULL;
    }

    /* Make a copy of the address */
    NameCopy = AsyncBlock + 1;
    strcpy(NameCopy, Name);

    /* Initialize the Async Block */
    AsyncBlock->Operation = WsAsyncGetProtoByName;
    AsyncBlock->GetProto.hWnd = hWnd;
    AsyncBlock->GetProto.wMsg = wMsg;
    AsyncBlock->GetProto.ByWhat = NameCopy;
    AsyncBlock->GetProto.Buffer = Buffer;
    AsyncBlock->GetProto.BufferLength = BufferLength;

    /* Save the task handle and queue the request */
    TaskHandle = AsyncBlock->TaskHandle;
    WsAsyncQueueRequest(AsyncBlock);

    /* Return the task handle */
    return TaskHandle;
}

/*
 * @implemented
 */
HANDLE
WSAAPI
WSAAsyncGetProtoByNumber(IN HWND hWnd,
                         IN UINT wMsg,
                         IN INT Number,
                         OUT CHAR FAR* Buffer,
                         IN INT BufferLength)
{
    HANDLE TaskHandle;
    PWSPROCESS Process;
    PWSTHREAD Thread;
    PWSASYNCBLOCK AsyncBlock;
    INT ErrorCode;
    DPRINT("WSAAsyncGetProtoByNumber: %lx, %lx, %lx\n", hWnd, wMsg, Number);
    
    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Leave now */
        SetLastError(ErrorCode);
        return NULL;
    }

    /* Initialize the Async Thread */
    if (!WsAsyncCheckAndInitThread())
    {
        /* Fail */
        SetLastError(WSAENOBUFS);
        return NULL;
    }

    /* Allocate an async block */
    if (!(AsyncBlock = WsAsyncAllocateBlock(0)))
    {
        /* Fail */
        SetLastError(WSAENOBUFS);
        return NULL;
    }

    /* Initialize the Async Block */
    AsyncBlock->Operation = WsAsyncGetProtoByNumber;
    AsyncBlock->GetProto.hWnd = hWnd;
    AsyncBlock->GetProto.wMsg = wMsg;
    AsyncBlock->GetProto.ByWhat = UlongToPtr(Number);
    AsyncBlock->GetProto.Buffer = Buffer;
    AsyncBlock->GetProto.BufferLength = BufferLength;

    /* Save the task handle and queue the request */
    TaskHandle = AsyncBlock->TaskHandle;
    WsAsyncQueueRequest(AsyncBlock);

    /* Return the task handle */
    return TaskHandle;
}

/*
 * @implemented
 */
HANDLE
WSAAPI
WSAAsyncGetServByName(IN HWND hWnd,
                      IN UINT wMsg,
                      IN CONST CHAR FAR *Name,
                      IN CONST CHAR FAR *Protocol,
                      OUT CHAR FAR *Buffer,
                      IN INT BufferLength)
{
    HANDLE TaskHandle;
    PWSPROCESS Process;
    PWSTHREAD Thread;
    PWSASYNCBLOCK AsyncBlock;
    INT ErrorCode;
    PVOID NameCopy;
    DPRINT("WSAAsyncGetProtoByNumber: %lx, %lx, %s\n", hWnd, wMsg, Name);
    
    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Leave now */
        SetLastError(ErrorCode);
        return NULL;
    }

    /* Initialize the Async Thread */
    if (!WsAsyncCheckAndInitThread())
    {
        /* Fail */
        SetLastError(WSAENOBUFS);
        return NULL;
    }

    /* Allocate an async block */
    if (!(AsyncBlock = WsAsyncAllocateBlock(strlen(Name) + sizeof(CHAR))))
    {
        /* Fail */
        SetLastError(WSAENOBUFS);
        return NULL;
    }

    /* Make a copy of the address */
    NameCopy = AsyncBlock + 1;
    strcpy(NameCopy, Name);

    /* Initialize the Async Block */
    AsyncBlock->Operation = WsAsyncGetProtoByName;
    AsyncBlock->GetServ.hWnd = hWnd;
    AsyncBlock->GetServ.wMsg = wMsg;
    AsyncBlock->GetServ.ByWhat = NameCopy;
    AsyncBlock->GetServ.Protocol = (PCHAR)Protocol;
    AsyncBlock->GetServ.Buffer = Buffer;
    AsyncBlock->GetServ.BufferLength = BufferLength;

    /* Save the task handle and queue the request */
    TaskHandle = AsyncBlock->TaskHandle;
    WsAsyncQueueRequest(AsyncBlock);

    /* Return the task handle */
    return TaskHandle;
}

/*
 * @implemented
 */
HANDLE
WSAAPI
WSAAsyncGetServByPort(IN HWND hWnd,
                      IN UINT wMsg,
                      IN INT Port,
                      IN CONST CHAR FAR *Protocol,
                      OUT CHAR FAR *Buffer,
                      IN INT BufferLength)
{
    HANDLE TaskHandle;
    PWSPROCESS Process;
    PWSTHREAD Thread;
    PWSASYNCBLOCK AsyncBlock;
    INT ErrorCode;
    DPRINT("WSAAsyncGetProtoByNumber: %lx, %lx, %lx\n", hWnd, wMsg, Port);
    
    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Leave now */
        SetLastError(ErrorCode);
        return NULL;
    }

    /* Initialize the Async Thread */
    if (!WsAsyncCheckAndInitThread())
    {
        /* Fail */
        SetLastError(WSAENOBUFS);
        return NULL;
    }

    /* Allocate an async block */
    if (!(AsyncBlock = WsAsyncAllocateBlock(0)))
    {
        /* Fail */
        SetLastError(WSAENOBUFS);
        return NULL;
    }

    /* Initialize the Async Block */
    AsyncBlock->Operation = WsAsyncGetServByPort;
    AsyncBlock->GetServ.hWnd = hWnd;
    AsyncBlock->GetServ.wMsg = wMsg;
    AsyncBlock->GetServ.ByWhat = UlongToPtr(Port);
    AsyncBlock->GetServ.Protocol = (PCHAR)Protocol;
    AsyncBlock->GetServ.Buffer = Buffer;
    AsyncBlock->GetServ.BufferLength = BufferLength;

    /* Save the task handle and queue the request */
    TaskHandle = AsyncBlock->TaskHandle;
    WsAsyncQueueRequest(AsyncBlock);

    /* Return the task handle */
    return TaskHandle;
}

/*
 * @implemented
 */
INT
WSAAPI
WSACancelAsyncRequest(IN HANDLE hAsyncTaskHandle)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode;
    DPRINT("WSACancelAsyncRequest: %lx\n", hAsyncTaskHandle);
    
    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) == ERROR_SUCCESS)
    {
        /* Call the Async code */
        ErrorCode = WsAsyncCancelRequest(hAsyncTaskHandle);

        /* Return */
        if (ErrorCode == ERROR_SUCCESS) return ERROR_SUCCESS;
    }
    
    /* Fail */
    SetLastError(ErrorCode);
    return SOCKET_ERROR;
}
