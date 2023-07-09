#define VERSION             4
#define REMOTE_SERVER       1
#define REMOTE_CLIENT       2

#define SERVER_READ_PIPE    "\\\\%s\\PIPE\\%sIN"   //Client Writes and Server Reads
#define SERVER_WRITE_PIPE   "\\\\%s\\PIPE\\%sOUT"  //Server Reads  and Client Writes

#define COMMANDCHAR         '@' //Commands intended for remote begins with this
#define CTRLC               3

#define HOSTNAMELEN         16

#define CHARS_PER_LINE      45

#define MAGICNUMBER     0x31109000
#define LINESTOSEND     200

#define MAX_SESSION     10

typedef struct {
    DWORD    Size;
    DWORD    Version;
    CHAR     ClientName[15];
    DWORD    LinesToSend;
    DWORD    Flag;
} SESSION_STARTUPINFO, *LPSESSION_STARTUPINFO;

typedef struct {
    DWORD MagicNumber;             // New Remote
    DWORD Size;                    // Size of structure
    DWORD FileSize;                // Num bytes sent
} SESSION_STARTREPLY, *LPSESSION_STARTREPLY;

typedef struct {
    CHAR          Name[HOSTNAMELEN];     // Name of client Machine;
    BOOL          Active;                // Client at the other end connected
    BOOL          CommandRcvd;           // True if a command recieved
    BOOL          SendOutput;            // True if Sendoutput output
    HANDLE        PipeReadH;             // Client sends its StdIn  through this
    HANDLE        PipeWriteH;            // Client gets  its StdOut through this
    OVERLAPPED    OverlappedRead;        //
    OVERLAPPED    OverlappedWrite;       //
    LPSTR         lpBuf;                 // Read buffer
    HANDLE        hThread;               // Session Thread
    HANDLE        hEventTerm;            //
} SESSION_TYPE, *LPSESSION_TYPE;

VOID
StartRemoteServer(
    LPSTR PipeName,
    BOOL  fAppend
    );

/*
VOID
SendClientOutput(
    LPCSTR lpBuf,
    DWORD  cbBuf
    );
*/