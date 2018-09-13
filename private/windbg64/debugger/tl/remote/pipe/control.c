/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    tlpipe.c

Abstract:

    This module contains the code for the named pipe transport layer
    which explicitly deals with the machanics of doing named pipes.

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

#include "od.h"

#include        "xport.h"
#include        "tlpipe.h"

#undef MessageBox

//
// there can only be 2 control pipes open at once because
// the connect thread can only service one client at a time
//
#define MAX_PIPES              2
#define BREAKIN_TIMEOUT        20   // in seconds

#define ControlClient(f)       ci[CiClient].f
#define ControlConnect(f)      ci[CiConnect].f

typedef struct _tagCONTROL_PIPE_INFO {
    HANDLE          hPipe;
    OVERLAPPED      olConnect;
    OVERLAPPED      olRead;
    OVERLAPPED      olWrite;
    BOOL            fConnected;
    CHAR            szName[MAX_PATH];
    CHAR            szClientId[MAX_PATH];
} CONTROL_PIPE_INFO, *LPCONTROL_PIPE_INFO;

CONTROL_PIPE_INFO   ci[MAX_PIPES];
CHAR                ClientId[MAX_PATH];

DWORD               CiClient  = 0;
DWORD               CiConnect = 0;

extern BOOL     FConnected;





//*********************************************************************************************
//   Message Box Utiluties
//*********************************************************************************************

typedef struct _tagMESSAGEBOX_INFO {
    HWND    hWnd;
    LPSTR   lpText;
    LPSTR   lpCaption;
    UINT    uType;
    DWORD   dwResponse;
} MESSAGEBOX_INFO, *LPMESSAGEBOX_INFO;


DWORD
MessageBoxThread(
    LPMESSAGEBOX_INFO lpMbi
    )
{
    lpMbi->dwResponse = MessageBoxA( lpMbi->hWnd, lpMbi->lpText, lpMbi->lpCaption, lpMbi->uType );
    return 0;
}


int
WINAPI
MessageBox(
    HWND   hWnd ,
    LPSTR  lpText,
    LPSTR  lpCaption,
    UINT   uType,
    DWORD  dwTimeout
    )
{
    MESSAGEBOX_INFO mbi;
    HANDLE          hThread;
    DWORD           id;
    int             rval;

    mbi.hWnd = hWnd;
    mbi.lpText = lpText;
    mbi.lpCaption = lpCaption;
    mbi.uType = uType;

    hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)MessageBoxThread, &mbi, 0, &id);
    if (!hThread) {
        return 0;
    }

    TlRegisterWorkerThread(hThread);

    if (WaitForSingleObject( hThread, dwTimeout ) == WAIT_TIMEOUT) {
        TerminateThread( hThread, 0 );
        rval = STATUS_TIMEOUT;
    } else {
        rval = mbi.dwResponse;
    }

    TlUnregisterWorkerThread(hThread);

    return rval;
}


VOID
TlControlInitialization(
    VOID
    )

