/*++

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    NtLmSsp service debug support

Author:

    Ported from Lan Man 2.0

Revision History:

    21-May-1991 (cliffv)
        Ported to NT.  Converted to NT style.
    09-Apr-1992 JohnRo
        Prepare for WCHAR.H (_wcsicmp vs _wcscmpi, etc).

    03-Aug-1996 ChandanS
        Stolen from net\svcdlls\ntlmssp\debug.h
--*/

//
// init.c will #include this file with DEBUG_ALLOCATE defined.
// That will cause each of these variables to be allocated.
//
#ifdef DEBUG_ALLOCATE
#define EXTERN
#else
#define EXTERN extern
#endif


////////////////////////////////////////////////////////////////////////
//
// Debug Definititions
//
////////////////////////////////////////////////////////////////////////

#define SSP_INIT            0x00000001 // Initialization
#define SSP_MISC            0x00000002 // Misc debug
#define SSP_API             0x00000004 // API processing
#define SSP_LPC             0x00000008 // LPC
#define SSP_CRITICAL        0x00000100 // Only real important errors
#define SSP_LEAK_TRACK      0x00000200 // calling PID etc


#define SSP_SESSION_KEYS    0x00001000  // keying material
#define SSP_NEGOTIATE_FLAGS 0x00002000  // negotiate flags.

//
// Very verbose bits
//

#define SSP_API_MORE        0x04000000 // verbose API
#define SSP_LPC_MORE        0x08000000 // verbose LPC

//
// Control bits.
//

#define SSP_NO_LOCAL        0x10000000 // Force client to use OEM character set
#define SSP_TIMESTAMP       0x20000000 // TimeStamp each output line
#define SSP_REQUEST_TARGET  0x40000000 // Force client to ask for target name
#define SSP_USE_OEM         0x80000000 // Force client to use OEM character set


//
// Name and directory of log file
//

#define DEBUG_DIR           L"\\debug"
#define DEBUG_FILE          L"\\ntlmssp.log"
#define DEBUG_BAK_FILE      L"\\ntlmssp.bak"

#if DBG

EXTERN DWORD SspGlobalDbflag;

#define IF_DEBUG(Function) \
     if (SspGlobalDbflag & SSP_ ## Function)

#define SspPrint(_x_) SspPrintRoutine _x_

VOID
SspPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR FORMATSTRING,     // PRINTF()-STYLE FORMAT STRING.
    ...                                 // OTHER ARGUMENTS ARE POSSIBLE.
    );

VOID
SspDumpHexData(
    IN DWORD DebugFlag,
    IN LPDWORD Buffer,
    IN DWORD BufferSize
    );

VOID
SspDumpSid(
    IN DWORD DebugFlag,
    IN PSID Sid
    );

VOID
SspDumpBuffer(
    IN DWORD DebugFlag,
    IN PVOID Buffer,
    IN DWORD BufferSize
    );

VOID
SspOpenDebugFile(
    IN BOOL ReopenFlag
    );

//
// Debug log file
//

EXTERN HANDLE SspGlobalLogFile;
#define DEFAULT_MAXIMUM_LOGFILE_SIZE 20000000
EXTERN DWORD SspGlobalLogFileMaxSize;

//
// To serialize access to log file.
//

EXTERN CRITICAL_SECTION SspGlobalLogFileCritSect;
EXTERN LPWSTR SspGlobalDebugSharePath;

#else

#define IF_DEBUG(Function) if (FALSE)

// Nondebug version.
#define SspDumpHexData        /* no output; ignore arguments */
#define SspDumpBuffer
#define SspDumpSid
#define SspPrint(_x_)

#endif // DBG

#undef EXTERN
