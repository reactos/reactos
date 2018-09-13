/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    tlpipe.c

Abstract:

    This module contains the code for the named pipe transport layer
    which explicitly deals with the machanics of doing named pipes.

Author:

    Jim Schaad  (jimsch)  11-Jun-93
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


#include "dbgver.h"
extern AVS Avs;

#include        "xport.h"
#include        "tlpipe.h"

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
    {  "Named pipe debugger transport" } // rgchInfo
};

LPTLIS
TlGetInfo(
    VOID
    )
{
    return &Tlis;
}

#ifdef DEBUGVER
DEBUG_VERSION('T','L',"Named Pipe Transport Layer (Debug)")
#else
RELEASE_VERSION('T','L',"Named Pipe Transport Layer")
#endif

DBGVERSIONCHECK()


extern CRITICAL_SECTION csExpecting;

BOOL            FVerbose = FALSE;
BOOL            FPipeConnected = FALSE;
BOOL            FDestroyTransport;
HANDLE          HandleNamedPipe = INVALID_HANDLE_VALUE;
HANDLE          HReadThread;
HANDLE          HControlReadThread;
OVERLAPPED      OverlappedPipe;
OVERLAPPED      OverlappedRead;
OVERLAPPED      OverlappedWrite;
CRITICAL_SECTION CsWritePipe;
CHAR            RgchPipeName[MAX_PATH];
BOOL            FDMSide = FALSE;
struct {
    char *      lpb;
    int         cb;
}               RgQueue[SIZE_OF_QUEUE];
int             IQueueFront = 0;
int             IQueueBack = 0;
CRITICAL_SECTION CsQueue = {0};
HANDLE          HQueueEvent;
HANDLE          HCallbackThread;

CHAR            MyName[] = "TLPIPE";
#define PARAM_PIPE  "Parameters"
CHAR            SzParamPipeName[MAX_PATH];
#define PARAM_HOST  "HostName"
CHAR            SzParamHostName[MAX_PATH];

CHAR            SzRemoteHostName[MAX_PATH];
CHAR            SzRemotePipeName[MAX_PATH];

REPLY            RgReplys[SIZE_OF_REPLYS];
CRITICAL_SECTION CsReplys;
int              IReplys;

char *  RgSzTypes[] = {"FirstAsync", "Async", "FirstReply", "Reply",
                       "Disconnect", "VersionRequest", "VersionReply"};
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
    MPT     mpt
    )
{
    Tlis.mptRemote = mpt;
    FDMSide = TRUE;
}


//#define TL_ERROR_LOGGING 1


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
                             el[ei].p=(LPDWORD)malloc(z);memcpy(el[ei].p,q,z); \
                             printel2(ei); \
                             ei++; \
                             if (ei==99) ei=0;}
#define LGREAD  1
#define LGWRITE 2
ERRLOG  el[100];
DWORD   ei=0;

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

