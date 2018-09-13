/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    Cmdexec0.c

Abstract:

    This file contains the front end code for parsing the various commands
    for the command window, and the code for the debugger control commands.

Author:

    David J. Gilman (davegi) 21-Apr-92

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop

//
// prototypes
//
ULONG64
WDBGAPI
ExtGetExpression(
    LPCSTR lpsz
    );

VOID
WDBGAPI
ExtGetSymbol(
    ULONG64 offset,
    PUCHAR pchBuffer,
    PULONG64 lpDisplacement
    );

ULONG
WDBGAPI
ExtDisasm(
    PULONG64 lpOffset,
    PCSTR lpBuffer,
    ULONG fShowEffectiveAddress
    );

ULONG
WDBGAPI
ExtReadProcessMemory(
    ULONG64 offset,
    LPVOID lpBuffer,
    DWORD cb,
    LPDWORD lpcbBytesRead
    );

ULONG
WDBGAPI
ExtWriteProcessMemory(
    ULONG64 offset,
    LPCVOID lpBuffer,
    DWORD cb,
    LPDWORD lpcbBytesWritten
    );

ULONG
WDBGAPI
ExtGetThreadContext(
    DWORD Processor,
    LPCONTEXT lpContext,
    DWORD cbSizeOfContext
    );

ULONG
WDBGAPI
ExtSetThreadContext(
    DWORD Processor,
    LPCONTEXT lpContext,
    DWORD cbSizeOfContext
    );

ULONG
WDBGAPI
ExtIoctl(
    USHORT IoctlType,
    LPVOID lpvData,
    DWORD cbSize
    );

DWORD
ExtCallStack(
    ULONG64 FramePointer,
    ULONG64 StackPointer,
    ULONG64 ProgramCounter,
    PEXTSTACKTRACE64 StackFrames,
    DWORD Frames
    );

DWORD dwOsVersion;


#define MAX_EXTDLLS           32
#define DEFAULT_EXTENSION_LIB "ntsdexts"

#define EXT_NOT_LOADED        0
#define EXT_LOADED            1
#define EXT_NEEDS_LOADING     2

typedef struct _EXTCOMMANDS {
    CMDID   CmdId;
    LPSTR   CmdString;
    LPSTR   HelpString;
} EXTCOMMANDS, *LPEXTCOMMANDS;

typedef struct _EXTLOAD {
    HMODULE                         hModule;
    DWORD                           dwCalls;
    BOOL                            bOldStyle;
    DWORD                           dwLoaded;
    BOOL                            bDoVersionCheck;
    // Does not include the ".dll" at the end.
    TCHAR                           szPathToDll[_MAX_PATH];
    TCHAR                           szDllName[_MAX_PATH];
    PWINDBG_EXTENSION_DLL_INIT64    pDllInit;
    PWINDBG_CHECK_VERSION           pCheckVersionRoutine;
    PWINDBG_EXTENSION_API_VERSION   pApiVersionRoutine;
    EXT_API_VERSION                 ApiVersion;
    EXCEPTION_RECORD                ExceptionRecord;
} EXTLOAD, *LPEXTLOAD;


WINDBG_EXTENSION_APIS64
WindbgExtensions =
{
    sizeof(WindbgExtensions),
    CmdLogFmt,
    ExtGetExpression,
    ExtGetSymbol,
    ExtDisasm,
    CheckCtrlCTrap,
    ExtReadProcessMemory,
    ExtWriteProcessMemory,
    ExtGetThreadContext,
    ExtSetThreadContext,
    ExtIoctl,
    ExtCallStack
};


//
// Old 32 bit support
//
ULONG
WDBGAPI
ExtGetExpression32(
    LPCSTR lpsz
    )
{
    return (ULONG)ExtGetExpression(lpsz);
}


VOID
WDBGAPI
ExtGetSymbol32(
    ULONG offset,
    PUCHAR pchBuffer,
    PULONG lpDisplacement
    )
{
    ULONG64 displacement;
    ExtGetSymbol( SE32To64(offset), pchBuffer, &displacement);
    *lpDisplacement = (ULONG)displacement;
}

ULONG
WDBGAPI
ExtDisasm32(
    PULONG lpOffset,
    PCSTR lpBuffer,
    ULONG fShowEffectiveAddress
    )
{
    ULONG64 offset = SE32To64( *lpOffset );
    return ExtDisasm(&offset, lpBuffer, fShowEffectiveAddress);
    *lpOffset = (ULONG)offset;
}

ULONG
WDBGAPI
ExtReadProcessMemory32(
    ULONG offset,
    LPVOID lpBuffer,
    DWORD cb,
    LPDWORD lpcbBytesRead
    )
{
    return ExtReadProcessMemory( SE32To64(offset), lpBuffer, cb, lpcbBytesRead);
}

ULONG
WDBGAPI
ExtWriteProcessMemory32(
    ULONG offset,
    LPCVOID lpBuffer,
    DWORD cb,
    LPDWORD lpcbBytesWritten
    )
{
    return ExtWriteProcessMemory( SE32To64(offset), lpBuffer, cb, lpcbBytesWritten);
}

ULONG
WDBGAPI
ExtIoctl32(
    USHORT IoctlType,
    LPVOID lpvData,
    DWORD cbSize
    );

DWORD
ExtCallStack32(
    ULONG FramePointer,
    ULONG StackPointer,
    ULONG ProgramCounter,
    PEXTSTACKTRACE32 StackFrames,
    DWORD Frames
    );

WINDBG_EXTENSION_APIS32
WindbgExtensions32 =
{
    sizeof(WindbgExtensions32),
    CmdLogFmt,
    ExtGetExpression32,
    ExtGetSymbol32,
    ExtDisasm32,
    CheckCtrlCTrap,
    ExtReadProcessMemory32,
    ExtWriteProcessMemory32,
    ExtGetThreadContext,
    ExtSetThreadContext,
    ExtIoctl32,
    ExtCallStack32
};

static
EXTCOMMANDS
ExtCommands[] = {
    CMDID_NULL,         NULL,            NULL,
    CMDID_HELP,         "?",             "Display this list",
    CMDID_DEFAULT,      "default",       "default <dll> - Change the default extension DLL.",
    CMDID_LOAD,         "load",          "load <path> - Load an extension DLL",
    CMDID_LIST_EXTS,    "listexts",      "List all loaded extension DLLs",
    CMDID_NOVERSION,    "noversion",     "Disable extension DLL version checking",
    CMDID_RELOAD,       "reload",        "Reload kernel symbols",
    CMDID_SYMPATH,      "sympath",       "Change or display the symbol path",
    CMDID_UNLOAD,       "unload",        "Unload the default extension DLL",
    };

#define ExtMaxCommands (sizeof(ExtCommands)/sizeof(EXTCOMMANDS))

static  EXTLOAD  LoadedExts[MAX_EXTDLLS];
static  DWORD    NumLoadedExts = 0;
static  LPSTR    pszDefaultExtName;
static  LPSTR    pszDefaultExtFullPath;

#define BUILD_MAJOR_VERSION 3
#define BUILD_MINOR_VERSION 5
API_VERSION ApiVersion = { BUILD_MAJOR_VERSION, BUILD_MINOR_VERSION, API_VERSION_NUMBER, 0 };

extern  LPSHF    Lpshf;   // vector table for symbol handler
extern  CXF      CxfIp;
extern LPPD    LppdCommand;
extern LPTD    LptdCommand;



ULONG64
WDBGAPI
ExtGetExpression(
    LPCSTR lpsz
    )

/*++

Routine Description:

  This function gets the expression specified by lpsz.

Arguments:

  lpsz - Supplies the expression string.

Return Value:

  Value of expression.

--*/

