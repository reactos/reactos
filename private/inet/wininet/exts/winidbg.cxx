/*++

Module Name:

    winidbg.cxx

Abstract:



Author:

    Jeff Roberts (jroberts)  13-May-1996

Revision History:

     13-May-1996     jroberts

        Created this module.
    Mazhar Mohammed (mazharm) 3-20-97 - changed it all for async RPC,
                                                added some cool stuff
                                                single dll for ntsd and kd

	Feroze Daud ( ferozed ) 1/21/98 - changed for Wininet.

--*/

#include <stddef.h>
#include <limits.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>
#include <stdlib.h>
#include <windows.h>
#include <stddef.h>
#include "wdbgexts.h"

#define private public
#define protected public

//#include <wininetp.h>
#include <wininetp.h>


#include "local.hxx"

WINDBG_EXTENSION_APIS ExtensionApis;
HANDLE ProcessHandle = 0;
BOOL fKD = 0;

#define ALLOC_SIZE 500
#define MAX_ARGS 4

// #include "rpcexts.hxx"

//
// stuff not common to kernel-mode and user-mode DLLs
//

void do_fsm (
			DWORD fsm );
#if INET_DEBUG
LPSTR
MapType(
    FSM_TYPE m_Type
    );
#endif // INET_DEBUG

LPSTR
MapState(
    DWORD m_Type
    );


#define CASE_OF(x) case x : return #x; break;

// VOID do_trans( DWORD dwAddr );

DECLARE_API( fsm );
DECLARE_API( ho );
DECLARE_API( iho );
DECLARE_API( icho );
DECLARE_API( hrho );
DECLARE_API( lste );
DECLARE_API( serialist );

DECLARE_API(badproxylste);
void do_BAD_PROXY_LIST_ENTRY( DWORD addr ); 

DECLARE_API(badproxylst);
void do_BAD_PROXY_LIST( DWORD addr ); 

DECLARE_API(proxybyplste);
void do_PROXY_BYPASS_LIST_ENTRY( DWORD addr ); 

DECLARE_API(proxybyplst);
void do_PROXY_BYPASS_LIST( DWORD addr ); 

DECLARE_API(proxysrvlste);
void do_PROXY_SERVER_LIST_ENTRY( DWORD addr ); 

DECLARE_API(proxysrvlst);
void do_PROXY_SERVER_LIST( DWORD addr ); 

DECLARE_API(ICSecureSocket);
void do_ICSecureSocket( DWORD addr ); 

#define OFFSET( x, y ) \
				((DWORD)addr + offsetof(x, y)) 


// define our own operators new and delete, so that we do not have to include the crt

void * _CRTAPI1
::operator new(unsigned int dwBytes)
{
    void *p;
    p = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytes);
    return (p);
}


void _CRTAPI1
::operator delete (void *p)
{
    HeapFree(GetProcessHeap(), 0, p);
}

BOOL
GetData(IN DWORD dwAddress,  IN LPVOID ptr, IN ULONG size, IN PCSTR type )
{
    BOOL b;
    ULONG BytesRead;
    ULONG count;

    if (fKD == 0)
        {
        return ReadProcessMemory(ProcessHandle, (LPVOID) dwAddress, ptr, size, 0);
        }

    while( size > 0 )
        {
        count = MIN( size, 3000 );

        b = d_ReadMemory((ULONG) dwAddress, ptr, count, &BytesRead );

        if (!b || BytesRead != count )
            {
            if (NULL == type)
                {
                type = "unspecified" ;
                }
            return FALSE;
            }

        dwAddress += count;
        size -= count;
        ptr = (LPVOID)((ULONG)ptr + count);
        }

    return TRUE;
}

#define MAX_MESSAGE_BLOCK_SIZE 1024
#define BLOCK_SIZE 16
/*
RPC_CHAR *
ReadProcessRpcChar(
    unsigned short * Address
    )
{
    DWORD dwAddr = (DWORD) Address;

    char       block[BLOCK_SIZE];
    RPC_CHAR   *RpcBlock  = (RPC_CHAR *)&block;
    char *string_block = new char[MAX_MESSAGE_BLOCK_SIZE];
    RPC_CHAR   *RpcString = (RPC_CHAR *)string_block;
    int  length = 0;
    int  i      = 0;
    BOOL b;
    BOOL end    = FALSE;

    if (dwAddr == NULL) {
        return (L'\0');
    }

    for (length = 0; length < MAX_MESSAGE_BLOCK_SIZE/2; ) {
        b = GetData( dwAddr, &block, BLOCK_SIZE, NULL);
        if (b == FALSE) {
            d_printf("couldn't read address %x\n", dwAddr);
            return (L'\0');
        }
        for (i = 0; i < BLOCK_SIZE/2; i++) {
            if (RpcBlock[i] == L'\0') {
                end = TRUE;
            }
            RpcString[length] = RpcBlock[i];
            length++;
        }
        if (end == TRUE) {
            break;
        }
        dwAddr += BLOCK_SIZE;
    }
    return (RpcString);
}
*/

long
myatol(char *string)
{
    int  i         = 0;
    BOOL minus     = FALSE;
    long number    = 0;
    long tmpnumber = 0 ;
    long chknum;

    if (string[0] == '-') {
        minus = TRUE;
        i++;
    }
    else
    if (string[0] == '+') {
        i++;
    }
    for (; string[i] != '\0'; i++) {
        if ((string[i] >= '0')&&(string[i] <= '9')) {
            tmpnumber = string[i] - '0';
            if (number != 0) {
                chknum = LONG_MAX/number;
            }
            if (chknum > 11) {
                number = number*10 + tmpnumber;
            }
        }
        else
            return 0;
    }
    if (minus == TRUE) {
        number = 0 - number;
    }
    return number;
}

PCHAR
MapSymbol(DWORD dwAddr)
{
    static CHAR Name[256];
    DWORD Displacement;

    d_GetSymbol((LPVOID)dwAddr, (UCHAR *)Name, &Displacement);
    strcat(Name, "+");
#if 0
	// nuked due to build break
    PCHAR p = strchr(Name, '\0');
    _ltoa(Displacement, p, 16);
#endif
    return(Name);
}

// checks the if the uuid is null, prints the uuid
/*
void
PrintUuid(UUID *Uuid)
{
    unsigned long PAPI * Vector;

    Vector = (unsigned long PAPI *) Uuid;
    if (   (Vector[0] == 0)
         && (Vector[1] == 0)
         && (Vector[2] == 0)
         && (Vector[3] == 0))
    {
        d_printf("(Null Uuid)");
    }
    else
    {
        d_printf("%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                       Uuid->Data1, Uuid->Data2, Uuid->Data3, Uuid->Data4[0], Uuid->Data4[1],
                       Uuid->Data4[2], Uuid->Data4[3], Uuid->Data4[4], Uuid->Data4[5],
                       Uuid->Data4[6], Uuid->Data4[7] );
    }
    return;
}
*/

/*
DWORD OSF_CCONNECTION_SIZE = sizeof(OSF_CCONNECTION);
DWORD OSF_SCONNECTION_SIZE = sizeof(OSF_SCONNECTION);
DWORD OSF_ADDRESS_SIZE = sizeof(OSF_ADDRESS);
DWORD DG_ADDRESS_SIZE = sizeof(DG_ADDRESS);
*/

VOID
do_sizes(
    )
{
	d_printf("BUGBUG .... Add stuff here \n");
   // d_printf("sizeof(ASSOCIATION_HANDLE) - 0x%x\n", sizeof(ASSOCIATION_HANDLE));
}

DECLARE_API( sizes )
{
   INIT_DPRINTF();
   do_sizes();
}

char *
GetError (DWORD dwError)
{
    DWORD   dwFlag = FORMAT_MESSAGE_FROM_SYSTEM;
    static CHAR   szErrorMessage[1024];
    static HANDLE  hSource = NULL;

    if ((dwError >= 2100) && (dwError < 6000))
    {
        if (hSource == NULL)
            {
            hSource = LoadLibrary("netmsg.dll");
            }

        if (hSource == NULL)
        {
            sprintf (szErrorMessage,
                      "Unable to load netmsg.dll. Error %d occured.\n",
                      dwError);
            return(szErrorMessage);
        }

        dwFlag = FORMAT_MESSAGE_FROM_HMODULE;
    }

    if (!FormatMessage (dwFlag,
                        hSource,
                        dwError,
                        0,
                        szErrorMessage,
                        1024,
                        NULL))
       {
        sprintf (szErrorMessage,
                  "An unknown error occured: 0x%x \n",
                  dwError);
       }

    return(szErrorMessage);
}