void printel2( int i )
{
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

#else

#define LOGIT(x,y,z,q)
#define LGREAD  1
#define LGWRITE 2

#endif

XOSD    PipeConnect(HANDLE,DWORD);
BOOL    PipeClose(void);
DWORD   ReadFromPipe(PUCHAR,DWORD);
DWORD   ReaderThread(LPVOID arg);
DWORD   ControlReaderThread(LPVOID arg);
DWORD   CallbackThread(LPVOID lpvArg);

int FAR PASCAL
CopyString(
    LPSTR * lplps,
    LPSTR lpT,
    char  chEscape,
    BOOL  fQuote
    );

BOOL
CreateStuff(
    VOID
    )
{
    int i;

    if (FDMSide && HReadThread) {
        return TRUE;
    }

#ifdef TL_ERROR_LOGGING
    printel();
#endif

    TlControlInitialization();

    //
    // Create random data strutures needed internally
    //

    OverlappedRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (OverlappedRead.hEvent == NULL) {
        return FALSE;
    }

    OverlappedWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (OverlappedWrite.hEvent == NULL) {
        return FALSE;
    }

    InitializeCriticalSection( &CsQueue );
    HQueueEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    InitializeCriticalSection( &CsReplys );
    for (i=0; i<SIZE_OF_REPLYS; i++) {
        RgReplys[i].hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    }

    InitializeCriticalSection(&CsWritePipe);

#if DBG
    InitializeCriticalSection(&csExpecting);
#endif

    return TRUE;
}


BOOL
StartWorkerThreads(
    BOOL fStartControlReader
    )
{
    DWORD       id;



    FDestroyTransport = FALSE;

    //
    // start the debugger pipe reader thread
    //
    HReadThread = CreateThread(NULL, 0, ReaderThread, 0, 0, &id);
    if (!HReadThread) {
        return FALSE;
    }

    SetThreadPriority( HReadThread, THREAD_PRIORITY_ABOVE_NORMAL );

    if (fStartControlReader) {
        //
        // start the control pipe reader thread, << ONLY FOR CLIENTS >>
        //
        HControlReadThread = CreateThread(NULL, 0, ControlReaderThread, 0, 0, &id);
        if (!HControlReadThread) {
            TerminateThread( HReadThread, 0 );
            return FALSE;
        }
        SetThreadPriority( HControlReadThread, THREAD_PRIORITY_ABOVE_NORMAL );
    }

    //
    // start the callback thread
    //
    HCallbackThread = CreateThread(NULL, 0, CallbackThread, 0, 0, &id);
    if (!HCallbackThread) {
        TerminateThread( HControlReadThread, 0 );
        TerminateThread( HReadThread, 0 );
        return FALSE;
    }
    SetThreadPriority( HCallbackThread, THREAD_PRIORITY_ABOVE_NORMAL );

    TlRegisterWorkerThread(HReadThread);
    TlRegisterWorkerThread(HCallbackThread);
    if (fStartControlReader) {
        TlRegisterWorkerThread(HControlReadThread);
    }

    return TRUE;
}


VOID
DestroyStuff(
    VOID
    )
{
    int         i;

    //
    //  If there is a reader thread -- then wait for it to be terminated
    //  and close the handle
    //
    if (HReadThread) {
        TerminateThread( HReadThread, 0 );
        WaitForSingleObject(HReadThread, INFINITE);
        TlUnregisterWorkerThread(HReadThread);
        HReadThread = NULL;
    }

    //
    //  If there is a control thread -- then wait for it to be termianted
    //  and close the handle
    //
    if (HControlReadThread) {
        TerminateThread( HControlReadThread, 0 );
        WaitForSingleObject(HControlReadThread, INFINITE);
        TlUnregisterWorkerThread(HControlReadThread);
        HControlReadThread = NULL;
    }

    if (HCallbackThread) {
        TerminateThread( HCallbackThread, 0 );
        WaitForSingleObject(HCallbackThread, INFINITE);
        TlUnregisterWorkerThread(HCallbackThread);
        HCallbackThread = NULL;
    }

    //
    //  Now delete all of the objects
    //
    CloseHandle(HQueueEvent);
    CloseHandle(OverlappedRead.hEvent);
    CloseHandle(OverlappedWrite.hEvent);
    DeleteCriticalSection(&CsQueue);
    DeleteCriticalSection(&CsReplys);
    DeleteCriticalSection(&CsWritePipe);
    for (i=0; i<SIZE_OF_REPLYS; i++) {
        CloseHandle(RgReplys[i].hEvent);
    }

#if DBG
    DeleteCriticalSection(&csExpecting);
#endif

    return;
}


VOID
TlPipeFailure(
    VOID
    )
{
    int  i;
    static int  f = FALSE;

    //
    // this reentry gate is almost as good as useless...
    //
    if (f) {
        return;
    }
    f = TRUE;
    EnterCriticalSection(&CsReplys);
    for (i=0; i<IReplys; i++) {
        SetEvent(RgReplys[i].hEvent);
    }
    LeaveCriticalSection(&CsReplys);
    ControlPipeFailure();
    f = FALSE;
    return;
}


XOSD
TlCreateTransport(
    LPSTR szName
    )

/*++

Routine Description:

    This function creates the pipe which will be connected to windbgrm (server).

Arguments:

    szName  - Supplies the name of the pipe to create

Return Value:

    XOSD error code.

--*/

{
    SECURITY_DESCRIPTOR securityDescriptor;
    SECURITY_ATTRIBUTES lsa;
    DWORD               error;


    if (HandleNamedPipe != INVALID_HANDLE_VALUE) {
        return xosdNone;
    }

    if (!CreateStuff()) {
        return xosdUnknown;
    }

    if (!szName) {
        szName = SzParamPipeName;
    }

    if (TlCreateControlPipe( szName ) != xosdNone) {
        return xosdBadPipeName;
    }

    DPRINT(("TlCreateTransport: Create ServerPipe\n"));

    //
    // create the event used for overlapped io
    //
    OverlappedPipe.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (OverlappedPipe.hEvent == NULL) {
        return xosdUnknown;
    }

    //
    //  Set a security descriptor
    //
    InitializeSecurityDescriptor( &securityDescriptor,
                                                SECURITY_DESCRIPTOR_REVISION );
    SetSecurityDescriptorDacl( &securityDescriptor, TRUE, NULL, FALSE );
    lsa.nLength = sizeof(SECURITY_ATTRIBUTES);
    lsa.lpSecurityDescriptor = &securityDescriptor;
    lsa.bInheritHandle = TRUE;

    _snprintf(RgchPipeName, sizeof(RgchPipeName), PIPE_NAME_FORMAT,
             ".", szName ? szName : DEFAULT_PIPE);

    HandleNamedPipe = CreateNamedPipe( RgchPipeName,
                              PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                              PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE |
                              PIPE_WAIT,
                              1,
                              PIPE_BUFFER_SIZE,
                              PIPE_BUFFER_SIZE,
                              1000,
                              &lsa
                            );

    if (HandleNamedPipe == INVALID_HANDLE_VALUE) {
        error = GetLastError();
        DPRINT(("TLCreateTransport: failed Error %u\n", error));
        return xosdBadPipeName;
    }

    DPRINT(("TLCreateTransport: created pipe %s == 0x%x\n",
            RgchPipeName, HandleNamedPipe));

    FPipeConnected = FALSE;

    StartWorkerThreads( FALSE );

    return xosdNone;
}


XOSD
TlConnectTransport(
    VOID
    )

/*++

Routine Description:

    This function attempts to connect the server pipe to a client.

Arguments:

    szName  - Supplies the name of the pipe to create

Return Value:

    XOSD error code.

--*/

{
    DWORD   ec;
    DWORD   status;


    if (TlConnectControlPipe() != xosdNone) {
        return xosdCannotConnect;
    }

    if (FPipeConnected) {
        return xosdNone;
    }

    DPRINT(("TlConnectTransport: ConnectNamedPipe(0x%p)\n", HandleNamedPipe));
    FPipeConnected = ConnectNamedPipe( HandleNamedPipe, &OverlappedPipe);

    if (!FPipeConnected) {
        ec = GetLastError();
        switch( ec ) {
            case ERROR_PIPE_CONNECTED:
                goto connected;

            case ERROR_IO_PENDING:
                break;

            default:
                DPRINT(("PLPIPE: ConnectNamedPipe failed, Error %u\n", ec));
                return xosdCannotConnect;
        }

        status = WaitForSingleObject( OverlappedPipe.hEvent,
                                                     MAX_CONNECT_WAIT * 1000 );
        switch ( status ) {
            case WAIT_OBJECT_0:
                goto connected;

            case WAIT_TIMEOUT:
                //DebugPrint("ConnectNamedPipe timed out\n");
                return xosdCannotConnect;
                break;

            default:
                ec = GetLastError();
                DPRINT((
                    "PLPIPE: ConnectNamedPipe failed, Status %u, ec=%u\n",
                    status, ec));
                //DebugPrint("ConnectNamedPipe failed, Error=%u\n", ec);
                return xosdCannotConnect;
        }
    }

connected:
    DPRINT(("TLConnectTransport: pipe connected.\n"));
    FPipeConnected = TRUE;

    return xosdNone;
}

XOSD
TlCreateClient(
    LPSTR szName
    )
{
    HANDLE      handle = INVALID_HANDLE_VALUE;
    DWORD       timeOut;
    DWORD       mode;
    XOSD        xosd = xosdNone;
    DWORD       error;
    char *      lpsz;


    if (!CreateStuff()) {
        return xosdUnknown;
    }

    if (!szName) {

        strcpy(SzRemoteHostName, SzParamHostName);
        strcpy(SzRemotePipeName, SzParamPipeName);

    } else {

        lpsz = szName;
        while (*lpsz && isspace(*lpsz)) {
            lpsz++;
        }

        *SzRemoteHostName = 0;
        *SzRemotePipeName = 0;

        if (CopyString(&lpsz, SzRemoteHostName, '\\', *lpsz == '"') > 0) {
            while (*lpsz && isspace(*lpsz)) {
                lpsz++;
            }
            CopyString(&lpsz, SzRemotePipeName, '\\', *lpsz == '"');
        }
    }

    if ((xosd = TlCreateClientControlPipe(SzRemoteHostName,
                                           SzRemotePipeName)) != xosdNone) {
        return xosd;
    }

    if (HandleNamedPipe != INVALID_HANDLE_VALUE) {
        DPRINT(("Named pipe is already open\n"));
        return xosdNone;
    }

    if ( *SzRemoteHostName == 0 ) {
        _snprintf( RgchPipeName, sizeof(RgchPipeName), PIPE_NAME_FORMAT,
                   DEFAULT_SERVER, DEFAULT_PIPE );
    } else {
        _snprintf(RgchPipeName, sizeof(RgchPipeName), PIPE_NAME_FORMAT,
                SzRemoteHostName, SzRemotePipeName);
    }

    DPRINT(("PLPIPE: OpenClientPipe %s\n", RgchPipeName));

    timeOut = TlUtilTime() + 10;
    while ((handle == INVALID_HANDLE_VALUE) && (TlUtilTime() < timeOut)) {

        WaitNamedPipe( RgchPipeName, 10000 );

        handle = CreateFile( RgchPipeName,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                             NULL );

        if (handle == INVALID_HANDLE_VALUE) {
            //
            //  server doesn't exist, don't wait around.
            //

            if (GetLastError() == ERROR_BAD_NETPATH) {
                break;
            }
            Sleep(500);
        }
    }

    if ( handle == INVALID_HANDLE_VALUE ) {

        error = GetLastError();
        DPRINT(("PLPIPE: CreateFile failed Error %u\n", error));
        switch (error) {
            case ERROR_BAD_NETPATH:
                xosd = xosdBadPipeServer;
                break;

            case ERROR_FILE_NOT_FOUND:
                xosd = xosdBadPipeName;
                break;

            default:
                xosd = xosdBadPipeServer;
                break;
        }

    } else {

        mode = PIPE_READMODE_MESSAGE | PIPE_WAIT;

        if ( !SetNamedPipeHandleState( handle, &mode, NULL, NULL ) ) {
            DEBUG_OUT1("PLPIPE: SetNamedPipeHandleState failed, Error %u\n",
                       GetLastError());
            xosd = xosdBadPipeName;
        }

        DPRINT(( "PLPIPE: Opened client side of pipe %s == 0x%p\n",
                RgchPipeName, handle));
        DPRINT(( "PLPIPE: Connected\n" ));

        HandleNamedPipe = handle;
        FPipeConnected   = TRUE;

        StartWorkerThreads( TRUE );
    }

    return(xosd);
}


XOSD
TlDestroyTransport(
    VOID
    )
{
    if (FPipeConnected) {
        FPipeConnected = FALSE;
        if (!FDMSide) {
            DestroyStuff();
        }
        TlDisconnectTransport();
        if (!FDMSide) {
            PipeClose();
            CloseHandle(OverlappedPipe.hEvent);
        }
    }
    FDestroyTransport = TRUE;
    return xosdNone;
}


BOOL
TlDisconnectTransport(
    VOID
    )
{
    BOOL    Ok = TRUE;
    DWORD   Error;
    extern BOOL FConnected;

    DPRINT(("TlDisconnectTransport: PipeDisconnect\n"));

    Ok = DisconnectNamedPipe( HandleNamedPipe );

    if ( !Ok ) {

        Error = GetLastError();

        switch( Error ) {

        case ERROR_PIPE_NOT_CONNECTED:
            Ok = TRUE;
            break;

        default:
            DEBUG_OUT1("TlDisconnectTransport: DisconnectNamedPipe failed, Error %u\n", Error);
            break;
        }
    }

    if ( Ok ) {
        DEBUG_OUT(( "TlDisconnectTransport: PLPIPE: Disconnected\n" ));
        FPipeConnected = FALSE;
        FConnected = FALSE;
    }

    return Ok;
}


BOOL
PipeClose(
    void
    )
{
    BOOL    Ok = TRUE;

    DEBUG_OUT(("PLPIPE: PipeClose\n"));

    if ( HandleNamedPipe != INVALID_HANDLE_VALUE ) {

        if ( FDMSide && FPipeConnected ) {

            DEBUG_OUT( ("PLPIPE: Pipe being closed without disconnecting\n") );
            Ok = FALSE;

        } else {

            CloseHandle( HandleNamedPipe );
            HandleNamedPipe = INVALID_HANDLE_VALUE;
            FPipeConnected   = FALSE;
            DEBUG_OUT( ("PLPIPE: Named Pipe now closed\n") );
        }
    }

    return Ok;
}


DWORD
TlUtilTime(VOID)
{
    DWORD   time;
    static DWORD        lTickCount = 0;

    time = GetCurrentTime();

    time = time / 1000;             // Convert from millisecs to secs

    if (time < lTickCount) {        // Take care of day wrap
        time += (24L * 3600);
    }

    lTickCount = time;

    return(time);
}


BOOL
TlWriteTransport(
    PUCHAR  pch,
    DWORD   cch
    )
{
    DWORD dwBytesWritten;
    DWORD ec;

    if ( !FPipeConnected ) {
        return FALSE;
    }

    DEBUG_OUT1("PLPIPE: Writing... (Count %u)\n",cch);

    EnterCriticalSection(&CsWritePipe);
    if (WriteFile(HandleNamedPipe, pch, cch, &dwBytesWritten, &OverlappedWrite )) {
        //
        // Write was successful and finished
        //
        LeaveCriticalSection(&CsWritePipe);

        if ( dwBytesWritten != cch ) {
            DEBUG_OUT2("PLPIPE: Wrote %u but asked for %u\n", dwBytesWritten, cch);
            LOGIT(LGWRITE,cch,dwBytesWritten,pch);
            goto errorWrite;
        }

        DEBUG_OUT1( "PLPIPE: Wrote (%u)\n", dwBytesWritten);

        LOGIT(LGWRITE,0,dwBytesWritten,pch);
        return TRUE;
    }

    ec = GetLastError();
    if (ec != ERROR_IO_PENDING) {
        LeaveCriticalSection(&CsWritePipe);
        goto errorWrite;
    }

    if (GetOverlappedResult(HandleNamedPipe, &OverlappedWrite, &dwBytesWritten, TRUE)) {
        //
        // Write was successful and finished
        //
        LeaveCriticalSection(&CsWritePipe);

        if ( dwBytesWritten != cch ) {
            DEBUG_OUT2("PLPIPE: Wrote %u but asked for %u\n", dwBytesWritten, cch);
            LOGIT(LGWRITE,cch,dwBytesWritten,pch);
            goto errorWrite;
        }

        DEBUG_OUT1("PLPIPE: Wrote (%u)\n", dwBytesWritten);

        LOGIT(LGWRITE,0,dwBytesWritten,pch);
        return TRUE;
    }

    LeaveCriticalSection(&CsWritePipe);

errorWrite:
    ec = GetLastError();
    LOGIT(LGWRITE,ec,dwBytesRead,pch);
    TlPipeFailure();

    return FALSE;
}


BOOL
TlFlushTransport(
    VOID
    )
{
    return FlushFileBuffers( HandleNamedPipe );
}


DWORD
ReadFromPipe(
    PUCHAR  pch,
    DWORD   cch
    )
{
    DWORD       dwBytesRead;
    DWORD       ec;
    PNLBLK pnlblk = (PNLBLK)pch;
    PTSTR   pszErrMsgBuf = NULL;

    if (!FPipeConnected) {
        return (DWORD) -1;
    }

    ResetEvent( OverlappedRead.hEvent );

    if (ReadFile(HandleNamedPipe, pch, cch, &dwBytesRead, &OverlappedRead)) {
        //
        // Read has successfully completed
        //
        DPRINT(("ReadFromPipe: succeeded, %d bytes read.\n", dwBytesRead));
        LOGIT(LGREAD,pnlblk->cchMessage,dwBytesRead,pch);
        return dwBytesRead;
    }

    ec = GetLastError();
    if (ec != ERROR_IO_PENDING) {
        goto errorRead;
    }

    if (GetOverlappedResult(HandleNamedPipe, &OverlappedRead, &dwBytesRead, TRUE)) {
        //
        // Read has successfully completed
        //
        DPRINT(("ReadFromPipe: succeeded, %d bytes read.\n", dwBytesRead));
        LOGIT(LGREAD,pnlblk->cchMessage,dwBytesRead,pch);
        return dwBytesRead;
    }

errorRead:
    ec = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (PTSTR) &pszErrMsgBuf,
        0,
        NULL
        );

    DPRINT(("ReadFromPipe: failed, error == %d -> %s\n", ec, pszErrMsgBuf));

    LocalFree( pszErrMsgBuf );

    LOGIT(LGREAD,ec,dwBytesRead,pch);
    TlPipeFailure();

    return (DWORD) -1;
}



