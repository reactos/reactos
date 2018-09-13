VOID
StopRemoteServer(
    VOID
    );

VOID
StartRemoteServer(
    LPSTR PipeName,
    BOOL  fAppend
    );

VOID
SendClientOutput(
    LPCSTR lpBuf,
    DWORD  cbBuf
    );