VOID
do_error (
    DWORD dwAddr
    )
{
    d_printf("0x%x: %s\n", dwAddr, GetError(dwAddr));
}


DECLARE_API( help )
{
    INIT_DPRINTF();

    if (lpArgumentString[0] == '\0') {
        d_printf("\n"
                "wininext help:\n\n"
                "\n"
                "!obj      <address>  - Dumps an RPC object \n"
                "\n"
                "!sizes - Prints sizes of the data structures\n"
                "!error - Translates and error value into the error message\n"
                "!symbol    (<address>|<symbol name>) - Returns symbol name/address\n"
				"!fsm		<address> - Dumps the CFsm object\n"
				"!ho		<address> - Dumps the HANDLE_OBJECT object\n"
				"!iho		<address> - Dumps the INTERNET_HANDLE_OBJECT object\n"
				"!icho		<address> - Dumps the INTERNET_CONNECT_HANDLE_OBJECT object\n"
				"!hrho		<address> - Dumps the /////////////////////////////////////////////////
//
//	HEADER_STRING structure
//
/////////////////////////////////////////////////

DECLARE_API( HEADER_STRING )
{
    DWORD dwAddr;

    INIT_DPRINTF();

    dwAddr = d_GetExpression(lpArgumentString);
    if ( !dwAddr )
        {
        return;
        }
   do_HEADER_STRING(dwAddr);
}

VOID
do_HEADER_STRING(
	   DWORD addr
    )
{
	BOOL b;
	char block[ sizeof( HEADER_STRING ) ];

	HEADER_STRING * obj  = (HEADER_STRING *) &block;

    b = GetData(
			addr, 
			block, 
			sizeof( HEADER_STRING ), 
			NULL);

	if ( !b ) {
		d_printf("couldn't read HEADER_STRING at 0x%x; sorry.\n", addr);
		return;
	}


	d_printf("HEADER_STRING @ 0x%x \n\n", addr);

	//DWORD  m_Hash
	d_printf("\tDWORD m_Hash %d\n", obj -> m_Hash);

	//LPSTR  value
	d_printf("\tLPSTR value %s\n", obj -> value);



	d_printf("\n");

}  // HEADER_STRING



/////////////////////////////////////////////////
//
//	HTTP_HEADERS structure
//
/////////////////////////////////////////////////

DECLARE_API( HTTP_HEADERS )
{
    DWORD dwAddr;

    INIT_DPRINTF();

    dwAddr = d_GetExpression(lpArgumentString);
    if ( !dwAddr )
        {
        return;
        }
   do_HTTP_HEADERS(dwAddr);
}

VOID
do_HTTP_HEADERS(
	   DWORD addr
    )
{
	BOOL b;
	char block[ sizeof( HTTP_HEADERS ) ];

	HTTP_HEADERS * obj  = (HTTP_HEADERS *) &block;

    b = GetData(
			addr, 
			block, 
			sizeof( HTTP_HEADERS ), 
			NULL);

	if ( !b ) {
		d_printf("couldn't read HTTP_HEADERS at 0x%x; sorry.\n", addr);
		return;
	}


	d_printf("HTTP_HEADERS @ 0x%x \n\n", addr);

#ifndef STRESS_BUG_DEBUG
#endif
#if INET_DEBUG
	//DWORD  _Signature
	d_printf("\tDWORD _Signature %d\n", obj -> _Signature);

#endif
	//HEADER_STRING * _lpHeaders
	d_printf("\tHEADER_STRING * _lpHeaders 0x%x\n", obj -> _lpHeaders);

	//DWORD  _TotalSlots
	d_printf("\tDWORD _TotalSlots %d\n", obj -> _TotalSlots);

	//DWORD  _NextOpenSlot
	d_printf("\tDWORD _NextOpenSlot %d\n", obj -> _NextOpenSlot);

	//DWORD  _FreeSlots
	d_printf("\tDWORD _FreeSlots %d\n", obj -> _FreeSlots);

	//DWORD  _HeadersLength
	d_printf("\tDWORD _HeadersLength %d\n", obj -> _HeadersLength);

	//BOOL  _IsRequestHeaders
	d_printf("\tBOOL _IsRequestHeaders %d\n", obj -> _IsRequestHeaders);

	//LPSTR  _lpszVerb
	d_printf("\tLPSTR _lpszVerb %s\n", obj -> _lpszVerb);

	//DWORD  _dwVerbLength
	d_printf("\tDWORD _dwVerbLength %d\n", obj -> _dwVerbLength);

	//LPSTR  _lpszObjectName
	d_printf("\tLPSTR _lpszObjectName %s\n", obj -> _lpszObjectName);

	//DWORD  _dwObjectNameLength
	d_printf("\tDWORD _dwObjectNameLength %d\n", obj -> _dwObjectNameLength);

	//LPSTR  _lpszVersion
	d_printf("\tLPSTR _lpszVersion %s\n", obj -> _lpszVersion);

	//DWORD  _dwVersionLength
	d_printf("\tDWORD _dwVersionLength %d\n", obj -> _dwVersionLength);

	//DWORD  _RequestVersionMajor
	d_printf("\tDWORD _RequestVersionMajor %d\n", obj -> _RequestVersionMajor);

	//DWORD  _RequestVersionMinor
	d_printf("\tDWORD _RequestVersionMinor %d\n", obj -> _RequestVersionMinor);

	//DWORD  _Error
	d_printf("\tDWORD _Error %d\n", obj -> _Error);

#ifdef STRESS_BUG_DEBUG
	//DWORD  _dwCritCount
	d_printf("\tDWORD _dwCritCount %d\n", obj -> _dwCritCount);

#endif
#ifdef STRESS_BUG_DEBUG
#endif
#if INET_DEBUG
#endif
#if INET_DEBUG
#endif
#ifdef STRESS_BUG_DEBUG
#endif
#ifdef STRESS_BUG_DEBUG
#endif
#ifdef COMPRESSED_HEADERS
#endif //COMPRESSED_HEADERS


	d_printf("\n");

}  // HTTP_HEADERS



/////////////////////////////////////////////////
//
//	HTTP_REQUEST_HANDLE_OBJECT structure
//
/////////////////////////////////////////////////

DECLARE_API( HTTP_REQUEST_HANDLE_OBJECT )
{
    DWORD dwAddr;

    INIT_DPRINTF();

    dwAddr = d_GetExpression(lpArgumentString);
    if ( !dwAddr )
        {
        return;
        }
   do_HTTP_REQUEST_HANDLE_OBJECT(dwAddr);
}

VOID
do_HTTP_REQUEST_HANDLE_OBJECT(
	   DWORD addr
    )
{
	BOOL b;
	char block[ sizeof( HTTP_REQUEST_HANDLE_OBJECT ) ];

	HTTP_REQUEST_HANDLE_OBJECT * obj  = (HTTP_REQUEST_HANDLE_OBJECT *) &block;

    b = GetData(
			addr, 
			block, 
			sizeof( HTTP_REQUEST_HANDLE_OBJECT ), 
			NULL);

	if ( !b ) {
		d_printf("couldn't read HTTP_REQUEST_HANDLE_OBJECT at 0x%x; sorry.\n", addr);
		return;
	}


	d_printf("HTTP_REQUEST_HANDLE_OBJECT @ 0x%x \n\n", addr);

	//LIST_ENTRY  m_PipelineList
	d_printf("\tLIST_ENTRY (*) m_PipelineList 0x%x\n", OFFSET( HTTP_REQUEST_HANDLE_OBJECT, m_PipelineList) );

	//LONG  m_lPriority
	d_printf("\tLONG m_lPriority %d\n", obj -> m_lPriority);

	//ICSocket * _Socket
	d_printf("\tICSocket * _Socket 0x%x\n", obj -> _Socket);

	//BOOL  _bKeepAliveConnection
	d_printf("\tBOOL _bKeepAliveConnection %d\n", obj -> _bKeepAliveConnection);

	//BOOL  _bNoLongerKeepAlive
	d_printf("\tBOOL _bNoLongerKeepAlive %d\n", obj -> _bNoLongerKeepAlive);

	//LPVOID  _QueryBuffer
	d_printf("\tLPVOID _QueryBuffer 0X%X\n", obj -> _QueryBuffer);

	//DWORD  _QueryBufferLength
	d_printf("\tDWORD _QueryBufferLength %d\n", obj -> _QueryBufferLength);

	//DWORD  _QueryOffset
	d_printf("\tDWORD _QueryOffset %d\n", obj -> _QueryOffset);

	//DWORD  _QueryBytesAvailable
	d_printf("\tDWORD _QueryBytesAvailable %d\n", obj -> _QueryBytesAvailable);

	//DWORD  _OpenFlags
	d_printf("\tDWORD _OpenFlags %d\n", obj -> _OpenFlags);

	//HTTPREQ_STATE  _State
	d_printf("\tHTTPREQ_STATE _State %d\n", obj -> _State);

	//HTTP_HEADERS  _RequestHeaders
	d_printf("\tHTTP_HEADERS _RequestHeaders 0X%X\n", obj -> _RequestHeaders);

	//HTTP_METHOD_TYPE  _RequestMethod
	d_printf("\tHTTP_METHOD_TYPE _RequestMethod %d\n", obj -> _RequestMethod);

	//DWORD  _dwOptionalSaved
	d_printf("\tDWORD _dwOptionalSaved %d\n", obj -> _dwOptionalSaved);

	//LPVOID  _lpOptionalSaved
	d_printf("\tLPVOID _lpOptionalSaved 0X%X\n", obj -> _lpOptionalSaved);

	//BOOL  _fOptionalSaved
	d_printf("\tBOOL _fOptionalSaved %d\n", obj -> _fOptionalSaved);

	//DWORD  _iSlotContentLength
	d_printf("\tDWORD _iSlotContentLength %d\n", obj -> _iSlotContentLength);

	//DWORD  _iSlotContentRange
	d_printf("\tDWORD _iSlotContentRange %d\n", obj -> _iSlotContentRange);

	//DWORD  _StatusCode
	d_printf("\tDWORD _StatusCode %d\n", obj -> _StatusCode);

	//LPBYTE  _ResponseBuffer
	d_printf("\tLPBYTE _ResponseBuffer 0X%X\n", obj -> _ResponseBuffer);

	//DWORD  _ResponseBufferLength
	d_printf("\tDWORD _ResponseBufferLength %d\n", obj -> _ResponseBufferLength);

	//DWORD  _BytesReceived
	d_printf("\tDWORD _BytesReceived %d\n", obj -> _BytesReceived);

	//DWORD  _ResponseScanned
	d_printf("\tDWORD _ResponseScanned %d\n", obj -> _ResponseScanned);

	//DWORD  _ResponseBufferDataReadyToRead
	d_printf("\tDWORD _ResponseBufferDataReadyToRead %d\n", obj -> _ResponseBufferDataReadyToRead);

	//DWORD  _DataOffset
	d_printf("\tDWORD _DataOffset %d\n", obj -> _DataOffset);

	//DWORD  _BytesRemaining
	d_printf("\tDWORD _BytesRemaining %d\n", obj -> _BytesRemaining);

	//DWORD  _ContentLength
	d_printf("\tDWORD _ContentLength %d\n", obj -> _ContentLength);

	//DWORD  _BytesInSocket
	d_printf("\tDWORD _BytesInSocket %d\n", obj -> _BytesInSocket);

	//FILETIME  _ftLastModified
	d_printf("\tFILETIME _ftLastModified %d\n", obj -> _ftLastModified);

	//FILETIME  _ftExpires
	d_printf("\tFILETIME _ftExpires %d\n", obj -> _ftExpires);

	//FILETIME  _ftPostCheck
	d_printf("\tFILETIME _ftPostCheck %d\n", obj -> _ftPostCheck);

	//CHUNK_TRANSFER  _ctChunkInfo
	d_printf("\tCHUNK_TRANSFER (*) _ctChunkInfo 0x%x\n", OFFSET( HTTP_REQUEST_HANDLE_OBJECT, _ctChunkInfo) );

	//BOOL  _fTalkingToSecureServerViaProxy
	d_printf("\tBOOL _fTalkingToSecureServerViaProxy %d\n", obj -> _fTalkingToSecureServerViaProxy);

	//BOOL  _fRequestUsingProxy
	d_printf("\tBOOL _fRequestUsingProxy %d\n", obj -> _fRequestUsingProxy);

	//BOOL  _bWantKeepAlive
	d_printf("\tBOOL _bWantKeepAlive %d\n", obj -> _bWantKeepAlive);

	//BOOL  _bRefresh
	d_printf("\tBOOL _bRefresh %d\n", obj -> _bRefresh);

	//HEADER_STRING  _RefreshHeader
	d_printf("\tHEADER_STRING (*) _RefreshHeader 0x%x\n", OFFSET( HTTP_REQUEST_HANDLE_OBJECT, _RefreshHeader) );

	//DWORD  _dwQuerySetCookieHeader
	d_printf("\tDWORD _dwQuerySetCookieHeader %d\n", obj -> _dwQuerySetCookieHeader);



	d_printf("\n");

}  // HTTP_REQUEST_HANDLE_OBJECT


#ifndef unix
#endif /* unix */
HANDLE_OBJECT object\n"
				"!tinf		<address> - Dumps the INTERNET_THREAD_INFO structure\n"
				"!lste		<address> - Dumps the LIST_ENTRY structure\n"
				"!serialist		<address> - Dumps the SERIALIZED_LIST structure\n"
				"!proxyinfo		<address> - Dumps the PROXY_INFO structure\n"
				"!proxysrvlst		<address> - Dumps the PROXY_INFO structure\n"
				"!proxysrvlste		<address> - Dumps the PROXY_INFO_LIST structure\n"
				"!proxybyplst	<address> - Dumps the PROXY_BYPASS_LIST structure\n"
				"!proxybyplste	<address> - Dumps the PROXY_BYPASS_LIST_ENTRY structure\n"
				);
    }
}