void
QueueKillMessage(
    void
    )
{
    LPSTR       lpb;
    PNLBLK      pnlblk;

    //
    //  Make sure that there is space to put the entry into the
    //  queue -- if not then wait until we can get enough space.
    //

    EnterCriticalSection(&CsQueue);
    while ((IQueueFront + 1) % SIZE_OF_QUEUE == IQueueBack) {
        LeaveCriticalSection(&CsQueue);
        Sleep(100);
        EnterCriticalSection(&CsQueue);
    }
    DEBUG_OUT("READER: Add killer\n");

    //
    //  Allocate space for the killer message
    //

    lpb = malloc(sizeof(NLBLK));
    pnlblk = (PNLBLK) lpb;
    pnlblk->mtypeBlk = mtypeTransportIsDead;
    pnlblk->cchMessage = 0;

    //
    //  Put the message in the queue
    //

    RgQueue[IQueueFront].lpb = lpb;
    RgQueue[IQueueFront].cb = sizeof(NLBLK);
    IQueueFront = (IQueueFront + 1) % SIZE_OF_QUEUE;

    //
    //  Wake up the other guy and terminate this thread
    //

    SetEvent(HQueueEvent);
    LeaveCriticalSection(&CsQueue);
}

DWORD
ReaderThread(
    LPVOID     lpvArg
    )
