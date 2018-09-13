/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    tlser.c

Abstract:

    This module contains the code for the serial transport layer.

Author:

    Wesley Witt (wesw)    25-Nov-93

Environment:

    Win32 User

--*/

#include        <windows.h>
#include        <stdio.h>
#include        <stdlib.h>

#include "tchar.h"

#include "cvinfo.h"

#include "odtypes.h"
#include "od.h"
#include "odp.h"
#include "odassert.h"
#include "emdm.h"

#include        "xport.h"
#include        "tlgambit.h"

TLIS Tlis = {
    TRUE,                // fCanSetup
    0xffffffff,           // dwMaxPacket
    0xffffffff,           // dwOptPacket
    TLISINFOSIZE,         // dwInfoSize ?? what is this for ??
    TRUE,                 // fRemote
#if defined(_M_IX86)
    mptix86,              // mpt
    -1,                   // mptRemote
#elif defined(_M_MRX000)
    mptmips,              // mpt
    -1,                   // mptRemote
#elif defined(_M_IA64)
    mptia64,              // mpt
    -1,                   // mptRemote
#elif defined(_M_ALPHA)
    mptdaxp,              // mpt
    -1,                   // mptRemote
#elif defined(_M_PPC)
    mptntppc,
    -1,
#else
#error( "unknown target machine" );
#endif
    {  "EIA Serial Debugger Transport" } // rgchInfo
};

LPTLIS
TlGetInfo(
    VOID
    )
{
    return &Tlis;
}

#define SIZE_OF_QUEUE           100

extern CRITICAL_SECTION csExpecting;
BOOL            FDestroyTransport;

BOOL            FVerbose = TRUE;
HANDLE          HReadThread;
BOOL            FDMSide = FALSE;

struct {
    char *      lpb;
    int         cb;
}               RgQueue[SIZE_OF_QUEUE];
int             IQueueFront = 0;
int             IQueueBack = 0;
CRITICAL_SECTION CsQueue = {0};
CRITICAL_SECTION CsSerial = {0};
HANDLE          HQueueEvent;
HANDLE          HCallbackThread;
HANDLE          TlComPort = INVALID_HANDLE_VALUE;
DWORD           TlBaudRate;
extern BOOL FConnected;

#define PARAM_KEY   "Parameters"
CHAR            SzGlobalParams[MAX_PATH];

REPLY            RgReplys[SIZE_OF_REPLYS];
CRITICAL_SECTION CsReplys;
int              IReplys;

char *  RgSzTypes[] = {"FirstAsync", "Async", "FirstReply", "Reply",
                       "Disconnect", "VersionReqest", "VersionReply"};
char * SzTypes(unsigned int i)
{
    static char rgch[20];
    if (i > sizeof(RgSzTypes)/sizeof(RgSzTypes[0])) {
        sprintf(rgch, "Type %x", i);
        return rgch;
    } else {
        return RgSzTypes[i];
    }
}

void
TlSetRemoteStatus(
    MPT mpt
    )
{
    Tlis.mptRemote = mpt;
    FDMSide = TRUE;
}


#define TL_ERROR_LOGGING 1


#ifdef TL_ERROR_LOGGING

typedef struct {
    DWORD   ty;
    DWORD   ec;
    DWORD   cb;
    DWORD   ln;
    DWORD   td;
    LPDWORD ob;
    LPDWORD p;
} ERRLOG;

#define LOGIT(x,y,z,q)      {el[ei].ty=x;el[ei].ec=y;el[ei].cb=z;el[ei].ln=__LINE__; \
                             el[ei].td=GetCurrentThreadId(); \
                             el[ei].ob=(LPDWORD)q; \
                             el[ei].p=(LPDWORD)malloc(z);memcpy(el[ei].p,q,z);ei++; \
                             if (ei==99) ei=0;}
#define LGREAD  1
#define LGWRITE 2
ERRLOG  el[100];
DWORD   ei=0;

#if DBG
void printel( void )
{
    DWORD i;

    for (i=0; i<ei; i++) {
        DebugPrint( "%d\t%d\t%x\t%d\t%08x\t%08x\t%x\n",
                    el[i].ty,
                    el[i].ec,
                    el[i].cb,
                    el[i].ln,
                    el[i].p,
                    el[i].ob,
                    el[i].td
                  );
    }
}
#endif

#else

