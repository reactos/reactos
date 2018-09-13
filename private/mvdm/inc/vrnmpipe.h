/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrnmpipe.h

Abstract:

    Prototypes, definitions and structures for VdmRedir named pipe handlers

Author:

    Richard L Firth (rfirth) 10-Sep-1991

Revision History:

    10-Sep-1991 RFirth
        Created

--*/

//
// manifests
//

#define MAXIMUM_ASYNC_PIPES 32

//
// async named pipe request types
//

#define ANP_READ    0x86
#define ANP_READ2   0x90
#define ANP_WRITE   0x8f
#define ANP_WRITE2  0x91

//
// VDM Named Pipe support routines. Prototypes
//

VOID
VrGetNamedPipeInfo(
    VOID
    );

VOID
VrGetNamedPipeHandleState(
    VOID
    );

VOID
VrSetNamedPipeHandleState(
    VOID
    );

VOID
VrPeekNamedPipe(
    VOID
    );

VOID
VrTransactNamedPipe(
    VOID
    );

VOID
VrCallNamedPipe(
    VOID
    );

VOID
VrWaitNamedPipe(
    VOID
    );

VOID
VrNetHandleGetInfo(
    VOID
    );

VOID
VrNetHandleSetInfo(
    VOID
    );

VOID
VrReadWriteAsyncNmPipe(
    VOID
    );

BOOLEAN
VrNmPipeInterrupt(
    VOID
    );

VOID
VrTerminateNamedPipes(
    IN WORD DosPdb
    );

//
// VDM open/close and read/write intercept routines
//

#ifdef VDMREDIR_DLL

BOOL
VrAddOpenNamedPipeInfo(
    IN  HANDLE  Handle,
    IN  LPSTR   lpFileName
    );

BOOL
VrRemoveOpenNamedPipeInfo(
    IN  HANDLE  Handle
    );

BOOL
VrReadNamedPipe(
    IN  HANDLE  Handle,
    IN  LPBYTE  Buffer,
    IN  DWORD   Buflen,
    OUT LPDWORD BytesRead,
    OUT LPDWORD Error
    );

BOOL
VrWriteNamedPipe(
    IN  HANDLE  Handle,
    IN  LPBYTE  Buffer,
    IN  DWORD   Buflen,
    OUT LPDWORD BytesWritten
    );

VOID
VrCancelPipeIo(
    IN DWORD Thread
    );

#else

BOOL
(*VrAddOpenNamedPipeInfo)(
    IN  HANDLE  Handle,
    IN  LPSTR   lpFileName
    );

BOOL
(*VrRemoveOpenNamedPipeInfo)(
    IN  HANDLE  Handle
    );

BOOL
(*VrReadNamedPipe)(
    IN  HANDLE  Handle,
    IN  LPBYTE  Buffer,
    IN  DWORD   Buflen,
    OUT LPDWORD BytesRead,
    OUT LPDWORD Error
    );

BOOL
(*VrWriteNamedPipe)(
    IN  HANDLE  Handle,
    IN  LPBYTE  Buffer,
    IN  DWORD   Buflen,
    OUT LPDWORD BytesWritten
    );

VOID
(*VrCancelPipeIo)(
    IN DWORD Thread
    );

#endif

//
// VDM pipe name to NT pipe name helper routines
//

#ifdef VDMREDIR_DLL

BOOL
VrIsNamedPipeName(
    IN  LPSTR   Name
    );

BOOL
VrIsNamedPipeHandle(
    IN  HANDLE  Handle
    );

LPSTR
VrConvertLocalNtPipeName(
    OUT LPSTR   Buffer OPTIONAL,
    IN  LPSTR   Name
    );

#else

BOOL
(*VrIsNamedPipeName)(
    IN  LPSTR   Name
    );

BOOL
(*VrIsNamedPipeHandle)(
    IN  HANDLE  Handle
    );

LPSTR
(*VrConvertLocalNtPipeName)(
    OUT LPSTR   Buffer OPTIONAL,
    IN  LPSTR   Name
    );

