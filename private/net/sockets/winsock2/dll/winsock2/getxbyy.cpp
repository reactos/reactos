/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    getxbyy.cpp

Abstract:

    This  module takes care of forwarding the "getxbyy" family of operations to
    the  correct getxbyy provider.  The preferred getxbyy provider is found via
    a  DLL  path  stored  in the registry.  If this registry entry is found, an
    attempt  is  made  to  load the DLL and retrieve entry points from it.  Any
    failures  cause  entry  points to be taken from the WSOCK32.DLL instead.  A
    getxbyy provider does not have to export all of the getxbyy functions since
    any it does not export are taken from WSOCK32.DLL.

    The  getxbyy  functions  handled  by  this module that can be replaced by a
    getxyy provider are the following:

    gethostbyaddr()
    gethostbyname()
    gethostname()
    getservbyname()
    getservbyport()
    WSAAsyncGetServByName()
    WSAAsyncGetServByPort()
    WSAAsyncGetProtoByName()
    WSAAsyncGetProtoByNumber()
    WSAAsyncGetHostByName()
    WSAAsyncGetHostByAddr()
    WSACancelAsyncRequest()

    The   actual  entry  points  supplied  by  the  getxbyy  provider  (or  the
    WSOCK32.DLL) are prefixed with a special string.  This prefix is defined by
    the manifest constant GETXBYYPREFIX.

Author:

    Paul Drews  drewsxpa@ashland.intel.com 12-19-1995

Revision History:

    12-19-1995 drewsxpa@ashland.intel.com
        Initial implementation
--*/



#include "precomp.h"
#include "svcguid.h"

//
// This is the initial buffer size passed by getxbyy functions
// to WSALookupServiceNext.  If it is insuffucient for the query,
// the amount specified by the provider is allocated and call is
// repeated.
// The initial buffer is allocated from stack, so we try to keep it
// relatively small, but still for performance reasons we want to be able
//  to satisfy most of the calls with just this amount
//
#define RNR_BUFFER_SIZE (sizeof(WSAQUERYSET) + 256)


LPBLOB
getxyDataEnt(
    IN OUT PCHAR *pResults,
    IN     DWORD dwLength,
    IN     LPSTR lpszName,
    IN     LPGUID lpType,
    OUT    LPSTR *  lppName OPTIONAL
    );

VOID
FixList(PCHAR ** List, PCHAR Base);

VOID
UnpackHostEnt(struct hostent * hostent);

VOID
UnpackServEnt(struct servent * servent);

GUID HostAddrByNameGuid = SVCID_INET_HOSTADDRBYNAME;
GUID HostNameGuid = SVCID_HOSTNAME;
GUID AddressGuid =  SVCID_INET_HOSTADDRBYINETSTRING;
GUID IANAGuid    =  SVCID_INET_SERVICEBYNAME;

//
// Utility to turn a list of offsets into a list of addresses. Used
// to convert structures returned as BLOBs.
//

VOID
FixList(PCHAR ** List, PCHAR Base)
{
    if(*List)
    {
        PCHAR * Addr;

        Addr = *List = (PCHAR *)( ((ULONG_PTR)*List + Base) );
        while(*Addr)
        {
            *Addr = (PCHAR)(((ULONG_PTR)*Addr + Base));
            Addr++;
        }
    }
}


//
// Routine to convert a hostent returned in a BLOB to one with
// usable pointers. The structure is converted in-place.
//
VOID
UnpackHostEnt(struct hostent * hostent)
{
     PCHAR pch;

     pch = (PCHAR)hostent;

     if(hostent->h_name)
     {
         hostent->h_name = (PCHAR)((ULONG_PTR)hostent->h_name + pch);
     }
     FixList(&hostent->h_aliases, pch);
     FixList(&hostent->h_addr_list, pch);
}

//
// Routine to unpack a servent returned in a BLOB to one with
// usable pointers. The structure is converted in-place
//

VOID
UnpackServEnt(struct servent * servent)
{
    PCHAR pch;

    pch = (PCHAR)servent;

    FixList(&servent->s_aliases, pch);
    servent->s_name = (PCHAR)(ULONG_PTR(servent->s_name) + pch);
    servent->s_proto = (PCHAR)(ULONG_PTR(servent->s_proto) + pch);
}


struct hostent FAR * WSAAPI
gethostbyaddr(
    IN const char FAR * addr,
    IN int len,
    IN int type
    )
/*++
Routine Description:

    Get host information corresponding to an address.

Arguments:

    addr - A pointer to an address in network byte order.

    len  - The length of the address, which must be 4 for PF_INET addresses.

    type - The type of the address, which must be PF_INET.

Returns:

    If  no  error  occurs,  gethostbyaddr()  returns  a  pointer to the hostent
    structure  described  above.   Otherwise  it  returns  a NULL pointer and a
    specific error code is stored with SetErrorCode().
--*/
{
    CHAR qbuf[100];
    struct hostent *ph;
    LPBLOB pBlob;
    PCHAR pResults;
    CHAR localResults[RNR_BUFFER_SIZE];
    INT ErrorCode;
    PDPROCESS Process;
    PDTHREAD Thread;

    ErrorCode = PROLOG(&Process,
                 &Thread);
    if(ErrorCode != ERROR_SUCCESS)
    {
        SetLastError(ErrorCode);
        return(NULL);
    }

    if ( !addr ) // Bug fix for #110476
    {
        SetLastError(WSAEINVAL);
        return(NULL);
    }

    pResults = localResults;

    //
    // BUGBUG. Only handles current inet address forms. Need to
    // fix it to handle any address length, or do we?
    //
    (void)sprintf(qbuf, "%u.%u.%u.%u",
            ((unsigned)addr[0] & 0xff),
            ((unsigned)addr[1] & 0xff),
            ((unsigned)addr[2] & 0xff),
            ((unsigned)addr[3] & 0xff));


    pBlob = getxyDataEnt(&pResults,
                         RNR_BUFFER_SIZE,
                         qbuf,
                         &AddressGuid,
                         0);
    if(pBlob)
    {
        ph = (struct hostent *)Thread->CopyHostEnt(pBlob);
        if(ph)
        {
            UnpackHostEnt(ph);
        }
    }
    else
    {
        ph = 0;
        if(GetLastError() == WSASERVICE_NOT_FOUND)
        {
            SetLastError(WSANO_ADDRESS);
        }
    }
    if (pResults!=localResults)
        delete pResults;
    return(ph);
}  // gethostbyaddr




struct hostent FAR * WSAAPI
gethostbyname(
    IN const char FAR * name
    )
