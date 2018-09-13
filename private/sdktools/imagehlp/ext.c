/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    imagehlp.c

Abstract:

    This function implements a generic simple symbol handler.

Author:

    Wesley Witt (wesw) 1-Sep-1994

Environment:

    User Mode

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntverp.h>
#include "private.h"
#include <symbols.h>

#define NOEXTAPI
#include <wdbgexts.h>
#undef DECLARE_API

#define DECLARE_API(s)                          \
    CPPMOD VOID                                 \
    s(                                          \
        HANDLE hCurrentProcess,                 \
        HANDLE hCurrentThread,                  \
        DWORD_PTR dwCurrentPc,                  \
        PWINDBG_EXTENSION_APIS lpExtensionApis, \
        LPSTR lpArgumentString                  \
     )

#define dprintf                 (ExtensionApis.lpOutputRoutine)
#define GetExpression           (ExtensionApis.lpGetExpressionRoutine)
#define GetSymbol               (ExtensionApis.lpGetSymbolRoutine)
#define Disasm                  (ExtensionApis.lpDisasmRoutine)
#define CheckControlC           (ExtensionApis.lpCheckControlCRoutine)
#define ReadMemory(a,b,c,d) \
    ((ExtensionApis.nSize == sizeof(WINDBG_OLD_EXTENSION_APIS)) ? \
    ReadProcessMemory( ExtensionCurrentProcess, (LPCVOID)(a), (b), (c), (d) ) \
  : ExtensionApis.lpReadProcessMemoryRoutine( (ULONG_PTR)(a), (b), (c), (d) ))

#define WriteMemory(a,b,c,d) \
    ((ExtensionApis.nSize == sizeof(WINDBG_OLD_EXTENSION_APIS)) ? \
    WriteProcessMemory( ExtensionCurrentProcess, (LPVOID)(a), (LPVOID)(b), (c), (d) ) \
  : ExtensionApis.lpWriteProcessMemoryRoutine( (ULONG_PTR)(a), (LPVOID)(b), (c), (d) ))

EXT_API_VERSION        ExtApiVersion = { (VER_PRODUCTVERSION_W >> 8), (VER_PRODUCTVERSION_W & 0xff), EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS  ExtensionApis;
ULONG                  STeip;
ULONG                  STebp;
ULONG                  STesp;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;
HANDLE                 ExtensionCurrentProcess;
BOOL                   fWinDbgExtension = FALSE;

VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    fWinDbgExtension = TRUE;
}

#if 0
VOID
CheckVersion(
    VOID
    )
{
#if DBG
    if ((SavedMajorVersion != 0x0c) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
    if ((SavedMajorVersion != 0x0f) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#endif
}
#endif

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ExtApiVersion;
}


/*++

Routine Description:

    This function is called as an NTSD extension to mimic the !handle
    kd command.  This will walk through the debuggee's handle table
    and duplicate the handle into the ntsd process, then call NtQueryobjectInfo
    to find out what it is.

    Called as:

        !handle [handle [flags [Type]]]

    If the handle is 0 or -1, all handles are scanned.  If the handle is not
    zero, that particular handle is examined.  The flags are as follows
    (corresponding to secexts.c):
        1   - Get type information (default)
        2   - Get basic information
        4   - Get name information
        8   - Get object specific info (where available)

    If Type is specified, only object of that type are scanned.  Type is a
    standard NT type name, e.g. Event, Semaphore, etc.  Case sensitive, of
    course.

    Examples:

        !handle     -- dumps the types of all the handles, and a summary table
        !handle 0 0 -- dumps a summary table of all the open handles
        !handle 0 f -- dumps everything we can find about a handle.
        !handle 0 f Event
                    -- dumps everything we can find about open events

--*/
DECLARE_API( sym )
{

    PWINDBG_EXTENSION_ROUTINE
    ExtensionCurrentProcess = hCurrentProcess;  

    if (!fWinDbgExtension && lpExtensionApis)
        ExtensionApis = *lpExtensionApis;

    SymParseArgs(lpArgumentString);
}

void
SymParseArgs(
    LPCSTR args
    )
{
    char argstr[1024];

    lstrcpy(argstr, args);
    _strlwr(argstr);

    if (strstr(argstr, "noisy") && !(SymOptions & SYMOPT_DEBUG)) {
        SymOptions |= SYMOPT_DEBUG;
        CPRINTF(NULL, ((SymOptions & SYMOPT_DEBUG) ? "Noisy mode on.\n" : "Quiet mode on.\n"));
    }

    if (strstr(argstr, "quiet") && (SymOptions & SYMOPT_DEBUG)) {
        SymOptions &= ~SYMOPT_DEBUG;
        CPRINTF(NULL, ((SymOptions & SYMOPT_DEBUG) ? "Noisy mode on.\n" : "Quiet mode on.\n"));
    }
}

