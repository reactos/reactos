/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    remote.c

Abstract:

    This file implements a remote server for REMOTE.EXE that services
    the windbg command window.  This provides a "NTSD/KD" style debugging
    from a REMOTE.EXE client to WINDBG.EXE.

Author:

    Wesley Witt (wesw) 5-Dec-1993

Environment:

    Win32, User Mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


extern HANDLE               hFileLog;
extern CRITICAL_SECTION     csLog;

#define BUFFSIZE      256

SESSION_TYPE ClientList[MAX_SESSION];

HANDLE  RemoteServerThreadH;
HANDLE  hEventRemoteTerm;
CHAR    HostName[MAX_PATH];
CHAR    PipeName[MAX_PATH];
CHAR    Server[MAX_PATH];



DWORD
RemoteServerThread(
    char* pipe
    );

BOOL
FilterCommand(
    LPSESSION_TYPE cl,
    LPSTR          buf,
    DWORD          cb
    );

DWORD
RemoteSessionThread(
    SESSION_TYPE* Client
    );

VOID
SendStatus(
    LPSESSION_TYPE cl
    );

DWORD
ShowPopupThread(
    char *mssg
    );

VOID
CloseClient(
    SESSION_TYPE *Client
    );

VOID
InitClientList(
    void
    );

DWORD
ReadFixBytes(
    LPSESSION_TYPE MyClient,
    LPSTR          lpBuf,
    DWORD          cbRead
    );

VOID
PrintDebuggerMsg(
    BOOL  fInsert,
    BOOL  fLog,
    LPSTR lpFmt,
    ...
    );

VOID
PrintCmdMsg(
    LPSESSION_TYPE MyClient,
    BOOL           fInsert,
    BOOL           fLog,
    LPSTR          InpBuf
    );



VOID
StartRemoteServer(
    LPSTR szPipeName,
    BOOL  fAppend
    )
{
    DWORD  ThreadID;
    DWORD  i;
    DWORD  j;
    int tmp_int;


    if (_stricmp(szPipeName,"stop")==0) {
        if (!RemoteRunning) {
            CmdLogFmt( "Remote server is not running\r\n" );
            return;
        }

        for (i=0;i<MAX_SESSION;i++) {
            if (ClientList[i].Active) {

                SetEvent( ClientList[i].hEventTerm );
                if (WaitForSingleObject( ClientList[i].hThread, 10 * 10000 ) == WAIT_TIMEOUT) {
                    TerminateThread( ClientList[i].hThread, 0 );
                }
                CloseHandle( ClientList[i].hEventTerm );

                CloseHandle( ClientList[i].PipeWriteH );
                CloseHandle( ClientList[i].PipeReadH );
                CloseHandle( ClientList[i].OverlappedRead.hEvent );
                CloseHandle( ClientList[i].OverlappedWrite.hEvent );
                CloseHandle( ClientList[i].hThread );
            }
        }

        SetEvent( hEventRemoteTerm );
        if (WaitForSingleObject( RemoteServerThreadH, 10 * 1000 ) == WAIT_TIMEOUT) {
            TerminateThread( RemoteServerThreadH, 0 );
        }
        CloseHandle( hEventRemoteTerm );

        CmdLogFmt( "Remote server stopped for %s\r\n", PipeName );
        RemoteRunning = FALSE;
        FREE_STR(g_contWorkspace_WkSp.m_pszRemotePipe);
        NoPopups = FALSE;

        return;
    }

    if (RemoteRunning) {
        CmdLogFmt( "Remote server running for pipe <%s>\r\n", PipeName );
        for (i=0,j=0;i<MAX_SESSION;i++) {
            if (ClientList[i].Active) {
                j++;
            }
        }
        if (j) {
            CmdLogFmt( "%d remote connections:\r\n", j );
            for (i=0;i<MAX_SESSION;i++) {
                if (ClientList[i].Active) {
                    CmdLogFmt( "\tClient: %s\r\n", ClientList[i].Name );
                }
            }
        }
        return;
    }

    if (!*szPipeName) {
        CmdLogFmt( "Remote server is not running\r\n" );
        return;
    }


    InitClientList();

    g_contWorkspace_WkSp.m_bIgnoreAllSymbolErrors = TRUE;

    NoPopups = TRUE;
    strcpy( PipeName, szPipeName );
    
    FREE_STR(g_contWorkspace_WkSp.m_pszRemotePipe);
    g_contWorkspace_WkSp.m_pszRemotePipe = _strdup(szPipeName);
    
    HostName[0]='\0';
    Server[0]='\0';

    if (hFileLog == INVALID_HANDLE_VALUE) {
        HANDLE hMap;
        LPBYTE lpBase, lpb;
        CHAR buf[MAX_PATH];
        char szLogFileName[MAX_PATH];
        strncpy( szLogFileName, szPipeName, 8 );
        tmp_int = strlen(szPipeName);
        szLogFileName[min(tmp_int ,8)] = 0;
        strcat( szLogFileName, ".log" );
        LogFileOpen( szLogFileName, fAppend );
        if (fAppend) {
            hMap = CreateFileMapping( hFileLog, NULL, PAGE_READONLY, 0, 0, NULL );
            if (hMap) {
                lpb = lpBase = (PUCHAR) MapViewOfFile( hMap, FILE_MAP_READ, 0, 0, 0 );
                i = GetFileSize( hFileLog, NULL );
                while( i ) {
                    j = 0;
                    while( (*lpb != '\n') && (i) ) {
                        buf[j++] = *lpb++;
                        i--;
                    }
                    if (*lpb == '\n') {
                        i--;
                        buf[j++] = *lpb++;
                    }
                    buf[j] = 0;
                    CmdLogFmt( "%s", buf );
                }
                UnmapViewOfFile( lpBase );
                CloseHandle( lpb );
            }
        }
    }

    hEventRemoteTerm = CreateEvent( NULL, TRUE, FALSE, NULL );


    RemoteServerThreadH = CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)RemoteServerThread,
        (LPVOID)PipeName,
        0,
        &ThreadID
        );

    if (!RemoteServerThreadH) {
        CmdLogFmt( "Cannot start remote server for %s\r\n", szPipeName );
        return;
    }

    CmdLogFmt( "Remote server started for %s\r\n", szPipeName );

    RemoteRunning = TRUE;

    return;
}