/*++
Routine Description:

    Get host information corresponding to a hostname.

Arguments:

    name - A pointer to the null terminated name of the host.

Returns:

    If  no  error  occurs,  gethostbyname()  returns  a  pointer to the hostent
    structure  described  above.   Otherwise  it  returns  a NULL pointer and a
    specific errorr code is stored with SetErrorCode().
--*/
{
    struct hostent * hent;
    LPBLOB pBlob;
    PCHAR pResults;
    CHAR localResults[RNR_BUFFER_SIZE];
    INT ErrorCode;
    PDPROCESS Process;
    PDTHREAD Thread;
    CHAR  szLocalName[200];   // for storing the local name. This
                              // is simply a big number assumed
                              // to be large enough. This is used
                              // only when the caller chooses not to
                              // provide a name. Very lazy.
    PCHAR pszName;

    ErrorCode = PROLOG(&Process,
                 &Thread);
    if(ErrorCode != ERROR_SUCCESS)
    {
        SetLastError(ErrorCode);
        return(NULL);
    }

    //
    // A NULL input name means look for the local name. So,
    // get it.
    //
    if(!name || !*name)
    {
        if(gethostname(szLocalName, 200) != NO_ERROR)
        {
            return(NULL);
        }
        pszName = szLocalName;
    }
    else
    {
        pszName = (PCHAR)name;
    }

    pResults = localResults;

    pBlob = getxyDataEnt( &pResults,
                          RNR_BUFFER_SIZE,
                          pszName,
                          &HostAddrByNameGuid,
                          0);

    if ( !pBlob &&
         ( !name || !*name ) )
    {
        pBlob = getxyDataEnt( &pResults,
                              RNR_BUFFER_SIZE,
                              NULL,
                              &HostAddrByNameGuid,
                              0);
    }

    if(pBlob)
    {
        hent = (struct hostent *)Thread->CopyHostEnt(pBlob);
        if(hent)
        {
            UnpackHostEnt(hent);
        }
    }
    else
    {
        hent = 0;

        if(GetLastError() == WSASERVICE_NOT_FOUND)
        {
            SetLastError(WSAHOST_NOT_FOUND);
        }
    }

    if (pResults!=localResults)
        delete pResults;
#ifdef RASAUTODIAL
    //
    // Inform Autodial of a successful name lookup.
    // This is important with DNS, since reverse lookups
    // do not provide complete information about name
    // aliases.
    //
    if (hent)
        WSNoteSuccessfulHostentLookup(name, *(PULONG)hent->h_addr);
#endif // RASAUTODIAL
    return(hent);
}  // gethostbyname




int WSAAPI
gethostname(
    OUT char FAR * name,
    IN int namelen
    )
/*++
Routine Description:

    Return the standard host name for the local machine.

Arguments:

    name    - A pointer to a buffer that will receive the host name.

    namelen - The length of the buffer.

Returns:

    Zero  on  success  else  SOCKET_ERROR.   The  error  code  is  stored  with
    SetErrorCode().
--*/
{
    PCHAR lpName = NULL;
    INT ErrorCode, ReturnValue;
    PDPROCESS Process;
    PDTHREAD Thread;
    PCHAR pResults;
    CHAR localResults[RNR_BUFFER_SIZE];
    int   ValueLength;

    ErrorCode = PROLOG(&Process,
                 &Thread);
    if(ErrorCode != ERROR_SUCCESS)
    {
        SetLastError(ErrorCode);
        return(SOCKET_ERROR);
    }

    if (!name || IsBadWritePtr (name, namelen))
    {
        SetLastError (WSAEFAULT);
        return(SOCKET_ERROR);
    }

    //
    // Fix for bug #94978
    //
    // First check to see if the cluster computername variable is set.
    // If so, this overrides the actual gethostname to fool the application
    // into working when its network name and computer name are different.
    //
    ValueLength = GetEnvironmentVariableA("_CLUSTER_NETWORK_NAME_",
                                          name,
                                          namelen);

    if (ValueLength != 0)
    {
        //
        // The environment variable exists, return it directly
        //
        if ( ValueLength > namelen )
        {
            SetLastError(WSAEFAULT);
            return (SOCKET_ERROR);
        }

        return (ERROR_SUCCESS);
    }

    pResults = localResults;

    ReturnValue = ERROR_SUCCESS;

    (void) getxyDataEnt( &pResults,
                         RNR_BUFFER_SIZE,
                         NULL,
                         &HostAddrByNameGuid,
                         &lpName);

    if ( lpName )
    {
        INT iSize = strlen(lpName) + 1;

        if(iSize <= namelen)
        {
            memcpy(name, lpName, iSize);
        }
        else
        {
            SetLastError(WSAEFAULT);
            ReturnValue = SOCKET_ERROR;
        }
    }
    else
    {
        //
        // If getxyDataEnt() fails for any reason (like no NSPs installed)
        // then do things the 1.x way, giving them back the computer name
        //

        DWORD len = namelen;

        if (!GetComputerNameA (name, &len))
        {
            ReturnValue = SOCKET_ERROR;

            if (len >= (DWORD) namelen)
            {
                WSASetLastError (WSAEFAULT);
            }
        }
    }
    if (pResults!=localResults)
        delete pResults;
    return(ReturnValue);
}  // gethostname




struct servent FAR * WSAAPI
getservbyport(
    IN int port,
    IN const char FAR * proto
    )
/*++
Routine Description:

    Get service information corresponding to a port and protocol.

Arguments:

    port  - The port for a service, in network byte order.

    proto - An  optional  pointer  to  a  protocol  name.   If  this  is  NULL,
            getservbyport()  returns the first service entry for which the port
            matches  the  s_port.   Otherwise  getservbyport() matches both the
            port and the proto.

Returns:

    If  no  error  occurs,  getservbyport()  returns  a  pointer to the servent
    structure  described  above.   Otherwise  it  returns  a NULL pointer and a
    specific error code is stored with SetErrorCode().
--*/
{
    PCHAR pszTemp;
    struct servent * sent;
    INT  ErrorCode;
    PDPROCESS Process;
    PDTHREAD Thread;
    LPBLOB pBlob;
    PCHAR pResults;
    CHAR localResults[RNR_BUFFER_SIZE];


    ErrorCode = PROLOG(&Process,
                 &Thread);
    if(ErrorCode != ERROR_SUCCESS)
    {
        SetLastError(ErrorCode);
        return(NULL);
    }

    pResults = localResults;

    if(!proto)
    {
        proto = "";
    }

    //
    // the 5 is the max number of digits in a port
    //
    pszTemp = new CHAR[strlen(proto) + 1 + 1 + 5];
    if (pszTemp==NULL) {
        SetLastError(WSA_NOT_ENOUGH_MEMORY);
        return(NULL);
    }

    sprintf(pszTemp, "%d/%s", (port & 0xffff), proto);
    pBlob =  getxyDataEnt(&pResults, RNR_BUFFER_SIZE, pszTemp, &IANAGuid, 0);
    delete pszTemp;

    if(!pBlob)
    {
        sent = NULL;
        if(GetLastError() == WSATYPE_NOT_FOUND)
        {
            SetLastError(WSANO_DATA);
        }
    }
    else
    {
        sent = (struct servent *)Thread->CopyServEnt(pBlob);
        if(sent)
        {
            UnpackServEnt(sent);
        }
    }
    if (pResults!=localResults)
        delete pResults;
    return(sent);
}  // getservbyport




struct servent FAR * WSAAPI
getservbyname(
    IN const char FAR * name,
    IN const char FAR * proto
    )
