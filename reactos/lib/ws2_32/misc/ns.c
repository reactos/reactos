/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        misc/ns.c
 * PURPOSE:     Namespace APIs
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#define __NO_CTYPE_INLINES
#include <ctype.h>
#include <ws2_32.h>

#ifndef BUFSIZ
#define BUFSIZ 1024
#endif/*BUFSIZ*/

/* Name resolution APIs */

/*
 * @unimplemented
 */
INT
EXPORT
WSAAddressToStringA(
    IN      LPSOCKADDR lpsaAddress,
    IN      DWORD dwAddressLength,
    IN      LPWSAPROTOCOL_INFOA lpProtocolInfo,
    OUT     LPSTR lpszAddressString,
    IN OUT  LPDWORD lpdwAddressStringLength)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAAddressToStringW(
    IN      LPSOCKADDR lpsaAddress,
    IN      DWORD dwAddressLength,
    IN      LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT     LPWSTR lpszAddressString,
    IN OUT  LPDWORD lpdwAddressStringLength)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAEnumNameSpaceProvidersA(
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSANAMESPACE_INFOA lpnspBuffer)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAEnumNameSpaceProvidersW(
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSANAMESPACE_INFOW lpnspBuffer)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAGetServiceClassInfoA(
    IN      LPGUID lpProviderId,
    IN      LPGUID lpServiceClassId,
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSASERVICECLASSINFOA lpServiceClassInfo)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAGetServiceClassInfoW(
    IN      LPGUID lpProviderId,
    IN      LPGUID lpServiceClassId,
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSASERVICECLASSINFOW lpServiceClassInfo)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAGetServiceClassNameByClassIdA(
    IN      LPGUID lpServiceClassId,
    OUT     LPSTR lpszServiceClassName,
    IN OUT  LPDWORD lpdwBufferLength)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAGetServiceClassNameByClassIdW(
    IN      LPGUID lpServiceClassId,
    OUT     LPWSTR lpszServiceClassName,
    IN OUT  LPDWORD lpdwBufferLength)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAInstallServiceClassA(
    IN  LPWSASERVICECLASSINFOA lpServiceClassInfo)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAInstallServiceClassW(
    IN  LPWSASERVICECLASSINFOW lpServiceClassInfo)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSALookupServiceBeginA(
    IN  LPWSAQUERYSETA lpqsRestrictions,
    IN  DWORD dwControlFlags,
    OUT LPHANDLE lphLookup)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSALookupServiceBeginW(
    IN  LPWSAQUERYSETW lpqsRestrictions,
    IN  DWORD dwControlFlags,
    OUT LPHANDLE lphLookup)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSALookupServiceEnd(
    IN  HANDLE hLookup)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSALookupServiceNextA(
    IN      HANDLE hLookup,
    IN      DWORD dwControlFlags,
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSAQUERYSETA lpqsResults)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSALookupServiceNextW(
    IN      HANDLE hLookup,
    IN      DWORD dwControlFlags,
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSAQUERYSETW lpqsResults)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSARemoveServiceClass(
    IN  LPGUID lpServiceClassId)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSASetServiceA(
    IN  LPWSAQUERYSETA lpqsRegInfo,
    IN  WSAESETSERVICEOP essOperation,
    IN  DWORD dwControlFlags)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSASetServiceW(
    IN  LPWSAQUERYSETW lpqsRegInfo,
    IN  WSAESETSERVICEOP essOperation,
    IN  DWORD dwControlFlags)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAStringToAddressA(
    IN      LPSTR AddressString,
    IN      INT AddressFamily,
    IN      LPWSAPROTOCOL_INFOA lpProtocolInfo,
    OUT     LPSOCKADDR lpAddress,
    IN OUT  LPINT lpAddressLength)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAStringToAddressW(
    IN      LPWSTR AddressString,
    IN      INT AddressFamily,
    IN      LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT     LPSOCKADDR lpAddress,
    IN OUT  LPINT lpAddressLength)
{
    UNIMPLEMENTED

    return 0;
}


/* WinSock 1.1 compatible name resolution APIs */

/*
 * @unimplemented
 */
LPHOSTENT
EXPORT
gethostbyaddr(
    IN  CONST CHAR FAR* addr,
    IN  INT len,
    IN  INT type)
{
    UNIMPLEMENTED

    return (LPHOSTENT)NULL;
}

/*
 * @unimplemented
 */