void do_symbol(DWORD dwAddr)
{
    CHAR Symbol[64];
    DWORD Displacement;

    d_GetSymbol((LPVOID)dwAddr,(unsigned char *)Symbol,&Displacement);
    d_printf("%lx   %s+%lx\n", dwAddr, Symbol, Displacement);
}

DECLARE_API( symbol )
{
    DWORD dwAddr;
    INIT_DPRINTF();

    dwAddr = d_GetExpression(lpArgumentString);
    if ( !dwAddr )
        {
        return;
        }
    do_symbol(dwAddr);
}



USHORT SavedMajorVersion;
USHORT SavedMinorVersion;
BOOL   ChkTarget;            // is debuggee a CHK build?
#define VER_PRODUCTBUILD 10
EXT_API_VERSION ApiVersion = { 3, 5, EXT_API_VERSION_NUMBER, 0 };

VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    fKD = 1;
    ExtensionApis = *lpExtensionApis ;
    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;
    ChkTarget = SavedMajorVersion == 0x0c ? TRUE : FALSE;
}

DECLARE_API( version )
{
#if    DBG
    PCSTR kind = "Checked";
#else
    PCSTR kind = "Free";
#endif

    d_printf(
        "%s RPC Extension dll for Build %d debugging %s kernel for Build %d\n",
        kind,
        VER_PRODUCTBUILD,
        SavedMajorVersion == 0x0c ? "Checked" : "Free",
        SavedMinorVersion
    );
}