/*++
Routine Description:

     Get service information corresponding to a service name and protocol.

Arguments:

     name  - A pointer to a null terminated service name.

     proto - An  optional  pointer to a null terminated protocol name.  If this
             pointer  is  NULL, getservbyname() returns the first service entry
             for  which  the  name  matches the s_name or one of the s_aliases.
             Otherwise getservbyname() matches both the name and the proto.

Returns:

     If  no  error  occurs,  getservbyname()  returns  a pointer to the servent
     structure  described  above.   Otherwise  it  returns a NULL pointer and a
     specific error code is stored with SetErrorCode().
--*/
{
    PCHAR pszTemp;
    struct servent * sent;
    INT ErrorCode;
    PDPROCESS Process;
    PDTHREAD Thread;
    LPBLOB pBlob;
    PCHAR pResults;
    CHAR localResults[RNR_BUFFER_SIZE];

    ErrorCode = PROLOG(&Process,
                 &Thread);
    if(ErrorCode != ERROR_SUCCESS)
    {
        SetLastError(ErrorCode);
        return(NULL);
    }

    if ( !name ) // Bug fix for #112969
    {
        SetLastError(WSAEINVAL);
        return(NULL);
    }

    pResults = localResults;

    if(!proto)
    {
        proto = "";
    }
    pszTemp = new CHAR[strlen(name) + strlen(proto) + 1 + 1];
    if (pszTemp==NULL) {
        SetLastError(WSA_NOT_ENOUGH_MEMORY);
        return(NULL);
    }
    sprintf(pszTemp, "%s/%s", name, proto);
    pBlob = getxyDataEnt(&pResults, RNR_BUFFER_SIZE, pszTemp, &IANAGuid, 0);
    delete pszTemp;
    if(!pBlob)
    {
        sent = NULL;
        if(GetLastError() == WSATYPE_NOT_FOUND)
        {
            SetLastError(WSANO_DATA);
        }
    }
    else
    {
        sent = (struct servent *)Thread->CopyServEnt(pBlob);
        if(sent)
        {
            UnpackServEnt(sent);
        }
    }
    if (pResults!=localResults)
        delete pResults;
    return(sent);
}  // getservbyname


//
// Common routine for obtaining a xxxent buffer. Input is used to
// execute the WSALookup series of APIs.
//
// Args:
//   pResults -- a pointer to a buffer supplied by the caller to be used in
//               the WASLookup calls. If the buffer is not large enough
//               this routine allocates a new one and modifies the value
//               in pResults.  The new buffer should be freed by the caller
//               using delete.
//   dwLength -- number of bytes in pResults (originally).
//   lpszName -- pointer to the service name. May by NULL
//   lpType   -- pointer to the service type . This should be one of
//               the SVCID_INET_xxxxx types. It may be anything
//               that produces a BLOB.
//   lppName  -- pointer to pointer where the resulting name pointer
//               is stored. May be NULL if the name is not needed.
//
// Returns:
//   0  --  No BLOB data was returned. In general, this means the operation
//          failed. Evev if the WSALookupNext succeeded and returned a
//          name, the name will not be returned.
//   else -- a pointer to the BLOB.
//
//


//
// The protocol restrictions list for all emulation operations. This should
// limit the invoked providers to the set that know about hostents and
// servents. If not, then the special SVCID_INET GUIDs should take care
// of the remainder.
//
AFPROTOCOLS afp[2] = {
                      {AF_INET, IPPROTO_UDP},
                      {AF_INET, IPPROTO_TCP}
                     };

LPBLOB
getxyDataEnt(
    IN OUT PCHAR *pResults,
    IN     DWORD dwLength,
    IN     LPSTR lpszName,
    IN     LPGUID lpType,
    OUT    LPSTR *  lppName OPTIONAL
    )
{

/*++
Routine Description:
   See comment above for details
--*/

    PWSAQUERYSETA pwsaq = (PWSAQUERYSETA)*pResults;
    int err;
    HANDLE hRnR;
    LPBLOB pvRet = 0;
    INT Err = 0;
    DWORD origLength = dwLength; // Safe the length of the original buffer
                                 // in case we need to reallocate it.

    if ( lppName )
    {
        *lppName = NULL;
    }

    //
    // create the query
    //
    memset(pwsaq, 0, sizeof(*pwsaq));
    pwsaq->dwSize = sizeof(*pwsaq);
    pwsaq->lpszServiceInstanceName = lpszName;
    pwsaq->lpServiceClassId = lpType;
    pwsaq->dwNameSpace = NS_ALL;
    pwsaq->dwNumberOfProtocols = 2;
    pwsaq->lpafpProtocols = &afp[0];

    err = WSALookupServiceBeginA(pwsaq,
                                 LUP_RETURN_BLOB | LUP_RETURN_NAME,
                                 &hRnR);

    if(err == NO_ERROR)
    {

        //
        // If the original buffer is small to contain the results
        // will allocate new one and retry the call.
        //
    Retry:

        //
        // The query was accepted, so execute it via the Next call.
        //
        err = WSALookupServiceNextA(
                                hRnR,
                                0,
                                &dwLength,
                                pwsaq);
        //
        // if NO_ERROR was returned and a BLOB is present, this
        // worked, just return the requested information. Otherwise,
        // invent an error or capture the transmitted one.
        //

        if(err == NO_ERROR)
        {
            if(pvRet = pwsaq->lpBlob)
            {
                if(lppName)
                {
                    *lppName = pwsaq->lpszServiceInstanceName;
                }
            }
            else
            {
                if ( lpType == &HostNameGuid )
                {
                    if(lppName)
                    {
                        *lppName = pwsaq->lpszServiceInstanceName;
                    }
                }
                else
                {
                    err = WSANO_DATA;
                }
            }
        }
        else
        {
            //
            // WSALookupServiceEnd clobbers LastError so save
            // it before closing the handle.
            //

            err = GetLastError();

            //
            // The provider returns WSAEFAULT if the result buffer
            // is not large enough (to make sure that this is not
            // just a random error or a result AV when accessing the
            // the buffer content, we check the returned buffer size
            // against the value that we initially supplied).
            //
            if ((err==WSAEFAULT) && (dwLength>origLength))
            {
                PCHAR   newBuffer = new CHAR[dwLength];
                if (newBuffer)
                {
                    //
                    // Remeber the new length, so that provider cannot
                    // force us to loop indefinitely (well if it keeps
                    // increasing required buffer size, we'll run into
                    // out of memory error sometimes).
                    // 
                    origLength = dwLength;

                    //
                    // Replace the callers pointer to the buffer, so
                    // it knows to free it
                    //
                    *pResults = newBuffer;

                    //
                    // Repoint results to new buffer.
                    //
                    pwsaq = (PWSAQUERYSETA)newBuffer;
                    
                    //
                    // Try the Next call again.
                    //
                    goto Retry;
                }
                else 
                {
                    err = WSA_NOT_ENOUGH_MEMORY;
                }
            }
        }
        WSALookupServiceEnd(hRnR);

        //
        // if an error happened, stash the value in LastError
        //

        if(err != NO_ERROR)
        {
            SetLastError(err);
        }
    }
    return(pvRet);
}