LPHOSTENT
EXPORT
gethostbyname(
    IN  CONST CHAR FAR* name)
{
    UNIMPLEMENTED

    return (LPHOSTENT)NULL;
}


/*
 * @unimplemented
 */
INT
EXPORT
gethostname(
    OUT CHAR FAR* name,
    IN  INT namelen)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * XXX arty -- Partial implementation pending a better one.  This one will
 * do for normal purposes.
 *
 * Return the address of a static LPPROTOENT corresponding to the named
 * protocol.  These structs aren't very interesting, so I'm not too ashamed
 * to have this function work on builtins for now.
 *
 * @unimplemented
 */
LPPROTOENT
EXPORT
getprotobyname(
    IN  CONST CHAR FAR* name)
{
    static CHAR *udp_aliases = 0;
    static PROTOENT udp = { "udp", &udp_aliases, 17 };
    static CHAR *tcp_aliases = 0;
    static PROTOENT tcp = { "tcp", &tcp_aliases, 6 };
    if( !_stricmp( name, "udp" ) ) {
	return &udp;
    } else if( !_stricmp( name, "tcp" ) ) {
	return &tcp;
    }
    
    return 0;
}

/*
 * @unimplemented
 */
LPPROTOENT
EXPORT
getprotobynumber(
    IN  INT number)
{
    UNIMPLEMENTED

    return (LPPROTOENT)NULL;
}

#define SKIPWS(ptr,act) \
{while(*ptr && isspace(*ptr)) ptr++; if(!*ptr) act;}
#define SKIPANDMARKSTR(ptr,act) \
{while(*ptr && !isspace(*ptr)) ptr++; \
 if(!*ptr) {act;} else { *ptr = 0; ptr++; }}
 

static BOOL DecodeServEntFromString( IN  PCHAR ServiceString,
				     OUT PCHAR *ServiceName,
				     OUT PCHAR *PortNumberStr,
				     OUT PCHAR *ProtocolStr,
				     IN  PCHAR *Aliases,
				     IN  DWORD MaxAlias ) {
    UINT NAliases = 0;

    WS_DbgPrint(MAX_TRACE, ("Parsing service ent [%s]\n", ServiceString));

    SKIPWS(ServiceString, return FALSE);
    *ServiceName = ServiceString;
    SKIPANDMARKSTR(ServiceString, return FALSE);
    SKIPWS(ServiceString, return FALSE);
    *PortNumberStr = ServiceString;
    SKIPANDMARKSTR(ServiceString, ;);

    while( *ServiceString && NAliases < MaxAlias - 1 ) {
	SKIPWS(ServiceString, break);
	if( *ServiceString ) {
	    SKIPANDMARKSTR(ServiceString, ;);
	    if( strlen(ServiceString) ) {
		WS_DbgPrint(MAX_TRACE, ("Alias: %s\n", ServiceString));
		*Aliases++ = ServiceString;
		NAliases++;
	    }
	}
    }
    *Aliases = NULL;

    *ProtocolStr = strchr(*PortNumberStr,'/');
    if( !*ProtocolStr ) return FALSE;
    **ProtocolStr = 0; (*ProtocolStr)++;

    WS_DbgPrint(MAX_TRACE, ("Parsing done: %s %s %s %d\n",
			    *ServiceName, *ProtocolStr, *PortNumberStr,
			    NAliases));

    return TRUE;
}

#define ADJ_PTR(p,b1,b2) p = (p - b1) + b2

/*
 * @implemented
 */