BOOL
PipeConnect(
    HANDLE        hPipe,
    LPOVERLAPPED  OverlappedPipe,
    DWORD         dwTimeout
    )
{
    DWORD   ec;
    DWORD   status;
    
    if (INVALID_HANDLE_VALUE == hPipe || NULL == hPipe) {
        return FALSE;
    }
    
    ResetEvent( OverlappedPipe->hEvent );
    status = ConnectNamedPipe( hPipe, OverlappedPipe);
    
    if (!status) {
        ec = GetLastError();
        switch( ec ) {
        case ERROR_PIPE_CONNECTED:
            goto connected;
            
        case ERROR_IO_PENDING:
            break;
            
        default:
            return FALSE;
        }
        
        status = WaitForSingleObject( OverlappedPipe->hEvent, dwTimeout );
        switch ( status ) {
        case WAIT_OBJECT_0:
            goto connected;
            
        case WAIT_TIMEOUT:
        default:
            return FALSE;
        }
    }
    
connected:
    
    return TRUE;
}

DWORD
PipeRead(
    LPSESSION_TYPE  MyClient,
    PUCHAR          pch,
    DWORD           cch
    )
{
    DWORD cb;


    ResetEvent( MyClient->OverlappedRead.hEvent );

    if (ReadFile(MyClient->PipeReadH, pch, cch, &cb, &MyClient->OverlappedRead)) {
        //
        // Read was successful and finished return packet size and exit.
        //
        return cb;
    }

    //
    //  We got a failure case -- there are now two possibities.
    //      1.  -- we have overlapped I/O, or
    //      2.  -- we are messed up.
    //

    if (GetLastError() == ERROR_IO_PENDING) {

        cb = 0;

    } else {

        return 0;

    }

    while (TRUE) {
        if (GetOverlappedResult(MyClient->PipeReadH,
                                &MyClient->OverlappedRead,
                                &cb,
                                FALSE)) {
            //
            // Read has successfully completed
            //
            return cb;
        }
        if (GetLastError() == ERROR_BROKEN_PIPE) {
            break;
        }
        if (WaitForSingleObject( MyClient->hEventTerm, 0 ) == WAIT_OBJECT_0) {
            break;
        }
        Sleep( 100 );
    }

    return 0;
}

