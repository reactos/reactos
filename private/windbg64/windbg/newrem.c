#include "precomp.h"
#pragma hdrstop

BOOL fRemoteIsRunning = FALSE;
HANDLE hStdIn;
HANDLE hStdOut;
HANDLE hStdErr;
HANDLE hListenerThread;
HANDLE hRemoteProcess;

extern char DebuggerName[];

VOID
ListenerThread(
    LPVOID Arg
    );




VOID
StopRemoteServer(
    VOID
    )
{
    if (!fRemoteIsRunning) {
        CmdLogFmt("Remote is not running\r\b");
        return;
    }

    //
    // Just kill the remote server - the listener thread will do the rest
    //

    TerminateProcess(hRemoteProcess, 0);
}

VOID
StartRemoteServer(
    PTSTR pszPipeName,
    BOOL  /* fAppend */
    )

/*++

Routine Description:

    "remotes" the current debugger by starting a copy of remote.exe in a
    special mode that causes it to attach to us, the debugger, as its
    "child" process.

Arguments:

    pszPipeName - Supplies the name of the pipe to use for this remote session,
        e.g. "windbg" means to connect one would use 'remote /c machinename "windbg"'.

Return Value:

    None.

--*/

{
    HANDLE                  hRemoteChildProcess;
    HANDLE                  hRemoteWriteChildStdIn;
    HANDLE                  hRemoteReadChildStdOut;
    SECURITY_ATTRIBUTES     sa = {0};
    STARTUPINFO             si = {0};
    PROCESS_INFORMATION     pi = {0};
    DWORD                   ThreadID;
    TCHAR                   szCmd[_MAX_PATH * 2] = {0};
    PTSTR                   pszQuotedPipeName;


    // Put quotes around the pipe name, so that verbose users won't trash
    // things and then blame us.
    // +3: quotes at both ends and null terminator.
    pszQuotedPipeName = (PTSTR) malloc( (_tcslen(pszPipeName) +3) * sizeof(TCHAR) );
    if (pszQuotedPipeName) {
        _tcscpy(pszQuotedPipeName, _T("\""));
        _tcscat(pszQuotedPipeName, pszPipeName);
        _tcscat(pszQuotedPipeName, _T("\""));    

        // Always use the quoted pipe name unless we couldn't allocate
        // memory.
        pszPipeName = pszQuotedPipeName;
    }

    if (fRemoteIsRunning) {
        CmdLogFmt("!remote: can't !remote twice.\r\n");
        goto Cleanup;
    }

    CmdLogFmt("Starting remote with pipename '%s'\n", pszPipeName);

    //
    // We'll pass remote.exe inheritable handles to this process,
    // our standard in/out handles (for it to use as stdin/stdout),
    // and pipe handles for it to write to our new stdin and read
    // from our new stdout.
    //

    //
    // Get an inheritable handle to our process.
    //

    if ( ! DuplicateHandle(
               GetCurrentProcess(),           // src process
               GetCurrentProcess(),           // src handle
               GetCurrentProcess(),           // targ process
               &hRemoteChildProcess,          // targ handle
               0,                             // access
               TRUE,                          // inheritable
               DUPLICATE_SAME_ACCESS          // options
               )) {

        CmdLogFmt("!remote: Unable to duplicate process handle.\n");
        goto Cleanup;
    }

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    //
    // Create remote->windbg pipe, our end of which will be read
    // by our listener thread.  The remote.exe end needs to be opened
    // for overlapped I/O, so yet another copy of MyCreatePipeEx
    // spreads through our source base.
    //

    if ( ! MyCreatePipeEx(
               &hStdIn,                    // read handle
               &hRemoteWriteChildStdIn,    // write handle
               &sa,                        // security
               0,                          // size
               0,                          // read handle overlapped?
               FILE_FLAG_OVERLAPPED        // write handle overlapped?
               )) {

        CmdLogFmt("!remote: Unable to create stdin pipe.\n");
        CloseHandle(hRemoteChildProcess);
        goto Cleanup;
    }

    //
    // We don't want remote.exe to inherit our end of the pipe
    // so duplicate it to a non-inheritable one.
    //

    if ( ! DuplicateHandle(
               GetCurrentProcess(),           // src process
               hStdIn,                        // src handle
               GetCurrentProcess(),           // targ process
               &hStdIn,                       // targ handle
               0,                             // access
               FALSE,                         // inheritable
               DUPLICATE_SAME_ACCESS |
               DUPLICATE_CLOSE_SOURCE         // options
               )) {

        CmdLogFmt("!remote: Unable to duplicate stdin handle.\n");
        CloseHandle(hRemoteChildProcess);
        CloseHandle(hRemoteWriteChildStdIn);
        goto Cleanup;
    }

    //
    // Create windbg->remote pipe, our end of which will be our
    // output to remote.
    //

    if ( ! MyCreatePipeEx(
               &hRemoteReadChildStdOut,    // read handle
               &hStdOut,                   // write handle
               &sa,                        // security
               0,                          // size
               FILE_FLAG_OVERLAPPED,       // read handle overlapped?
               0                           // write handle overlapped?
               )) {

        CmdLogFmt("!remote: Unable to create stdout pipe.\n");
        CloseHandle(hRemoteChildProcess);
        CloseHandle(hRemoteWriteChildStdIn);
        CloseHandle(hStdIn);
        goto Cleanup;
    }

    //
    // We don't want remote.exe to inherit our end of the pipe
    // so duplicate it to a non-inheritable one.
    //

    if ( ! DuplicateHandle(
               GetCurrentProcess(),           // src process
               hStdOut,                       // src handle
               GetCurrentProcess(),           // targ process
               &hStdOut,                      // targ handle
               0,                             // access
               FALSE,                         // inheritable
               DUPLICATE_SAME_ACCESS |
               DUPLICATE_CLOSE_SOURCE         // options
               )) {

        CmdLogFmt("!remote: Unable to duplicate stdout handle.\n");
        CloseHandle(hRemoteChildProcess);
        CloseHandle(hRemoteWriteChildStdIn);
        CloseHandle(hStdIn);
        CloseHandle(hRemoteReadChildStdOut);
        goto Cleanup;
    }

    //
    // Duplicate our new stdout to a new stderr.
    //

    if ( ! DuplicateHandle(
               GetCurrentProcess(),           // src process
               hStdOut,                       // src handle
               GetCurrentProcess(),           // targ process
               &hStdErr,                      // targ handle
               0,                             // access
               FALSE,                         // inheritable
               DUPLICATE_SAME_ACCESS          // options
               )) {

        CmdLogFmt("!remote: Unable to duplicate stdout handle.\n");
        CloseHandle(hRemoteChildProcess);
        CloseHandle(hRemoteWriteChildStdIn);
        CloseHandle(hStdIn);
        CloseHandle(hRemoteReadChildStdOut);
        CloseHandle(hStdOut);
        goto Cleanup;
    }

    //
    // We now have all the handles we need.  Let's launch remote.
    //

    sprintf(
            szCmd,
            "remote.exe /a %u %u %u \"%s\" %s",
            HandleToUlong(hRemoteChildProcess),
            HandleToUlong(hRemoteWriteChildStdIn),
            HandleToUlong(hRemoteReadChildStdOut),
            DebuggerName,
            pszPipeName
            );

    si.cb            = sizeof(si);
    si.dwFlags       = 0;
    si.wShowWindow   = SW_SHOWMINIMIZED;

    //
    // Create Child Process
    //

    if ( ! CreateProcess(
               NULL,
               szCmd,
               NULL,
               NULL,
               TRUE,
               GetPriorityClass( GetCurrentProcess() ),
               NULL,
               NULL,
               &si,
               &pi)) {

        if (GetLastError()==2) {
            CmdLogFmt("remote.exe not found\n");
        } else {
            CmdLogFmt("CreateProcess(%s) failed, error %d.\n", szCmd, GetLastError());
        }

        CloseHandle(hRemoteChildProcess);
        CloseHandle(hRemoteWriteChildStdIn);
        CloseHandle(hStdIn);
        CloseHandle(hRemoteReadChildStdOut);
        CloseHandle(hStdOut);
        CloseHandle(hStdErr);
        goto Cleanup;
    }

    CloseHandle(hRemoteChildProcess);
    CloseHandle(hRemoteWriteChildStdIn);
    CloseHandle(hRemoteReadChildStdOut);
    CloseHandle(pi.hThread);

    hRemoteProcess = pi.hProcess;

    //
    // Create listener thread
    //

    hListenerThread = CreateThread(
                                   NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE)ListenerThread,
                                   NULL,
                                   0,
                                   &ThreadID
                                   );

    CloseHandle(hListenerThread);

    // If remote is terminates, then re-enable popups.
    NoPopups = TRUE;
    fRemoteIsRunning = TRUE;

    CmdLogFmt("\"%s\": now running under remote.exe pipename \"%s\"\n", DebuggerName, pszPipeName);

  Cleanup:
    if (pszQuotedPipeName) {
        free(pszQuotedPipeName);
    }

    return;

}

VOID
ListenerThread(
    LPVOID Unused
    )
{
    // This needs to be zeroed out or else the remote cmd line may have garbage in it.
    char Buf[MAX_USER_LINE+1];
    DWORD cb;

    while (ReadFile(hStdIn, Buf, sizeof(Buf), &cb, NULL)) {
        Buf[cb] = 0;
        LPSTR p = _tcsdup(Buf);
        PostMessage( Views[cmdView].hwndClient, WU_LOG_REMOTE_CMD, TRUE, (LPARAM)p );
    }

    //
    // If this exits, remote is gone, so disconnect
    //

    // If remote is running, then disable popups.
    NoPopups = FALSE;
    fRemoteIsRunning = FALSE;

    CloseHandle(hRemoteProcess);
    CloseHandle(hStdIn);
    CloseHandle(hStdOut);
    CloseHandle(hStdErr);
}



VOID
SendClientOutput(
    LPCSTR lpBuf,
    DWORD  cbBuf
    )
{
    DWORD  cb;
    if (fRemoteIsRunning) {
        WriteFile(hStdOut, lpBuf, cbBuf, &cb, NULL);
    }
}