HANDLE
WSAAPI
WSAAsyncGetServByName(
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN const char FAR * Name,
    IN const char FAR * Protocol,
    IN char FAR * Buffer,
    IN int BufferLength
    )

/*++

Routine Description:

    This function is an asynchronous version of getservbyname(), and is
    used to retrieve service information corresponding to a service
    name.  The Windows Sockets implementation initiates the operation
    and returns to the caller immediately, passing back an asynchronous
    task handle which the application may use to identify the operation.
    When the operation is completed, the results (if any) are copied
    into the buffer provided by the caller and a message is sent to the
    application's window.

    When the asynchronous operation is complete the application's window
    hWnd receives message wMsg.  The wParam argument contains the
    asynchronous task handle as returned by the original function call.
    The high 16 bits of lParam contain any error code.  The error code
    may be any error as defined in winsock.h.  An error code of zero
    indicates successful completion of the asynchronous operation.  On
    successful completion, the buffer supplied to the original function
    call contains a hostent structure.  To access the elements of this
    structure, the original buffer address should be cast to a hostent
    structure pointer and accessed as appropriate.

    Note that if the error code is WSAENOBUFS, it indicates that the
    size of the buffer specified by buflen in the original call was too
    small to contain all the resultant information.  In this case, the
    low 16 bits of lParam contain the size of buffer required to supply
    ALL the requisite information.  If the application decides that the
    partial data is inadequate, it may reissue the
    WSAAsyncGetHostByAddr() function call with a buffer large enough to
    receive all the desired information (i.e.  no smaller than the low
    16 bits of lParam).

    The error code and buffer length should be extracted from the lParam
    using the macros WSAGETASYNCERROR and WSAGETASYNCBUFLEN, defined in
    winsock.h as:

        #define WSAGETASYNCERROR(lParam) HIWORD(lParam)
        #define WSAGETASYNCBUFLEN(lParam) LOWORD(lParam)

    The use of these macros will maximize the portability of the source
    code for the application.

    The buffer supplied to this function is used by the Windows Sockets
    implementation to construct a hostent structure together with the
    contents of data areas referenced by members of the same hostent
    structure.  To avoid the WSAENOBUFS error noted above, the
    application should provide a buffer of at least MAXGETHOSTSTRUCT
    bytes (as defined in winsock.h).

Arguments:

    hWnd - The handle of the window which should receive a message when
       the asynchronous request completes.

    wMsg - The message to be received when the asynchronous request
       completes.

    name - A pointer to a service name.

    proto - A pointer to a protocol name.  This may be NULL, in which
        case WSAAsyncGetServByName() will search for the first service
        entry for which s_name or one of the s_aliases matches the given
        name.  Otherwise WSAAsyncGetServByName() matches both name and
        proto.

    buf - A pointer to the data area to receive the servent data.  Note
       that this must be larger than the size of a servent structure.
       This is because the data area supplied is used by the Windows
       Sockets implementation to contain not only a servent structure
       but any and all of the data which is referenced by members of the
       servent structure.  It is recommended that you supply a buffer of
       MAXGETHOSTSTRUCT bytes.

    buflen    The size of data area buf above.

Return Value:

    The return value specifies whether or not the asynchronous operation
    was successfully initiated.  Note that it does not imply success or
    failure of the operation itself.

    If the operation was successfully initiated, WSAAsyncGetHostByAddr()
    returns a nonzero value of type HANDLE which is the asynchronous
    task handle for the request.  This value can be used in two ways.
    It can be used to cancel the operation using
    WSACancelAsyncRequest().  It can also be used to match up
    asynchronous operations and completion messages, by examining the
    wParam message argument.

    If the asynchronous operation could not be initiated,
    WSAAsyncGetHostByAddr() returns a zero value, and a specific error
    number may be retrieved by calling WSAGetLastError().

--*/

{

    PDPROCESS              Process;
    PDTHREAD               Thread;
    INT                    ErrorCode;
    PWINSOCK_CONTEXT_BLOCK contextBlock;
    HANDLE                 taskHandle;
    PCHAR                  localName;

    ErrorCode = PROLOG(&Process,
                         &Thread);

    if (ErrorCode!=ERROR_SUCCESS)
    {
        SetLastError(ErrorCode);
        return NULL;
    } //if

    //
    // Initialize the async thread if it hasn't already been started.
    //

    if( !SockCheckAndInitAsyncThread() ) {

        // !!! better error code?
        SetLastError( WSAENOBUFS );
        return NULL;

    }

    //
    // Get an async context block with enough extra space for
    // the name.  We must preserve the name until we're done using
    // it, since the application may reuse the buffer.
    //

    contextBlock = SockAllocateContextBlock( strlen(Name) + 1 );

    if( contextBlock == NULL ) {

        SetLastError( WSAENOBUFS );
        return NULL;

    }

    localName = (PCHAR)( contextBlock + 1 );

    strcpy( localName, Name );

    //
    // Initialize the context block for this operation.
    //

    contextBlock->OpCode = WS_OPCODE_GET_SERV_BY_NAME;
    contextBlock->Overlay.AsyncGetServ.hWnd = hWnd;
    contextBlock->Overlay.AsyncGetServ.wMsg = wMsg;
    contextBlock->Overlay.AsyncGetServ.Filter = localName;
    contextBlock->Overlay.AsyncGetServ.Protocol = (PCHAR)Protocol;;
    contextBlock->Overlay.AsyncGetServ.Buffer = Buffer;
    contextBlock->Overlay.AsyncGetServ.BufferLength = BufferLength;

    //
    // Save the task handle so that we can return it to the caller.
    // After we post the context block, we're not allowed to access
    // it in any way.
    //

    taskHandle = contextBlock->TaskHandle;

    //
    // Queue the request to the async thread.
    //

    SockQueueRequestToAsyncThread( contextBlock );

    return taskHandle;

}   // WSAAsyncGetServByName



HANDLE
WSAAPI
WSAAsyncGetServByPort(
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN int Port,
    IN const char FAR * Protocol,
    IN char FAR * Buffer,
    IN int BufferLength
    )