#endif

//
// structures
//

//typedef struct {
//    PDOSNMPINFO Next;           // pointer to next info structure in list
//    WORD    DosPdb;
//    WORD    Handle16;
//    HANDLE  Handle32;           // handle returned from CreateFile call
//    DWORD   NameLength;         // length of ASCIZ pipe name
//    LPSTR   Name;               // ASCIZ pipe name
//    DWORD   Instances;          // current instances
//} DOSNMPINFO, *PDOSNMPINFO;

//
// OPEN_NAMED_PIPE_INFO - this structure contains information recorded when a
// named pipe is opened on behalf of the VDM. DosQNmPipeInfo wants the name
// of the pipe
//

typedef struct _OPEN_NAMED_PIPE_INFO* POPEN_NAMED_PIPE_INFO;
typedef struct _OPEN_NAMED_PIPE_INFO {
    POPEN_NAMED_PIPE_INFO Next; // linked list
    HANDLE  Handle;             // open named pipe handle
    DWORD   NameLength;         // including terminating 0
    WORD    DosPdb;             // the process which owns this named pipe
    CHAR    Name[2];            // full pipe name
} OPEN_NAMED_PIPE_INFO;

//
// DOS_ASYNC_NAMED_PIPE_INFO - in this structure we keep all the information
// required to complete an asynchronous named pipe operation
//

typedef struct _DOS_ASYNC_NAMED_PIPE_INFO {
    struct _DOS_ASYNC_NAMED_PIPE_INFO* Next;  // linked list
    OVERLAPPED Overlapped;      // contains 32-bit event handle
    BOOL    Type2;              // TRUE if request is Read2 or Write2
    BOOL    Completed;          // TRUE if this request has completed
    HANDLE  Handle;             // 32-bit named pipe handle
    DWORD   Buffer;             // 16:16 address of buffer
    DWORD   BytesTransferred;   // actual number of bytes read/written
    LPWORD  pBytesTransferred;  // flat-32 pointer to returned read/write count in VDM
    LPWORD  pErrorCode;         // flat-32 pointer to returned error code in VDM
    DWORD   ANR;                // 16:16 address of ANR
    DWORD   Semaphore;          // 16:16 address of 'semaphore' in VDM
#if DBG
    DWORD   RequestType;
#endif
} DOS_ASYNC_NAMED_PIPE_INFO, *PDOS_ASYNC_NAMED_PIPE_INFO;

//
// DOS_CALL_NAMED_PIPE_STRUCT - this structure is created and handed to the DOS
// CallNmPipe routine because there is too much information to get into a 286's
// registers. This structure should be in apistruc.h, but it aint
//

//#include <packon.h>
#pragma pack(1)
typedef struct {
    DWORD   Timeout;            // Time to wait for pipe to become available
    LPWORD  lpBytesRead;        // pointer to returned bytes read
    WORD    nOutBufferLen;      // size of send data
    LPBYTE  lpOutBuffer;        // pointer to send data
    WORD    nInBufferLen;       // size of receive buffer
    LPBYTE  lpInBuffer;         // pointer to receive buffer
    LPSTR   lpPipeName;         // pointer to pipe name
} DOS_CALL_NAMED_PIPE_STRUCT, *PDOS_CALL_NAMED_PIPE_STRUCT;
//#include <packoff.h>
#pragma pack()

//
// DOS_ASYNC_NAMED_PIPE_STRUCT - as with the above, this structure is used
// to pass all the info to DosReadAsyncNmPipe which won't fit into registers.
// Used for read and write operations. Should be defined in apistruc.h
//

//#include <packon.h>
#pragma pack(1)
typedef struct {
    LPWORD  lpBytesRead;        // pointer to returned bytes read/written
    WORD    BufferLength;       // size of caller's buffer
    LPBYTE  lpBuffer;           // pointer to caller's buffer
    LPWORD  lpErrorCode;        // pointer to returned error code
    LPVOID  lpANR;              // pointer to Asynchronous Notification Routine
    WORD    PipeHandle;         // named pipe handle
    LPBYTE  lpSemaphore;        // pointer to caller's 'semaphore'
} DOS_ASYNC_NAMED_PIPE_STRUCT, *PDOS_ASYNC_NAMED_PIPE_STRUCT;
//#include <packoff.h>
#pragma pack()