DWORD
PipeWrite(
    LPSESSION_TYPE  MyClient,
    const UCHAR *   pch,
    DWORD           cch,
    BOOL            fAsynch
    )
{
    DWORD cb;


    if (WriteFile(MyClient->PipeWriteH, pch, cch, &cb, &MyClient->OverlappedWrite )) {
        //
        // Write was successful and finished
        //
        return cb;
    }

    //
    //  We got a failure case -- there are now two possiblities.
    //  1.  -- we have overlapped I/O or
    //  2.  -- we are messed up
    //

    if (GetLastError() == ERROR_IO_PENDING) {

        cb = 0;

    } else {

        return 0;

    }

    if (GetOverlappedResult(MyClient->PipeWriteH, &MyClient->OverlappedWrite, &cb, !fAsynch)) {
        //
        // Write has successfully completed
        //
        return cb;
    }

    return 0;
}

DWORD
RemoteServerThread(
   LPSTR pipename
   )
{
    int                  i;
    DWORD                ThreadID;
    HANDLE               PipeH[2] = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE };
    OVERLAPPED           OverlappedPipe[2];
    HANDLE               TokenHandle;
    char                 fullnameIn[BUFFSIZE];
    char                 fullnameOut[BUFFSIZE];
    SECURITY_ATTRIBUTES  lsa;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    BYTE                 SDBuffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PACL                 Acl;
    BYTE                 AclBuffer[1024];

    static PSID EveryoneSid = NULL;


    sprintf( fullnameIn,  SERVER_READ_PIPE,  ".", pipename );
    sprintf( fullnameOut, SERVER_WRITE_PIPE, ".", pipename );

    //
    // Initialize the security descriptor that we're going to
    // use.
    //

    if (!EveryoneSid) {
        SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY ;
        AllocateAndInitializeSid( &WorldAuthority,
                                  1,
                                  SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &EveryoneSid
                                  );
    }

    SecurityDescriptor = (PSECURITY_DESCRIPTOR) SDBuffer;

    Acl = (PACL) AclBuffer;

    InitializeSecurityDescriptor(SecurityDescriptor,
                                 SECURITY_DESCRIPTOR_REVISION
                                 );

    InitializeAcl(Acl,
                  1024,
                  ACL_REVISION
                  );

    AddAccessAllowedAce(Acl,
                        ACL_REVISION,
                        GENERIC_ALL,
                        EveryoneSid
                        );

    SetSecurityDescriptorDacl(SecurityDescriptor,
                              TRUE,
                              Acl,
                              FALSE
                              );

    lsa.nLength = sizeof(SECURITY_ATTRIBUTES);
    lsa.lpSecurityDescriptor = SecurityDescriptor;
    lsa.bInheritHandle = FALSE;

    OverlappedPipe[0].hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    OverlappedPipe[1].hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    while(TRUE) {
        PipeH[0] = CreateNamedPipe( fullnameIn ,
                                    PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                                    PIPE_TYPE_BYTE,
                                    PIPE_UNLIMITED_INSTANCES,
                                    0,
                                    0,
                                    1000,
                                    &lsa
                                  );

        PipeH[1] = CreateNamedPipe( fullnameOut,
                                    PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
                                    PIPE_TYPE_BYTE,
                                    PIPE_UNLIMITED_INSTANCES,
                                    0,
                                    0,
                                    1000,
                                    &lsa
                                 );

        if (INVALID_HANDLE_VALUE == PipeH[0] 
            || INVALID_HANDLE_VALUE == PipeH[1]
            || !PipeConnect( PipeH[0], &OverlappedPipe[0], 1000 )) {

            CloseHandle(PipeH[0]);
            CloseHandle(PipeH[1]);
            if (WaitForSingleObject( hEventRemoteTerm, 0 ) == WAIT_OBJECT_0) {
                return 0;
            }
            continue;
        }

        if (!PipeConnect( PipeH[1], &OverlappedPipe[1], INFINITE )) {
            CloseHandle(PipeH[0]);
            CloseHandle(PipeH[1]);
            continue;
        }

        //
        // Look For a Free Slot & if not- then terminate connection
        //

        for (i=1;i<MAX_SESSION;i++) {
            //
            // Locate a Free Client block
            //
            if (!ClientList[i].Active) {
                break;
            }
        }

        if (i<MAX_SESSION) {

            //
            // Initialize the Client
            //
            ClientList[i].PipeReadH = PipeH[0];
            ClientList[i].PipeWriteH = PipeH[1];
            ClientList[i].Active = TRUE;
            ClientList[i].SendOutput = TRUE;
            ClientList[i].CommandRcvd = FALSE;
            ClientList[i].OverlappedRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            ClientList[i].OverlappedWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            ClientList[i].hEventTerm = CreateEvent(NULL, TRUE, FALSE, NULL);

        } else {

            PrintDebuggerMsg( TRUE, FALSE, "Remote:Closing New Session - No more slots\n");
            CloseHandle(PipeH[0]);
            CloseHandle(PipeH[1]);
            continue;

        }

        //
        // start new thread for this connection
        //

        ClientList[i].hThread = CreateThread(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE)RemoteSessionThread,
            &ClientList[i],
            0,
            &ThreadID
            );

        if (!ClientList[i].hThread) {
            CloseClient(&ClientList[i]);
            continue;
        }
    }

    return 0;
}