/*++

Routine Description:

    This function initializes all control pipe structures

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD i;


    ZeroMemory( ci, sizeof(ci) );

    for (i=0; i<MAX_PIPES; i++) {
        ci[i].hPipe = INVALID_HANDLE_VALUE;
        ci[i].olConnect.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        ci[i].olRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        ci[i].olWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    }

    return;
}


XOSD
TlCreateControlPipe(
    LPSTR  szName
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
    DWORD               ec;
    DWORD               i;


    for (i=0; i<MAX_PIPES; i++) {
        ci[i].fConnected = FALSE;

        //
        //  Set a security descriptor
        //
        InitializeSecurityDescriptor( &securityDescriptor,
                                      SECURITY_DESCRIPTOR_REVISION );
        SetSecurityDescriptorDacl( &securityDescriptor, TRUE, NULL, FALSE );
        lsa.nLength = sizeof(SECURITY_ATTRIBUTES);
        lsa.lpSecurityDescriptor = &securityDescriptor;
        lsa.bInheritHandle = TRUE;

        sprintf( ci[i].szName, "\\\\.\\pipe\\%scontrol", szName );

        ci[i].hPipe =
                 CreateNamedPipe( ci[i].szName,
                                  PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                  PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                  PIPE_UNLIMITED_INSTANCES,
                                  PIPE_BUFFER_SIZE,
                                  PIPE_BUFFER_SIZE,
                                  1000,
                                  &lsa
                                );

        if (ci[i].hPipe == INVALID_HANDLE_VALUE) {
            ec = GetLastError();
            DEBUG_OUT1("TLCreateControlPipe: failed ec=%u\n", ec);
            return xosdBadPipeName;
        }
    }

    return xosdNone;
}


XOSD
TlCreateClientControlPipe(
    LPSTR  lpHostName,
    LPSTR  lpPipeName
    )

/*++

Routine Description:

    This function creates the pipe which will be connected to windbgrm (server).

Arguments:

    lpHostName - Supplies name of remote debuggee host

    lpPipeName - Supplies remote pipe name

Return Value:

    XOSD error code.

--*/

{
    SECURITY_DESCRIPTOR securityDescriptor;
    SECURITY_ATTRIBUTES lsa;
    DWORD               timeOut;
    DWORD               mode;
    BYTE                buf[256];
    LPCONTROLPACKET     cp = (LPCONTROLPACKET) &buf[0];



    ControlClient(fConnected) = FALSE;

    //
    //  Set a security descriptor
    //
    InitializeSecurityDescriptor( &securityDescriptor,
                                               SECURITY_DESCRIPTOR_REVISION );
    SetSecurityDescriptorDacl( &securityDescriptor, TRUE, NULL, FALSE );
    lsa.nLength = sizeof(SECURITY_ATTRIBUTES);
    lsa.lpSecurityDescriptor = &securityDescriptor;
    lsa.bInheritHandle = TRUE;

    if (!lpHostName || !*lpHostName) {
        lpHostName = DEFAULT_SERVER;
    }
    if (!lpPipeName || !*lpPipeName) {
        lpPipeName = DEFAULT_PIPE;
    }
    sprintf( ControlClient(szName), "\\\\%s\\pipe\\%scontrol",
                    lpHostName, lpPipeName);

    mode = sizeof(ControlClient(szClientId));
    GetComputerName( ControlClient(szClientId), &mode );
    strcpy( cp->ClientId, ControlClient(szClientId) );

    ControlClient(hPipe) = INVALID_HANDLE_VALUE;
    timeOut = TlUtilTime() + 10;

    while ((ControlClient(hPipe) == INVALID_HANDLE_VALUE) && (TlUtilTime() < timeOut)) {
        //
        // wait for the server to make a pipe available...
        //
        WaitNamedPipe( ControlClient(szName), 10000 );

        //
        // create the client pipe
        //
        ControlClient(hPipe) = CreateFile( ControlClient(szName),
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                             NULL );

        if (ControlClient(hPipe) == INVALID_HANDLE_VALUE &&
            GetLastError() == ERROR_BAD_NETPATH) {
                return xosdBadPipeServer;
        }
    }

    if (ControlClient(hPipe) == INVALID_HANDLE_VALUE) {
        return xosdCannotConnect;
    }

    mode = PIPE_READMODE_MESSAGE | PIPE_WAIT;

    if ( !SetNamedPipeHandleState( ControlClient(hPipe), &mode, NULL, NULL ) ) {
        DEBUG_OUT1("SetNamedPipeHandleState failed, ec=%u\n", GetLastError());
        CloseHandle( ControlClient(hPipe) );
        return xosdBadPipeName;
    }

    ControlClient(fConnected) = TRUE;
    cp->Length = sizeof(CONTROLPACKET);
    cp->Type = CP_REQUEST_CONNECTION;
    cp->Response = 0;
    TlWriteControl( CiClient, (PUCHAR)cp, cp->Length );
    TlReadControl( CiClient, (PUCHAR)cp, sizeof(buf) );

    if (cp->Response) {

        //
        // we have permission to open a control pipe
        // try to open a client debuggeer pipe
        //
        DEBUG_OUT(("Control pipe OK\n"));
        return xosdNone;

    } else {

        //
        // the server did not give us permission to
        // open a debugger connection
        //
        ControlClient(fConnected) = FALSE;
        CloseHandle( ControlClient(hPipe) );
        DEBUG_OUT(("Control pipe failed\n"));
        return xosdCannotConnect;

    }
}