/*++

Routine Description:

    This is the main function for the reader thread in this transport layer.
    Its sole purpose is to pull things from the named pipe queue as fast
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
    int         cb = 0;
    int         cb2;
    int         i;
    LPSTR       lpb;
    MPACKET *   pMpacket;



    bufSize = MAX_INTERNAL_PACKET + sizeof(NLBLK) + sizeof(MPACKET);
    pnlblk = (PNLBLK) malloc( bufSize );

ReaderWaitForPipe:
    DPRINT(("ReaderThread: waiting for pipe...\n"));
    while (HandleNamedPipe == INVALID_HANDLE_VALUE || (!FPipeConnected)) {
        if (FDestroyTransport) {
            QueueKillMessage();
            return 0;
        }
        Sleep( 500 );
    }
    DPRINT(("ReaderThread: pipe is ready.\n"));

    while (TRUE) {

        if (FDestroyTransport) {
            QueueKillMessage();
            return 0;
        }

        //
        //  Read the next packet item from the network
        //
        cb2 = cb;
        DPRINT(("ReaderThread: reading...\n"));
        cb = ReadFromPipe((PUCHAR)pnlblk, bufSize);

        assert( cb != 0 );

        //
        //  Did the read routine decide that the pipe has completely
        //      failed and caused a shut down of it to occur?  if so
        //      then go ahead and exit this thread from the system.
        //

        if (cb == -1) {

            DPRINT(("ReaderThread: pipe shut down.\n"));


            QueueKillMessage();

            if (FDMSide) {

                if (FPipeConnected) {
                    TlDisconnectTransport();
                }
                goto ReaderWaitForPipe;

            } else {

                return 0;

            }
        }

        //
        //  If the readed item had some length -- then we need to process
        //      the message packet just recieved
        //
        else if ( cb > 0) {

            //
            //  Print a message about this packet type.
            //

            //DEBUG_OUT2("READER: %s %d\n", SzTypes(pnlblk->mtypeBlk), cb);
            DEBUG_OUT2("READER: %x %x\n", pnlblk->mtypeBlk, cb )

            //
            //  For a reply or a version reply message.  Place the reply
            //  into the buffer which was supplied for that purpose.
            //

            if ((pnlblk->mtypeBlk == mtypeVersionReply) ||
                (pnlblk->mtypeBlk == mtypeReply) ||
                (pnlblk->mtypeBlk == mtypeLoadDMReply)) {
                EnterCriticalSection(&CsReplys);
                i = IReplys - 1;
                assert(i != -1);
                if (i != -1) {
                    assert( WaitForSingleObject( RgReplys[i].hEvent, 0 ) != WAIT_OBJECT_0 );
                    cb = min(pnlblk->cchMessage, RgReplys[i].cbBuffer);
                    memcpy(RgReplys[i].lpb, pnlblk->rgchData, cb);
                    RgReplys[i].cbRet = cb;
                    SetEvent(RgReplys[i].hEvent);
                }
                LeaveCriticalSection(&CsReplys);
            } else if (pnlblk->mtypeBlk == mtypeReplyMulti) {
                EnterCriticalSection(&CsReplys);
                i = IReplys - 1;
                if (i != -1) {
                    pMpacket = (MPACKET *) pnlblk->rgchData;
                    cb2 = pMpacket->packetNum * MAX_INTERNAL_PACKET;
                    cb = pnlblk->cchMessage - sizeof(MPACKET);
                    cb = min(cb + cb2, RgReplys[i].cbBuffer);
                    if (cb > cb2) {
                        memcpy(RgReplys[i].lpb + cb2, pMpacket->rgchData,
                               cb - cb2);
                        RgReplys[i].cbRet = cb;
                    }
                    if (pMpacket->packetNum + 1 == pMpacket->packetCount) {
                        SetEvent(RgReplys[i].hEvent);
                    }
                }
                LeaveCriticalSection(&CsReplys);
            } else {

                assert( cb == (int) (pnlblk->cchMessage + sizeof(NLBLK)) );

                lpb = malloc(cb);
                memcpy(lpb, pnlblk, cb);

                EnterCriticalSection( &CsQueue );
                while ((IQueueFront + 1) % SIZE_OF_QUEUE == IQueueBack) {
                    LeaveCriticalSection( &CsQueue );
                    Sleep(100);
                    EnterCriticalSection( &CsQueue );
                }
                DEBUG_OUT2("READER: Add queue Front=%d End=%d\n", IQueueFront, IQueueBack);
                RgQueue[IQueueFront].lpb = lpb;
                RgQueue[IQueueFront].cb = cb;
                IQueueFront = (IQueueFront + 1) % SIZE_OF_QUEUE;
                SetEvent(HQueueEvent);
                LeaveCriticalSection( &CsQueue );
            }
        }
    }

    return (DWORD) -1;
}


DWORD
CallbackThread(
    LPVOID lpvArg
    )
{
    LPSTR       lpb;
    int         cb;
    BOOL        Quit;

    while (TRUE) {

        EnterCriticalSection( &CsQueue );
        if (IQueueFront == IQueueBack) {
            ResetEvent( HQueueEvent);
            LeaveCriticalSection( &CsQueue );
            WaitForSingleObject( HQueueEvent, INFINITE );
            EnterCriticalSection( &CsQueue );
        }

        lpb = RgQueue[IQueueBack].lpb;
        DEBUG_OUT3("CALLBACK: %x Back=%d Front=%d\n",((PNLBLK)lpb)->mtypeBlk,IQueueBack,IQueueFront);
        cb = RgQueue[IQueueBack].cb;
        RgQueue[IQueueBack].lpb = NULL;
        RgQueue[IQueueBack].cb = 0;
        IQueueBack = (IQueueBack + 1) % SIZE_OF_QUEUE;
        LeaveCriticalSection( &CsQueue );

        Quit = !CallBack((PNLBLK) lpb, cb);

        free(lpb);

        if (Quit && (!FDMSide || FDestroyTransport)) {
            return 0;
        }

    }
    return (DWORD) -1;
}


int
PASCAL
CopyString(
    LPSTR * lplps,
    LPSTR lpT,
    char  chEscape,
    BOOL  fQuote
    )
/*++

Routine Description:

    Scan and copy an optionally quoted C-style string.  If the first
    character is a quote, a matching quote will terminate the string,
    otherwise the scanning will stop at the first whitespace encountered.
    The target string will be null terminated if any characters are copied.

Arguments:

    lplps    - Supplies a pointer to a pointer to the source string

    lpt      - Supplies a pointer to the target string

    chEscape - Supplies the escape character (typically '\\')

    fQuote   - Supplies a flag indicating whether the first character is a quote

Return Value:

    The number of characters copied into lpt[].  If an error occurs, -1 is
    returned.

--*/
{
    LPSTR lps = *lplps;
    LPSTR lpt = lpT;
    int   i;
    int   n;
    int   err = 0;
    char  cQuote;

    if (fQuote) {
        if (*lps) cQuote = *lps++;
    }

    while (!err) {

        if (*lps == 0)
        {
            if (fQuote) err = 1;
            else        *lpt = '\0';
            break;
        }
        else if (fQuote && *lps == cQuote)
        {
            *lpt = '\0';
            // eat the quote
            lps++;
            break;
        }
        else if (!fQuote &&
                    (!*lps ||
                      *lps == ' ' ||
                      *lps == '\t' ||
                      *lps == '\r' ||
                      *lps == '\n'))
        {
            *lpt = '\0';
            break;
        }

        else if (*lps != chEscape)
        {
            *lpt++ = *lps++;
        }
        else
        {
            switch (*++lps) {
              case 0:
                err = 1;
                --lps;
                break;

              default:     // any char - usually escape or quote
                *lpt++ = *lps;
                break;

              case 'b':    // backspace
                *lpt++ = '\b';
                break;

              case 'f':    // formfeed
                *lpt++ = '\f';
                break;

              case 'n':    // newline
                *lpt++ = '\n';
                break;

              case 'r':    // return
                *lpt++ = '\r';
                break;

              case 's':    // space
                *lpt++ = ' ';
                break;

              case 't':    // tab
                *lpt++ = '\t';
                break;

              case '0':    // octal escape
                for (n = 0, i = 0; i < 3; i++) {
                    ++lps;
                    if (*lps < '0' || *lps > '7') {
                        --lps;
                        break;
                    }
                    n = (n<<3) + *lps - '0';
                }
                *lpt++ = (UCHAR)(n & 0xff);
                break;
            }
            lps++;    // skip char from switch
        }

    }  // while

    if (err) {
        return -1;
    } else {
        *lplps = lps;
        return (int)(lpt - lpT);
    }
}

XOSD
TlDoTlSetup(
    LPTLSS lptlss
    )
{
    PTSTR lpsz;
    TCHAR szModuleParams[MAX_PATH];
    
    if (lptlss->fLoad) {
        if (!lptlss->pfnGetModuleInfo(0, szModuleParams)) {
            return xosdGeneral;
        }
        
        lpsz = szModuleParams;
        
        while (*lpsz && isspace(*lpsz)) {
            lpsz++;
        }
        if (!FDMSide) {
            CopyString(&lpsz, SzParamHostName, '\\', *lpsz == '"');
            while (*lpsz && isspace(*lpsz)) {
                lpsz++;
            }
        }
        
        CopyString(&lpsz, SzParamPipeName, '\\', *lpsz == '"');
    }
    
    return xosdNone;
}