BOOL
DeliverHistory(
    LPSESSION_TYPE MyClient,
    DWORD          MaxLines
    )
{
    HANDLE LogMap;
    LPBYTE lpLog;
    LPBYTE p;
    DWORD  i;
    DWORD  cb;
    DWORD  lines;


    if (hFileLog == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    EnterCriticalSection( &csLog );
    SetFilePointer( hFileLog, 0, NULL, FILE_BEGIN );

    LogMap = CreateFileMapping(
        hFileLog,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL
        );

    if (LogMap == 0) {
        LeaveCriticalSection( &csLog );
        return FALSE;
    }

    lpLog = (PUCHAR) MapViewOfFile(
        LogMap,
        FILE_MAP_READ,
        0,
        0,
        0
        );

    if (lpLog == NULL) {
        LeaveCriticalSection( &csLog );
        return FALSE;
    }

    cb = GetFileSize( hFileLog, NULL );
    p = lpLog + cb - 1;
    i = 0;
    lines = 0;
    while (i < cb) {
        if (*p == '\n') {
            ++lines;
        }
        if (lines == MaxLines) {
            break;
        }
        --p;
        ++i;
    }

    PipeWrite( MyClient, p+1, i, FALSE );
    SetFilePointer( hFileLog, 0, NULL, FILE_END );
    UnmapViewOfFile( lpLog );
    CloseHandle( LogMap );
    LeaveCriticalSection( &csLog );

    return TRUE;
}


DWORD
RemoteSessionThread(
    LPSESSION_TYPE MyClient
    )
{
    DWORD                cb;
    DWORD                ReadCnt;
    SESSION_STARTUPINFO  ssi;
    LPSTR                headerbuff;
    CHAR                 msg[BUFFSIZE];
    SESSION_STARTREPLY   ssr;
    SYSTEMTIME           st;
    DWORD                reply=0;
    LPSTR                ptr;
    LPSTR                pPacket;


    GetLocalTime( &st );
    ZeroMemory( &ssi, sizeof(ssi) );

    ReadFixBytes( MyClient, (LPSTR)MyClient->Name, HOSTNAMELEN-1 );

    //
    //Last four Bytes contains a code
    //
    memcpy((LPSTR)&reply,(LPSTR)&(MyClient->Name[11]),4);

    if (reply!=MAGICNUMBER) {
        //
        // Unknown client
        //
        CloseClient(MyClient);
        return 1;
    }

    ssr.MagicNumber = MAGICNUMBER;
    ssr.Size = sizeof(ssr);
    ssr.FileSize = 0;

    PipeWrite( MyClient, (PUCHAR) &ssr, sizeof(ssr), FALSE );

    if (ReadFixBytes(MyClient,(char *)&(ssi.Size),sizeof(ssi.Size))!=0) {
        CloseClient(MyClient);
        return 1;
    }

    if (ssi.Size>1024) {
        sprintf(msg,"%s","Server:Unknown Header..Terminating session\n");
        PipeWrite(MyClient, (PUCHAR) msg,strlen(msg), FALSE );
        CloseClient(MyClient);
        return 1;
    }

    if ((headerbuff=(char *)calloc(ssi.Size,1))==NULL) {
        sprintf(msg,"%s","Server:Not Enough Memory..Terminating session\n");
        PipeWrite(MyClient, (PUCHAR) msg,strlen(msg), FALSE );
        CloseClient(MyClient);
        return 1;
    }

    ReadCnt=ssi.Size-sizeof(ssi.Size);
    if (ReadFixBytes(MyClient,(char *)headerbuff,ReadCnt)!=0) {
        CloseClient(MyClient);
        return 1;
    }

    memcpy((char *)&ssi+sizeof(ssi.Size),headerbuff,sizeof(ssi)-sizeof(ssi.Size));
    free(headerbuff);

    //
    // Version
    //
    if (ssi.Version!=VERSION) {
         sprintf(msg,"Remote Warning:Server Version=%d Client Version=%d\n",VERSION,ssi.Version);
         PipeWrite(MyClient, (PUCHAR) msg,strlen(msg), FALSE );
    }

    //
    // Name
    //
    memcpy(MyClient->Name,ssi.ClientName,15);
    MyClient->Name[14]=0;

    DeliverHistory( MyClient, ssi.LinesToSend );

    PrintDebuggerMsg(TRUE, FALSE, "Remote:Connected To %s [%02d:%02d]\n",MyClient->Name,st.wHour,st.wMinute);

    MyClient->lpBuf = (PSTR) malloc( BUFFSIZE );

    CmdExecutePrompt( TRUE, FALSE );

    while( TRUE ) {

        cb = PipeRead( MyClient, (PUCHAR) MyClient->lpBuf, BUFFSIZE );
        if (!cb) {
            break;
        }

        MyClient->lpBuf[cb]=0;
        MyClient->CommandRcvd=TRUE;

        if (FilterCommand( MyClient, MyClient->lpBuf, cb)) {
            continue;
        }

        ptr = &MyClient->lpBuf[strlen(MyClient->lpBuf) - 1];
        while ( *ptr == '\n' || *ptr == '\r')  {
            *ptr-- = '\0';
        }

        PrintCmdMsg( MyClient, FALSE, FALSE, (LPSTR)MyClient->lpBuf );

        pPacket = (PSTR) malloc(strlen(MyClient->lpBuf) + 1);
        Assert(pPacket);
        strcpy(pPacket, MyClient->lpBuf);
        PostMessage( Views[cmdView].hwndClient, WU_LOG_REMOTE_CMD, TRUE,
                                                             (LPARAM)pPacket );
    }

    PrintDebuggerMsg( TRUE, FALSE, "Remote:Disconnected From %s [%02d:%02d]\n",MyClient->Name,st.wHour,st.wMinute);

    CloseClient( MyClient );
    return 0;
}


/*
VOID
SendClientOutput(
    LPCSTR lpBuf,
    DWORD  cbBuf
    )
{
    DWORD  i;

    for (i=0;i<MAX_SESSION;i++) {
        if (ClientList[i].Active) {
            PipeWrite( &ClientList[i], (PUCHAR) lpBuf, cbBuf, TRUE );
        }
    }
}
*/


BOOL
FilterCommand(
    LPSESSION_TYPE  cl,
    LPSTR           buf,
    DWORD           cb
    )
{
    SYSTEMTIME  st;
    char        inp_buf[4096];
    DWORD       ThreadID;
    LPSTR       mssg;
    LPSTR       ack;
    BOOL        ret = FALSE;


    if (!cb) {
        return FALSE;
    }

    buf[cb]=0;

    GetLocalTime( &st );

    if (buf[0] == COMMANDCHAR) {
        switch(tolower(buf[1])) {
            case 'o':
                cl->SendOutput = !cl->SendOutput;
                break;

            case 's':
                SendStatus( cl );
                break;

            case 'p':
                mssg = (LPSTR) calloc( 4096, 1 );
                ack = "Remote:Popup Shown..\n";

                if (!mssg) {
                    break;
                }

                sprintf( mssg, "From %s [%d:%d]\n\n%s\n",
                         cl->Name,
                         st.wHour,
                         st.wMinute,
                         &buf[2]
                       );
                CreateThread( NULL,
                              0,
                              (LPTHREAD_START_ROUTINE)ShowPopupThread,
                              mssg,
                              0,
                              &ThreadID
                            );
                PipeWrite( cl, (PUCHAR) ack, strlen(ack), FALSE );
                break;

            case 'm':
                buf[cb-2]=0;
                PrintCmdMsg( cl, TRUE, TRUE, buf );
                break;

            case '@':
                buf[cb-2]=0;
                PrintCmdMsg( cl, TRUE, TRUE, buf );
                //
                // Remove the first @ sign
                //
                MoveMemory( buf, &buf[1], cb-1 );
                buf[cb-1]=' ';
                return FALSE;
                break;

            default :
                sprintf( inp_buf, "%s","** Unknown Command **\n" );
                PipeWrite( cl, (PUCHAR) inp_buf, strlen(inp_buf), FALSE );
                //
                // fall through
                //

            case 'h':
            case 'H':
                sprintf( inp_buf, "%cM: To Send Message\n", COMMANDCHAR );
                PipeWrite( cl, (PUCHAR) inp_buf, strlen(inp_buf), FALSE );
                sprintf( inp_buf, "%cP: To Generate popup\n", COMMANDCHAR );
                PipeWrite( cl, (PUCHAR) inp_buf, strlen(inp_buf), FALSE );
                sprintf( inp_buf,"%cK: To kill the server\n", COMMANDCHAR );
                PipeWrite( cl, (PUCHAR) inp_buf, strlen(inp_buf), FALSE );
                sprintf( inp_buf, "%cH: This Help\n", COMMANDCHAR );
                PipeWrite( cl, (PUCHAR) inp_buf, strlen(inp_buf), FALSE );
                break;
        }
        return TRUE;
    }

    if (buf[0]<26) {
        if (buf[0] == CTRLC) {
            cl->CommandRcvd = FALSE;
            DispatchCtrlCEvent();
            return TRUE;
        }
    }

    return FALSE;
}

VOID
SendStatus(
    LPSESSION_TYPE  cl
    )
{
    CHAR   buf[1024];
    DWORD  i;
    LPSTR  env;
    DWORD  ver;


    env = GetEnvironmentStrings();
    ver = GetVersion();

    sprintf(buf,"Server = %s PIPE=%s\n",HostName,PipeName);
    PipeWrite(cl, (PUCHAR) buf,strlen(buf), FALSE);

    sprintf(buf,"Build = %d \n",((WORD *)&ver)[1]);
    PipeWrite(cl, (PUCHAR) buf,strlen(buf), FALSE);

    for (i=1;i<MAX_SESSION;i++) {
        if (ClientList[i].Active) {
            sprintf(buf,"ACTIVE SESSION=%s\n",ClientList[i].Name);
            PipeWrite(cl, (PUCHAR) buf,strlen(buf), FALSE);
        }
    }

    sprintf(buf,"====================\n"/*,Server,PipeName*/);
    PipeWrite(cl, (PUCHAR) buf,strlen(buf), FALSE);

    sprintf(buf,"ENVIRONMENT VARIABLES\n"/*,Server,PipeName*/);
    PipeWrite(cl, (PUCHAR) buf,strlen(buf), FALSE);

    sprintf(buf,"====================\n"/*,Server,PipeName*/);
    PipeWrite(cl, (PUCHAR) buf,strlen(buf), FALSE);


    __try {
        while (*env!=0) {
            sprintf(buf,"%s\n",env);
            PipeWrite(cl, (PUCHAR) buf,strlen(buf), FALSE);

            while(*(env++)!=0);
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        sprintf(buf,"Exception Generated Getting Environment Block\n",env);
        PipeWrite(cl, (PUCHAR) buf,strlen(buf), FALSE);
    }

    sprintf(buf,"====================\n",Server,PipeName);
    PipeWrite(cl, (PUCHAR) buf,strlen(buf), FALSE);
    return;
}

DWORD
ShowPopupThread(
    LPSTR mssg
    )
{
    MessageBox( GetActiveWindow(), mssg, "***WinDbg Remote***", MB_OK | MB_SETFOREGROUND );
    free( mssg );
    return 0;
}

VOID
InitClientList(
    void
    )
{
    int i;

    for (i=0;i<MAX_SESSION;i++) {
        ZeroMemory( ClientList[i].Name, HOSTNAMELEN );
        ClientList[i].PipeReadH   = INVALID_HANDLE_VALUE;
        ClientList[i].PipeWriteH  = INVALID_HANDLE_VALUE;
        ClientList[i].Active      = FALSE;
        ClientList[i].CommandRcvd = FALSE;
        ClientList[i].SendOutput  = FALSE;
        ClientList[i].hThread     = NULL;
    }

    return;
}

DWORD
ReadFixBytes(
    LPSESSION_TYPE MyClient,
    LPSTR          lpBuf,
    DWORD          cbRead
    )
{
    DWORD cb;
    DWORD xyzBytesRead=0;
    DWORD xyzBytesToRead=cbRead;
    LPSTR xyzbuf=lpBuf;

    while(xyzBytesToRead!=0) {
        cb = PipeRead(MyClient, (PUCHAR) xyzbuf, xyzBytesToRead );

        if (!cb) {
            return xyzBytesToRead;
        }

        xyzBytesToRead -= cb;
        xyzbuf += cb;
    }

    return 0;
}

VOID
CloseClient(
    SESSION_TYPE *Client
    )
{
    ZeroMemory(Client->Name,HOSTNAMELEN);

    if (Client->PipeReadH!=INVALID_HANDLE_VALUE)  {
        CloseHandle(Client->PipeReadH);
        Client->PipeReadH=INVALID_HANDLE_VALUE;
    }

    if (Client->PipeWriteH!=INVALID_HANDLE_VALUE) {
        CloseHandle(Client->PipeWriteH);
        Client->PipeWriteH=INVALID_HANDLE_VALUE;
    }


    Client->Active=FALSE; //Keep it last else synch problem.
    return;
}


VOID
PrintCmdMsg(
    LPSESSION_TYPE MyClient,
    BOOL           fInsert,
    BOOL           fLog,
    LPSTR          InpBuf
    )
{
    SYSTEMTIME st;

    GetLocalTime( &st );

    PrintDebuggerMsg(
        fInsert,
        fLog,
        "%-15s    [%-15s %2d:%02d %2d/%2d/%4d]\n" ,
        InpBuf,
        MyClient->Name,
        st.wHour,
        st.wMinute,
        st.wMonth,
        st.wDay,
        st.wYear
        );
}


VOID
PrintDebuggerMsg(
    BOOL  fInsert,
    BOOL  fLog,
    LPSTR lpFmt,
    ...
    )
{
    LPSTR   lpText;
    va_list vargs;

    lpText = (PSTR) malloc( MAX_VAR_MSG_TXT );

    va_start(vargs, lpFmt);
    _vsnprintf( lpText, MAX_VAR_MSG_TXT, lpFmt, vargs);
    va_end(vargs);

    if (fLog) {
        LogFileWrite( (PUCHAR) lpText, strlen(lpText) );
    }

    PostMessage( Views[cmdView].hwndClient, WU_LOG_REMOTE_MSG, fInsert, (LPARAM)lpText );
}