{
    ADDR  addr;
    int   cch;
    char  *expr;


    lpsz = CPSkipWhitespace(lpsz);

    if (CPGetAddress(lpsz, &cch, &addr, radix, &CxfIp, fCaseSensitive, TRUE) != 0) {
        if (pszDefaultExtName &&
            tolower(pszDefaultExtName[0]) == 'k' &&
            tolower(pszDefaultExtName[1]) == 'd') {

            expr = (PSTR) malloc( strlen(lpsz) + 32 );
            Assert(expr);
            strcpy( expr, "nt!" );
            strcat( expr, lpsz );
            if (CPGetAddress(expr, &cch, &addr, radix, &CxfIp, fCaseSensitive, TRUE) != 0) {
                free( expr );
                return 0;
            }
            free( expr );
        } else {
            return 0;
        }
    }

    SYFixupAddr(&addr);

    return addr.addr.off;
}



VOID
WDBGAPI
ExtGetSymbol(
    ULONG64  offset,
    PUCHAR  pchBufferArg,
    PULONG64 lpDisplacement
    )

/*++

Routine Description:

  This function gets the symbol string.

Arguments:

  offset - Address offset.

  pchBuffer - Pointer to the buffer to store symbol string.

  lpDisplacement - Pointer to the displacement.

Return Value:

  None.

--*/

{
    PSTR pchBuffer = (PSTR) pchBufferArg;
    ADDR   addr;
    LPADDR loc = SHpADDRFrompCXT(&CxfIp.cxt);
    ODR    odr;

    AddrInit( &addr, 0, 0, offset, TRUE, TRUE, FALSE, FALSE );
    SYFixupAddr(&addr);

    if (pchBuffer[0] == '!') {
        if (SHGetModule(&addr, pchBuffer)) {
            _strupr( pchBuffer );
            strcat( pchBuffer, "!" );
            pchBuffer += strlen( pchBuffer);
        } else {
            *pchBuffer = '\0';
        }
    } else {
        *pchBuffer = '\0';
    }

    odr.lszName = pchBuffer;

    SHGetSymbol(&addr, loc, sopNone, &odr);

    *lpDisplacement = odr.dwDeltaOff;
}




ULONG
WDBGAPI
ExtDisasm(
    PULONG64 lpOffset,
    PCSTR    lpBuffer,
    ULONG    fShowEffectiveAddress
    )

/*++

Routine Description:

    This function does the disassembly.

Arguments:

    lpOffset - Pointer to address offset.

    lpBuffer - Pointer to the buffer to store instruction.

    fShowEffectiveAddress - Show effective address if TRUE.


Return Value:

    TRUE for success; FALSE for failure.

--*/

{
    SDI    sdi;
    BOOL   f;
    LPSTR lpch;
    int tmp_int;

    /*
    **  Get the current CS:IP.
    */

    OSDGetAddr(LppdCur->hpid, LptdCur->htid, adrPC, &sdi.addr);

    /*
    **  Set address and do the disassembly.
    */

    sdi.addr.addr.off = *lpOffset;
    SYFixupAddr(&sdi.addr);
    sdi.dop = (g_contWorkspace_WkSp.m_dopDisAsmOpts & ~(0x800)) | dopAddr | dopOpcode
      | dopOperands | (fShowEffectiveAddress ? dopEA : 0);

    f = (OSDUnassemble(LppdCur->hpid, LptdCur->htid, &sdi) == xosdNone);

    /*
     **  See if the disassembly is successful.
     */

    if(f == 0 || sdi.lpch[sdi.ichOpcode] == '?') {
        return FALSE;
    }

    /*
    **  Copy instruction to lpBuffer.
    */

    lpch = (PSTR) lpBuffer;
    if(sdi.ichAddr != -1) {
        sprintf(lpch, "%s  ", &sdi.lpch[sdi.ichAddr]);
        lpch += strlen(&sdi.lpch[sdi.ichAddr]) + 2; // + 2 -- 2 spaces
    }

    if(sdi.ichBytes != -1) {
        sprintf(lpch, "%-17s", &sdi.lpch[sdi.ichBytes]);
        tmp_int = strlen(&sdi.lpch[sdi.ichBytes]);
        lpch += max(17, tmp_int);
    }

    if ((LppdCur->mptProcessorType == mptia64) && (sdi.ichPreg != -1)) {
        sprintf(lpch, "%-5s", &sdi.lpch[sdi.ichPreg]);
        tmp_int = strlen(&sdi.lpch[sdi.ichPreg]);
        lpch += max(10, tmp_int);
    }

    sprintf(lpch, "%-12s", &sdi.lpch[sdi.ichOpcode]);
    tmp_int = strlen(&sdi.lpch[sdi.ichOpcode]);
    lpch += max(12, tmp_int);

    if(sdi.ichOperands != -1) {
        sprintf(lpch, "%-25s", &sdi.lpch[sdi.ichOperands]);
        lpch += max(25, strlen(&sdi.lpch[sdi.ichOperands]));
    }

    if(sdi.ichComment != -1) {
        sprintf(lpch, "%s", &sdi.lpch[sdi.ichComment]);
        lpch += strlen(&sdi.lpch[sdi.ichComment]);
    }

    if(fShowEffectiveAddress) {

        *lpch++ = ';';
        if(sdi.ichEA0 != -1) {
            sprintf(lpch, "%s  ", &sdi.lpch[sdi.ichEA0]);
            lpch += strlen(&sdi.lpch[sdi.ichEA0]) + 2; // + 2 -- 2 spaces
        }

        if(sdi.ichEA1 != -1) {
            sprintf(lpch, "%s  ", &sdi.lpch[sdi.ichEA1]);
            lpch += strlen(&sdi.lpch[sdi.ichEA1]) + 2; // + 2 -- 2 spaces
        }

        if(sdi.ichEA2 != -1) {
            sprintf(lpch, "%s", &sdi.lpch[sdi.ichEA2]);
            lpch += strlen(&sdi.lpch[sdi.ichEA2]);
        }
    }

    *lpch++ = '\r';
    *lpch++ = '\n';
    *lpch = '\0';

    /*
    **  Set next offset before return.
    */

    *lpOffset = sdi.addr.addr.off;

    return TRUE;
}




ULONG
WDBGAPI
ExtReadProcessMemory(
    ULONG64 offset,
    LPVOID  lpBuffer,
    DWORD   cb,
    LPDWORD lpcbBytesRead
    )

/*++

Routine Description:

    This function reads the debuggee's memory.

Arguments:

    offset - Address offset.

    lpBuffer - Supplies pointer to the buffer.

    cb - Specifies the number of bytes to read.

    lpcbBytesRead - Actual number of bytes read; this parameter is optional.

Return Value:

    TRUE for success; FALSE for failure.

--*/

{
    ADDR  addr;
    DWORD cbBytesRead = 0;

    AddrInit( &addr, 0, 0, offset, TRUE, TRUE, FALSE, FALSE );

    OSDFixupAddr(LppdCur->hpid, LptdCur->htid, &addr);
    OSDReadMemory(LppdCur->hpid, LptdCur->htid, &addr, lpBuffer, cb, &cbBytesRead);

    if(lpcbBytesRead) {
        *lpcbBytesRead = cbBytesRead;
    }

    return cbBytesRead == cb;
}




ULONG
WDBGAPI
ExtWriteProcessMemory(
    ULONG64 offset,
    LPCVOID lpBuffer,
    DWORD   cb,
    LPDWORD lpcbBytesWritten
    )

/*++

Routine Description:

    This function writes the debuggee's memory.

Arguments:

    offset - Address offset.

    lpBuffer - Supplies pointer to the buffer.

    cb - Specifies the number of bytes to write.

    lpcbBytesWritten - Actual number of bytes written;
                      this parameter is optional.

Return Value:

    TRUE for success; FALSE for failure.

--*/