#define LOGIT(x,y,z,q)
#define LGREAD  1
#define LGWRITE 2

#endif

DWORD   ReaderThread(LPVOID arg);
DWORD   CallbackThread(LPVOID lpvArg);

#ifdef DEBUGVER
DEBUG_VERSION('T','L',"Serial Transport Layer (Debug)")
#else
RELEASE_VERSION('T','L',"Serial Transport Layer")
#endif

DBGVERSIONCHECK()


BOOL
CreateStuff(
    VOID
    )
{
    int         i;

    FDestroyTransport = FALSE;

    
	if (FDMSide && HReadThread) {
        return TRUE;
    }

    //
    // Create random data strutures needed internally
    //

    InitializeCriticalSection( &CsQueue );
    InitializeCriticalSection( &CsSerial );

    HQueueEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    InitializeCriticalSection( &CsReplys );
    for (i=0; i<SIZE_OF_REPLYS; i++) {
        RgReplys[i].hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    }

#if DBG
    InitializeCriticalSection(&csExpecting);
#endif

    return TRUE;
}


BOOL
StartWorkerThreads(
    VOID
    )
{
    DWORD       id;

    //
    // start the debugger reader thread
    //
    HReadThread = CreateThread(NULL, 0, ReaderThread, 0, 0, &id);
    if (!HReadThread) {
        FDestroyTransport = TRUE;
        return FALSE;
    }
    //
    // start the callback thread
    //
    HCallbackThread = CreateThread(NULL, 0, CallbackThread, 0, 0, &id);
    if (!HCallbackThread) {
        FDestroyTransport = TRUE;
        WaitForSingleObject(HReadThread, INFINITE);
        CloseHandle(HReadThread);
        HReadThread = NULL;
        return FALSE;
    }

	SetThreadPriority( HReadThread, THREAD_PRIORITY_ABOVE_NORMAL );
	SetThreadPriority( HCallbackThread, THREAD_PRIORITY_ABOVE_NORMAL );
	TlRegisterWorkerThread(HReadThread);
	TlRegisterWorkerThread(HCallbackThread);
    return TRUE;
}


VOID
DestroyStuff(
    VOID
    )
{
    int         i;

    //
    //  If there is a reader thread -- then wait for it to be termianted
    //  and close the handle
    //
    FDestroyTransport = TRUE;
    if (HReadThread) {
        WaitForSingleObject(HReadThread, INFINITE);
        CloseHandle(HReadThread);
        HReadThread = NULL;
    }

    if (HCallbackThread) {
        SetEvent(HQueueEvent);
        WaitForSingleObject(HCallbackThread, INFINITE);
        CloseHandle(HCallbackThread);
        HCallbackThread = NULL;
    }

    //
    //  Now delete all of the objects
    //
    CloseHandle(HQueueEvent);
    DeleteCriticalSection(&CsSerial);
    DeleteCriticalSection(&CsQueue);
    DeleteCriticalSection(&CsReplys);
    for (i=0; i<SIZE_OF_REPLYS; i++) {
        CloseHandle(RgReplys[i].hEvent);
    }

#if DBG
    DeleteCriticalSection(&csExpecting);
#endif

	FreeConsole();

    return;
}


XOSD
TlCreateTransport(   LPSTR szParams
    )

/*++

Routine Description:

    This function creates the connection to windbgrm (server).

Arguments:

    szParams - Supplies the parameters

Return Value:

    XOSD error code.

--*/