LPSERVENT
EXPORT
getservbyname(
    IN  CONST CHAR FAR* name, 
    IN  CONST CHAR FAR* proto)
{
    BOOL  Found = FALSE;
    HANDLE ServicesFile;
    CHAR ServiceDBData[BUFSIZ] = { 0 };
    PCHAR SystemDirectory = ServiceDBData; /* Reuse this stack space */
    PCHAR ServicesFileLocation = "\\drivers\\etc\\services";
    PCHAR ThisLine = 0, NextLine = 0, ServiceName = 0, PortNumberStr = 0, 
	ProtocolStr = 0, Comment = 0;
    PCHAR Aliases[WS2_INTERNAL_MAX_ALIAS] = { 0 };
    UINT i,SizeNeeded = 0, 
	SystemDirSize = sizeof(ServiceDBData) - 1;
    DWORD ReadSize = 0, ValidData = 0;
    PWINSOCK_THREAD_BLOCK p = NtCurrentTeb()->WinSockData;
    
    if( !p ) {
	WSASetLastError( WSANOTINITIALISED );
	return NULL;
    }

    if( !name ) {
	WSASetLastError( WSANO_RECOVERY );
	return NULL;
    }
    
    if( !GetSystemDirectoryA( SystemDirectory, SystemDirSize ) ) {
	WSASetLastError( WSANO_RECOVERY );
	WS_DbgPrint(MIN_TRACE, ("Could not get windows system directory.\n"));
	return NULL; /* Can't get system directory */
    }
    
    strncat( SystemDirectory, ServicesFileLocation, SystemDirSize );

    ServicesFile = CreateFileA( SystemDirectory,
				GENERIC_READ,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL | 
				FILE_FLAG_SEQUENTIAL_SCAN,
				NULL );

    if( ServicesFile == INVALID_HANDLE_VALUE ) {
	WSASetLastError( WSANO_RECOVERY );
	return NULL;
    }

    /* Scan the services file ... 
     *
     * We will read up to BUFSIZ bytes per pass, until the buffer does not
     * contain a full line, then we will try to read more.
     *
     * We fall from the loop if the buffer does not have a line terminator.
     */

    /* Initial Read */
    while( !Found &&
	   ReadFile( ServicesFile, ServiceDBData + ValidData,
		     sizeof( ServiceDBData ) - ValidData,
		     &ReadSize, NULL ) ) {
	ValidData += ReadSize;
	ReadSize = 0;
	NextLine = ThisLine = ServiceDBData;

	/* Find the beginning of the next line */
	while( NextLine < ServiceDBData + ValidData &&
	       *NextLine != '\r' && *NextLine != '\n' ) NextLine++;
	
	/* Zero and skip, so we can treat what we have as a string */
	if( NextLine >= ServiceDBData + ValidData ) 
	    break;

	*NextLine = 0; NextLine++;

	Comment = strchr( ThisLine, '#' );
	if( Comment ) *Comment = 0; /* Terminate at comment start */

	if( DecodeServEntFromString( ThisLine, 
				     &ServiceName, 
				     &PortNumberStr,
				     &ProtocolStr,
				     Aliases,
				     WS2_INTERNAL_MAX_ALIAS ) &&
	    !strcmp( ServiceName, name ) &&
	    (proto ? !strcmp( ProtocolStr, proto ) : TRUE) ) {
	    WS_DbgPrint(MAX_TRACE,("Found the service entry.\n"));

	    Found = TRUE;
	    SizeNeeded = sizeof(WINSOCK_GETSERVBYNAME_CACHE) + 
		(NextLine - ThisLine);
	    break;
	}

	/* Get rid of everything we read so far */
	while( NextLine <= ServiceDBData + ValidData &&
	       isspace( *NextLine ) ) NextLine++;

	WS_DbgPrint(MAX_TRACE,("About to move %d chars\n", 
			       ServiceDBData + ValidData - NextLine));

	memmove( ServiceDBData, NextLine, 
		 ServiceDBData + ValidData - NextLine );
	ValidData -= NextLine - ServiceDBData;
	WS_DbgPrint(MAX_TRACE,("Valid bytes: %d\n", ValidData));
    }

    /* This we'll do no matter what */
    CloseHandle( ServicesFile );
    
    if( !Found ) {
	WS_DbgPrint(MAX_TRACE,("Not found\n"));
	WSASetLastError( WSANO_DATA );
	return NULL;
    }
    
    if( !p->Getservbyname || p->Getservbyname->Size < SizeNeeded ) {
	/* Free previous getservbyname buffer, allocate bigger */
	if( p->Getservbyname ) 
	    HeapFree(GlobalHeap, 0, p->Getservbyname);
	p->Getservbyname = HeapAlloc(GlobalHeap, 0, SizeNeeded);
	if( !p->Getservbyname ) {
	    WS_DbgPrint(MIN_TRACE,("Couldn't allocate %d bytes\n", 
				   SizeNeeded));
	    WSASetLastError( WSATRY_AGAIN );
	    return NULL;
	}
	p->Getservbyname->Size = SizeNeeded;
    }

    /* Copy the data */
    memmove( p->Getservbyname->Data, 
	     ThisLine,
	     NextLine - ThisLine );

    ADJ_PTR(ServiceName,ThisLine,p->Getservbyname->Data);
    ADJ_PTR(ProtocolStr,ThisLine,p->Getservbyname->Data);
    WS_DbgPrint(MAX_TRACE,
		("ServiceName: %s, Protocol: %s\n", ServiceName, ProtocolStr));
		
    for( i = 0; Aliases[i]; i++ ) {
	ADJ_PTR(Aliases[i],ThisLine,p->Getservbyname->Data);
	WS_DbgPrint(MAX_TRACE,("Aliase %d: %s\n", i, Aliases[i]));
    }

    memcpy(p->Getservbyname,Aliases,sizeof(Aliases));

    /* Create the struct proper */
    p->Getservbyname->ServerEntry.s_name = ServiceName;
    p->Getservbyname->ServerEntry.s_aliases = p->Getservbyname->Aliases;
    p->Getservbyname->ServerEntry.s_port = htons(atoi(PortNumberStr));
    p->Getservbyname->ServerEntry.s_proto = ProtocolStr;

    return &p->Getservbyname->ServerEntry;
}