{
    ADDR    addr;
    XOSD    xosd;
    DWORD   cbBytesWritten;

    AddrInit( &addr, 0, 0, offset, TRUE, TRUE, FALSE, FALSE );

    OSDFixupAddr(LppdCur->hpid, LptdCur->htid, &addr);
    //kcarlos - BUGBUG -> BUGCAST
    //xosd = OSDWriteMemory(LppdCur->hpid, LptdCur->htid, &addr, lpBuffer, cb,
    xosd = OSDWriteMemory(LppdCur->hpid, LptdCur->htid, &addr, (PVOID) lpBuffer, cb,
                                                              &cbBytesWritten);
    if ( xosd == xosdNone ) {
        if (lpcbBytesWritten) {
            *lpcbBytesWritten = cbBytesWritten;
        }
    }

    return (xosd == xosdNone);
}




ULONG
WDBGAPI
ExtGetThreadContext(
    DWORD       Processor,
    LPCONTEXT   lpContext,
    DWORD       cbSizeOfContext
    )

/*++

Routine Description:

    This function gets thread context.

Arguments:

    lpContext - Supplies pointer to CONTEXT strructure.

    cbSizeOfContext - Supplies the size of CONTEXT structure.

Return Value:

    TRUE for success; FALSE for failure.

--*/

{
    DWORD dw;
    UNREFERENCED_PARAMETER(cbSizeOfContext);

    /*
    **  Note: use of sizeof(LPCONTEXT) and (LPV)&lpContext below is an
    **  implementation-specific choice.
    **
    **  The data being passed into the IOCTL routine is in fact the pointer
    **  to where to place the context rather than an actual context itself.
    **  This explains the use of sizeof(LPCONTEXT) rather than size(CONTEXT)
    **  as the parameter being passed in.
    */

    return OSDSystemService( LppdCur->hpid,
                             g_contWorkspace_WkSp.m_bKernelDebugger ? (HTID)Processor : LptdCur->htid,
                             (SSVC) ssvcGetThreadContext,
                             (LPV)&lpContext,
                             sizeof(LPCONTEXT),
                             &dw
                           ) == xosdNone;
}




ULONG
WDBGAPI
ExtSetThreadContext(
    DWORD       Processor,
    LPCONTEXT   lpContext,
    DWORD       cbSizeOfContext
    )

/*++

Routine Description:

    This function sets thread context.

Arguments:

    lpContext - Supplies pointer to CONTEXT strructure.

    cbSizeOfContext - Supplies the size of CONTEXT structure.

Return Value:

    TRUE for success; FALSE for failure.

--*/

{
    DWORD dw;
    return OSDSystemService( LppdCur->hpid,
                             g_contWorkspace_WkSp.m_bKernelDebugger ? (HTID)Processor : LptdCur->htid,
                             (SSVC) ssvcSetThreadContext,
                             (LPV)lpContext,
                             cbSizeOfContext,
                             &dw
                           ) == xosdNone;
}



ULONG
WDBGAPI
ExtIoctl(
    USHORT   IoctlType,
    LPVOID   lpvData,
    DWORD    cbSize
    )
/*++

Routine Description:


Arguments:


Return Value:

    TRUE for success
    FALSE for failure

--*/

{
    XOSD            xosd;
    PIOCTLGENERIC   pig;
    DWORD           dw;


    switch (IoctlType) {

        case IG_GET_SET_SYMPATH:

        {
            PGET_SET_SYMPATH pgs = (PGET_SET_SYMPATH) lpvData;

            if (pgs->Args && *pgs->Args) {
                ModListSetSearchPath( (PCHAR)pgs->Args );
            }
            if (pgs->Result) {
                ModListGetSearchPath( pgs->Result, pgs->Length );
            }
            xosd = xosdNone;

            break;
        }

        case IG_RELOAD_SYMBOLS:
            LogReload((LPSTR)lpvData, 0);
            break;

        default:

            pig = (PIOCTLGENERIC) malloc( cbSize + sizeof(IOCTLGENERIC) );
            if (!pig) {
                return FALSE;
            }

            pig->ioctlSubType = IoctlType;
            pig->length = cbSize;
            memcpy( pig->data, lpvData, cbSize );
            xosd = OSDSystemService( LppdCur->hpid,
                                     LptdCur->htid,
                                     (SSVC) ssvcGeneric,
                                     (LPV)pig,
                                     cbSize + sizeof(IOCTLGENERIC),
                                     &dw
                                    );

            //
            // If it succeeded, we get the IOCTLGENERIC header back;
            // look in it to see if any data came back.
            //
            if (xosd == xosdNone && dw && pig->length) {
                memcpy( lpvData, pig->data, pig->length );
            }

            free( pig );

            break;
    }

    return xosd == xosdNone;
}


ULONG
WDBGAPI
ExtIoctl32(
    USHORT   IoctlType,
    LPVOID   lpvData,
    DWORD    cbSize
    )
{
    //
    // punt for now
    //

    return ExtIoctl(IoctlType, lpvData, cbSize);
}


DWORD
ExtCallStack(
    ULONG64           FramePointer,
    ULONG64           StackPointer,
    ULONG64           ProgramCounter,
    PEXTSTACKTRACE64  StackFrames,
    DWORD             Frames
    )
/*++

Routine Description:

    This function will dump a call stack to the command window.

Arguments:

    lpsz - arguments to callstack

Return Value:

    log error code

--*/
{
    STACKINFO   si[50];
    DWORD       dwFrames = 50;
    DWORD       i;


    if (!GetCompleteStackTrace( FramePointer,
                                StackPointer,
                                ProgramCounter,
                                si,
                                &dwFrames,
                                FALSE,
                                TRUE )) {
        return 0;
    }

    for (i=0; i<dwFrames; i++) {
        StackFrames[i].ProgramCounter = si[i].StkFrame.AddrPC.Offset;
        StackFrames[i].ReturnAddress  = si[i].StkFrame.AddrReturn.Offset;
        StackFrames[i].FramePointer   = si[i].StkFrame.AddrFrame.Offset;
        StackFrames[i].Args[0]        = si[i].StkFrame.Params[0];
        StackFrames[i].Args[1]        = si[i].StkFrame.Params[1];
        StackFrames[i].Args[2]        = si[i].StkFrame.Params[2];
        StackFrames[i].Args[3]        = si[i].StkFrame.Params[3];
    }

    return dwFrames;
}

DWORD
ExtCallStack32(
    ULONG             FramePointer,
    ULONG             StackPointer,
    ULONG             ProgramCounter,
    PEXTSTACKTRACE32  StackFrames,
    DWORD             Frames
    )
{
    //
    // thunks r us
    //

    STACKINFO   si[50];
    DWORD       dwFrames = 50;
    DWORD       i;


    if (!GetCompleteStackTrace( SE32To64(FramePointer),
                                SE32To64(StackPointer),
                                SE32To64(ProgramCounter),
                                si,
                                &dwFrames,
                                FALSE,
                                TRUE )) {
        return 0;
    }

    for (i=0; i<dwFrames; i++) {
        StackFrames[i].ProgramCounter = (DWORD)si[i].StkFrame.AddrPC.Offset;
        StackFrames[i].ReturnAddress  = (DWORD)si[i].StkFrame.AddrReturn.Offset;
        StackFrames[i].FramePointer   = (DWORD)si[i].StkFrame.AddrFrame.Offset;
        StackFrames[i].Args[0]        = (DWORD)si[i].StkFrame.Params[0];
        StackFrames[i].Args[1]        = (DWORD)si[i].StkFrame.Params[1];
        StackFrames[i].Args[2]        = (DWORD)si[i].StkFrame.Params[2];
        StackFrames[i].Args[3]        = (DWORD)si[i].StkFrame.Params[3];
    }

    return dwFrames;
}