XOSD
TlConnectControlPipe(
    VOID
    )
{
    DWORD               ec;
    DWORD               status;
    LPSTR               NewClientId;
    BYTE                buf[256];
    LPCONTROLPACKET     cp = (LPCONTROLPACKET) &buf[0];


    assert( ControlConnect(fConnected) == FALSE );

    ControlConnect(fConnected) =
         ConnectNamedPipe( ControlConnect(hPipe), &ControlConnect(olConnect) );

    if (!ControlConnect(fConnected)) {
        ec = GetLastError();
        switch( ec ) {
            case ERROR_PIPE_CONNECTED:
                goto connected;

            case ERROR_IO_PENDING:
                break;

            default:
                DEBUG_OUT1("PLPIPE: ConnectNamedPipe failed, Error %u\n", ec);
                //DebugPrint("ConnectNamedPipe failed, Error=%u\n", ec);
                return xosdCannotConnect;
        }

        status = WaitForSingleObject( ControlConnect(olConnect.hEvent),
                                                     MAX_CONNECT_WAIT * 1000 );
        switch ( status ) {
            case WAIT_OBJECT_0:
                goto connected;

            case WAIT_TIMEOUT:
                //DebugPrint("ConnectNamedPipe timed out\n");
                return xosdCannotConnect;

            default:
                ec = GetLastError();
                DEBUG_OUT2("PLPIPE: ConnectNamedPipe failed, Status %u, ec=%u\n", status, ec);
                //DebugPrint("ConnectNamedPipe failed, Error=%u\n", ec);
                return xosdCannotConnect;
        }
    }

connected:
    ControlConnect(fConnected) = TRUE;
    //DebugPrint("controlpipe connected\n");

    //
    // the control pipe is now connected so we
    // must now negotiate a 'real' connection
    //

    if (!TlReadControl( CiConnect, (PUCHAR)cp, sizeof(buf) )) {
        //DebugPrint("read #1 failed\n");
        TlDisconnectControl( CiConnect );
        return xosdCannotConnect;
    }

    //DebugPrint("read #1 completed\n");

    if (cp->Type == CP_REQUEST_CONNECTION) {

        NewClientId = _strdup( cp->ClientId );

        if (FConnected) {
            //
            // in this case there is already a valid debugger connection.
            // the connection must be disconnected and a new connection
            // established with the new client.
            //

            //
            // forward the request to the currently connected client
            // the current client has the right to decline the new connection
            //
            cp->Type = CP_BREAKIN_CONNECTION;
            if (!TlWriteControl( CiClient, (PUCHAR)cp, cp->Length )) {
                //DebugPrint("write failed\n");
                TlDisconnectControl( CiConnect );
                return xosdCannotConnect;
            }
            if (!TlReadControl( CiClient, (PUCHAR)cp, sizeof(buf) )) {
                //DebugPrint("read failed\n");
                TlDisconnectControl( CiConnect );
                return xosdCannotConnect;
            }
            if (cp->Response) {
                TlPipeFailure();
                TlDisconnectTransport();
            } else {
                TlDisconnectControl( CiConnect );
                return xosdCannotConnect;
            }
        }

        //
        // save the client id
        //
        strcpy( ClientId, NewClientId );
        free( NewClientId );

        //
        // tell the new client that the connection request is granted
        //
        cp->Response = 1;
        if (!TlWriteControl( CiConnect, (PUCHAR)cp, cp->Length )) {
            //DebugPrint("write #2 failed\n");
            TlDisconnectControl( CiConnect );
            return xosdCannotConnect;
        }

        //DebugPrint("write #2 completed\n");

        if (CiConnect == 0) {
            CiClient = CiConnect;
            CiConnect = 1;
        } else {
            CiClient = CiConnect;
            CiConnect = 0;
        }
    }

    //DebugPrint("connection established\n");

    return xosdNone;
}