/*++

Routine Description:

    This function is an asynchronous version of getservbyport(), and is
    used to retrieve service information corresponding to a port number.
    The Windows Sockets implementation initiates the operation and
    returns to the caller immediately, passing back an asynchronous task
    handle which the application may use to identify the operation.
    When the operation is completed, the results (if any) are copied
    into the buffer provided by the caller and a message is sent to the
    application's window.

    When the asynchronous operation is complete the application's window
    hWnd receives message wMsg.  The wParam argument contains the
    asynchronous task handle as returned by the original function call.
    The high 16 bits of lParam contain any error code.  The error code
    may be any error as defined in winsock.h.  An error code of zero
    indicates successful completion of the asynchronous operation.  On
    successful completion, the buffer supplied to the original function
    call contains a servent structure.  To access the elements of this
    structure, the original buffer address should be cast to a servent
    structure pointer and accessed as appropriate.

    Note that if the error code is WSAENOBUFS, it indicates that the
    size of the buffer specified by buflen in the original call was too
    small to contain all the resultant information.  In this case, the
    low 16 bits of lParam contain the size of buffer required to supply
    ALL the requisite information.  If the application decides that the
    partial data is inadequate, it may reissue the
    WSAAsyncGetServByPort() function call with a buffer large enough to
    receive all the desired information (i.e.  no smaller than the low
    16 bits of lParam).

    The error code and buffer length should be extracted from the lParam
    using the macros WSAGETASYNCERROR and WSAGETASYNCBUFLEN, defined in
    winsock.h as:

        #define WSAGETASYNCERROR(lParam) HIWORD(lParam)
        #define WSAGETASYNCBUFLEN(lParam) LOWORD(lParam)

    The use of these macros will maximize the portability of the source
    code for the application.


    The buffer supplied to this function is used by the Windows Sockets
    implementation to construct a servent structure together with the
    contents of data areas referenced by members of the same servent
    structure.  To avoid the WSAENOBUFS error noted above, the
    application should provide a buffer of at least MAXGETHOSTSTRUCT
    bytes (as defined in winsock.h).

Arguments:

    hWnd - The handle of the window which should receive a message when
       the asynchronous request completes.

    wMsg - The message to be received when the asynchronous request
       completes.

    port - The port for the service, in network byte order.

    proto - A pointer to a protocol name.  This may be NULL, in which
        case WSAAsyncGetServByPort() will search for the first service
        entry for which s_port match the given port.  Otherwise
        WSAAsyncGetServByPort() matches both port and proto.

    buf - A pointer to the data area to receive the servent data.  Note
       that this must be larger than the size of a servent structure.
       This is because the data area supplied is used by the Windows
       Sockets implementation to contain not only a servent structure
       but any and all of the data which is referenced by members of the
       servent structure.  It is recommended that you supply a buffer of
       MAXGETHOSTSTRUCT bytes.

    buflen    The size of data area buf above.

Return Value:

    The return value specifies whether or not the asynchronous operation
    was successfully initiated.  Note that it does not imply success or
    failure of the operation itself.

    If the operation was successfully initiated, WSAAsyncGetServByPort()
    returns a nonzero value of type HANDLE which is the asynchronous
    task handle for the request.  This value can be used in two ways.
    It can be used to cancel the operation using
    WSACancelAsyncRequest().  It can also be used to match up
    asynchronous operations and completion messages, by examining the
    wParam message argument.

    If the asynchronous operation could not be initiated,
    WSAAsyncGetServByPort() returns a zero value, and a specific error
    number may be retrieved by calling WSAGetLastError().

--*/

{

    PDPROCESS              Process;
    PDTHREAD               Thread;
    INT                    ErrorCode;
    PWINSOCK_CONTEXT_BLOCK contextBlock;
    HANDLE                 taskHandle;

    ErrorCode = PROLOG(&Process,
                         &Thread);

    if (ErrorCode!=ERROR_SUCCESS)
    {
        SetLastError(ErrorCode);
        return NULL;
    } //if

    //
    // Initialize the async thread if it hasn't already been started.
    //

    if( !SockCheckAndInitAsyncThread() ) {

        // !!! better error code?
        SetLastError( WSAENOBUFS );
        return NULL;

    }

    //
    // Get an async context block.
    //

    contextBlock = SockAllocateContextBlock( 0 );

    if ( contextBlock == NULL ) {

        SetLastError( WSAENOBUFS );
        return NULL;

    }

    //
    // Initialize the context block for this operation.
    //

    contextBlock->OpCode = WS_OPCODE_GET_SERV_BY_PORT;
    contextBlock->Overlay.AsyncGetServ.hWnd = hWnd;
    contextBlock->Overlay.AsyncGetServ.wMsg = wMsg;
    contextBlock->Overlay.AsyncGetServ.Filter = (PCHAR)Port;
    contextBlock->Overlay.AsyncGetServ.Protocol = (PCHAR)Protocol;
    contextBlock->Overlay.AsyncGetServ.Buffer = Buffer;
    contextBlock->Overlay.AsyncGetServ.BufferLength = BufferLength;

    //
    // Save the task handle so that we can return it to the caller.
    // After we post the context block, we're not allowed to access
    // it in any way.
    //

    taskHandle = contextBlock->TaskHandle;

    //
    // Queue the request to the async thread.
    //

    SockQueueRequestToAsyncThread( contextBlock );

    return taskHandle;

}   // WSAAsyncGetServByPort



HANDLE
WSAAPI
WSAAsyncGetHostByName(
    HWND hWnd,
    unsigned int wMsg,
    const char FAR * Name,
    char FAR * Buffer,
    int BufferLength
    )

/*++

Routine Description:

    This function is an asynchronous version of gethostbyname(), and is
    used to retrieve host name and address information corresponding to
    a hostname.  The Windows Sockets implementation initiates the
    operation and returns to the caller immediately, passing back an
    asynchronous task handle which the application may use to identify
    the operation.  When the operation is completed, the results (if
    any) are copied into the buffer provided by the caller and a message
    is sent to the application's window.

    When the asynchronous operation is complete the application's window
    hWnd receives message wMsg.  The wParam argument contains the
    asynchronous task handle as returned by the original function call.
    The high 16 bits of lParam contain any error code.  The error code
    may be any error as defined in winsock.h.  An error code of zero
    indicates successful completion of the asynchronous operation.  On
    successful completion, the buffer supplied to the original function
    call contains a hostent structure.  To access the elements of this
    structure, the original buffer address should be cast to a hostent
    structure pointer and accessed as appropriate.

    Note that if the error code is WSAENOBUFS, it indicates that the
    size of the buffer specified by buflen in the original call was too
    small to contain all the resultant information.  In this case, the
    low 16 bits of lParam contain the size of buffer required to supply
    ALL the requisite information.  If the application decides that the
    partial data is inadequate, it may reissue the
    WSAAsyncGetHostByName() function call with a buffer large enough to
    receive all the desired information (i.e.  no smaller than the low
    16 bits of lParam).

    The error code and buffer length should be extracted from the lParam
    using the macros WSAGETASYNCERROR and WSAGETASYNCBUFLEN, defined in
    winsock.h as:

        #define WSAGETASYNCERROR(lParam) HIWORD(lParam)
        #define WSAGETASYNCBUFLEN(lParam) LOWORD(lParam)

    The use of these macros will maximize the portability of the source
    code for the application.

    The buffer supplied to this function is used by the Windows Sockets
    implementation to construct a hostent structure together with the
    contents of data areas referenced by members of the same hostent
    structure.  To avoid the WSAENOBUFS error noted above, the
    application should provide a buffer of at least MAXGETHOSTSTRUCT
    bytes (as defined in winsock.h).

Arguments:

    hWnd - The handle of the window which should receive a message when
       the asynchronous request completes.

    wMsg - The message to be received when the asynchronous request
       completes.

    name - A pointer to the name of the host.

    buf - A pointer to the data area to receive the hostent data.  Note
       that this must be larger than the size of a hostent structure.
       This is because the data area supplied is used by the Windows
       Sockets implementation to contain not only a hostent structure
       but any and all of the data which is referenced by members of the
       hostent structure.  It is recommended that you supply a buffer of
       MAXGETHOSTSTRUCT bytes.

    buflen - The size of data area buf above.

Return Value:

    The return value specifies whether or not the asynchronous operation
    was successfully initiated.  Note that it does not imply success or
    failure of the operation itself.

    If the operation was successfully initiated, WSAAsyncGetHostByName()
    returns a nonzero value of type HANDLE which is the asynchronous
    task handle for the request.  This value can be used in two ways.
    It can be used to cancel the operation using
    WSACancelAsyncRequest().  It can also be used to match up
    asynchronous operations and completion messages, by examining the
    wParam message argument.

    If the asynchronous operation could not be initiated,
    WSAAsyncGetHostByName() returns a zero value, and a specific error
    number may be retrieved by calling WSAGetLastError().

--*/