LONG
ExtensionExceptionFilterFunction(
    LPSTR                msg,
    LPEXCEPTION_POINTERS lpep
    )
{
    PTSTR pszFmt;

    if (4 == sizeof(lpep->ExceptionRecord->ExceptionAddress)) {
        pszFmt = _T("\r\n%s addr=0x%08x, ec=0x%08x\r\n\r\n");
    } else {
        pszFmt = _T("\r\n%s addr=0x%016I64x, ec=0x%08x\r\n\r\n");
    }

    CmdLogFmt( pszFmt,
               msg,
               lpep->ExceptionRecord->ExceptionAddress,
               lpep->ExceptionRecord->ExceptionCode );

    return EXCEPTION_EXECUTE_HANDLER;
}


LPSTR
ParseBangCommand(
    LPSTR   lpsz,
    LPSTR   *lpszMod,
    LPSTR   *lpszFnc,
    PCMDID  pCmdId
    )
{
    DWORD i;


    //
    //  Start assuming ExtensionMod exists: ExtensionMod
    //                                     "^"
    //                                   lpszMod
    //

    *lpszMod = lpsz;
    while((*lpsz != '.') && (*lpsz != ' ') && (*lpsz != '\t') && (*lpsz != '\0')) {
        lpsz++;
    }

    if (*lpsz != '.') {

        //
        //  If ExtensionMod is absent(no '.'), set this: Function'\0'
        //                                               ^           ^
        //                                            lpszFnc       lpsz
        //
        //  and use the default dll.
        //

        *lpszFnc = *lpszMod;
        if ( *lpsz != '\0' ) {
            *lpsz++ = '\0';
        }

        if (pszDefaultExtName) {

            Assert(pszDefaultExtFullPath);
            *lpszMod = pszDefaultExtFullPath;

        } else {

            if (g_contWorkspace_WkSp.m_bKernelDebugger) {
                switch( LppdCur->mptProcessorType ) {
                    case mptmips:
                        *lpszMod = "kdextmip";
                        break;

                    case mptdaxp:
                        *lpszMod = "kdextalp";
                        break;

                    case mptmppc:
                        *lpszMod = "kdextppc";
                        break;

                    case mptia64:
                        *lpszMod = "kdexti64";
                        break;

                    case mptix86:
                    default:
                        *lpszMod = "kdextx86";
                        break;
                }
            } else {
                *lpszMod = DEFAULT_EXTENSION_LIB;
            }

        }

    } else {

        //
        //  If we found '.', set this: ExtensionMod'\0'
        //                             ^               ^
        //                          lpszMod           lpsz
        //

        *lpsz++ = '\0';

        //
        //  Set function: Function'\0'
        //                ^           ^
        //             lpszFnc       lpsz
        //

        *lpszFnc = lpsz;
        while((*lpsz != ' ') && (*lpsz != '\t') && (*lpsz != '\0')) {
            lpsz++;
        }

        if(*lpsz != '\0') {
            *lpsz++ = '\0';
        }
    }

    //
    // look to see if the user entered one of the built-in commands
    //
    *pCmdId = CMDID_NULL;
    for (i=1; i<ExtMaxCommands; i++) {
        if (_stricmp( *lpszFnc, ExtCommands[i].CmdString ) == 0) {
            *pCmdId = ExtCommands[i].CmdId;
            if (ExtCommands[i].CmdId == CMDID_LOAD) {
                *lpszMod = lpsz;
            }
            break;
        }
    }

    _strlwr( *lpszFnc );

    return lpsz;
}


BOOL
KD_GetKernelVersion(
    PDBGKD_GET_VERSION64 pKdVersionInfo
    )
{
    Assert(pKdVersionInfo);

    LPPD            lppd = NULL;
    LPTD            lptd = NULL;
    XOSD            xosd = xosdUnknown;
    PIOCTLGENERIC   pig;
    const DWORD     dwSize = sizeof(*pKdVersionInfo) + sizeof(IOCTLGENERIC);
    DWORD           dwRetSize = 0;

    // Is a kernel commmand allowed.
    if (!g_contWorkspace_WkSp.m_bKernelDebugger || IsProcRunning(LppdCur) ) {
        return FALSE;
    }

    if (DebuggeeActive()) {
        lppd = LppdCur;
        lptd = LptdCur;
        if (!lppd || !lppd->hpid) {
            Assert(LppdFirst);
            lppd = LppdFirst;
            lptd = lppd->lptdList;
        }

        if (lppd && lppd->hpid) {
            pig = (PIOCTLGENERIC) calloc(dwSize, 1);
            if (!pig) {
                return FALSE;
            }
            pig->ioctlSubType = IG_GET_KERNEL_VERSION;
            pig->length = sizeof(*pKdVersionInfo);
            xosd = OSDSystemService( lppd->hpid,
                                     lptd->htid,
                                     (SSVC) ssvcGeneric,
                                     (LPV)pig,
                                     dwSize,
                                     &dwRetSize
                                     );

            if (xosdNone == xosd) {
                memcpy(pKdVersionInfo, pig->data, sizeof(*pKdVersionInfo));
            }
            free( pig );
        }
    }

    return xosdNone == xosd;
}

BOOL
User_GetKernelVersion(
    PDBG_GET_OS_VERSION pUserVersionInfo
    )
{
    Assert(pUserVersionInfo);

    LPPD            lppd = NULL;
    LPTD            lptd = NULL;
    XOSD            xosd = xosdUnknown;
    PIOCTLGENERIC   pig;
    const DWORD     dwSize = sizeof(*pUserVersionInfo) + sizeof(IOCTLGENERIC);
    DWORD           dwRetSize = 0;

    // Is a kernel commmand allowed.
    if (g_contWorkspace_WkSp.m_bKernelDebugger || IsProcRunning(LppdCur) ) {
        return FALSE;
    }

    if (DebuggeeActive()) {
        lppd = LppdCur;
        lptd = LptdCur;
        if (!lppd || !lppd->hpid) {
            Assert(LppdFirst);
            lppd = LppdFirst;
            lptd = lppd->lptdList;
        }

        if (lppd && lppd->hpid) {
            pig = (PIOCTLGENERIC) calloc(dwSize, 1);
            if (!pig) {
                return FALSE;
            }
            pig->ioctlSubType = IG_GET_OS_VERSION;
            pig->length = sizeof(*pUserVersionInfo);
            xosd = OSDSystemService( lppd->hpid,
                                     lptd->htid,
                                     (SSVC) ssvcGeneric,
                                     (LPV)pig,
                                     dwSize,
                                     &dwRetSize
                                     );

            if (xosdNone == xosd) {
                memcpy(pUserVersionInfo, pig->data, sizeof(*pUserVersionInfo));
            }
            free( pig );
        }
    }

    return xosdNone == xosd;
}