//
// The following selectively copied from BSEDOS.H and other Lanman include
// files
//

/*** Data structures and equates used with named pipes ***/

//#include <packon.h>
#pragma pack(1)
typedef struct _PIPEINFO { /* nmpinf */
    USHORT cbOut;
    USHORT cbIn;
    BYTE   cbMaxInst;
    BYTE   cbCurInst;
    BYTE   cbName;
    CHAR   szName[1];
} PIPEINFO;
//#include <packoff.h>
#pragma pack()
typedef PIPEINFO FAR *PPIPEINFO;

/* defined bits in pipe mode */
#define NP_NBLK         0x8000 /* non-blocking read/write */
#define NP_SERVER       0x4000 /* set if server end   */
#define NP_WMESG        0x0400 /* write messages      */
#define NP_RMESG        0x0100 /* read as messages    */
#define NP_ICOUNT       0x00FF /* instance count field    */


/*  Named pipes may be in one of several states depending on the actions
 *  that have been taken on it by the server end and client end.  The
 *  following state/action table summarizes the valid state transitions:
 *
 *  Current state       Action          Next state
 *
 *   <none>         server DosMakeNmPipe    DISCONNECTED
 *   DISCONNECTED       server connect      LISTENING
 *   LISTENING      client open         CONNECTED
 *   CONNECTED      server disconn      DISCONNECTED
 *   CONNECTED      client close        CLOSING
 *   CLOSING        server disconn      DISCONNECTED
 *   CONNECTED      server close        CLOSING
 *   <any other>        server close        <pipe deallocated>
 *
 *  If a server disconnects his end of the pipe, the client end will enter a
 *  special state in which any future operations (except close) on the file
 *  descriptor associated with the pipe will return an error.
 */

/*
 *  Values for named pipe state
 */

#define NP_DISCONNECTED     1 /* after pipe creation or Disconnect */
#define NP_LISTENING        2 /* after DosNmPipeConnect        */
#define NP_CONNECTED        3 /* after Client open             */
#define NP_CLOSING      4 /* after Client or Server close      */

/* DosMakeNmPipe open modes */

#define NP_ACCESS_INBOUND   0x0000
#define NP_ACCESS_OUTBOUND  0x0001
#define NP_ACCESS_DUPLEX    0x0002
#define NP_INHERIT      0x0000
#define NP_NOINHERIT        0x0080
#define NP_WRITEBEHIND      0x0000
#define NP_NOWRITEBEHIND    0x4000

/* DosMakeNmPipe and DosQNmPHandState state */

#define NP_READMODE_BYTE    0x0000
#define NP_READMODE_MESSAGE 0x0100
#define NP_TYPE_BYTE        0x0000
#define NP_TYPE_MESSAGE     0x0400
#define NP_END_CLIENT       0x0000
#define NP_END_SERVER       0x4000
#define NP_WAIT         0x0000
#define NP_NOWAIT       0x8000
#define NP_UNLIMITED_INSTANCES  0x00FF

typedef struct _AVAILDATA   {   /* PeekNMPipe Bytes Available record */
    USHORT  cbpipe;     /* bytes left in the pipe        */
    USHORT  cbmessage;  /* bytes left in current message     */
} AVAILDATA;
typedef AVAILDATA FAR *PAVAILDATA;

//
// handle info level 1 - this is different to the structure in lmchdev.h
//

//#include <packon.h>
#pragma pack(1)
typedef struct _VDM_HANDLE_INFO_1 {
    ULONG   CharTime;
    USHORT  CharCount;
} VDM_HANDLE_INFO_1, *LPVDM_HANDLE_INFO_1;
#pragma pack()
//#include <packoff.h>