{

    PDPROCESS              Process;
    PDTHREAD               Thread;
    INT                    ErrorCode;
    PWINSOCK_CONTEXT_BLOCK contextBlock;
    HANDLE                 taskHandle;
    PCHAR                  localName;

    ErrorCode = PROLOG(&Process,
                         &Thread);

    if (ErrorCode!=ERROR_SUCCESS)
    {
        SetLastError(ErrorCode);
        return NULL;
    } //if

    //
    // Initialize the async thread if it hasn't already been started.
    //

    if( !SockCheckAndInitAsyncThread() ) {

        // !!! better error code?
        SetLastError( WSAENOBUFS );
        return NULL;

    }

    //
    // Get an async context block with enough extra space for
    // the name.  We must preserve the name until we're done using
    // it, since the application may reuse the buffer.
    //

    contextBlock = SockAllocateContextBlock( strlen(Name) + 1 );

    if( contextBlock == NULL ) {

        SetLastError( WSAENOBUFS );
        return NULL;

    }

    localName = (PCHAR)( contextBlock + 1 );

    strcpy( localName, Name );

    //
    // Initialize the context block for this operation.
    //

    contextBlock->OpCode = WS_OPCODE_GET_HOST_BY_NAME;
    contextBlock->Overlay.AsyncGetHost.hWnd = hWnd;
    contextBlock->Overlay.AsyncGetHost.wMsg = wMsg;
    contextBlock->Overlay.AsyncGetHost.Filter = localName;
    contextBlock->Overlay.AsyncGetHost.Buffer = Buffer;
    contextBlock->Overlay.AsyncGetHost.BufferLength = BufferLength;

    //
    // Save the task handle so that we can return it to the caller.
    // After we post the context block, we're not allowed to access
    // it in any way.
    //

    taskHandle = contextBlock->TaskHandle;

    //
    // Queue the request to the async thread.
    //

    SockQueueRequestToAsyncThread( contextBlock );

    return taskHandle;

}   // WSAAsyncGetHostByName


HANDLE
WSAAPI
WSAAsyncGetHostByAddr(
    HWND hWnd,
    unsigned int wMsg,
    const char FAR * Address,
    int Length,
    int Type,
    char FAR * Buffer,
    int BufferLength
    )

/*++

Routine Description:

    This function is an asynchronous version of gethostbyaddr(), and is
    used to retrieve host name and address information corresponding to
    a network address.  The Windows Sockets implementation initiates the
    operation and returns to the caller immediately, passing back an
    asynchronous task handle which the application may use to identify
    the operation.  When the operation is completed, the results (if
    any) are copied into the buffer provided by the caller and a message
    is sent to the application's window.

    When the asynchronous operation is complete the application's window
    hWnd receives message wMsg.  The wParam argument contains the
    asynchronous task handle as returned by the original function call.
    The high 16 bits of lParam contain any error code.  The error code
    may be any error as defined in winsock.h.  An error code of zero
    indicates successful completion of the asynchronous operation.  On
    successful completion, the buffer supplied to the original function
    call contains a hostent structure.  To access the elements of this
    structure, the original buffer address should be cast to a hostent
    structure pointer and accessed as appropriate.

    Note that if the error code is WSAENOBUFS, it indicates that the
    size of the buffer specified by buflen in the original call was too
    small to contain all the resultant information.  In this case, the
    low 16 bits of lParam contain the size of buffer required to supply
    ALL the requisite information.  If the application decides that the
    partial data is inadequate, it may reissue the
    WSAAsyncGetHostByAddr() function call with a buffer large enough to
    receive all the desired information (i.e.  no smaller than the low
    16 bits of lParam).

    The error code and buffer length should be extracted from the lParam
    using the macros WSAGETASYNCERROR and WSAGETASYNCBUFLEN, defined in
    winsock.h as:

        #define WSAGETASYNCERROR(lParam) HIWORD(lParam)
        #define WSAGETASYNCBUFLEN(lParam) LOWORD(lParam)

    The use of these macros will maximize the portability of the source
    code for the application.

    The buffer supplied to this function is used by the Windows Sockets
    implementation to construct a hostent structure together with the
    contents of data areas referenced by members of the same hostent
    structure.  To avoid the WSAENOBUFS error noted above, the
    application should provide a buffer of at least MAXGETHOSTSTRUCT
    bytes (as defined in winsock.h).

Arguments:

    hWnd - The handle of the window which should receive a message when
       the asynchronous request completes.

    wMsg - The message to be received when the asynchronous request
       completes.

    addr - A pointer to the network address for the host.  Host
       addresses are stored in network byte order.

    len - The length of the address, which must be 4 for PF_INET.

    type - The type of the address, which must be PF_INET.

    buf - A pointer to the data area to receive the hostent data.  Note
       that this must be larger than the size of a hostent structure.
       This is because the data area supplied is used by the Windows
       Sockets implementation to contain not only a hostent structure
       but any and all of the data which is referenced by members of the
       hostent structure.  It is recommended that you supply a buffer of
       MAXGETHOSTSTRUCT bytes.

    buflen - The size of data area buf above.

Return Value:

    The return value specifies whether or not the asynchronous operation
    was successfully initiated.  Note that it does not imply success or
    failure of the operation itself.

    If the operation was successfully initiated, WSAAsyncGetHostByAddr()
    returns a nonzero value of type HANDLE which is the asynchronous
    task handle for the request.  This value can be used in two ways.
    It can be used to cancel the operation using
    WSACancelAsyncRequest().  It can also be used to match up
    asynchronous operations and completion messages, by examining the
    wParam message argument.

    If the asynchronous operation could not be initiated,
    WSAAsyncGetHostByAddr() returns a zero value, and a specific error
    number may be retrieved by calling WSAGetLastError().

--*/