VOID
CheckVersion(
    VOID
    )
{
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

///////////////////////////////////
//
// Dump the FSM class structure
//
///////////////////////////////////

DECLARE_API( fsm )
{
    DWORD dwAddr;

    INIT_DPRINTF();

    dwAddr = d_GetExpression(lpArgumentString);
    if ( !dwAddr )
        {
        return;
        }
   do_fsm(dwAddr);
}

VOID
do_fsm(
	   DWORD addr
    )
{
	BOOL b;
	char block[ sizeof( CFsm ) ];

	CFsm * fsm  = (CFsm *) &block;

    b = GetData(
			addr, 
			block, 
			sizeof( CFsm ), 
			NULL);

	if ( !b ) {
		d_printf("couldn't read CFsm at 0x%x; sorry.\n", addr);
		return;
	}

	d_printf("\nCFsm at 0x%x \n", addr);

	// CFsm * m_Link;
	d_printf("\tCFsm * m_Link = 0x%x\n", fsm -> m_Link );

	// DWORD m_dwError;
	d_printf("\tDWORD m_dwError = %d\n", fsm -> m_dwError );

	// LPINTERNET_THREAD_INFO m_lpThreadInfo;
	d_printf("\tm_lpThreadInfo = 0x%x\n", fsm -> m_lpThreadInfo );

	// DWORD m_dwContext;
	d_printf("\tDWORD_PTR m_dwContext = %d\n", fsm -> m_dwContext );

	// HINTERNET m_hObject;
	d_printf("\tHINTERNET m_hObject = 0x%x\n", fsm -> m_hObject );

	//INTERNET_HANDLE_OBJECT * m_hObjectMapped;
	d_printf("\tINTERNET_HANDLE_OBJECT * m_hObjectMapped = 0x%x\n", fsm -> m_hObjectMapped );

	//DWORD m_dwMappedErrorCode;
	d_printf("\tDWORD m_dwMappedErrorCode = %d\n", fsm -> m_dwMappedErrorCode );

	//FSM_STATE m_State;
	d_printf("\tFSM_STATE m_State = %d ( %s )\n", fsm -> m_State  , MapState( fsm -> m_NextState));

	//FSM_STATE m_NextState;
	d_printf("\tFSM_STATE m_NextState = %d ( %s ) \n", fsm -> m_NextState , MapState( fsm -> m_NextState));


	//FSM_STATE m_FunctionState;
	d_printf("\tFSM_STATE m_FunctionState = %d ( %s ) \n", fsm -> m_FunctionState , MapState( fsm -> m_FunctionState ));

#if INET_DEBUG
	//FSM_TYPE m_Type;
	d_printf("\tFSM_TYPE m_Type = %d(%s)\n", fsm -> m_Type, MapType( fsm -> m_Type ));
#endif

	/*
	DWORD (*m_lpfnHandler)(CFsm *);
	d_printf("\tCFsm * m_Link = 0x%x\n", fsm -> m_Link );

	LPVOID m_lpvContext;
	d_printf("\tCFsm * m_Link = 0x%x\n", fsm -> m_Link );

	FSM_HINT m_Hint;
	d_printf("\tCFsm * m_Link = 0x%x\n", fsm -> m_Link );
	*/

	//SOCKET m_Socket;
	d_printf("\tSOCKET m_Socket = 0x%x\n", fsm -> m_Socket );

	FSM_ACTION m_Action;
	d_printf("\tCFsm * m_Link = 0x%x\n", fsm -> m_Link );

	/*
	DWORD m_dwBlockId;
	d_printf("\tCFsm * m_Link = 0x%x\n", fsm -> m_Link );

	DWORD m_dwTimeout;
	d_printf("\tCFsm * m_Link = 0x%x\n", fsm -> m_Link );

	DWORD m_dwTimer;
	d_printf("\tCFsm * m_Link = 0x%x\n", fsm -> m_Link );

	BOOL m_bTimerStarted;
	d_printf("\tCFsm * m_Link = 0x%x\n", fsm -> m_Link );
	*/

	//BOOL m_bIsApi;
	d_printf("\tBOOL m_bIsApi = %d\n", fsm -> m_bIsApi );

	//API_TYPE m_ApiType;
	d_printf("\tAPI_TYPE m_ApiType = %d\n", fsm -> m_ApiType );

	//DWORD m_dwApiData;
	d_printf("\tDWORD m_dwApiData = %d\n", fsm -> m_dwApiData );

	d_printf("\n");

	/*
	union {
		BOOL Bool;
		HINTERNET Handle;
	} m_ApiResult;
	*/


    


}


///////////////////////////////////
//
// Dump the HANDLE_OBJECT class structure
//
///////////////////////////////////
VOID
do_handle_object(
	   DWORD addr
    )
{
	BOOL b;
	char block[ sizeof( HANDLE_OBJECT ) ];

	HANDLE_OBJECT * fsm  = (HANDLE_OBJECT *) &block;

    b = GetData(
			addr, 
			block, 
			sizeof( HANDLE_OBJECT ), 
			NULL);

	if ( !b ) {
		d_printf("couldn't read HANDLE_OBJECT at 0x%x; sorry.\n", addr);
		return;
	}


	d_printf("HANDLE_OBJECT @ 0x%x \n", addr);

    //LIST_ENTRY _List;
	//d_printf("\tLIST_ENTRY _List\n");
	d_printf("\tLIST_ENTRY (*) _List = 0x%x \n", ((DWORD) addr + offsetof(HANDLE_OBJECT,_List)) );
	/*
	d_printf("\t\tLIST_ENTY * Flink = 0x%x\n", (fsm -> _List).Flink);
	d_printf("\t\tLIST_ENTY * Blink = 0x%x\n", (fsm -> _List).Blink);
	*/


    //SERIALIZED_LIST _Children;
	d_printf("\tSERIALIZED_LIST (*) _Children = 0x%x \n", ((DWORD) addr + offsetof(HANDLE_OBJECT,_Children)) );

    //LIST_ENTRY _Siblings;
	d_printf("\tLIST_ENTRY (*) _Siblings = 0x%x \n", ((DWORD) addr + offsetof(HANDLE_OBJECT,_Siblings)) );

    //HANDLE_OBJECT* _Parent;

	d_printf("\tHANDLE_OBJECT * _Parent = 0x%x \n", fsm -> _Parent);

    //BOOL _DeleteWithChild;
	d_printf("\tBOOL _DeleteWithChild = %d \n", fsm -> _DeleteWithChild);

    //HINTERNET _Handle;
	d_printf("\tHINTERNET _Handle = 0x%x \n", fsm -> _Handle);

    //HINTERNET_HANDLE_TYPE _ObjectType;
	d_printf("\tHINTERNET_HANDLE_TYPE _ObjectType = %d \n", fsm -> _ObjectType);

    //LONG _ReferenceCount;
	d_printf("\tLONG _ReferenceCount = %ld \n", fsm -> _ReferenceCount);

    //BOOL _Invalid;
	d_printf("\tBOOL _Invalid = %d \n", fsm -> _Invalid);

    //DWORD _Error;
	d_printf("\tDWORD _Error = %d \n", fsm -> _Error);

    //DWORD _Signature;
	d_printf("\tDWORD _Signature = 0x%x \n", fsm -> _Signature);

    //DWORD _Context;
	d_printf("\tDWORD _Context = %d \n", fsm -> _Context);

    //DWORD _Status;
	d_printf("\tDWORD _Status = %d \n", fsm -> _Status);

	d_printf("\n");

}

DECLARE_API( ho )
{
    DWORD dwAddr;

    INIT_DPRINTF();

    dwAddr = d_GetExpression(lpArgumentString);
    if ( !dwAddr )
        {
        return;
        }
   do_handle_object(dwAddr);
}

///////////////////////////////////
//
// Dump the Thread Info structure
//
///////////////////////////////////
VOID
do_thread_info(
	   DWORD addr
    )
{
	BOOL b;
	char block[ sizeof( INTERNET_THREAD_INFO ) ];

	LPINTERNET_THREAD_INFO fsm  = (LPINTERNET_THREAD_INFO) &block;

    b = GetData(
			addr, 
			block, 
			sizeof( INTERNET_THREAD_INFO ), 
			NULL);

	if ( !b ) {
		d_printf("couldn't read INTERNET_THREAD_INFO at 0x%x; sorry.\n", addr);
		return;
	}



	d_printf("INTERNET_THREAD_INFO @ 0x%x \n", addr );

    //LIST_ENTRY List;
	d_printf("\tLIST_ENTRY List = 0x%x \n", fsm -> List);


    //    DWORD ThreadId;
	d_printf("\tDWORD ThreadId = %d \n", fsm -> ThreadId);

    //DWORD ErrorNumber;
	d_printf("\tDWORD ErrorNumber = %d \n", fsm -> ErrorNumber);

    //DWORD Context;
	d_printf("\tDWORD Context = %d \n", fsm -> Context);

    //HINTERNET hObject;
	d_printf("\tHINTERNET hObject = 0x%x \n", fsm -> hObject);

     //HINTERNET hObjectMapped;
	d_printf("\tHINTERNET hObjectMapped = 0x%x \n", fsm -> hObjectMapped);

   //BOOL IsAsyncWorkerThread;
	d_printf("\tBOOL IsAsyncWorkerThread = %d \n", fsm -> IsAsyncWorkerThread);

   //BOOL InCallback;
	d_printf("\tBOOL InCallback = %d \n", fsm -> InCallback);

   //BOOL IsAutoProxyProxyThread;
	d_printf("\tBOOL IsAutoProxyProxyThread = %d \n", fsm -> IsAutoProxyProxyThread);

   //DWORD NestedRequests;
	d_printf("\tDWORD NestedRequests = %d \n", fsm -> NestedRequests);

   //DWORD dwMappedErrorCode;
	d_printf("\tDWORD dwMappedErrorCode = %d \n", fsm -> dwMappedErrorCode);

    //CFsm * Fsm;
	d_printf("\tCFsm * Fsm = 0x%x \n", fsm -> Fsm);

	d_printf("\n");
}

DECLARE_API( tinf )
{
    DWORD dwAddr;

    INIT_DPRINTF();

    dwAddr = d_GetExpression(lpArgumentString);
    if ( !dwAddr )
        {
        return;
        }
   do_thread_info(dwAddr);
}

///////////////////////////////////
//
// Dump the INTERNET_HANDLE_OBJECT class structure
//
///////////////////////////////////
VOID
do_internet_handle_object(
	   DWORD addr
    )
{
	BOOL b;
	char block[ sizeof( INTERNET_HANDLE_OBJECT ) ];

	INTERNET_HANDLE_OBJECT * obj  = (INTERNET_HANDLE_OBJECT *) &block;

    b = GetData(
			addr, 
			block, 
			sizeof( INTERNET_HANDLE_OBJECT ), 
			NULL);

	if ( !b ) {
		d_printf("couldn't read INTERNET_HANDLE_OBJECT at 0x%x; sorry.\n", addr);
		return;
	}


	//do_internet_handle_object( addr );

	d_printf("INTERNET_HANDLE_OBJECT @ 0x%x \n\n", addr);

	//HINTERNET  _INetHandle
	d_printf("\tHINTERNET _INetHandle = 0x%x\n", obj -> _INetHandle);

	//BOOL  _IsCopy
	d_printf("\tBOOL _IsCopy = %d\n", obj -> _IsCopy);

	//ICSTRING  _UserAgent
	d_printf("\tICSTRING _UserAgent = 0x%x\n", obj -> _UserAgent);

	//PROXY_INFO * _ProxyInfo
	d_printf("\tPROXY_INFO * _ProxyInfo = 0x%x\n", obj -> _ProxyInfo);

	//RESOURCE_LOCK  _ProxyInfoResourceLock
	d_printf("\tRESOURCE_LOCK (*) _ProxyInfoResourceLock = 0x%x\n", &(obj -> _ProxyInfoResourceLock));

	//DWORD  _dwInternetOpenFlags
	d_printf("\tDWORD _dwInternetOpenFlags = %d\n", obj -> _dwInternetOpenFlags);

	//BOOL  _WinsockLoaded
	d_printf("\tBOOL _WinsockLoaded = %d\n", obj -> _WinsockLoaded);

	//ICSocket * _pICSocket
	d_printf("\tICSocket * _pICSocket = 0x%x\n", obj -> _pICSocket);

	//DWORD  _ConnectTimeout
	d_printf("\tDWORD _ConnectTimeout = %d\n", obj -> _ConnectTimeout);

	//DWORD  _ConnectRetries
	d_printf("\tDWORD _ConnectRetries = %d\n", obj -> _ConnectRetries);

	//DWORD  _SendTimeout
	d_printf("\tDWORD _SendTimeout = %d\n", obj -> _SendTimeout);

	//DWORD  _DataSendTimeout
	d_printf("\tDWORD _DataSendTimeout = %d\n", obj -> _DataSendTimeout);

	//DWORD  _ReceiveTimeout
	d_printf("\tDWORD _ReceiveTimeout = %d\n", obj -> _ReceiveTimeout);

	//DWORD  _DataReceiveTimeout
	d_printf("\tDWORD _DataReceiveTimeout = %d\n", obj -> _DataReceiveTimeout);

	//DWORD  _SocketSendBufferLength
	d_printf("\tDWORD _SocketSendBufferLength = %d\n", obj -> _SocketSendBufferLength);

	//DWORD  _SocketReceiveBufferLength
	d_printf("\tDWORD _SocketReceiveBufferLength = %d\n", obj -> _SocketReceiveBufferLength);

	//BOOL  _Async
	d_printf("\tBOOL _Async = %d\n", obj -> _Async);

	//DWORD  _DataAvailable
	d_printf("\tDWORD _DataAvailable = %d\n", obj -> _DataAvailable);

	//BOOL  _EndOfFile
	d_printf("\tBOOL _EndOfFile = %d\n", obj -> _EndOfFile);

	d_printf("\n");

}

DECLARE_API( iho )
{
    DWORD dwAddr;

    INIT_DPRINTF();

    dwAddr = d_GetExpression(lpArgumentString);
    if ( !dwAddr )
        {
        return;
        }
   do_internet_handle_object(dwAddr);
}

/////////////////////////////////////////////////
//
//	INTERNET_CONNECT_HANDLE_OBJECT structure
//
/////////////////////////////////////////////////

DECLARE_API( INTERNET_CONNECT_HANDLE_OBJECT )
{
    DWORD dwAddr;

    INIT_DPRINTF();

    dwAddr = d_GetExpression(lpArgumentString);
    if ( !dwAddr )
        {
        return;
        }
   do_INTERNET_CONNECT_HANDLE_OBJECT(dwAddr);
}

VOID
do_INTERNET_CONNECT_HANDLE_OBJECT(
	   DWORD addr
    )
{
	BOOL b;
	char block[ sizeof( INTERNET_CONNECT_HANDLE_OBJECT ) ];

	INTERNET_CONNECT_HANDLE_OBJECT * obj  = (INTERNET_CONNECT_HANDLE_OBJECT *) &block;

    b = GetData(
			addr, 
			block, 
			sizeof( INTERNET_CONNECT_HANDLE_OBJECT ), 
			NULL);

	if ( !b ) {
		d_printf("couldn't read INTERNET_CONNECT_HANDLE_OBJECT at 0x%x; sorry.\n", addr);
		return;
	}


	d_printf("INTERNET_CONNECT_HANDLE_OBJECT @ 0x%x \n\n", addr);

	//HINTERNET  _InternetConnectHandle
	d_printf("\tHINTERNET _InternetConnectHandle 0X%X\n", obj -> _InternetConnectHandle);

	//BOOL  _IsCopy
	d_printf("\tBOOL _IsCopy %d\n", obj -> _IsCopy);

	//DWORD  _ServiceType
	d_printf("\tDWORD _ServiceType %d\n", obj -> _ServiceType);

	//HINTERNET_HANDLE_TYPE  _HandleType
	d_printf("\tHINTERNET_HANDLE_TYPE _HandleType %d\n", obj -> _HandleType);

	//BOOL  _Flags
	d_printf("\tBOOL _Flags %d\n", obj -> _Flags);

	//BOOL  _InUse
	d_printf("\tBOOL _InUse %d\n", obj -> _InUse);

	//BOOL  _fDeleteDataFile
	d_printf("\tBOOL _fDeleteDataFile %d\n", obj -> _fDeleteDataFile);

	//LPSTR  _CacheCWD
	d_printf("\tLPSTR _CacheCWD %s\n", obj -> _CacheCWD);

	//LPSTR  _CacheUrlName
	d_printf("\tLPSTR _CacheUrlName %s\n", obj -> _CacheUrlName);

	//XSTRING  _xsPrimaryCacheKey
	d_printf("\tXSTRING (*) _xsPrimaryCacheKey 0x%x\n", OFFSET( INTERNET_CONNECT_HANDLE_OBJECT, _xsPrimaryCacheKey) );

	//XSTRING  _xsSecondaryCacheKey
	d_printf("\tXSTRING (*) _xsSecondaryCacheKey 0x%x\n", OFFSET( INTERNET_CONNECT_HANDLE_OBJECT, _xsSecondaryCacheKey) );

	//LPSTR  _CacheFileName
	d_printf("\tLPSTR _CacheFileName %s\n", obj -> _CacheFileName);

	//DWORD  _CacheHeaderLength
	d_printf("\tDWORD _CacheHeaderLength %d\n", obj -> _CacheHeaderLength);

	//BOOL  _CacheReadInProgress
	d_printf("\tBOOL _CacheReadInProgress %d\n", obj -> _CacheReadInProgress);

	//BOOL  _CacheWriteInProgress
	d_printf("\tBOOL _CacheWriteInProgress %d\n", obj -> _CacheWriteInProgress);

	//DWORD  _RealCacheFileSize
	d_printf("\tDWORD _RealCacheFileSize %d\n", obj -> _RealCacheFileSize);

	//DWORD  _VirtualCacheFileSize
	d_printf("\tDWORD _VirtualCacheFileSize %d\n", obj -> _VirtualCacheFileSize);

#ifdef LAZY_WRITE
	//LPBYTE  _CacheScratchBuf
	d_printf("\tLPBYTE _CacheScratchBuf 0X%X\n", obj -> _CacheScratchBuf);

	//DWORD  _CacheScratchBufLen
	d_printf("\tDWORD _CacheScratchBufLen %d\n", obj -> _CacheScratchBufLen);

	//DWORD  _CacheScratchUsedLen
	d_printf("\tDWORD _CacheScratchUsedLen %d\n", obj -> _CacheScratchUsedLen);

#endif
	//DWORD  _dwCacheFlags
	d_printf("\tDWORD _dwCacheFlags %d\n", obj -> _dwCacheFlags);

	//DWORD  _dwStreamRefCount
	d_printf("\tDWORD _dwStreamRefCount %d\n", obj -> _dwStreamRefCount);

	//DWORD  _dwCurrentStreamPosition
	d_printf("\tDWORD _dwCurrentStreamPosition %d\n", obj -> _dwCurrentStreamPosition);

	//BOOL  _fFromCache
	d_printf("\tBOOL _fFromCache %d\n", obj -> _fFromCache);

	//BOOL  _fCacheWriteDisabled
	d_printf("\tBOOL _fCacheWriteDisabled %d\n", obj -> _fCacheWriteDisabled);

	//BOOL  _fIsHtmlFind
	d_printf("\tBOOL _fIsHtmlFind %d\n", obj -> _fIsHtmlFind);

	//BOOL  _CacheCopy
	d_printf("\tBOOL _CacheCopy %d\n", obj -> _CacheCopy);

	//BOOL  _CachePerUserItem
	d_printf("\tBOOL _CachePerUserItem %d\n", obj -> _CachePerUserItem);

	//BOOL  _fForcedExpiry
	d_printf("\tBOOL _fForcedExpiry %d\n", obj -> _fForcedExpiry);

	//BOOL  _fLazyUpdate
	d_printf("\tBOOL _fLazyUpdate %d\n", obj -> _fLazyUpdate);

	//LPSTR  _OriginalUrl
	d_printf("\tLPSTR _OriginalUrl %s\n", obj -> _OriginalUrl);

#ifdef LAZY_WRITE
#endif // LAZY_WRITE
	//DWORD  _ReadBufferSize
	d_printf("\tDWORD _ReadBufferSize %d\n", obj -> _ReadBufferSize);

	//DWORD  _WriteBufferSize
	d_printf("\tDWORD _WriteBufferSize %d\n", obj -> _WriteBufferSize);

	//ICSTRING  _HostName
	d_printf("\tICSTRING (*) _HostName 0x%x\n", OFFSET( INTERNET_CONNECT_HANDLE_OBJECT, _HostName) );

	//XSTRING  _xsUser
	d_printf("\tXSTRING (*) _xsUser 0x%x\n", OFFSET( INTERNET_CONNECT_HANDLE_OBJECT, _xsUser) );

	//XSTRING  _xsPass
	d_printf("\tXSTRING (*) _xsPass 0x%x\n", OFFSET( INTERNET_CONNECT_HANDLE_OBJECT, _xsPass) );

	//XSTRING  _xsProxyUser
	d_printf("\tXSTRING (*) _xsProxyUser 0x%x\n", OFFSET( INTERNET_CONNECT_HANDLE_OBJECT, _xsProxyUser) );

	//XSTRING  _xsProxyPass
	d_printf("\tXSTRING (*) _xsProxyPass 0x%x\n", OFFSET( INTERNET_CONNECT_HANDLE_OBJECT, _xsProxyPass) );

	//INTERNET_PORT  _HostPort
	d_printf("\tINTERNET_PORT _HostPort %d\n", obj -> _HostPort);

	//INTERNET_SCHEME  _SchemeType
	d_printf("\tINTERNET_SCHEME _SchemeType %d\n", obj -> _SchemeType);

	//LPSTR  _LastResponseInfo
	d_printf("\tLPSTR _LastResponseInfo %s\n", obj -> _LastResponseInfo);

	//DWORD  _LastResponseInfoLength
	d_printf("\tDWORD _LastResponseInfoLength %d\n", obj -> _LastResponseInfoLength);

	//BOOL  _bViaProxy
	d_printf("\tBOOL _bViaProxy %d\n", obj -> _bViaProxy);

	//BOOL  _bNoHeaders
	d_printf("\tBOOL _bNoHeaders %d\n", obj -> _bNoHeaders);

	//BOOL  _bNetFailed
	d_printf("\tBOOL _bNetFailed %d\n", obj -> _bNetFailed);



	d_printf("\n");

}  // INTERNET_CONNECT_HANDLE_OBJECT


///////////////////////////////////
//
// Dump the LIST_ENTRY  structure
//
///////////////////////////////////
VOID
do_list_entry(
	   DWORD addr
    )
{
	BOOL b;
	char block[ sizeof( LIST_ENTRY ) ];

	LIST_ENTRY * obj  = (LIST_ENTRY *) &block;

    b = GetData(
			addr, 
			block, 
			sizeof( LIST_ENTRY ), 
			NULL);

	if ( !b ) {
		d_printf("couldn't read LIST_ENTRY at 0x%x; sorry.\n", addr);
		return;
	}


	d_printf("LIST_ENTRY @ 0x%x \n\n", addr);

	//struct _LIST_ENTRY * Flink
	d_printf("\tstruct _LIST_ENTRY * Flink = 0x%X\n", obj -> Flink);

	//struct _LIST_ENTRY * Blink
	d_printf("\tstruct _LIST_ENTRY * Blink = 0x%X\n", obj -> Blink);


	d_printf("\n");

}

DECLARE_API( lste )
{
    DWORD dwAddr;

    INIT_DPRINTF();

    dwAddr = d_GetExpression(lpArgumentString);
    if ( !dwAddr )
        {
        return;
        }
   do_list_entry(dwAddr);
}

///////////////////////////////////
//
// Dump the SERIALIZED_LIST structure
//
///////////////////////////////////
VOID
do_serialized_list(
	   DWORD addr
    )
{
	BOOL b;
	char block[ sizeof( SERIALIZED_LIST ) ];

	SERIALIZED_LIST * obj  = (SERIALIZED_LIST *) &block;

    b = GetData(
			addr, 
			block, 
			sizeof( SERIALIZED_LIST ), 
			NULL);

	if ( !b ) {
		d_printf("couldn't read SERIALIZED_LIST at 0x%x; sorry.\n", addr);
		return;
	}


	d_printf("SERIALIZED_LIST @ 0x%x \n\n", addr);

#if INET_DEBUG
	//DWORD  Signature
	d_printf("\tDWORD Signature %d\n", obj -> Signature);

	//RESOURCE_INFO  ResourceInfo
	d_printf("\tRESOURCE_INFO (*) ResourceInfo 0X%X\n", 
				((DWORD)addr + offsetof(SERIALIZED_LIST, ResourceInfo)) 
			);

	//LONG  LockCount
	d_printf("\tLONG LockCount %d\n", obj -> LockCount);

#endif // INET_DEBUG

	//LIST_ENTRY  List
	d_printf("\tLIST_ENTRY (*) List 0X%X\n",
				((DWORD)addr + offsetof(SERIALIZED_LIST, List)) 
			);

	//do_list_entry( (DWORD) &obj->List );

	//LONG  ElementCount
	d_printf("\tLONG ElementCount %ld\n", obj -> ElementCount);



	d_printf("\n");

}

DECLARE_API( serialist )
{
    DWORD dwAddr;

    INIT_DPRINTF();

    dwAddr = d_GetExpression(lpArgumentString);
    if ( !dwAddr )
        {
        return;
        }
   do_serialized_list(dwAddr);
}

/////////////////////////////////////////////////
//
//	PROXY_INFO structure
//
/////////////////////////////////////////////////
VOID
do_proxy_info(
	   DWORD addr
    )
{
	BOOL b;
	char block[ sizeof( PROXY_INFO ) ];

	PROXY_INFO * obj  = (PROXY_INFO *) &block;

    b = GetData(
			addr, 
			block, 
			sizeof( PROXY_INFO ), 
			NULL);

	if ( !b ) {
		d_printf("couldn't read PROXY_INFO at 0x%x; sorry.\n", addr);
		return;
	}


	d_printf("PROXY_INFO @ 0x%x \n\n", addr);

	//PROXY_SERVER_LIST * _ProxyServerList
	d_printf("\tPROXY_SERVER_LIST * _ProxyServerList = 0X%X\n",obj -> _ProxyServerList);

	//AUTO_PROXY_DLLS * _AutoProxyList
	d_printf("\tAUTO_PROXY_DLLS * _AutoProxyList = 0X%X\n", obj -> _AutoProxyList);

	//PROXY_BYPASS_LIST * _ProxyBypassList
	d_printf("\tPROXY_BYPASS_LIST * _ProxyBypassList = 0X%X\n", obj -> _ProxyBypassList);

	//DWORD  _Error
	d_printf("\tDWORD _Error %d\n", obj -> _Error);

	//RESOURCE_LOCK  _Lock
	d_printf("\tRESOURCE_LOCK (*) _Lock 0X%X\n", OFFSET(PROXY_INFO, _Lock));

	//BOOL  _Modified
	d_printf("\tBOOL _Modified %d\n", obj -> _Modified);

	//BAD_PROXY_LIST  _BadProxyList
	d_printf("\tBAD_PROXY_LIST _BadProxyList 0X%X\n", OFFSET(PROXY_INFO, _BadProxyList));

	d_printf("\n");

}

DECLARE_API( proxyinfo )
{
    DWORD dwAddr;

    INIT_DPRINTF();

    dwAddr = d_GetExpression(lpArgumentString);
    if ( !dwAddr )
        {
        return;
        }
   do_proxy_info(dwAddr);
}


/////////////////////////////////////////////////
//
//	PROXY_BYPASS_LIST_ENTRY structure
//
/////////////////////////////////////////////////

DECLARE_API( proxybyplste )
{
    DWORD dwAddr;

    INIT_DPRINTF();

    dwAddr = d_GetExpression(lpArgumentString);
    if ( !dwAddr )
        {
        return;
        }
   do_PROXY_BYPASS_LIST_ENTRY(dwAddr);
}

VOID
do_PROXY_BYPASS_LIST_ENTRY(
	   DWORD addr
    )
{
	BOOL b;
	char block[ sizeof( PROXY_BYPASS_LIST_ENTRY ) ];

	PROXY_BYPASS_LIST_ENTRY * obj  = (PROXY_BYPASS_LIST_ENTRY *) &block;

    b = GetData(
			addr, 
			block, 
			sizeof( PROXY_BYPASS_LIST_ENTRY ), 
			NULL);

	if ( !b ) {
		d_printf("couldn't read PROXY_BYPASS_LIST_ENTRY at 0x%x; sorry.\n", addr);
		return;
	}


	d_printf("PROXY_BYPASS_LIST_ENTRY @ 0x%x \n\n", addr);

	//LIST_ENTRY  _List
	d_printf("\tLIST_ENTRY (*) _List 0x%x\n", OFFSET( PROXY_BYPASS_LIST_ENTRY, _List) );

	//INTERNET_SCHEME  _Scheme
	d_printf("\tINTERNET_SCHEME _Scheme %d\n", obj -> _Scheme);

	//ICSTRING  _Name
	d_printf("\tICSTRING (*) _Name 0x%x\n", OFFSET( PROXY_BYPASS_LIST_ENTRY, _Name) );

	//INTERNET_PORT  _Port
	d_printf("\tINTERNET_PORT _Port %d\n", obj -> _Port);

	//BOOL  _LocalSemantics
	d_printf("\tBOOL _LocalSemantics %d\n", obj -> _LocalSemantics);



	d_printf("\n");

}  // PROXY_BYPASS_LIST_ENTRY



/////////////////////////////////////////////////
//
//	PROXY_BYPASS_LIST structure
//
/////////////////////////////////////////////////

DECLARE_API( proxybyplst )
{
    DWORD dwAddr;

    INIT_DPRINTF();

    dwAddr = d_GetExpression(lpArgumentString);
    if ( !dwAddr )
        {
        return;
        }
   do_PROXY_BYPASS_LIST(dwAddr);
}

VOID
do_PROXY_BYPASS_LIST(
	   DWORD addr
    )
{
	BOOL b;
	char block[ sizeof( PROXY_BYPASS_LIST ) ];

	PROXY_BYPASS_LIST * obj  = (PROXY_BYPASS_LIST *) &block;

    b = GetData(
			addr, 
			block, 
			sizeof( PROXY_BYPASS_LIST ), 
			NULL);

	if ( !b ) {
		d_printf("couldn't read PROXY_BYPASS_LIST at 0x%x; sorry.\n", addr);
		return;
	}


	d_printf("PROXY_BYPASS_LIST @ 0x%x \n\n", addr);

	//SERIALIZED_LIST  _List
	d_printf("\tSERIALIZED_LIST (*) _List 0x%x\n", OFFSET( PROXY_BYPASS_LIST, _List) );

	//DWORD  _Error
	d_printf("\tDWORD _Error %d\n", obj -> _Error);



	d_printf("\n");

}  // PROXY_BYPASS_LIST



/////////////////////////////////////////////////
//
//	PROXY_SERVER_LIST_ENTRY structure
//
/////////////////////////////////////////////////

DECLARE_API( proxysrvlste )
{
    DWORD dwAddr;

    INIT_DPRINTF();

    dwAddr = d_GetExpression(lpArgumentString);
    if ( !dwAddr )
        {
        return;
        }
   do_PROXY_SERVER_LIST_ENTRY(dwAddr);
}

VOID
do_PROXY_SERVER_LIST_ENTRY(
	   DWORD addr
    )
{
	BOOL b;
	char block[ sizeof( PROXY_SERVER_LIST_ENTRY ) ];

	PROXY_SERVER_LIST_ENTRY * obj  = (PROXY_SERVER_LIST_ENTRY *) &block;

    b = GetData(
			addr, 
			block, 
			sizeof( PROXY_SERVER_LIST_ENTRY ), 
			NULL);

	if ( !b ) {
		d_printf("couldn't read PROXY_SERVER_LIST_ENTRY at 0x%x; sorry.\n", addr);
		return;
	}


	d_printf("PROXY_SERVER_LIST_ENTRY @ 0x%x \n\n", addr);

	//LIST_ENTRY  _List
	d_printf("\tLIST_ENTRY (*) _List 0x%x\n", OFFSET( PROXY_SERVER_LIST_ENTRY, _List) );

	//INTERNET_SCHEME  _Protocol
	d_printf("\tINTERNET_SCHEME _Protocol %d\n", obj -> _Protocol);

	//INTERNET_SCHEME  _Scheme
	d_printf("\tINTERNET_SCHEME _Scheme %d\n", obj -> _Scheme);

	//ICSTRING  _ProxyName
	d_printf("\tICSTRING (*) _ProxyName 0x%x\n", OFFSET( PROXY_SERVER_LIST_ENTRY, _ProxyName) );

	//INTERNET_PORT  _ProxyPort
	d_printf("\tINTERNET_PORT _ProxyPort %d\n", obj -> _ProxyPort);


	d_printf("\n");

}  // PROXY_SERVER_LIST_ENTRY



/////////////////////////////////////////////////
//
//	PROXY_SERVER_LIST structure
//
/////////////////////////////////////////////////

DECLARE_API( proxysrvlst )
{
    DWORD dwAddr;

    INIT_DPRINTF();

    dwAddr = d_GetExpression(lpArgumentString);
    if ( !dwAddr )
        {
        return;
        }
   do_PROXY_SERVER_LIST(dwAddr);
}

VOID
do_PROXY_SERVER_LIST(
	   DWORD addr
    )
{
	BOOL b;
	char block[ sizeof( PROXY_SERVER_LIST ) ];

	PROXY_SERVER_LIST * obj  = (PROXY_SERVER_LIST *) &block;

    b = GetData(
			addr, 
			block, 
			sizeof( PROXY_SERVER_LIST ), 
			NULL);

	if ( !b ) {
		d_printf("couldn't read PROXY_SERVER_LIST at 0x%x; sorry.\n", addr);
		return;
	}


	d_printf("PROXY_SERVER_LIST @ 0x%x \n\n", addr);

	//SERIALIZED_LIST  _List
	d_printf("\tSERIALIZED_LIST (*) _List 0x%x\n", OFFSET( PROXY_SERVER_LIST, _List) );

	//DWORD  _Error
	d_printf("\tDWORD _Error %d\n", obj -> _Error);


	d_printf("\n");

}  // PROXY_SERVER_LIST

/////////////////////////////////////////////////
//
//      ICSocket structure
//
/////////////////////////////////////////////////

DECLARE_API( ICSocket )
{
    DWORD dwAddr;

    INIT_DPRINTF();

    dwAddr = d_GetExpression(lpArgumentString);
    if ( !dwAddr )
        {
        return;
        }
   do_ICSocket(dwAddr);
}

VOID
do_ICSocket(
           DWORD addr
    )
{
        BOOL b;
        char block[ sizeof( ICSocket ) ];

        ICSocket * obj  = (ICSocket *) &block;

    b = GetData(
                        addr,
                        block,
                        sizeof( ICSocket ),
                        NULL);

        if ( !b ) {
                d_printf("couldn't read ICSocket at 0x%x; sorry.\n", addr);
                return;
        }


        d_printf("ICSocket @ 0x%x \n\n", addr);

        //LIST_ENTRY  m_List
        d_printf("\tLIST_ENTRY (*) m_List 0x%x\n", OFFSET( ICSocket, m_List) );

        //DWORD  m_dwTimeout
        d_printf("\tDWORD m_dwTimeout %d\n", obj -> m_dwTimeout);

        //LONG  m_ReferenceCount
        d_printf("\tLONG m_ReferenceCount %d\n", obj -> m_ReferenceCount);

        //DWORD  m_dwFlags
        d_printf("\tDWORD m_dwFlags %d\n", obj -> m_dwFlags);

	//    SOCKET m_Socket;
	d_printf("\tSOCKET m_Socket %d\n", obj -> m_Socket);

        //INTERNET_PORT  m_Port
        d_printf("\tINTERNET_PORT m_Port %d\n", obj -> m_Port);

        //INTERNET_PORT  m_SourcePort
        d_printf("\tINTERNET_PORT m_SourcePort %d\n", obj -> m_SourcePort);

        //BOOL  m_bAborted
        d_printf("\tBOOL m_bAborted %d\n", obj -> m_bAborted);

        //DWORD  m_SocksAddress
        d_printf("\tDWORD m_SocksAddress %d\n", obj -> m_SocksAddress);

        //INTERNET_PORT  m_SocksPort
        d_printf("\tINTERNET_PORT m_SocksPort %d\n", obj -> m_SocksPort);

#if INET_DEBUG
        //DWORD  m_Signature
        d_printf("\tDWORD m_Signature %d\n", obj -> m_Signature);

#endif


        d_printf("\n");

}  // ICSocket

/////////////////////////////////////////////////
//
//      ICSecureSocket structure
//
/////////////////////////////////////////////////

DECLARE_API( ICSecureSocket )
{
    DWORD dwAddr;

    INIT_DPRINTF();

    dwAddr = d_GetExpression(lpArgumentString);
    if ( !dwAddr )
        {
        return;
        }
   do_ICSecureSocket(dwAddr);
}

VOID
do_ICSecureSocket(
           DWORD addr
    )
{
        BOOL b;
        char block[ sizeof( ICSecureSocket ) ];

        ICSecureSocket * obj  = (ICSecureSocket *) &block;

	b = GetData(
                        addr,
                        block,
                        sizeof( ICSecureSocket ),
                        NULL);

        if ( !b ) {
                d_printf("couldn't read ICSecureSocket at 0x%x; sorry.\n", addr);
                return;
        }


        d_printf("ICSecureSocket @ 0x%x \n\n", addr);

        //CtxtHandle  m_hContext
        d_printf("\tCtxtHandle (*) m_hContext 0x%x\n", OFFSET( ICSecureSocket, m_hContext) );

        //DWORD  m_dwProviderIndex
        d_printf("\tDWORD m_dwProviderIndex %d\n", obj -> m_dwProviderIndex);

        //LPSTR  m_lpszHostName
        d_printf("\tLPSTR m_lpszHostName %s\n", obj -> m_lpszHostName);

        //DBLBUFFER * m_pdblbufBuffer
        d_printf("\tDBLBUFFER * m_pdblbufBuffer 0x%x\n", obj -> m_pdblbufBuffer);

        //DWORD  m_dwErrorFlags
        d_printf("\tDWORD m_dwErrorFlags %d\n", obj -> m_dwErrorFlags);

        //SECURITY_CACHE_LIST_ENTRY * m_pSecurityInfo
        d_printf("\tSECURITY_CACHE_LIST_ENTRY * m_pSecurityInfo 0x%x\n", obj -> m_pSecurityInfo);

#if INET_DEBUG
#endif


        d_printf("\n");

}  // ICSecureSocket



#if INET_DEBUG

LPSTR
MapType(
    FSM_TYPE m_Type
    ) {
    switch (m_Type) {
    case FSM_TYPE_NONE:                     return "NONE";
    case FSM_TYPE_WAIT_FOR_COMPLETION:      return "WAIT_FOR_COMPLETION";
    case FSM_TYPE_RESOLVE_HOST:             return "RESOLVE_HOST";
    case FSM_TYPE_SOCKET_CONNECT:           return "SOCKET_CONNECT";
    case FSM_TYPE_SOCKET_SEND:              return "SOCKET_SEND";
    case FSM_TYPE_SOCKET_RECEIVE:           return "SOCKET_RECEIVE";
    case FSM_TYPE_SOCKET_QUERY_AVAILABLE:   return "SOCKET_QUERY_AVAILABLE";
    case FSM_TYPE_SECURE_CONNECT:           return "SECURE_CONNECT";
    case FSM_TYPE_SECURE_HANDSHAKE:         return "SECURE_HANDSHAKE";
    case FSM_TYPE_SECURE_NEGOTIATE:         return "SECURE_NEGOTIATE";
    case FSM_TYPE_NEGOTIATE_LOOP:           return "NEGOTIATE_LOOP";
    case FSM_TYPE_SECURE_SEND:              return "SECURE_SEND";
    case FSM_TYPE_SECURE_RECEIVE:           return "SECURE_RECEIVE";
    case FSM_TYPE_GET_CONNECTION:           return "GET_CONNECTION";
    case FSM_TYPE_HTTP_SEND_REQUEST:        return "HTTP_SEND_REQUEST";
    case FSM_TYPE_MAKE_CONNECTION:          return "MAKE_CONNECTION";
    case FSM_TYPE_OPEN_CONNECTION:          return "OPEN_CONNECTION";
    case FSM_TYPE_OPEN_PROXY_TUNNEL:        return "OPEN_PROXY_TUNNEL";
    case FSM_TYPE_SEND_REQUEST:             return "SEND_REQUEST";
    case FSM_TYPE_RECEIVE_RESPONSE:         return "RECEIVE_RESPONSE";
    case FSM_TYPE_HTTP_READ:                return "HTTP_READ";
    case FSM_TYPE_HTTP_WRITE:               return "HTTP_WRITE";
    case FSM_TYPE_READ_DATA:                return "READ_DATA";
    case FSM_TYPE_HTTP_QUERY_AVAILABLE:     return "HTTP_QUERY_AVAILABLE";
    case FSM_TYPE_DRAIN_RESPONSE:           return "DRAIN_RESPONSE";
    case FSM_TYPE_REDIRECT:                 return "REDIRECT";
    case FSM_TYPE_READ_LOOP:                return "READ_LOOP";
    case FSM_TYPE_PARSE_HTTP_URL:           return "PARSE_HTTP_URL";
    case FSM_TYPE_PARSE_URL_FOR_HTTP:       return "PARSE_URL_FOR_HTTP";
    case FSM_TYPE_READ_FILE:                return "READ_FILE";
    case FSM_TYPE_READ_FILE_EX:             return "READ_FILE_EX";
    case FSM_TYPE_WRITE_FILE:               return "WRITE_FILE";
    case FSM_TYPE_QUERY_DATA_AVAILABLE:     return "QUERY_DATA_AVAILABLE";
    }
    return "?";
}

#endif // INET_DEBUG

LPSTR
MapState(
    IN DWORD State
    ) {
    switch (State) {
    CASE_OF(FSM_STATE_BAD);
    CASE_OF(FSM_STATE_INIT);
    CASE_OF(FSM_STATE_WAIT);
    CASE_OF(FSM_STATE_DONE);
    CASE_OF(FSM_STATE_ERROR);
    CASE_OF(FSM_STATE_CONTINUE);
    CASE_OF(FSM_STATE_FINISH);
    CASE_OF(FSM_STATE_1);
    CASE_OF(FSM_STATE_2);
    CASE_OF(FSM_STATE_3);
    CASE_OF(FSM_STATE_4);
    CASE_OF(FSM_STATE_5);
    CASE_OF(FSM_STATE_6);
    CASE_OF(FSM_STATE_7);
    CASE_OF(FSM_STATE_8);
    CASE_OF(FSM_STATE_9);
    CASE_OF(FSM_STATE_10);
    }
    return "?";
}

