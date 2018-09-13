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
#include        "tlser.h"

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
#elif defined(_M_ALPHA)
    mptdaxp,              // mpt
    -1,                   // mptRemote
#elif defined(_M_PPC)
    mptntppc,
    -1,
#elif defined(_M_IA64)
    mptia64,
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

BOOL            FVerbose = TRUE;
HANDLE          HReadThread;
BOOL            FDMSide = FALSE;
BOOL            fConnected;

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
CHAR            ClientId[MAX_PATH];

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
        return FALSE;
    }
    SetThreadPriority( HReadThread, THREAD_PRIORITY_ABOVE_NORMAL );

    //
    // start the callback thread
    //
    HCallbackThread = CreateThread(NULL, 0, CallbackThread, 0, 0, &id);
    if (!HCallbackThread) {
        TerminateThread( HReadThread, 0 );
        return FALSE;
    }
    SetThreadPriority( HCallbackThread, THREAD_PRIORITY_ABOVE_NORMAL );

    return TRUE;
}


VOID
DestroyStuff(
    VOID
    )
{
    int         i;

    if (!fConnected) {
        return;
    }

    //
    //  If there is a reader thread -- then wait for it to be termianted
    //  and close the handle
    //
    if (HReadThread) {
        TerminateThread( HReadThread, 0 );
        WaitForSingleObject(HReadThread, INFINITE);
        CloseHandle(HReadThread);
        HReadThread = NULL;
    }

    if (HCallbackThread) {
        TerminateThread( HCallbackThread, 0 );
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

    return;
}


XOSD
TlCreateTransport(
    LPSTR szParams
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

    if (TlComPort != INVALID_HANDLE_VALUE) {
        return xosdNone;
    }

    {
        LPSTR         pszCom, pszBaud, pszBuf;

        // Modification of the original kept causing a crash when
        //  attempting to connect to a port occupied by a mouse.
        if (!szParams) {
            pszBuf = _strdup(SzGlobalParams);
        } else {
            pszBuf = _strdup(szParams);
        }
        assert(pszBuf);

        if (pszBuf) {
            pszCom = strtok(pszBuf, ":");
            pszBaud = strtok(NULL, ":" );
            TlBaudRate = strtoul(pszBaud, NULL, 0);

            TlComPort = CreateFile( pszCom,
                                    GENERIC_READ | GENERIC_WRITE,
                                    0,
                                    NULL,
                                    OPEN_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL
                                  );

            free (pszBuf);
        }
    }

    if (TlComPort == INVALID_HANDLE_VALUE || TlComPort == NULL) {
        return xosdUnknown;
    }

    SetupComm( TlComPort, 4096, 4096 );


    if (!GetCommState( TlComPort, &LocalDcb )) {
        CloseHandle( TlComPort );
        return xosdUnknown;
    }

    LocalDcb.BaudRate     = TlBaudRate;
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
    To.ReadIntervalTimeout          = 0; //MAXDWORD;
    To.ReadTotalTimeoutMultiplier   = 0;
    To.ReadTotalTimeoutConstant     = 4 * 1000;
    To.WriteTotalTimeoutMultiplier  = 0;
    To.WriteTotalTimeoutConstant    = 4 * 1000;

    if (!SetCommTimeouts( TlComPort, &To )) {
        CloseHandle( TlComPort );
        return xosdUnknown;
    }

    if (!CreateStuff()) {
        CloseHandle( TlComPort );
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
    if (TlComPort) {
        if (fConnected) {
            Sleep( 1000 * 10 );
            return xosdCannotConnect;
        } else {
            fConnected = TRUE;
            return xosdNone;
        }
    }

    Sleep( 1000 * 10 );
    return xosdCannotConnect;
}

XOSD
TlCreateClient(
    LPSTR szName
    )
{
    return TlCreateTransport( szName );
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


    EnterCriticalSection(&CsSerial);
    rc = WriteFile( TlComPort, Buffer, SizeOfBuffer, &BytesWritten, NULL );

    if (!rc) {
        //
        // Device could be locked up.  Clear it just in case.
        //
        ClearCommError( TlComPort, &TrashErr, &TrashStat );
    }

    LeaveCriticalSection(&CsSerial);
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
    DWORD       BytesRead = 0;



    bufSize = MAX_INTERNAL_PACKET + sizeof(NLBLK) + sizeof(MPACKET);
    pnlblk = (PNLBLK) malloc( bufSize );
    pMpacket = (MPACKET *) pnlblk->rgchData;

    while (TRUE) {
        //
        //  Read the next packet item from the network
        //
        ZeroMemory( (LPVOID)pnlblk, bufSize );


        //
        // read the block header
        //
        rc = ReadFile( TlComPort, (LPVOID)pnlblk, sizeof(NLBLK), &BytesRead, NULL );
        if (!rc) {
            cb = GetLastError();
            ClearCommError( TlComPort, &TrashErr, &TrashStat );
#if DBG
            DebugPrint( "COMERR: %d %x\n", cb, TrashErr );
            DebugBreak();
#endif
            Sleep( 50 );
            continue;
        }

        if (BytesRead == 0) {
            Sleep( 50 );
            continue;
        }

        DPRINT(("BytesRead == %d\n", BytesRead));
        assert( BytesRead == sizeof(NLBLK) );

        if (pnlblk->cchMessage) {
            if (pnlblk->cchMessage) {
                //
                // read the data
                //
                rc = ReadFile( TlComPort, (LPVOID)pnlblk->rgchData,
                               pnlblk->cchMessage, &BytesRead, NULL );
                if (!rc) {
                    cb = GetLastError();
                    ClearCommError( TlComPort, &TrashErr, &TrashStat );
#if DBG
                    DebugPrint( "COMERR: %d %x\n", cb, TrashErr );
                    DebugBreak();
#endif
                    Sleep( 50 );
                    continue;
                }
                assert( BytesRead == (DWORD)pnlblk->cchMessage );
            }
            cb = pnlblk->cchMessage + sizeof(NLBLK);
        } else {
            cb = sizeof(NLBLK);
        }

        //
        //  Print a message about this packet type.
        //
        DEBUG_OUT2("READER: %s %d\n", SzTypes(pnlblk->mtypeBlk), cb);
#if DBG
        DebugPrint( "PACKET: %02x %d\n", pnlblk->mtypeBlk, pnlblk->cchMessage );
#endif
        if ((pnlblk->mtypeBlk == mtypeVersionReply) ||
            (pnlblk->mtypeBlk == mtypeReply) ||
            (pnlblk->mtypeBlk == mtypeLoadDMReply)) {
            EnterCriticalSection(&CsReplys);
            i = IReplys - 1;
            if (i != -1) {
                cb = min(pnlblk->cchMessage, RgReplys[i].cbBuffer);
                memcpy(RgReplys[i].lpb, pnlblk->rgchData, cb);
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
                    memcpy( RgReplys[i].lpb + cb2, pMpacket->rgchData, cb - cb2 );
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

    while (TRUE) {
        EnterCriticalSection( &CsQueue );
        if (IQueueFront == IQueueBack) {
            ResetEvent( HQueueEvent);
            LeaveCriticalSection( &CsQueue );
            WaitForSingleObject( HQueueEvent, INFINITE );
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

XOSD
TlDoTlSetup(
    LPTLSS lptlss
    )
{
    TCHAR szModuleParams[MAX_PATH];
    
    if (lptlss->fLoad) {
        if (!lptlss->pfnGetModuleInfo(NULL, szModuleParams)) {
            return xosdGeneral;
        }
        
        strcpy(SzGlobalParams, szModuleParams);
    }
    
    return xosdNone;
}