{

    PDPROCESS              Process;
    PDTHREAD               Thread;
    INT                    ErrorCode;
    PWINSOCK_CONTEXT_BLOCK contextBlock;
    HANDLE                 taskHandle;
    PCHAR                  localAddress;

    ErrorCode = PROLOG(&Process,
                         &Thread);

    if (ErrorCode!=ERROR_SUCCESS)
    {
        SetLastError(ErrorCode);
        return NULL;
    } //if

    //
    // Initialize the async thread if it hasn't already been started.
    //

    if( !SockCheckAndInitAsyncThread() ) {

        // !!! better error code?
        SetLastError( WSAENOBUFS );
        return NULL;

    }

    //
    // Get an async context block with enough extra space for
    // the address.  We must preserve the address until we're done
    // using it, since the application may reuse the buffer.
    //

    contextBlock = SockAllocateContextBlock( Length );

    if( contextBlock == NULL ) {

        SetLastError( WSAENOBUFS );
        return NULL;

    }

    localAddress = (PCHAR)( contextBlock + 1 );

    CopyMemory(
        localAddress,
        Address,
        Length
        );

    //
    // Initialize the context block for this operation.
    //

    contextBlock->OpCode = WS_OPCODE_GET_HOST_BY_ADDR;
    contextBlock->Overlay.AsyncGetHost.hWnd = hWnd;
    contextBlock->Overlay.AsyncGetHost.wMsg = wMsg;
    contextBlock->Overlay.AsyncGetHost.Filter = localAddress;
    contextBlock->Overlay.AsyncGetHost.Length = Length;
    contextBlock->Overlay.AsyncGetHost.Type = Type;
    contextBlock->Overlay.AsyncGetHost.Buffer = Buffer;
    contextBlock->Overlay.AsyncGetHost.BufferLength = BufferLength;

    //
    // Save the task handle so that we can return it to the caller.
    // After we post the context block, we're not allowed to access
    // it in any way.
    //

    taskHandle = contextBlock->TaskHandle;

    //
    // Queue the request to the async thread.
    //

    SockQueueRequestToAsyncThread( contextBlock );

    return taskHandle;

}   // WSAAsyncGetHostByAddr


HANDLE
WSAAPI
WSAAsyncGetProtoByName (
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN const char FAR *Name,
    IN char FAR *Buffer,
    IN int BufferLength
    )

/*++

Routine Description:

    This function is an asynchronous version of getprotobyname(), and is
    used to retrieve the protocol name and number corresponding to a
    protocol name.  The Windows Sockets implementation initiates the
    operation and returns to the caller immediately, passing back an
    asynchronous task handle which the application may use to identify
    the operation.  When the operation is completed, the results (if
    any) are copied into the buffer provided by the caller and a message
    is sent to the application's window.

    When the asynchronous operation is complete the application's window
    hWnd receives message wMsg.  The wParam argument contains the
    asynchronous task handle as returned by the original function call.
    The high 16 bits of lParam contain any error code.  The error code
    may be any error as defined in winsock.h.  An error code of zero
    indicates successful completion of the asynchronous operation.  On
    successful completion, the buffer supplied to the original function
    call contains a protoent structure.  To access the elements of this
    structure, the original buffer address should be cast to a protoent
    structure pointer and accessed as appropriate.

    Note that if the error code is WSAENOBUFS, it indicates that the
    size of the buffer specified by buflen in the original call was too
    small to contain all the resultant information.  In this case, the
    low 16 bits of lParam contain the size of buffer required to supply
    ALL the requisite information.  If the application decides that the
    partial data is inadequate, it may reissue the
    WSAAsyncGetProtoByName() function call with a buffer large enough to
    receive all the desired information (i.e.  no smaller than the low
    16 bits of lParam).

    The error code and buffer length should be extracted from the lParam
    using the macros WSAGETASYNCERROR and WSAGETASYNCBUFLEN, defined in
    winsock.h as:

        #define WSAGETASYNCERROR(lParam) HIWORD(lParam)
        #define WSAGETASYNCBUFLEN(lParam) LOWORD(lParam)

    The use of these macros will maximize the portability of the source
    code for the application.

    The buffer supplied to this function is used by the Windows Sockets
    implementation to construct a protoent structure together with the
    contents of data areas referenced by members of the same protoent
    structure.  To avoid the WSAENOBUFS error noted above, the
    application should provide a buffer of at least MAXGETHOSTSTRUCT
    bytes (as defined in winsock.h).

Arguments:

    hWnd - The handle of the window which should receive a message when
       the asynchronous request completes.

    wMsg - The message to be received when the asynchronous request
       completes.

    name - A pointer to the protocol name to be resolved.

    buf - A pointer to the data area to receive the protoent data.  Note
       that this must be larger than the size of a protoent structure.
       This is because the data area supplied is used by the Windows
       Sockets implementation to contain not only a protoent structure
       but any and all of the data which is referenced by members of the
       protoent structure.  It is recommended that you supply a buffer
       of MAXGETHOSTSTRUCT bytes.

    buflen - The size of data area buf above.

Return Value:

    The return value specifies whether or not the asynchronous operation
    was successfully initiated.  Note that it does not imply success or
    failure of the operation itself.

    If the operation was successfully initiated,
    WSAAsyncGetProtoByName() returns a nonzero value of type HANDLE
    which is the asynchronous task handle for the request.  This value
    can be used in two ways.  It can be used to cancel the operation
    using WSACancelAsyncRequest().  It can also be used to match up
    asynchronous operations and completion messages, by examining the
    wParam message argument.

    If the asynchronous operation could not be initiated,
    WSAAsyncGetProtoByName() returns a zero value, and a specific error
    number may be retrieved by calling WSAGetLastError().

--*/

{

    PDPROCESS              Process;
    PDTHREAD               Thread;
    INT                    ErrorCode;
    PWINSOCK_CONTEXT_BLOCK contextBlock;
    HANDLE                 taskHandle;
    PCHAR                  localName;

    ErrorCode = PROLOG(&Process,
                         &Thread);

    if (ErrorCode!=ERROR_SUCCESS)
    {
        SetLastError(ErrorCode);
        return NULL;
    } //if

    //
    // Initialize the async thread if it hasn't already been started.
    //

    if( !SockCheckAndInitAsyncThread() ) {

        // !!! better error code?
        SetLastError( WSAENOBUFS );
        return NULL;

    }

    //
    // Get an async context block with enough extra space for
    // the name.  We must preserve the name until we're done using
    // it, since the application may reuse the buffer.
    //

    contextBlock = SockAllocateContextBlock( strlen(Name) + 1 );

    if( contextBlock == NULL ) {

        SetLastError( WSAENOBUFS );
        return NULL;

    }

    localName = (PCHAR)( contextBlock + 1 );

    strcpy( localName, Name );

    //
    // Initialize the context block for this operation.
    //

    contextBlock->OpCode = WS_OPCODE_GET_PROTO_BY_NAME;
    contextBlock->Overlay.AsyncGetProto.hWnd = hWnd;
    contextBlock->Overlay.AsyncGetProto.wMsg = wMsg;
    contextBlock->Overlay.AsyncGetProto.Filter = localName;
    contextBlock->Overlay.AsyncGetProto.Buffer = Buffer;
    contextBlock->Overlay.AsyncGetProto.BufferLength = BufferLength;

    //
    // Save the task handle so that we can return it to the caller.
    // After we post the context block, we're not allowed to access
    // it in any way.
    //

    taskHandle = contextBlock->TaskHandle;

    //
    // Queue the request to the async thread.
    //

    SockQueueRequestToAsyncThread( contextBlock );

    return taskHandle;

}   // WSAAsyncGetProtoByName