PTSTR
GetExtensionDllSearchPath(
    VOID
    )
{
    PTSTR           pszSearchPath = NULL;
    TCHAR           szOsVer[200] = { 0 };
    PTSTR           pszTmp = NULL;
    DWORD           dwSize = 0;
    TCHAR           szWinDbgLaunchDir[_MAX_PATH] = {0};
    // Kernel mode
    DBGKD_GET_VERSION64   KdVersionInfo = {0};
    // User mode
    DBG_GET_OS_VERSION  UserVersionInfo = {0};

    // Set the default seach dir
    _tcscpy(szOsVer, _T("W2KFre"));

    // If we are doing kernel, then we need version specific extensions
    if (g_contWorkspace_WkSp.m_bKernelDebugger) {
        if (KD_GetKernelVersion(&KdVersionInfo)) {
            if (KdVersionInfo.MinorVersion <= 1381) {
                _tcscpy(szOsVer, _T("NT4"));
            } else {
                _tcscpy(szOsVer, _T("W2K"));
            }

            if (KdVersionInfo.MajorVersion == 0xC) {
                _tcscat(szOsVer, _T("Chk"));
            } else {
                _tcscat(szOsVer, _T("Fre"));
            }
        }
    } else {
        if (User_GetKernelVersion(&UserVersionInfo)) {

            if (VER_PLATFORM_WIN32_NT == UserVersionInfo.osi.dwPlatformId) {
                if (UserVersionInfo.osi.dwBuildNumber <= 1381) {
                    _tcscpy(szOsVer, _T("NT4"));
                } else {
                    _tcscpy(szOsVer, _T("W2K"));
                }

                if (UserVersionInfo.fChecked) {
                    _tcscat(szOsVer, _T("Chk"));
                } else {
                    _tcscat(szOsVer, _T("Fre"));
                }
            }
        }
    }

    // Calc the path size needed
    dwSize = GetEnvironmentVariable(_T("PATH"), NULL, 0) + 3 * _MAX_PATH;

    // Get the name of the dir we launched from
    GetModuleFileName( NULL, szWinDbgLaunchDir, sizeof(szWinDbgLaunchDir) );
    // Remove the executable name.
    pszTmp = _tcsrchr(szWinDbgLaunchDir, _T('\\'));
    if (pszTmp) {
        *pszTmp = 0;
    }

    // Allocate a buffer for the new search path.
    pszSearchPath = (PTSTR) calloc(dwSize, sizeof(TCHAR));
    if (!pszSearchPath) {
        return NULL;
    }

    // Copy launch path
    pszTmp = pszSearchPath;
    _stprintf(pszTmp, _T("%s;"), szWinDbgLaunchDir);
    pszTmp = pszTmp + _tcslen(pszTmp);

    // Copy envorinment path
    GetEnvironmentVariable(_T("PATH"), pszTmp, (DWORD)(dwSize - (pszTmp - pszSearchPath)));
    pszTmp = pszTmp + _tcslen(pszTmp);

    // Copy OS version path
    _stprintf(pszTmp, _T(";%s\\%s"), szWinDbgLaunchDir, szOsVer);

    Assert(_tcslen(pszTmp) < dwSize);

    return pszSearchPath;
}

BOOL
LoadExtensionDll(
    LPEXTLOAD ExtLoad,
    PTSTR pszSearchPath
    )
{
    TCHAR szFullPath[_MAX_PATH] = {0};

    if (!ExtLoad->szPathToDll[0]) {
        if (!SearchPath(pszSearchPath,
                        ExtLoad->szDllName,
                        ".dll",
                        sizeof(ExtLoad->szPathToDll),
                        ExtLoad->szPathToDll,
                        NULL)) {
            return FALSE;
        } else {
            // We located an instance of the DLL. We must get the
            // module name without the full path, since the user may
            // have specified the full path.
            _tsplitpath(ExtLoad->szPathToDll, NULL, NULL, ExtLoad->szDllName, NULL);

            // Remove the file name extension ".dll"
            ExtLoad->szPathToDll[_tcslen(ExtLoad->szPathToDll) -4] = 0;
        }
    }

    // Add the file extension
    _tcscpy(szFullPath, ExtLoad->szPathToDll);
    _tcscat(szFullPath, ".dll");


    ExtLoad->hModule = LoadLibrary( szFullPath );
    if (!ExtLoad->hModule) {
        return FALSE;
    }

    ExtLoad->dwLoaded = EXT_LOADED;
    NumLoadedExts++;

    pszDefaultExtName = ExtLoad->szDllName;
    pszDefaultExtFullPath = ExtLoad->szPathToDll;

    CmdLogFmt( "Debugger extension library [%s] loaded\n", ExtLoad->szPathToDll );

    ExtLoad->pDllInit = (PWINDBG_EXTENSION_DLL_INIT64)GetProcAddress
              ( ExtLoad->hModule, "WinDbgExtensionDllInit" );

    if (ExtLoad->pDllInit == NULL) {
        
        ExtLoad->bOldStyle = TRUE;
        ExtLoad->ApiVersion.Revision = EXT_API_VERSION_NUMBER64;
        ExtLoad->pApiVersionRoutine = NULL;
        ExtLoad->pCheckVersionRoutine = NULL;
        ExtLoad->bDoVersionCheck = FALSE;

    } else {

        ExtLoad->bOldStyle = FALSE;

        ExtLoad->pApiVersionRoutine = (PWINDBG_EXTENSION_API_VERSION)
                    GetProcAddress(ExtLoad->hModule, "ExtensionApiVersion");

        ExtLoad->pCheckVersionRoutine = (PWINDBG_CHECK_VERSION)
                    GetProcAddress( ExtLoad->hModule, "CheckVersion");

        if (ExtLoad->pCheckVersionRoutine && !g_contWorkspace_WkSp.m_bNoVersion) {
            ExtLoad->bDoVersionCheck = TRUE;
        } else {
            ExtLoad->bDoVersionCheck = FALSE;
        }

        if (dwOsVersion == 0) {
            OSDGetDebugMetric( LppdCur->hpid,
                               LptdCur->htid,
                               mtrcOSVersion,
                               &dwOsVersion );
        }

        __try {

            if (ExtLoad->pApiVersionRoutine) {
                ExtLoad->ApiVersion = *(ExtLoad->pApiVersionRoutine());

                if (ExtLoad->ApiVersion.Revision == EXT_API_VERSION_NUMBER32) {
                    
                    //
                    // 32 bit api may cause problems when debugging a 64 bit kernel
                    //
                    CmdLogFmt("%s uses the old 32 bit extension API and may not be fully\n", 
                              ExtLoad->szPathToDll
                              );
                    CmdLogFmt("compatible with current systems.\n");

                } else if (ExtLoad->ApiVersion.Revision < EXT_API_VERSION_NUMBER32) {
                    
                    //
                    // really old 32 bit api. Same as above may cause problems 
                    // when debugging a 64 bit kernel
                    //
                    CmdLogFmt("%s uses an earlier version of the extension API than that\n", 
                              ExtLoad->szPathToDll
                              );
                    CmdLogFmt("is no longer supported by this debugger.\n");

                } else if (ExtLoad->ApiVersion.Revision > EXT_API_VERSION_NUMBER64) {
                    
                    //
                    // Very latest api. Anybodies guess as to whether it will work.
                    //
                    CmdLogFmt("%s uses a later version of the extension API than that\n", 
                              ExtLoad->szPathToDll
                              );
                    CmdLogFmt("supported by this debugger, and might not function correctly.\n");
                    CmdLogFmt("You should use the debugger from the SDK or DDK which was used\n");
                    CmdLogFmt("to build the extension library.\n");

                }

                if (ExtLoad->ApiVersion.Revision >= EXT_API_VERSION_NUMBER64) {
                    (ExtLoad->pDllInit)(&WindbgExtensions,
                                HIWORD(dwOsVersion),
                                LOWORD(dwOsVersion)
                                );
                } else {
                    (ExtLoad->pDllInit)((PWINDBG_EXTENSION_APIS64)&WindbgExtensions32,
                                HIWORD(dwOsVersion),
                                LOWORD(dwOsVersion)
                                );
                }
            }
        } __except (ExtensionExceptionFilterFunction(
                      "DllInit() failed", GetExceptionInformation())) {
            return FALSE;

        }
    }

    return TRUE;
}


LOGERR
LogExtension(
    LPSTR pszArgString
    )