BOOL
TlWriteControl(
    DWORD   ciIdx,
    PUCHAR  pch,
    DWORD   cch
    )
{
    DWORD dwBytesWritten;
    static DWORD cblast;
    DWORD error;

    if ( !ci[ciIdx].fConnected ) {
        return FALSE;
    }

    DEBUG_OUT1("PLPIPE: Writing... (Count %u)\n",cch);

    if (WriteFile(ci[ciIdx].hPipe, pch, cch, &dwBytesWritten,
                                                        &ci[ciIdx].olWrite )) {
        //
        // Write was successful and finished
        //
        FlushFileBuffers( ci[ciIdx].hPipe );

        if ( dwBytesWritten != cch ) {
            DEBUG_OUT2("PLPIPE: Wrote %u but asked for %u\n", dwBytesWritten, cch);
            return FALSE;
        }

        DEBUG_OUT1( "PLPIPE: Wrote (%u)\n", dwBytesWritten);

        cblast = cch;
        return TRUE;
    }

    //
    //  We got a failure case -- there are now two possiblities.
    //  1.  -- we have overlapped I/O or
    //  2.  -- we are messed up
    //
    error = GetLastError();
    switch ( error ) {
        case ERROR_PIPE_NOT_CONNECTED:
        case ERROR_BROKEN_PIPE:
        case ERROR_NO_DATA:
            DEBUG_OUT1("PLPIPE: Pipe is gone (1), error  %u\n", error);
            break;

        case ERROR_IO_PENDING:
            dwBytesWritten = 0;
            goto WaitWrite;

        default:
            DEBUG_OUT1("PLPIPE: WriteFile failed, error  %u\n", error);
            break;
    }

    return FALSE;

 WaitWrite:
    if (GetOverlappedResult(ci[ciIdx].hPipe, &ci[ciIdx].olWrite, &dwBytesWritten, TRUE)) {
        //
        // Read has successfully completed
        //
        FlushFileBuffers( ci[ciIdx].hPipe );

        if ( dwBytesWritten != cch ) {
            DEBUG_OUT2("PLPIPE: Wrote %u but asked for %u\n", dwBytesWritten, cch);
            return FALSE;
        }

        DEBUG_OUT1("PLPIPE: Wrote (%u)\n", dwBytesWritten);

        cblast = cch;
        return TRUE;
    }

    error = GetLastError();
    switch ( error ) {
        case ERROR_PIPE_NOT_CONNECTED:
        case ERROR_BROKEN_PIPE:
        case ERROR_NO_DATA:
            DEBUG_OUT1("PLPIPE: Pipe is gone (3), error  %u\n", error);
            break;

        default:
            DEBUG_OUT1("PLPIPE: Get Read result failed, error  %u\n", error);
            break;
    }

    return FALSE;
}