/*
 * @unimplemented
 */
LPSERVENT
EXPORT
getservbyport(
    IN  INT port, 
    IN  CONST CHAR FAR* proto)
{
    UNIMPLEMENTED

    return (LPSERVENT)NULL;
}


/*
 * @implemented
 */
ULONG
EXPORT
inet_addr(
    IN  CONST CHAR FAR* cp)
/*
 * FUNCTION: Converts a string containing an IPv4 address to an unsigned long
 * ARGUMENTS:
 *     cp = Pointer to string with address to convert
 * RETURNS:
 *     Binary representation of IPv4 address, or INADDR_NONE
 */
{
    UINT i;
    PCHAR p;
    ULONG u = 0;

    p = (PCHAR)cp;

    if (strlen(p) == 0)
        return INADDR_NONE;

    if (strcmp(p, " ") == 0)
        return 0;

    for (i = 0; i <= 3; i++) {
        u += (strtoul(p, &p, 0) << (i * 8));

        if (strlen(p) == 0)
            return u;

        if (p[0] != '.')
            return INADDR_NONE;

        p++;
    }

    return u;
}


/*
 * @implemented
 */
CHAR FAR*
EXPORT
inet_ntoa(
    IN  IN_ADDR in)
{
    CHAR b[10];
    PCHAR p;

    p = ((PWINSOCK_THREAD_BLOCK)NtCurrentTeb()->WinSockData)->Intoa;
    _itoa(in.S_un.S_addr & 0xFF, b, 10);
    strcpy(p, b);
    _itoa((in.S_un.S_addr >> 8) & 0xFF, b, 10);
    strcat(p, ".");
    strcat(p, b);
    _itoa((in.S_un.S_addr >> 16) & 0xFF, b, 10);
    strcat(p, ".");
    strcat(p, b);
    _itoa((in.S_un.S_addr >> 24) & 0xFF, b, 10);
    strcat(p, ".");
    strcat(p, b);
    return (CHAR FAR*)p;
}


/*
 * @unimplemented
 */
HANDLE
EXPORT
WSAAsyncGetHostByAddr(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  CONST CHAR FAR* addr, 
    IN  INT len,
    IN  INT type, 
    OUT CHAR FAR* buf, 
    IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}


/*
 * @unimplemented
 */
HANDLE
EXPORT
WSAAsyncGetHostByName(
    IN  HWND hWnd, 
    IN  UINT wMsg,  
    IN  CONST CHAR FAR* name, 
    OUT CHAR FAR* buf, 
    IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}


/*
 * @unimplemented
 */
HANDLE
EXPORT
WSAAsyncGetProtoByName(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  CONST CHAR FAR* name,
    OUT CHAR FAR* buf,
    IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}


/*
 * @unimplemented
 */
HANDLE
EXPORT
WSAAsyncGetProtoByNumber(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  INT number,
    OUT CHAR FAR* buf,
    IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}


/*
 * @unimplemented
 */
HANDLE
EXPORT
WSAAsyncGetServByName(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  CONST CHAR FAR* name,
    IN  CONST CHAR FAR* proto,
    OUT CHAR FAR* buf,
    IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}


/*
 * @unimplemented
 */
HANDLE
EXPORT
WSAAsyncGetServByPort(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  INT port,
    IN  CONST CHAR FAR* proto,
    OUT CHAR FAR* buf,
    IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}

/* EOF */