HANDLE
WSAAPI
WSAAsyncGetProtoByNumber (
    HWND hWnd,
    unsigned int wMsg,
    int Number,
    char FAR * Buffer,
    int BufferLength
    )

/*++

Routine Description:

    This function is an asynchronous version of getprotobynumber(), and
    is used to retrieve the protocol name and number corresponding to a
    protocol number.  The Windows Sockets implementation initiates the
    operation and returns to the caller immediately, passing back an
    asynchronous task handle which the application may use to identify
    the operation.  When the operation is completed, the results (if
    any) are copied into the buffer provided by the caller and a message
    is sent to the application's window.

    When the asynchronous operation is complete the application's window
    hWnd receives message wMsg.  The wParam argument contains the
    asynchronous task handle as returned by the original function call.
    The high 16 bits of lParam contain any error code.  The error code
    may be any error as defined in winsock.h.  An error code of zero
    indicates successful completion of the asynchronous operation.  On
    successful completion, the buffer supplied to the original function
    call contains a protoent structure.  To access the elements of this
    structure, the original buffer address should be cast to a protoent
    structure pointer and accessed as appropriate.

    Note that if the error code is WSAENOBUFS, it indicates that the
    size of the buffer specified by buflen in the original call was too
    small to contain all the resultant information.  In this case, the
    low 16 bits of lParam contain the size of buffer required to supply
    ALL the requisite information.  If the application decides that the
    partial data is inadequate, it may reissue the
    WSAAsyncGetProtoByNumber() function call with a buffer large enough
    to receive all the desired information (i.e.  no smaller than the
    low 16 bits of lParam).

    The error code and buffer length should be extracted from the lParam
    using the macros WSAGETASYNCERROR and WSAGETASYNCBUFLEN, defined in
    winsock.h as:

        #define WSAGETASYNCERROR(lParam) HIWORD(lParam)
        #define WSAGETASYNCBUFLEN(lParam) LOWORD(lParam)

    The use of these macros will maximize the portability of the source
    code for the application.

    The buffer supplied to this function is used by the Windows Sockets
    implementation to construct a protoent structure together with the
    contents of data areas referenced by members of the same protoent
    structure.  To avoid the WSAENOBUFS error noted above, the
    application should provide a buffer of at least MAXGETHOSTSTRUCT
    bytes (as defined in winsock.h).

Arguments:

    hWnd - The handle of the window which should receive a message when
       the asynchronous request completes.

    wMsg - The message to be received when the asynchronous request
       completes.

    number - The protocol number to be resolved, in host byte order.

    buf - A pointer to the data area to receive the protoent data.  Note
       that this must be larger than the size of a protoent structure.
       This is because the data area supplied is used by the Windows
       Sockets implementation to contain not only a protoent structure
       but any and all of the data which is referenced by members of the
       protoent structure.  It is recommended that you supply a buffer
       of MAXGETHOSTSTRUCT bytes.

    buflen - The size of data area buf above.

Return Value:

    The return value specifies whether or not the asynchronous operation
    was successfully initiated.  Note that it does not imply success or
    failure of the operation itself.

    If the operation was successfully initiated,
    WSAAsyncGetProtoByNumber() returns a nonzero value of type HANDLE
    which is the asynchronous task handle for the request.  This value
    can be used in two ways.  It can be used to cancel the operation
    using WSACancelAsyncRequest().  It can also be used to match up
    asynchronous operations and completion messages, by examining the
    wParam message argument.

    If the asynchronous operation could not be initiated,
    WSAAsyncGetProtoByNumber() returns a zero value, and a specific
    error number may be retrieved by calling WSAGetLastError().

--*/

{

    PDPROCESS              Process;
    PDTHREAD               Thread;
    INT                    ErrorCode;
    PWINSOCK_CONTEXT_BLOCK contextBlock;
    HANDLE                 taskHandle;

    ErrorCode = PROLOG(&Process,
                         &Thread);

    if (ErrorCode!=ERROR_SUCCESS)
    {
        SetLastError(ErrorCode);
        return NULL;
    } //if

    //
    // Initialize the async thread if it hasn't already been started.
    //

    if( !SockCheckAndInitAsyncThread() ) {

        // !!! better error code?
        SetLastError( WSAENOBUFS );
        return NULL;

    }

    //
    // Get an async context block.
    //

    contextBlock = SockAllocateContextBlock( 0 );

    if( contextBlock == NULL ) {

        SetLastError( WSAENOBUFS );
        return NULL;

    }

    //
    // Initialize the context block for this operation.
    //

    contextBlock->OpCode = WS_OPCODE_GET_PROTO_BY_NUMBER;
    contextBlock->Overlay.AsyncGetProto.hWnd = hWnd;
    contextBlock->Overlay.AsyncGetProto.wMsg = wMsg;
    contextBlock->Overlay.AsyncGetProto.Filter = (PCHAR)Number;
    contextBlock->Overlay.AsyncGetProto.Buffer = Buffer;
    contextBlock->Overlay.AsyncGetProto.BufferLength = BufferLength;

    //
    // Save the task handle so that we can return it to the caller.
    // After we post the context block, we're not allowed to access
    // it in any way.
    //

    taskHandle = contextBlock->TaskHandle;

    //
    // Queue the request to the async thread.
    //

    SockQueueRequestToAsyncThread( contextBlock );

    return taskHandle;

}   // WSAAsyncGetProtoByNumber



int
WSAAPI
WSACancelAsyncRequest (
    HANDLE hAsyncTaskHandle
    )

/*++

Routine Description:

    The WSACancelAsyncRequest() function is used to cancel an
    asynchronous operation which was initiated by one of the
    WSAAsyncGetXByY() functions such as WSAAsyncGetHostByName().  The
    operation to be canceled is identified by the hAsyncTaskHandle
    parameter, which should be set to the asynchronous task handle as
    returned by the initiating function.

Arguments:

    hAsyncTaskHandle - Specifies the asynchronous operation to be
        canceled.

Return Value:

    The value returned by WSACancelAsyncRequest() is 0 if the operation
    was successfully canceled.  Otherwise the value SOCKET_ERROR is
    returned, and a specific error number may be retrieved by calling
    WSAGetLastError().

--*/

{

    PDPROCESS Process;
    PDTHREAD  Thread;
    INT       ErrorCode;

    ErrorCode = PROLOG(&Process,
                         &Thread);

    if (ErrorCode!=ERROR_SUCCESS)
    {
        SetLastError(ErrorCode);
        return SOCKET_ERROR;
    } //if

    //
    // Let the async code do the grunt work.
    //

    ErrorCode = SockCancelAsyncRequest( hAsyncTaskHandle );

    if (ErrorCode == ERROR_SUCCESS) {
        return (ERROR_SUCCESS);
    }
    else {
        SetLastError(ErrorCode);
        return (SOCKET_ERROR);
    }

}   // WSACancelAsyncRequest