/*++

Routine Description:

    Invoke ntsd extensions in WinDbg command window. Extension command syntax:

    ![ExtensionMod.]Function Argument-string

    If ExtensionMod is absent, use the default "ntsdexts" dll. This function
    first parses pszArgString and ends up with pszMod ==> ExtensionMod(or "ntsdexts"),
    pszFnc ==> Function, and pszArgString ==> Argument-string. It then does
    LoadLibrary()(as needed), GetProcAddress(), and (ExtensionRoutine)().

Arguments:

    pszArgString - Supplies argument string and points one position right after '!'.

Return Value:

    Log error code.

  --*/

{
    //
    // Define this so that we can execute ntsdexts.dll
    //
    typedef
    VOID
    (*PNTSD_EXTENSION_ROUTINE)(
        HANDLE hCurrentProcess,
        HANDLE hCurrentThread,
        DWORD dwCurrentPc,
        PVOID lpExtensionApis,
        LPSTR lpArgumentString
        );

    
    PWINDBG_EXTENSION_ROUTINE       pWindbgExtRoutine;
    PWINDBG_EXTENSION_ROUTINE32     pWindbgExtRoutine32;    // 32 bit version
    //PWINDBG_OLD_EXTENSION_ROUTINE   pWindbgOldExtRoutine;   // really old version - no longer supported
    PNTSD_EXTENSION_ROUTINE         pNtsdExtRoutine;
    LPPD                            LppdT = LppdCur;
    LPTD                            LptdT = LptdCur;
    LOGERR                          rVal = LOGERROR_NOERROR;
    ADDR                            addrPC;
    LPSTR                           pszMod;
    LPSTR                           pszFnc;
    LPSTR                           pszArgs;
    PSTR                            pszExtensionSearchPath = GetExtensionDllSearchPath();
    long                            l;
    HANDLE                          hProcess;
    HANDLE                          hThread;
    INT                             nLen;
    INT                             nExtIdx;
    CMDID                           CmdId;
    DWORD                           dwRet;


    IsKdCmdAllowed();
    TDWildInvalid();
    PDWildInvalid();

    LppdCur = LppdCommand;
    LptdCur = LptdCommand;

    CmdInsertInit();


    if (!DebuggeeActive()) {
        CmdLogFmt( "Bang commands are not allowed until the debuggee is loaded\n" );
        rVal = LOGERROR_QUIET;
        goto done;
    }

    pszArgString = CPSkipWhitespace(pszArgString);

    if (!*pszArgString) {
        rVal = LOGERROR_UNKNOWN;
        goto done;
    }

    pszArgs = ParseBangCommand( pszArgString, &pszMod, &pszFnc, &CmdId );

    switch (CmdId) {
    case CMDID_LIST_EXTS:
        {
            PTSTR pszLoaded = "loaded";
            PTSTR pszLoadPending = "load pending";
            PTSTR pszTmp = NULL;
            UINT uMaxNameLen = 0;

            nLen = max( _tcslen(pszLoaded), _tcslen(pszLoadPending) );

            // List the default extension
            if (pszDefaultExtName) {
                Assert(pszDefaultExtFullPath);
                CmdLogFmt("Default extension: %s\r\n", pszDefaultExtFullPath);
            } else {
                CmdLogFmt("Default extension: none\r\n");
            }

            // Get the length of the longest name
            for( nExtIdx=0; nExtIdx<MAX_EXTDLLS; nExtIdx++) {
                if (LoadedExts[nExtIdx].dwLoaded == EXT_LOADED
                    || LoadedExts[nExtIdx].dwLoaded == EXT_NEEDS_LOADING) {

                    uMaxNameLen = max(uMaxNameLen, _tcslen(LoadedExts[nExtIdx].szDllName));
                }
            }

            // Display the list of extensions
            for( nExtIdx=0; nExtIdx<MAX_EXTDLLS; nExtIdx++) {
                pszTmp = NULL;

                if (LoadedExts[nExtIdx].dwLoaded == EXT_LOADED) {
                    pszTmp = pszLoaded;
                } else if (LoadedExts[nExtIdx].dwLoaded == EXT_NEEDS_LOADING) {
                    pszTmp = pszLoadPending;
                }

                if (pszTmp) {
                    CmdLogFmt("%-*s %-*s %s \r\n",
                        uMaxNameLen,
                        LoadedExts[nExtIdx].szDllName,
                        nLen,
                        pszTmp,
                        LoadedExts[nExtIdx].szPathToDll
                        );
                }
            }
        }
        rVal = LOGERROR_NOERROR;
        goto done;

        case CMDID_DEFAULT:
            if (!pszArgs) {
                CmdLogFmt("Extension dll %s unloaded\r\n", pszDefaultExtFullPath);
                goto done;
            }

            for( nExtIdx=0; nExtIdx<MAX_EXTDLLS; nExtIdx++) {
                if (!_stricmp( LoadedExts[nExtIdx].szPathToDll, pszArgs)
                    || !_stricmp( LoadedExts[nExtIdx].szDllName, pszArgs)) {

                    CmdLogFmt( "%s is now the default extension dll\n", LoadedExts[nExtIdx].szPathToDll );
                    pszDefaultExtName = LoadedExts[nExtIdx].szDllName;
                    pszDefaultExtFullPath = LoadedExts[nExtIdx].szPathToDll;
                    rVal = LOGERROR_NOERROR;
                    goto done;
                }
            }

            CmdLogFmt( "%s extension dll is not loaded\n", pszArgs );
            rVal = LOGERROR_QUIET;
            goto done;

        case CMDID_RELOAD:
            rVal = LogReload(pszArgs, 0);
            goto done;

        case CMDID_SYMPATH:
            pszArgs = CPSkipWhitespace(pszArgs);

            if (!*pszArgs) {
                nLen =  ModListGetSearchPath( NULL, 0 );
                if (!nLen) {
                    CmdLogFmt( "Sympath =\n" );
                } else {
                    pszArgString = (PSTR) malloc(nLen);
                    ModListGetSearchPath( pszArgString, nLen );
                    CmdLogFmt( "Sympath = %s\n", pszArgString );
                    free(pszArgString);
                }
                rVal = LOGERROR_NOERROR;
            } else {
                char szBuf[MAX_USER_LINE];
                CPCopyString(&pszArgs, szBuf, '\0', *pszArgs == '"');
                ModListSetSearchPath( szBuf );
                rVal = LogReload("", 0);
            }
            goto done;

    }

    if(!DebuggeeActive()) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    for( nExtIdx=0; nExtIdx<MAX_EXTDLLS; nExtIdx++) {
        if (!_stricmp( LoadedExts[nExtIdx].szPathToDll, pszMod)
            || !_stricmp( LoadedExts[nExtIdx].szDllName, pszMod) ) {
            //
            // it has already been loaded
            //
            break;
        }
    }

    if (nExtIdx == MAX_EXTDLLS) {
        //
        // could not find the module, so it must need to be loaded
        // so lets look for an empty slot
        //
        for( nExtIdx=0; nExtIdx<MAX_EXTDLLS; nExtIdx++) {
            if (LoadedExts[nExtIdx].dwLoaded == EXT_NOT_LOADED) {
                break;
            }
        }
    }

    if (nExtIdx == MAX_EXTDLLS) {
        CmdLogFmt( "Too many extension dlls loaded\n" );
        rVal = LOGERROR_QUIET;
        goto done;
    }

    if (LoadedExts[nExtIdx].dwLoaded == EXT_NOT_LOADED ||
        LoadedExts[nExtIdx].dwLoaded == EXT_NEEDS_LOADING) {
        //
        // either this ext dll has never been loaded or
        // it has been unloaded.  in either case we need
        // to load the sucker now.
        //
        strcpy( LoadedExts[nExtIdx].szDllName, pszMod );
        if (!LoadExtensionDll( &LoadedExts[nExtIdx], pszExtensionSearchPath )) {
            CmdLogFmt("Cannot load '%s'\r\n", pszMod);
            rVal = LOGERROR_QUIET;
            goto done;
        }
    }

    switch (CmdId) {
    case CMDID_LOAD:
        rVal = LOGERROR_NOERROR;
        goto done;
        
    case CMDID_UNLOAD:
        FreeLibrary( LoadedExts[nExtIdx].hModule );
        LoadedExts[nExtIdx].hModule = NULL;
        LoadedExts[nExtIdx].dwLoaded = EXT_NOT_LOADED;
        NumLoadedExts--;
        pszDefaultExtName = NULL;
        pszDefaultExtFullPath = NULL;
        CmdLogFmt("Extension dll %s unloaded\r\n", LoadedExts[nExtIdx].szPathToDll);
        rVal = LOGERROR_NOERROR;
        goto done;
        
    case CMDID_NOVERSION:
        CmdLogFmt("Extension dll system version checking is disabled\r\n");
        LoadedExts[nExtIdx].bDoVersionCheck = FALSE;
        g_contWorkspace_WkSp.m_bNoVersion = TRUE;
        rVal = LOGERROR_NOERROR;
        goto done;
        
    case CMDID_HELP:
        pszFnc = "help";
        break;
    }

    pWindbgExtRoutine =
          (PWINDBG_EXTENSION_ROUTINE)GetProcAddress( LoadedExts[nExtIdx].hModule, pszFnc );

    if (!pWindbgExtRoutine) {
        for( nExtIdx=0; nExtIdx<MAX_EXTDLLS; nExtIdx++) {
            if (LoadedExts[nExtIdx].dwLoaded == EXT_NEEDS_LOADING) {
                if (!LoadExtensionDll( &LoadedExts[nExtIdx], pszExtensionSearchPath )) {
                    CmdLogFmt("Cannot load '%s'\r\n", LoadedExts[nExtIdx].szPathToDll);
                    rVal = LOGERROR_QUIET;
                    goto done;
                }
            }
            if (LoadedExts[nExtIdx].dwLoaded == EXT_LOADED) {
                pWindbgExtRoutine =
                      (PWINDBG_EXTENSION_ROUTINE)GetProcAddress( LoadedExts[nExtIdx].hModule, pszFnc );
                if (pWindbgExtRoutine) {
                    break;
                } else {
                    CmdLogFmt("Missing extension: '%s.%s'\r\n", LoadedExts[nExtIdx].szPathToDll, pszFnc);
                }
            }
        }

        CmdLogFmt("Could not find extension: '%s'\r\n", pszFnc);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    if (LoadedExts[nExtIdx].bDoVersionCheck) {
        __try {

            if (!g_contWorkspace_WkSp.m_bKernelDebugger) {
                // User mode
                LoadedExts[nExtIdx].ApiVersion.Revision = EXT_API_VERSION_NUMBER;
                LoadedExts[nExtIdx].pApiVersionRoutine = NULL;
                LoadedExts[nExtIdx].pCheckVersionRoutine = NULL;
            }
            
            if (LoadedExts[nExtIdx].pCheckVersionRoutine) {
                (LoadedExts[nExtIdx].pCheckVersionRoutine)();
            }

        } __except (ExtensionExceptionFilterFunction(
                      "CheckVersion() failed", GetExceptionInformation())) {
            rVal = LOGERROR_QUIET;
            goto done;

        }
    }

    //
    //  Get debuggee's process handle.
    //
    OSDSystemService(LppdCur->hpid,
                     0,
                     (SSVC) ssvcGetProcessHandle,
                     &hProcess,
                     sizeof(hProcess),
                     &dwRet
                     );

    //
    //  Get debuggee's thread handle.
    //
    OSDSystemService(LppdCur->hpid,
                     LptdCur->htid,
                     (SSVC) ssvcGetThreadHandle,
                     &hThread,
                     sizeof(hThread),
                     &dwRet
                     );

    //
    //  Get the current CS:IP.
    //
    OSDGetAddr(LppdCur->hpid, LptdCur->htid, adrPC, &addrPC);

    //
    //  Enable the extension to detect ControlC.
    //
    SetCtrlCTrap();

    //
    //  Call the extension function. Note: if ExtensionRountine is a pointer
    //  to a function that is in fact declared as PNTSD_EXTENSION_ROUTINE(like
    //  those in ntsdexts.c), then last 4 remote apis of varible WindbgExtensions
    //  are not applicable.
    //

    if (CmdId == CMDID_HELP) {
        //
        // print the extension dll list
        //
        if (NumLoadedExts > 1) {
            CmdLogFmt( "*** Loaded Extension Dlls:\n" );
            for( l=0; l<MAX_EXTDLLS; l++) {
                if (LoadedExts[l].dwLoaded == EXT_LOADED) {
                    CmdLogFmt( "%d %-20s %s\n", l+1, LoadedExts[l].szDllName,
                        LoadedExts[l].szPathToDll);
                }
            }
            CmdLogFmt( "\n" );
        }

        //
        // print the help for the built-in commands
        //
        CmdLogFmt( "\nWinDbg built in bang commands:\n\n" );
        for (l=1; l<ExtMaxCommands; l++) {
            CmdLogFmt( "%-27s - %s\n", ExtCommands[l].CmdString, ExtCommands[l].HelpString );
        }
        CmdLogFmt( "\n%s Bang Commands:\n\n", LoadedExts[nExtIdx].szPathToDll );
    }

    __try {

        if (LoadedExts[nExtIdx].bOldStyle) {

            //
            // User Mode
            //
            pNtsdExtRoutine = (PNTSD_EXTENSION_ROUTINE) pWindbgExtRoutine;
            (pNtsdExtRoutine)(hProcess,
                              hThread,
                              (ULONG)addrPC.addr.off,
#ifdef _WIN64
                              &WindbgExtensions,
#else
                              &WindbgExtensions32,
#endif
                              pszArgs );

        } else if (LoadedExts[nExtIdx].ApiVersion.Revision <= EXT_API_VERSION_NUMBER32) {
            
            // 32 bit Kernel mode
            pWindbgExtRoutine32 = (PWINDBG_EXTENSION_ROUTINE32)pWindbgExtRoutine;
            (pWindbgExtRoutine32)( hProcess,
                                   hThread,
                                   (ULONG)addrPC.addr.off,
                                   LptdCur ? LptdCur->itid : 0,
                                   pszArgs );
        } else {

            // 64 bit kernel mode
            ((PWINDBG_EXTENSION_ROUTINE64)pWindbgExtRoutine)( hProcess,
                                 hThread,
                                 addrPC.addr.off,
                                 LptdCur ? LptdCur->itid : 0,
                                 pszArgs );
        }

    } __except (ExtensionExceptionFilterFunction(
                  "Extension function faulted", GetExceptionInformation())) {

    }


    LoadedExts[nExtIdx].dwCalls++;

    //
    //  Disable ControlC detection.
    //

    ClearCtrlCTrap();

    CmdLogFmt("\r\n");

    rVal = LOGERROR_NOERROR;

 done:
    LppdCur = LppdT;
    LptdCur = LptdT;

    if (pszExtensionSearchPath) {
        free(pszExtensionSearchPath);
    }

    return rVal;
}

PSTR
GetTextualTimeForLoadedLibrary(
    HMODULE hMod
    )
/*++

Routine description:

    Returns a textual description of the module's time
    stamp. Uses ctime to do the formatting.

Arguments:

    hMod - handle to a module.

Return Values:

    Returns a pointer to a static buffer. See the ctime
    docs for more info.

--*/
{
    DWORD   dwTStamp;
    time_t  tstamp;

    dwTStamp = GetTimestampForLoadedLibrary( hMod );
    tstamp = dwTStamp;
    return ctime( &tstamp );
}

VOID
PrintDllBuildInfo(
    LPSTR  DllBaseName,
    BOOL   GetPlatform
    )
{
    LPSTR   p;
    CHAR    name[MAX_PATH];
    CHAR    buf[MAX_PATH];
    HMODULE hMod;
    BOOL    unloadit = FALSE;


    if (!GetPlatform) {
        p = "";
    } else if (!LppdCur) {
        return;
    } else {
        switch( LppdCur->mptProcessorType ) {
            case mptix86:
                p = "x86";
                break;

            case mptmips:
                p = "mip";
                break;

            case mptdaxp:
                p = "alp";
                break;

            case mptia64:
                p = "i64";
                break;

            case mptmppc:
                p = "ppc";
                break;

            default:
                p = "";
                break;
        }
    }

    sprintf( name, "%s%s.dll", DllBaseName, p );

    hMod = GetModuleHandle( name );
    if (!hMod) {
        hMod = LoadLibrary(name);
        unloadit = TRUE;
    }

    if (!hMod) {
        CmdLogFmt("%-20s: Not found\n", name);
    } else {
        buf[0] = 0;
        GetModuleFileName( hMod, buf, sizeof(buf) );
        _strlwr( buf );

        p = GetTextualTimeForLoadedLibrary(hMod);

        p[strlen(p)-1] = 0;
        CmdLogFmt(
            "%-20s: %d.%d.%d, built: %s [name: %s]\n",
            name,
            0,
            0,
            0,
            p,
            buf
            );
    }

    if (unloadit && hMod) {
        FreeLibrary(hMod);
    }

    return;
}


LOGERR
LogVersion(
    LPSTR lpsz
    )
{
    CHAR                           buf[MAX_PATH];
    DWORD                          mi;
    LPSTR                          p;
    LPAPI_VERSION                  lpav;
    LPEXT_API_VERSION              lpextav;
    PWINDBG_EXTENSION_API_VERSION  ExtensionApiVersion;
    LPPD                           LppdT = LppdCur;
    LPTD                           LptdT = LptdCur;
    LOGERR                         rVal;
    BOOL                           save;



    //IsKdCmdAllowed();
    TDWildInvalid();
    PDWildInvalid();

    LppdCur = LppdCommand;
    LptdCur = LptdCommand;

    CmdInsertInit();

    //if(!DebuggeeActive()) {
        //CmdLogVar(ERR_Debuggee_Not_Alive);
        //rVal = LOGERROR_QUIET;
        //goto done;
    //}

    CmdLogFmt( "\n" );

    GetModuleFileName( NULL, buf, sizeof(buf) );
    _strlwr( buf );

    p = GetTextualTimeForLoadedLibrary( GetModuleHandle( NULL ) );

    p[strlen(p)-1] = 0;
    CmdLogFmt(
        "%-20s: %d.%d.%d, built: %s [name: %s]\n",
        "windbg.exe",
        ApiVersion.MajorVersion,
        ApiVersion.MinorVersion,
        ApiVersion.Revision,
        p,
        buf
        );

    PrintDllBuildInfo( "shcv",    FALSE );
    PrintDllBuildInfo( "symcvt",  FALSE );
    PrintDllBuildInfo( "tlloc",   FALSE );
    PrintDllBuildInfo( "tlpipe",  FALSE );
    PrintDllBuildInfo( "tlser",   FALSE );
    PrintDllBuildInfo( "em",      FALSE );
    PrintDllBuildInfo( "eecxx",   FALSE );
    PrintDllBuildInfo( "dm",      FALSE );
    PrintDllBuildInfo( "dmkd",    TRUE  );
    //PrintDllBuildInfo( "msdbi",   FALSE );

    lpav = ImagehlpApiVersionEx(&ApiVersion);
    GetModuleFileName( GetModuleHandle("dbghelp.dll"), buf, sizeof(buf) );

    p = GetTextualTimeForLoadedLibrary( GetModuleHandle( "dbghelp.dll" ) );

    p[strlen(p)-1] = 0; // kill the trailing newline
    _strlwr( buf );
    CmdLogFmt(
        "%-20s: %d.%d.%d, built: %s [name: %s]\n",
        "dbghelp.dll",
        lpav->MajorVersion,
        lpav->MinorVersion,
        lpav->Revision,
        p,
        buf
        );

    if (pszDefaultExtName) {
        Assert(pszDefaultExtFullPath);
        for( mi=0; mi<MAX_EXTDLLS; mi++) {
            if (!_stricmp( LoadedExts[mi].szPathToDll, pszDefaultExtFullPath )) {
                break;
            }
        }
        Assert(mi < MAX_EXTDLLS);

        ExtensionApiVersion = (PWINDBG_EXTENSION_API_VERSION)
        GetProcAddress( LoadedExts[mi].hModule, "ExtensionApiVersion" );
        if (ExtensionApiVersion) {
            lpextav = ExtensionApiVersion();
            GetModuleFileName( LoadedExts[mi].hModule, buf, sizeof(buf) );
            p = GetTextualTimeForLoadedLibrary( LoadedExts[mi].hModule );
            p[strlen(p)-1] = 0;
            _strlwr( buf );
            CmdLogFmt(
                "%-20s: %d.%d.%d, built: %s [name: %s]\n",
                LoadedExts[mi].szPathToDll,
                lpextav->MajorVersion,
                lpextav->MinorVersion,
                lpextav->Revision,
                p,
                buf );
        }
        save = LoadedExts[mi].bDoVersionCheck;
        LoadedExts[mi].bDoVersionCheck = FALSE;
        LogExtension( "version" );
        LoadedExts[mi].bDoVersionCheck = save;
    }

    CmdLogFmt( "\n" );

    rVal = LOGERROR_NOERROR;

    LppdCur = LppdT;
    LptdCur = LptdT;

    return rVal;
}


LPSTR
GetExtensionDllNames(
    LPDWORD len
    )
{
    DWORD mi;
    LPSTR DllNames;
    LPSTR p;


    if (!NumLoadedExts) {
        return NULL;
    }
    for( mi=0,*len=0; mi<MAX_EXTDLLS; mi++) {
        if (LoadedExts[mi].dwLoaded == EXT_LOADED) {
            *len += (strlen(LoadedExts[mi].szPathToDll) + 1);
        }
    }
    if (!*len) {
        return NULL;
    }
    *len += 2;
    DllNames = (PSTR) malloc( *len );
    if (!DllNames) {
        return NULL;
    }
    ZeroMemory( DllNames, *len );
    for( mi=0,p=DllNames; mi<MAX_EXTDLLS; mi++) {
        if (LoadedExts[mi].dwLoaded == EXT_LOADED) {
            strcpy( p, LoadedExts[mi].szPathToDll );
            p += (strlen(p) + 1);
        }
    }
    return DllNames;
}


VOID
SetExtensionDllNames(
    LPSTR DllNames
    )
{
    DWORD mi;
    LPSTR p;


    p = DllNames;
    for( mi=0; mi<MAX_EXTDLLS; mi++) {
        if (LoadedExts[mi].dwLoaded == EXT_LOADED) {
            FreeLibrary( LoadedExts[mi].hModule );
            LoadedExts[mi].hModule = NULL;
            LoadedExts[mi].dwLoaded = EXT_NOT_LOADED;
            NumLoadedExts--;
        }
    }
    mi = 0;
    ZeroMemory( LoadedExts, sizeof(LoadedExts) );
    while( p && *p ) {
        strcpy( LoadedExts[mi].szPathToDll, p );
        _tsplitpath(p, NULL, NULL, LoadedExts[mi].szDllName, NULL );
        LoadedExts[mi].dwLoaded = EXT_NEEDS_LOADING;
        mi += 1;
        p += (strlen(p) + 1);
    }
}