DWORD
TlReadControl(
    DWORD   ciIdx,
    PUCHAR  pch,
    DWORD   cch
    )
{
    DWORD       dwBytesRead;
    DWORD       error;

    if ( !ci[ciIdx].fConnected ) {
        return (DWORD) -1;
    }

    ResetEvent( ci[ciIdx].olRead.hEvent );

    if (ReadFile(ci[ciIdx].hPipe, pch, cch, &dwBytesRead, &ci[ciIdx].olRead)) {
        //
        // Read was successful and finished return packet size and exit.
        //
        return dwBytesRead;
    }

    //
    //  We got a failure case -- there are now two possibities.
    //      1.  -- we have overlapped I/O, or
    //      2.  -- we are messed up.
    //

    error  = GetLastError();
    switch ( error ) {
        case ERROR_MORE_DATA:
            DEBUG_OUT(("Message is too long\n"));
            assert( "Message is too long" && FALSE );
            break;

        case ERROR_IO_PENDING:
            DEBUG_OUT1("PLPIPE: Read pending (%u bytes)...\n", cch);
            dwBytesRead = 0;
            goto WaitRead;

        case ERROR_PIPE_NOT_CONNECTED:
        case ERROR_BROKEN_PIPE:
        case ERROR_NO_DATA:
            DEBUG_OUT1("PLPIPE: Pipe is gone (2), error  %u\n", error);
            return (DWORD) -1;

        default:
            DEBUG_OUT1("PLPIPE: ReadFile failed, error  %u\n", error);
            break;
    }

    return 0;

 WaitRead:
    if (GetOverlappedResult(ci[ciIdx].hPipe, &ci[ciIdx].olRead, &dwBytesRead, TRUE)) {
        //
        // Read has successfully completed
        //
        DEBUG_OUT1("PLPIPE: Read complete (%u bytes)...\n", dwBytesRead);
        return dwBytesRead;
    }

    error  = GetLastError();
    DEBUG_OUT1("PLPIPE: Pipe is gone (3), error  %u\n", error);
    return (DWORD) -1;
}

VOID
ControlPipeFailure(
    VOID
    )
{
    TlDisconnectControl( CiClient );
}

BOOL
TlDisconnectControl(
    DWORD   ciIdx
    )
{
    BOOL    Ok = TRUE;
    DWORD   Error;

    DEBUG_OUT("TlDisconnectControl: PipeDisconnect\n");

    Ok = DisconnectNamedPipe( ci[ciIdx].hPipe );

    if ( !Ok ) {

        Error = GetLastError();

        switch( Error ) {

        case ERROR_PIPE_NOT_CONNECTED:
            Ok = TRUE;
            break;

        default:
            DEBUG_OUT1("TlDisconnectControl: DisconnectNamedPipe failed, Error %u\n", Error);
            break;
        }
    }

    if ( Ok ) {
        DEBUG_OUT(( "TlDisconnectControl: PLPIPE: Disconnected\n" ));
        ci[ciIdx].fConnected = FALSE;
    }

    return Ok;
}


DWORD
ControlReaderThread(
    LPVOID lpv
    )
{
    int                 cb;
    int                 response;
    BYTE                buf[512];
    LPCONTROLPACKET     cp = (LPCONTROLPACKET) &buf[0];
    CHAR                szMsg[256];


    while (TRUE) {
        //
        //  Read the next packet item from the network
        //
        cb = TlReadControl( CiClient, (PUCHAR)cp, sizeof(buf) );

        if (cb == (DWORD)-1) {
            break;
        }

        if (cb > 0) {
            assert( cp->Length == (DWORD)cb );
            if (cp->Type == CP_BREAKIN_CONNECTION) {
                //
                // some hosehead wants to break in and use this
                // client's debug pipe
                //
                sprintf( szMsg,
                         "%s is requesting permission to interrupt your debug session.\n\nYou have %d seconds to respond or permission will be granted\n\nWould you like to grant permission?",
                         cp->ClientId,
                         BREAKIN_TIMEOUT
                       );
                response = MessageBox( NULL,
                                       szMsg,
                                       "Windbg Remote Debugger",
                                       MB_ICONQUESTION | MB_OKCANCEL | MB_SETFOREGROUND,
                                       1000 * BREAKIN_TIMEOUT
                                     );

                if (response == STATUS_TIMEOUT || response == IDOK) {
                    cp->Response = TRUE;
                } else {
                    cp->Response = FALSE;
                }
                TlWriteControl( CiClient, (PUCHAR)cp, sizeof(*cp) );
            }
        }
    }

    return 0;
}