{
    DCB           LocalDcb;
    COMMTIMEOUTS  To;

	DEBUG_OUT("Creating transport\n");
    if (TlComPort != INVALID_HANDLE_VALUE) {
        return xosdNone;
    }
#if defined(_M_IA64)//v-vadimp - gambit side

    TlComPort = CreateFile( "COM1:",GENERIC_READ | GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

    if (TlComPort == INVALID_HANDLE_VALUE || TlComPort == NULL) {
		DEBUG_OUT("Cannot open Com port\n");
        return xosdUnknown;
    }

    SetupComm( TlComPort, 4096, 4096);


    if (!GetCommState( TlComPort, &LocalDcb )) {
        CloseHandle( TlComPort );
		DEBUG_OUT("Failed on GetCommState\n");
        return xosdUnknown;
    }

    LocalDcb.BaudRate     = 115200;
    LocalDcb.ByteSize     = 8;
    LocalDcb.Parity       = NOPARITY;
    LocalDcb.StopBits     = ONESTOPBIT;
    LocalDcb.fDtrControl  = DTR_CONTROL_ENABLE;
    LocalDcb.fRtsControl  = RTS_CONTROL_ENABLE;
    LocalDcb.fBinary      = TRUE;
    LocalDcb.fOutxCtsFlow = FALSE;
    LocalDcb.fOutxDsrFlow = FALSE;
    LocalDcb.fOutX        = FALSE;
    LocalDcb.fInX         = FALSE;

    if (!SetCommState( TlComPort, &LocalDcb )) {
        CloseHandle( TlComPort );
        return xosdUnknown;
    }

    //
    // Set the normal read and write timeout time.
    //
    To.ReadIntervalTimeout          = MAXDWORD;
    To.ReadTotalTimeoutMultiplier   = 0;
    To.ReadTotalTimeoutConstant     = 0; //4 * 1000;
    To.WriteTotalTimeoutMultiplier  = 0;
    To.WriteTotalTimeoutConstant    = 4 * 1000;

    if (!SetCommTimeouts( TlComPort, &To )) {
        CloseHandle( TlComPort );
		DEBUG_OUT("Failed on SetCommState\n");
        return xosdUnknown;
    }


	{
        //
        // Device could be locked up.  Clear it just in case.
        //
        DWORD   TrashErr;
        COMSTAT TrashStat;  
        ClearCommError( TlComPort, &TrashErr, &TrashStat );
    }

        FDMSide = TRUE;
#else
    {
        CHAR String[MAX_PATH];
        strcpy(String,"\\\\");
        strcat(String,SzGlobalParams);
        strcat(String,"\\pipe\\gambitpipe");
        TlComPort = CreateFile(String, GENERIC_READ | GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
    }

    if (TlComPort == INVALID_HANDLE_VALUE || TlComPort == NULL) {
		return xosdUnknown;
    }

	{
		DWORD dwMode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
        BYTE bAttempts = 0;
        BOOL rc = FALSE; 
        while ( !rc && bAttempts < 3) { //v-vadimp - this seems to not always be working the first time (pipe server?), wait and try a few more times 
            rc = SetNamedPipeHandleState(TlComPort, &dwMode ,NULL,NULL);
            Sleep (100);
            bAttempts++;
        }
        if ( !rc ) {
            return xosdCannotConnect;
        }
	}
#endif

    if (!CreateStuff()) {
        CloseHandle( TlComPort );
		DEBUG_OUT("Failed on CreateStuff\n");
        return xosdUnknown;
    }

    StartWorkerThreads();

    return xosdNone;
}


XOSD
TlConnectTransport(
    VOID
    )

/*++

Routine Description:

    This function attempts to connect the server to a client.

Arguments:

    None.

Return Value:

    XOSD error code.

--*/

{
    if (TlComPort && TlComPort != INVALID_HANDLE_VALUE) {
        FConnected = TRUE;
        return xosdNone;
    } else {
        return xosdCannotConnect;
    }
}


XOSD
TlDestroyTransport(
    VOID
    )
{
    DestroyStuff();
    CloseHandle( TlComPort );

    return xosdNone;
}


BOOL
TlDisconnectTransport(
    VOID
    )
{
    FConnected = FALSE;
	return TRUE;
}


BOOL
TlWriteTransport(
    PUCHAR   Buffer,
    DWORD    SizeOfBuffer
    )
{
    BOOL    rc;
    DWORD   TrashErr;
    COMSTAT TrashStat;
    DWORD   BytesWritten;
	DWORD i;


    EnterCriticalSection(&CsSerial);
    rc = WriteFile( TlComPort, Buffer, SizeOfBuffer, &BytesWritten, NULL );
	LeaveCriticalSection(&CsSerial);
    DEBUG_OUT3( "Write: Thread %x Requested %i bytes; written %i bytes\n", GetCurrentThreadId(),SizeOfBuffer, BytesWritten);
    //{ UINT i; for(i=0;i<BytesWritten;i++) DebugPrint("%02x ",*(Buffer+i));DebugPrint("\n");}
#if defined(_M_IA64) //only on Gambit side
    if (!rc) 
	{
        //
        // Device could be locked up.  Clear it just in case.
        //
        ClearCommError( TlComPort, &TrashErr, &TrashStat );
    }
#endif
    return rc;
}


BOOL
TlFlushTransport(
    VOID
    )
{
    return FlushFileBuffers( TlComPort );
}

DWORD
ReaderThread(
    LPVOID     lpvArg
    )
/*++

Routine Description:

    This is the main function for the reader thread in this transport layer.
    Its sole purpose is to pull things from the transport queue as fast
    as possible and place them into an internal queue.  This will prevent
    us from getting piled up in the network queue to fast.

Arguments:

    lpvArg  - Supplies the starting parameter -- which is ignored

Return Value:

    0 on a normal exit and -1 otherwise

--*/

{
    DWORD       bufSize;
    PNLBLK      pnlblk;
    DWORD       cb = 0;
    DWORD       cb2;
    DWORD       i;
    LPSTR       lpb;
    MPACKET *   pMpacket;
//  LPBYTE      b;
    BOOL        rc;
    DWORD       TrashErr;
    COMSTAT     TrashStat;
    DWORD       BytesReadNow, BytesRead = 0;
	HANDLE		hThread = GetCurrentThread();


    bufSize = MAX_INTERNAL_PACKET + sizeof(NLBLK) + sizeof(MPACKET);
    pnlblk = (PNLBLK) malloc( bufSize );
    pMpacket = (MPACKET *) pnlblk->rgchData;

    while (!FDestroyTransport) {
        //
        //  Read the next packet item from the network
        //
        ZeroMemory( (LPVOID)pnlblk, bufSize );


        //
        // read the block header
        //
		EnterCriticalSection(&CsSerial);
        rc = ReadFile( TlComPort, (LPVOID)pnlblk, sizeof(NLBLK), &BytesRead, NULL );
        LeaveCriticalSection(&CsSerial);

		if (!rc || BytesRead == 0) {
            Sleep( 50 );
            continue;
        }

        DEBUG_OUT4("NLBLK size Read: Requested %i bytes; read %i bytes pnlblk->cchMessage:%i,pnlblk->mtypeBlk:%i\n", 
                   sizeof(NLBLK), 
                   BytesRead,
                   pnlblk->cchMessage,
                   pnlblk->mtypeBlk 
                   );
        //DebugPrint("%i-%i-%i-%i-%i-%i-%i\n",sizeof(pnlblk),sizeof(pnlblk->hpid),sizeof(pnlblk->seq),sizeof(pnlblk->cchMessage),sizeof(pnlblk->mtypeBlk),sizeof(pnlblk->pad),sizeof(pnlblk->rgchData));
        //{ UINT i; for(i=0;i<BytesRead;i++) DebugPrint("%02x ",*((LPBYTE)(pnlblk)+i));DebugPrint("\n");}
        assert( BytesRead == sizeof(NLBLK) );

        if (pnlblk->cchMessage) {
			//
            // read the data
            //
			EnterCriticalSection(&CsSerial);
			BytesRead = 0;
			while ((WORD)pnlblk->cchMessage > BytesRead) {
				rc = ReadFile( TlComPort, (LPVOID)(&pnlblk->rgchData[BytesRead]),(pnlblk->cchMessage-BytesRead), &BytesReadNow, NULL );
				BytesRead+=BytesReadNow;
			}
			LeaveCriticalSection(&CsSerial);
            if (!rc) {
				cb = GetLastError();
                Sleep( 50 );
                continue;
            }
            DEBUG_OUT2("NLBLK Read:Requested %i bytes; read %i bytes:\n", pnlblk->cchMessage, BytesRead );
            //{ UINT i; for(i=0;i<BytesRead;i++) DebugPrint("%02x ",*((LPBYTE)(pnlblk->rgchData)+i));DebugPrint("\n");}
            assert( BytesRead == (DWORD)pnlblk->cchMessage );
            cb = pnlblk->cchMessage + sizeof(NLBLK);
        } else {
            cb = sizeof(NLBLK);
        }

        //
        //  Print a message about this packet type.
        //
        //DEBUG_OUT2("READER: %s %d\n", SzTypes(pnlblk->mtypeBlk), cb);
#if DBG
        //DebugPrint( "PACKET: %02x %d\n", pnlblk->mtypeBlk, pnlblk->cchMessage );
#endif
        if ((pnlblk->mtypeBlk == mtypeVersionReply) ||
            (pnlblk->mtypeBlk == mtypeReply) ||
            (pnlblk->mtypeBlk == mtypeLoadDMReply)) {
            EnterCriticalSection(&CsReplys);
            i = IReplys - 1;
            if (i != -1) {
                cb = min(pnlblk->cchMessage, RgReplys[i].cbBuffer);
                memcpy(RgReplys[i].lpb, (LPVOID)pnlblk->rgchData, cb);
                RgReplys[i].cbRet = cb;
                SetEvent(RgReplys[i].hEvent);
            }
            LeaveCriticalSection(&CsReplys);
            continue;
        }

        if (pnlblk->mtypeBlk == mtypeReplyMulti) {
            EnterCriticalSection( &CsReplys );
            i = IReplys - 1;
            if (i != -1) {
                cb2 = pMpacket->packetNum * MAX_INTERNAL_PACKET;
                cb = pnlblk->cchMessage - sizeof(MPACKET);
                cb = min( cb + cb2, (DWORD)RgReplys[i].cbBuffer );
                if (cb > cb2) {
                    memcpy( RgReplys[i].lpb + cb2, (LPVOID)pMpacket->rgchData, cb - cb2 );
                    RgReplys[i].cbRet = cb;
                }
                if (pMpacket->packetNum + 1 == pMpacket->packetCount) {
                    SetEvent( RgReplys[i].hEvent );
                }
            }
            LeaveCriticalSection( &CsReplys );
            continue;
        }

        lpb = malloc( cb );
        memcpy( lpb, pnlblk, cb );
        EnterCriticalSection( &CsQueue );
        while ((IQueueFront + 1) % SIZE_OF_QUEUE == IQueueBack) {
            LeaveCriticalSection( &CsQueue );
            Sleep(100);
            EnterCriticalSection( &CsQueue );
        }
        RgQueue[IQueueFront].lpb = lpb;
        RgQueue[IQueueFront].cb = cb;
        IQueueFront = (IQueueFront + 1) % SIZE_OF_QUEUE;
        SetEvent(HQueueEvent);
        LeaveCriticalSection( &CsQueue );
    }

	return 0;
}


DWORD
CallbackThread(
    LPVOID lpvArg
    )
{
    LPSTR       lpb;
    int         cb;

    while (!FDestroyTransport) {
        EnterCriticalSection( &CsQueue );
        if (IQueueFront == IQueueBack) {
            ResetEvent( HQueueEvent);
            LeaveCriticalSection( &CsQueue );
            WaitForSingleObject( HQueueEvent, INFINITE );
            if(FDestroyTransport)
            {
                break;
            }
            EnterCriticalSection( &CsQueue );
        }

        lpb = RgQueue[IQueueBack].lpb;
        cb = RgQueue[IQueueBack].cb;
        RgQueue[IQueueBack].lpb = NULL;
        RgQueue[IQueueBack].cb = 0;
        IQueueBack = (IQueueBack + 1) % SIZE_OF_QUEUE;
        LeaveCriticalSection( &CsQueue );

        if (!CallBack((PNLBLK) lpb, cb)) {

            if (!FDMSide) {
                return 0;
            }

        }

        free(lpb);

    }
    return (DWORD) -1;
}

XOSD EXPENTRY
TLSetup(
    LPTLSS lptlss
    )
{

    // struct _TLSS {
    // DWORD fLoad;
    // DWORD fInteractive;
    // DWORD fSave;
    // LPVOID lpvPrivate;
    // LPARAM lParam;
    // LPGETSETPROFILEPROC lpfnGetSet;
    // MPT mpt;
    // BOOL fRMAttached;
    // }
    //
    // typedef LONG (OSDAPI * LPGETSETPROFILEPROC)(
    //      LPTSTR          KeyName,   // SubKey name (must be relative)
    //      LPTSTR          ValueName, // value name
    //      DWORD*          dwType,    // type of data (valid only in the case of Set)
    //      BYTE*           Data,      // pointer to data
    //      DWORD           cbData,    // size of data (in bytes)
    //      BOOL            fSet,      // TRUE = setting, FALSE = getting
    //      LPARAM          lParam     // Instance data from shell
    //     );

    DWORD dwType = 0, dwLen;
    
    lptlss->lpfnGetSet(NULL,"Parameters",	&dwType, SzGlobalParams, &dwLen, FALSE, 0);

    return xosdNone;

}
