/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    api.c

Abstract:

    This module implements the all apis that simulate their
    WIN32 counterparts.

Author:

    Wesley Witt (wesw) 8-Mar-1992

Environment:

    NT 3.1

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop

//
// structures & defines for queue management
//
typedef struct tagCQUEUE {
    struct tagCQUEUE *next;
    DWORD             pid;
    DWORD             tid;
    DWORD             typ;
    DWORD             len;
    DWORD64           data;
} CQUEUE, *LPCQUEUE;

LPCQUEUE           lpcqFirst;
LPCQUEUE           lpcqLast;
LPCQUEUE           lpcqFree;
CQUEUE             cqueue[200];
CRITICAL_SECTION   csContinueQueue;

//
// context cache
//
typedef struct _tagCONTEXTCACHE {
    CONTEXT                 Context;
#if defined(TARGET_i386) || defined(TARGET_IA64)
    KSPECIAL_REGISTERS      sregs;
    BOOL                    fSContextStale;
    BOOL                    fSContextDirty;
#endif // i386
    BOOL                    fContextStale;
    BOOL                    fContextDirty;
} CONTEXTCACHE, *LPCONTEXTCACHE;

CONTEXTCACHE ContextCache[MAXIMUM_PROCESSORS];
DWORD        CacheProcessors = 1;                   // up machine by default

USHORT ContextSize = 0;

extern MODULEALIAS  ModuleAlias[];

//
// Debug data cache
//
typedef struct _DEBUG_DATA_CACHE {
    LIST_ENTRY                  List;
    DWORD                       Time;
    DBGKD_DEBUG_DATA_HEADER64   Block;
} DEBUG_DATA_CACHE, *PDEBUG_DATA_CACHE;

LIST_ENTRY DebugDataCacheList;

KDDEBUGGER_DATA64 KdDebuggerData;


BOOL
LoadKdDataBlock(
    VOID
    );

//#define GetNtDebuggerData(NAME) \
//    (LoadKdDataBlock() ? KdDebuggerData.NAME: GetSymbolAddress("nt!"#NAME))


//
// globals
//
DWORD                    DmKdState = S_UNINITIALIZED;
BOOL                     DmKdExit;
DBGKD_WAIT_STATE_CHANGE64 sc;
BOOL                     fScDirty;
HANDLE                   hEventContinue;
BOOL                     fCrashDump;
DBGKD_WRITE_BREAKPOINT64 bps[64];
BOOL                     bpcheck[64];
HANDLE                   hThreadDmPoll;
DBGKD_GET_VERSION64      vs;
PDUMP_HEADER             _DmpHeader;
char                     szProgName[MAX_PATH];
DWORD                    PollThreadId;
PKPRCB                   KiProcessors[MAXIMUM_PROCESSORS];
PCONTEXT                 DmpContext;
BOOL                     fPacketTrace;
DWORD                    LastStopTime;

#define DUMPHEADER(f) ((PDUMP_HEADER)_DmpHeader)->f

//
// kernel symbol addresses
//
ULONG64                  DcbAddr;
ULONG64                  MmLoadedUserImageList;
ULONG64                  KiPcrBaseAddress;
ULONG64                  KiProcessorBlockAddr;


CRITICAL_SECTION    csApiInterlock;
CRITICAL_SECTION    csSynchronizeTargetInterlock;



#define NoApiForCrashDump()  if (fCrashDump)    return 0;
#define ConsumeAllEvents()   DequeueAllEvents(FALSE,TRUE)

#define END_OF_CONTROL_SPACE    (sizeof(KPROCESSOR_STATE))

#define CRASH_BUGCHECK_CODE   0xDEADDEAD

//
// local prototypes
//
BOOL GenerateKernelModLoad(HPRCX hprc, LPSTR lpProgName);


//
// externs
//
extern jmp_buf    JumpBuffer;
extern BOOL       DmKdBreakIn;
extern BOOL       KdResync;
extern BOOL       InitialBreak;
extern HANDLE     hEventCreateProcess;
extern HANDLE     hEventCreateThread;
extern HANDLE     hEventRemoteQuit;
extern HANDLE     hEventContinue;
extern HPRCX      prcList;
extern BOOL       fDisconnected;

extern LPDM_MSG     LpDmMsg;

extern PKILLSTRUCT           KillQueue;
extern CRITICAL_SECTION      csKillQueue;

extern HTHDX        thdList;
extern HPRCX        prcList;
extern CRITICAL_SECTION csThreadProcList;
extern CRITICAL_SECTION csProcessDebugEvent;

extern BOOL    fSmartRangeStep;
extern HANDLE hEventNoDebuggee;
extern HANDLE hEventRemoteQuit;
extern BOOL         fDisconnected;
extern BOOL         fUseRoot;
extern char       nameBuffer[];



DWORD64 GetSymbolAddress( LPSTR sym );
BOOL  UnloadModule( DWORD64 BaseOfDll, LPSTR NameOfDll );
VOID  UnloadAllModules( VOID );
VOID  DisableEmCache( VOID );
VOID  InitializeKiProcessor(VOID);
VOID  ProcessCacheCmd(LPSTR pchCommand);
VOID  GetKernelSymbolAddresses(VOID);



BOOL
DbgReadMemory(
    HPRCX   hprc,
    ULONG64 lpBaseAddress,
    PVOID   lpBuffer,
    DWORD   nSize,
    PDWORD  lpcbRead
    )
{
    DWORD                         cb;
    int                           iDll;
    int                           iobj;
    static PIMAGE_SECTION_HEADER  s = NULL;
    static ULONG64                SavedBaseOfImage = 0;
    BOOL                          non_discardable = FALSE;
    PDLLLOAD_ITEM                 d;


    if (nSize == 0) {
        return TRUE;
    }

    //
    // the following code is necessary to determine if the requested
    // base address is in a read-only page or is in a page that contains
    // code.  if the base address meets these conditions then is is marked
    // as non-discardable and will never be purged from the cache.
    //
    if (s &&
        lpBaseAddress >= SavedBaseOfImage + s->VirtualAddress &&
        lpBaseAddress < SavedBaseOfImage + s->VirtualAddress+s->SizeOfRawData &&
            ((s->Characteristics & IMAGE_SCN_CNT_CODE) ||
             (!s->Characteristics & IMAGE_SCN_MEM_WRITE))) {

                non_discardable = TRUE;

    }
    else {
        d = prcList->next->rgDllList;
        for (iDll=0; iDll<prcList->next->cDllList; iDll++) {

            assert( Is64PtrSE(d[iDll].offBaseOfImage) );

            if (lpBaseAddress >= d[iDll].offBaseOfImage &&
                lpBaseAddress < d[iDll].offBaseOfImage + d[iDll].cbImage) {

                if (!d[iDll].Sections) {
                    if (d[iDll].sec) {
                        d[iDll].Sections = d[iDll].sec;
                        for (iobj=0; iobj<(int)d[iDll].NumberOfSections; iobj++) {
                            d[iDll].Sections[iobj].VirtualAddress += (DWORD)d[iDll].offBaseOfImage;
                        }
                    }
                }

                s = d[iDll].Sections;
                SavedBaseOfImage = d[iDll].offBaseOfImage & 0xffffffff00000000UI64;

                cb = d[iDll].NumberOfSections;
                while (cb) {
                    if (lpBaseAddress >= SavedBaseOfImage + s->VirtualAddress &&
                        lpBaseAddress < SavedBaseOfImage + s->VirtualAddress+s->SizeOfRawData &&
                        ((s->Characteristics & IMAGE_SCN_CNT_CODE) ||
                         (!s->Characteristics & IMAGE_SCN_MEM_WRITE))) {

                        non_discardable = TRUE;
                        break;

                    }
                    else {
                        s++;
                        cb--;
                    }
                }
                if (!cb) {
                    s = NULL;
                }

                break;
            }
        }
    }

    if (fCrashDump) {

        cb = DmpReadMemory( (DWORD64)lpBaseAddress, lpBuffer, nSize );

    } else {

        if (DmKdReadCachedVirtualMemory( lpBaseAddress,
                                         nSize,
                                         (PUCHAR) lpBuffer,
                                         &cb,
                                         non_discardable) != STATUS_SUCCESS ) {
            cb = 0;
        }

    }

    if ( cb > 0 && non_discardable ) {
        BREAKPOINT *bp;
        ADDR        Addr;
        BP_UNIT     instr;
        DWORD       offset;
        LPVOID      lpb;

        AddrInit( &Addr, 0, 0, lpBaseAddress, TRUE, TRUE, FALSE, FALSE );
        lpb = lpBuffer;

        for (bp=bpList->next; bp; bp=bp->next) {
            if (BPInRange((HPRCX)0, (HTHDX)0, bp, &Addr, cb, &offset, &instr)) {
                if (instr) {
                    if (offset < 0) {
                        memcpy(lpb, ((char *) &instr) - offset,
                               sizeof(BP_UNIT) + offset);
                    } else if (offset + sizeof(BP_UNIT) > cb) {
                        memcpy(((char *)lpb)+offset, &instr, cb - offset);
                    } else {
                        *((BP_UNIT UNALIGNED *)((char *)lpb+offset)) = instr;
                    }
#if defined(TARGET_IA64)
                    //
                    // restore template to MLI if displaced instruction is MOVL and in range
                    //
                    if(((bp->flags & BREAKPOINT_IA64_MOVL) && (offset >= 0)) && ((GetAddrOff(bp->addr) & 0xf) == 4)) {
                        *((char *)lpb + offset - 4) &= ~(0x1e);
                        *((char *)lpb + offset - 4) |= 0x4;
                    }
#endif
                }
            }
        }
    }

    if (cb > 0) {
        if (lpcbRead) {
            *lpcbRead = cb;
        }
        return TRUE;
    } else {
        return FALSE;
    }
}


BOOL
DbgWriteMemory(
    HPRCX   hprc,
    ULONG64  lpBaseAddress,
    PVOID  lpBuffer,
    DWORD   nSize,
    LPDWORD lpcbWrite
    )
{
    ULONG   cb;


    if (nSize == 0) {
        return TRUE;
    }

    if (fCrashDump) {

        cb = DmpWriteMemory( (DWORD64)lpBaseAddress, lpBuffer, nSize );

    } else {

        if (DmKdWriteVirtualMemory( lpBaseAddress,
                                    lpBuffer,
                                    nSize,
                                    &cb ) != STATUS_SUCCESS ) {
            cb = 0;
        }

    }

    if (cb > 0) {
        if (lpcbWrite) {
            *lpcbWrite = cb;
        }
        return TRUE;
    } else {
        return FALSE;
    }
}

ULONG
DbgReadNtHeader(
    ULONG64 BaseOfDll,
    PIMAGE_NT_HEADERS64     pNtHeaders
    )
{    
    ULONG Result;
    ULONG Status;
    //IMAGE_NT_HEADERS ImNtHdr;
    IMAGE_DOS_HEADER ImDosHdr;
    

    ZeroMemory(pNtHeaders, sizeof(*pNtHeaders) );

    //
    // Get the DOS hdr
    //
    Status = DmKdReadMemoryWrapper(BaseOfDll, 
                                   (PVOID)&ImDosHdr, 
                                   sizeof(ImDosHdr),
                                   &Result
                                   );

    if ( sizeof(ImDosHdr) != Result ) {
        // Error reading header
        return Status;
    }

    //
    // Get the NT hdr
    //
    if (ImDosHdr.e_magic == IMAGE_DOS_SIGNATURE) {
        DWORD64 dw64NtHdrAddress = BaseOfDll + ImDosHdr.e_lfanew;

        Status = DmKdReadMemoryWrapper(dw64NtHdrAddress, 
                                       //(PVOID)&ImNtHdr, 
                                       //sizeof(ImNtHdr)
                                       pNtHeaders,
                                       sizeof(*pNtHeaders),
                                       &Result
                                       );

    }

    return Status;
}

BOOL
DbgGetThreadContext(
    IN  HTHDX     hthd,
    OUT LPCONTEXT lpContext
    )
{
    BOOL rc = TRUE;
    USHORT processor;
    DWORD Flags = lpContext->ContextFlags;

    DPRINT(1, ( "DbgGetThreadContext( 0x%p )\n", lpContext ));

    if (!hthd) {
        return FALSE;
    }

    processor = (USHORT)hthd->tid - 1;

    if (fCrashDump) {
        if (processor == sc.Processor && KiProcessors[processor] == 0) {
            memcpy( lpContext, DmpContext, ContextSize);
            rc = TRUE;
        } else {
            rc = DmpGetContext( processor, lpContext );
        }

    } else {

        if (ContextCache[processor].fContextStale) {

            rc = (DmKdGetContext( processor, &ContextCache[processor].Context )
                                                            == STATUS_SUCCESS);
            if (rc) {
                ContextCache[processor].fContextDirty = FALSE;
                ContextCache[processor].fContextStale = FALSE;
            }
        }

        if (rc) {
            memcpy( lpContext,
                    &ContextCache[processor].Context,
                    ContextSize );
        }

    }

    if (rc) {
        hthd->fContextStale = FALSE;
    }

    return rc;
}

BOOL
DbgSetThreadContext(
    IN HTHDX     hthd,
    IN LPCONTEXT lpContext
    )
{
    BOOL rc = TRUE;
    USHORT processor;


    DEBUG_PRINT_1( "DbgSetThreadContext( 0x%p )\n", lpContext );

    NoApiForCrashDump();

    processor = (USHORT)hthd->tid - 1;
    memcpy( &ContextCache[processor].Context, lpContext, ContextSize );

    if (lpContext != &hthd->context) {
        memcpy(&hthd->context, &ContextCache[processor].Context, ContextSize );
    }

    ContextCache[processor].fContextDirty = FALSE;
    ContextCache[processor].fContextStale = FALSE;


    if (DmKdSetContext( processor, lpContext ) != STATUS_SUCCESS) {
        rc = FALSE;
    }

    return rc;
}


BOOL
WriteBreakPoint(
    IN PBREAKPOINT Breakpoint
    )
{
    BOOL rc = TRUE;

    DEBUG_PRINT_2( "WriteBreakPoint( 0x%08x, 0x%08x )\n",
                   GetAddrOff(Breakpoint->addr),
                   Breakpoint->hBreakPoint);

    NoApiForCrashDump();

    if (DmKdWriteBreakPoint( GetAddrOff(Breakpoint->addr),
                             &Breakpoint->hBreakPoint ) != STATUS_SUCCESS) {
        rc = FALSE;
    }

    return rc;
}

BOOL
WriteBreakPointEx(
    IN HTHDX  hthd,
    IN ULONG  BreakPointCount,
    IN OUT PDBGKD_WRITE_BREAKPOINT64 BreakPoints,
    IN ULONG ContinueStatus
    )
{
    BOOL rc = TRUE;

    assert( BreakPointCount > 0 );
    assert( BreakPoints );

    DEBUG_PRINT_2( "WriteBreakPointEx( %d, 0x%016I64x )\n",
                   BreakPointCount, 
                   (ULONGLONG) BreakPoints 
                   );

    NoApiForCrashDump();

    if (DmKdWriteBreakPointEx( BreakPointCount, BreakPoints, ContinueStatus ) != STATUS_SUCCESS) {
        rc = FALSE;
    }

    return rc;
}


BOOL
RestoreBreakPoint(
    IN PBREAKPOINT Breakpoint
    )
{
    BOOL rc = TRUE;

    DEBUG_PRINT_1( "RestoreBreakPoint( 0x%08x )\n", Breakpoint->hBreakPoint );

    NoApiForCrashDump();

    if (DmKdRestoreBreakPoint( Breakpoint->hBreakPoint ) != STATUS_SUCCESS) {
        rc = FALSE;
    }

    return rc;
}


BOOL
RestoreBreakPointEx(
    IN ULONG  BreakPointCount,
    IN PDBGKD_RESTORE_BREAKPOINT BreakPointHandles
    )
{
    BOOL rc = TRUE;

    assert( BreakPointCount > 0 );
    assert( BreakPointHandles );

    DEBUG_PRINT_2( "WriteBreakPointEx( %d, 0x%016I64x )\n",
                   BreakPointCount, 
                   (ULONGLONG)BreakPointHandles );

    NoApiForCrashDump();

    if (DmKdRestoreBreakPointEx( BreakPointCount, BreakPointHandles ) != STATUS_SUCCESS) {
        rc = FALSE;
    }

    return rc;
}

BOOL
ReadControlSpace(
    USHORT  Processor,
    ULONG64 TargetBaseAddress,
    PVOID   UserInterfaceBuffer,
    ULONG   TransferCount,
    PULONG  ActualBytesRead
    )
{
    DWORD Status;


    if (fCrashDump) {
        return DmpReadControlSpace(
            Processor,
            (DWORD64)TargetBaseAddress,
            UserInterfaceBuffer,
            TransferCount,
            ActualBytesRead
            );
    }

    Status = DmKdReadControlSpace(
        Processor,
        TargetBaseAddress,
        UserInterfaceBuffer,
        TransferCount,
        ActualBytesRead
        );

    if (Status || (ActualBytesRead && *ActualBytesRead != TransferCount)) {
        return FALSE;
    }

    return TRUE;
}

VOID
ContinueTargetSystem(
    DWORD               ContinueStatus,
    PDBGKD_CONTROL_SET  ControlSet
    )
{
    DWORD   rc;

    //
    // Api lock remains held until target system stops
    //

    TakeApiLock();

    if (ControlSet) {

        rc = DmKdContinue2( ContinueStatus, ControlSet );

    } else {

        rc = DmKdContinue( ContinueStatus );

    }

    ReleaseApiLock();

}

ULONG
UnicodeStringToAnsiString(
    PANSI_STRING    DestinationString,
    PUNICODE_STRING64 SourceString,
    BOOL         AllocateDestinationString
    )
{
    if (AllocateDestinationString) {
        DestinationString->Buffer = MHAlloc( DestinationString->MaximumLength );
        if (!DestinationString->Buffer) {
            return 1;
        }
    }

    DestinationString->Length = (WORD)WideCharToMultiByte(
        CP_ACP,
        WC_COMPOSITECHECK,
        (PVOID)SourceString->Buffer,
        SourceString->Length / 2,
        (PVOID)DestinationString->Buffer,
        DestinationString->MaximumLength,
        NULL,
        NULL
        );

    return 0;
}


VOID
InitUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR          SourceString
    )
{
    wcsncpy( DestinationString->Buffer, SourceString, DestinationString->MaximumLength );
    DestinationString->Length = wcslen( DestinationString->Buffer ) * 2;
}


BOOL
ReloadModule(
    HTHDX                  hthd,
    PLDR_DATA_TABLE_ENTRY64 DataTableBuffer,
    BOOL                   fDontUseLoadAddr,
    BOOL                   fLocalBuffer
    )
{
    UNICODE_STRING64            BaseName;
    CHAR                        AnsiBuffer[512];
    WCHAR                       UnicodeBuffer[512];
    ANSI_STRING                 AnsiString;
    NTSTATUS                    Status;
    DEBUG_EVENT64               de;
    CHAR                        fname[_MAX_FNAME];
    CHAR                        ext[_MAX_EXT];
    ULONG                       cb;
    LPBYTE                      lpbPacket;
    WORD                        cbPacket;

    //
    // Get the base DLL name.
    //
    if (DataTableBuffer->BaseDllName.Length != 0 &&
        DataTableBuffer->BaseDllName.Buffer != 0 ) {

        BaseName = DataTableBuffer->BaseDllName;

    } else
    if (DataTableBuffer->FullDllName.Length != 0 &&
        DataTableBuffer->FullDllName.Buffer != 0 ) {

        BaseName = DataTableBuffer->FullDllName;

    } else {

        return FALSE;

    }

    if (BaseName.Length > sizeof(UnicodeBuffer)) {
        DMPrintShellMsg( "cannot complete modload %08x\n", BaseName.Length );
        return FALSE;
    }

    if (!fLocalBuffer) {
        if (!DbgReadMemory( hthd->hprc, BaseName.Buffer, (PVOID)UnicodeBuffer, BaseName.Length, &cb )) {
            return FALSE;
        }
        BaseName.Buffer = (UINT_PTR)UnicodeBuffer;
        BaseName.Length = (USHORT)cb;
        BaseName.MaximumLength = (USHORT)(cb + sizeof( UNICODE_NULL ));
        UnicodeBuffer[ cb / sizeof( WCHAR ) ] = UNICODE_NULL;
    }

    AnsiString.Buffer = AnsiBuffer;
    AnsiString.MaximumLength = 256;
    Status = UnicodeStringToAnsiString(&AnsiString, &BaseName, FALSE);
    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }
    AnsiString.Buffer[AnsiString.Length] = '\0';

    _splitpath( AnsiString.Buffer, NULL, NULL, fname, ext );
    _makepath( AnsiString.Buffer, NULL, NULL, fname, ext );

    de.dwDebugEventCode                 = LOAD_DLL_DEBUG_EVENT;
    de.dwProcessId                      = KD_PROCESSID;
    de.dwThreadId                       = KD_THREADID;
    de.u.LoadDll.hFile                  = (HANDLE)DataTableBuffer->CheckSum;
    de.u.LoadDll.lpBaseOfDll            = fDontUseLoadAddr ? 0 : DataTableBuffer->DllBase;
    de.u.LoadDll.lpImageName            = (UINT_PTR)AnsiString.Buffer;
    de.u.LoadDll.dwDebugInfoFileOffset  = DataTableBuffer->SizeOfImage;
    de.u.LoadDll.fUnicode               = FALSE;
    de.u.LoadDll.nDebugInfoSize         = 0;

    assert( Is64PtrSE(de.u.LoadDll.lpBaseOfDll) );
  
    if (LoadDll(&de, hthd, &cbPacket, &lpbPacket, FALSE) && (cbPacket != 0)) {
        NotifyEM(&de, hthd, cbPacket, (UINT_PTR)lpbPacket);
    }

    return TRUE;
}


BOOL
ReloadModulesFromList(
    HTHDX hthd,
    ULONG64 ListAddr,
    BOOL  fDontUseLoadAddr,
    LPSTR JustLoadThisOne,
    ULONG64 UseThisAddress
    )
{
    LIST_ENTRY64                List;
    DWORDLONG                   Next;
    ULONG                       len = 0;
    DWORDLONG                   DataTable;
    LDR_DATA_TABLE_ENTRY64      DataTableBuffer;
    WCHAR                       UnicodeBuffer[_MAX_PATH];
    WCHAR                       UnicodeBuffer2[_MAX_PATH];
    int                         Len;
    BOOL                        LoadedSomething;

    assert( Is64PtrSE(ListAddr) );
    assert( Is64PtrSE(UseThisAddress) );


    if (!ListAddr) {
        return FALSE;
    }

    //
    // convert the module name to unicode
    //

    *UnicodeBuffer = 0;

    if (JustLoadThisOne && *JustLoadThisOne) {

        Len = _tcslen(JustLoadThisOne);
        MultiByteToWideChar(
            CP_OEMCP,
            0,
            JustLoadThisOne,
            Len,
            UnicodeBuffer,
            sizeof(UnicodeBuffer)
            );
    }

    if (!NT_SUCCESS(DmKdReadListEntry(ListAddr, &List))) {
        return FALSE;
    }

    Next = List.Flink;
    if (Next == 0) {
        return FALSE;
    }

    LoadedSomething = FALSE;

    while (Next != ListAddr) {
        if (DmKdPtr64) {
            DataTable = CONTAINING_RECORD64( Next,
                                             LDR_DATA_TABLE_ENTRY64,
                                             InLoadOrderLinks
                                            );
        } else {
            DataTable = SE32To64( CONTAINING_RECORD32( Next,
                                             LDR_DATA_TABLE_ENTRY32,
                                             InLoadOrderLinks
                                            ));
        }

        if (!NT_SUCCESS(DmKdReadLoaderEntry( DataTable, &DataTableBuffer ))) {
            break;
        }

        Next = DataTableBuffer.InLoadOrderLinks.Flink;

        if (!(JustLoadThisOne && *JustLoadThisOne) ) {
            ReloadModule( hthd, &DataTableBuffer, fDontUseLoadAddr, FALSE );
            LoadedSomething = TRUE;
        } else {
            if (2*Len == DataTableBuffer.BaseDllName.Length) {
                if (!DbgReadMemory( hthd->hprc,
                                    DataTableBuffer.BaseDllName.Buffer,
                                    (PVOID)UnicodeBuffer2,
                                    DataTableBuffer.BaseDllName.Length,
                                    NULL )) {
                    continue;
                }
                if (_wcsnicmp(UnicodeBuffer, UnicodeBuffer2, Len) == 0) {
                    if (UseThisAddress) {
                        DataTableBuffer.DllBase = UseThisAddress;
                    }
                    ReloadModule( hthd, &DataTableBuffer, fDontUseLoadAddr, FALSE );
                    LoadedSomething = TRUE;
                    break;
                }
            }
        }
    }

    return LoadedSomething;
}

#if 0
BOOL
ReloadCrashModules(
    HTHDX hthd
    )
{
    ULONG                       ListAddr;
    ULONG                       DcbPtr;
    ULONG                       i;
    DUMP_CONTROL_BLOCK          dcb;
    PLIST_ENTRY                 Next;
    ULONG                       len = 0;
    PMINIPORT_NODE              mpNode;
    MINIPORT_NODE               mpNodeBuf;
    PLDR_DATA_TABLE_ENTRY       DataTable;
    LDR_DATA_TABLE_ENTRY        DataTableBuffer;
    CHAR                        AnsiBuffer[512];
    WCHAR                       UnicodeBuffer[512];


    if (!DcbAddr) {
        //
        // kernel symbols are hosed
        //
        return FALSE;
    }

    if (!DbgReadMemory( hthd->hprc, (PVOID)DcbAddr, (PVOID)&DcbPtr, sizeof(DWORD), NULL)) {
        return FALSE;
    }

    if (!DcbPtr) {
        //
        // crash dumps are not enabled
        //
        return FALSE;
    }

    if (!DbgReadMemory( hthd->hprc, (PVOID)DcbPtr, (PVOID)&dcb, sizeof(dcb), NULL)) {
        return FALSE;
    }

    ListAddr = DcbPtr + FIELD_OFFSET( DUMP_CONTROL_BLOCK, MiniportQueue );

    Next = dcb.MiniportQueue.Flink;
    if (Next == NULL) {
        return FALSE;
    }

    while ((ULONG)Next != ListAddr) {
        mpNode = CONTAINING_RECORD( Next, MINIPORT_NODE, ListEntry );

        if (!DbgReadMemory( hthd->hprc, (PVOID)mpNode, (PVOID)&mpNodeBuf, sizeof(MINIPORT_NODE), NULL )) {
            return FALSE;
        }

        Next = mpNodeBuf.ListEntry.Flink;

        DataTable = mpNodeBuf.DriverEntry;
        if (!DataTable) {
            continue;
        }

        if (!DbgReadMemory( hthd->hprc, (PVOID)DataTable, (PVOID)&DataTableBuffer, sizeof(LDR_DATA_TABLE_ENTRY), NULL)) {
            return FALSE;
        }

        //
        // find an empty module alias slot
        //
        for (i=0; i<MAX_MODULEALIAS; i++) {
            if (ModuleAlias[i].ModuleName[0] == 0) {
                break;
            }
         }

        if (i == MAX_MODULEALIAS) {
            //
            // module alias table is full, ignore this module
            //
            continue;
        }

        //
        // convert the module name to ansi
        //

        ZeroMemory( UnicodeBuffer, sizeof(UnicodeBuffer) );
        ZeroMemory( AnsiBuffer, sizeof(AnsiBuffer) );

        if (!DbgReadMemory( hthd->hprc,
                            (PVOID)DataTableBuffer.BaseDllName.Buffer,
                            (PVOID)UnicodeBuffer,
                            DataTableBuffer.BaseDllName.Length,
                            NULL )) {
            continue;
        }

        WideCharToMultiByte(
            CP_OEMCP,
            0,
            UnicodeBuffer,
            DataTableBuffer.BaseDllName.Length / 2,
            AnsiBuffer,
            sizeof(AnsiBuffer),
            NULL,
            NULL
            );

        //
        // establish an alias for the crash driver
        //
        _tcscpy( ModuleAlias[i].Alias, AnsiBuffer );
        ModuleAlias[i].ModuleName[0] = 'c';
        _splitpath( AnsiBuffer, NULL, NULL, &ModuleAlias[i].ModuleName[1], NULL );
        ModuleAlias[i].ModuleName[8] = 0;
        ModuleAlias[i].Special = 2;     // One shot alias...

        //
        // reload the module
        //
        ReloadModule( hthd, &DataTableBuffer, FALSE, FALSE );
    }

    //
    // now do the magic diskdump.sys driver
    //
    if (!DbgReadMemory( hthd->hprc, (PVOID)dcb.DiskDumpDriver, (PVOID)&DataTableBuffer, sizeof(LDR_DATA_TABLE_ENTRY), NULL)) {
        return FALSE;
    }

    //
    // change the driver name from scsiport.sys to diskdump.sys
    //
    DataTableBuffer.BaseDllName.Buffer = UnicodeBuffer;
    InitUnicodeString( &DataTableBuffer.BaseDllName, L"diskdump.sys" );

    //
    // load the module
    //
    ReloadModule( hthd, &DataTableBuffer, FALSE, TRUE );

    return TRUE;
}
#endif


BOOL
FindModuleInList(
    HPRCX                  hprc,
    LPSTR                  lpModName,
    ULONG64                ListAddr,
    LPIMAGEINFO            ii
    )
{
    LIST_ENTRY64                List;
    ULONG64                     Next;
    ULONG                       len = 0;
    ULONG                       cb;
    ULONG64                     DataTable;
    LDR_DATA_TABLE_ENTRY64      DataTableBuffer;
    UNICODE_STRING64            BaseName;
    CHAR                        AnsiBuffer[512];
    WCHAR                       UnicodeBuffer[512];
    ANSI_STRING                 AnsiString;
    NTSTATUS                    Status;


    ii->CheckSum     = 0;
    ii->SizeOfImage  = 0;
    ii->BaseOfImage  = 0;

    if (!ListAddr) {
        return FALSE;
    }

    if (!NT_SUCCESS(DmKdReadListEntry(ListAddr, &List))) {
        return FALSE;
    }

    Next = List.Flink;
    if (Next == 0) {
        return FALSE;
    }

    while (Next != ListAddr) {
        if (DmKdPtr64) {
            DataTable = CONTAINING_RECORD64( Next,
                                       LDR_DATA_TABLE_ENTRY64,
                                       InLoadOrderLinks
                                     );
        } else {
            DataTable = SE32To64( CONTAINING_RECORD32( Next,
                                       LDR_DATA_TABLE_ENTRY32,
                                       InLoadOrderLinks
                                     ));
        }

        if (!NT_SUCCESS(DmKdReadLoaderEntry( DataTable, &DataTableBuffer))) {
            return FALSE;
        }

        Next = DataTableBuffer.InLoadOrderLinks.Flink;

        //
        // Get the base DLL name.
        //
        if (DataTableBuffer.BaseDllName.Length != 0 &&
            DataTableBuffer.BaseDllName.Buffer != 0
           ) {
            BaseName = DataTableBuffer.BaseDllName;
        }
        else
        if (DataTableBuffer.FullDllName.Length != 0 &&
            DataTableBuffer.FullDllName.Buffer != 0
           ) {
            BaseName = DataTableBuffer.FullDllName;
        }
        else {
            continue;
        }

        if (BaseName.Length > sizeof(UnicodeBuffer)) {
            continue;
        }

        cb = DbgReadMemory( hprc,
                            BaseName.Buffer,
                            (PVOID)UnicodeBuffer,
                            BaseName.Length,
                            NULL );
        if (!cb) {
            return FALSE;
        }

        BaseName.Buffer = (UINT_PTR)UnicodeBuffer;
        BaseName.Length = (USHORT)cb;
        BaseName.MaximumLength = (USHORT)(cb + sizeof( UNICODE_NULL ));
        UnicodeBuffer[ cb / sizeof( WCHAR ) ] = UNICODE_NULL;
        AnsiString.Buffer = AnsiBuffer;
        AnsiString.MaximumLength = 256;
        Status = UnicodeStringToAnsiString(&AnsiString, &BaseName, FALSE);
        if (!NT_SUCCESS(Status)) {
            return FALSE;
        }
        AnsiString.Buffer[AnsiString.Length] = '\0';

        if (_stricmp(AnsiString.Buffer, lpModName) == 0) {
            ii->BaseOfImage = DataTableBuffer.DllBase;
            ii->SizeOfImage = DataTableBuffer.SizeOfImage;
            ii->CheckSum    = DataTableBuffer.CheckSum;
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
ReadImageInfo(
    LPSTR                  lpImageName,
    LPSTR                  lpFoundName,
    LPSTR                  lpPath,
    LPIMAGEINFO            ii
    )

/*++

Routine Description:

    This routine locates the file specified by lpImageName and reads the
    IMAGE_NT_HEADERS and the IMAGE_SECTION_HEADER from the image.

Arguments:


Return Value:

    True on success and FALSE on failure

--*/

{
    HANDLE                      hFile;
    IMAGE_DOS_HEADER            dh;
    IMAGE_NT_HEADERS            nh;
    IMAGE_SEPARATE_DEBUG_HEADER sdh;
    IMAGE_ROM_OPTIONAL_HEADER   rom;
    DWORD                       sig;
    DWORD                       cb;
    char                        rgch[MAX_PATH];
    CHAR                        fname[_MAX_FNAME];
    CHAR                        ext[_MAX_EXT];
    CHAR                        drive[_MAX_DRIVE];
    CHAR                        dir[_MAX_DIR];
    CHAR                        modname[MAX_PATH];


    hFile = FindExecutableImage( lpImageName, lpPath, rgch );
    if (hFile) {

        if (lpFoundName) {
            _tcscpy(lpFoundName, rgch);
        }
        //
        // read in the pe/file headers from the EXE file
        //
        SetFilePointer( hFile, 0, 0, FILE_BEGIN );
        ReadFile( hFile, &dh, sizeof(IMAGE_DOS_HEADER), &cb, NULL );

        if (dh.e_magic == IMAGE_DOS_SIGNATURE) {
            SetFilePointer( hFile, dh.e_lfanew, 0, FILE_BEGIN );
        } else {
            SetFilePointer( hFile, 0, 0, FILE_BEGIN );
        }

        ReadFile( hFile, &sig, sizeof(sig), &cb, NULL );
        SetFilePointer( hFile, -4, NULL, FILE_CURRENT );

        if (sig != IMAGE_NT_SIGNATURE) {
            ReadFile( hFile, &nh.FileHeader, sizeof(IMAGE_FILE_HEADER), &cb, NULL );
            if (nh.FileHeader.SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
                ReadFile( hFile, &rom, sizeof(rom), &cb, NULL );
                ZeroMemory( &nh.OptionalHeader, sizeof(nh.OptionalHeader) );
                nh.OptionalHeader.SizeOfImage      = rom.SizeOfCode;
                nh.OptionalHeader.ImageBase        = rom.BaseOfCode;
            } else {
                CloseHandle( hFile );
                return FALSE;
            }
        } else {
            ReadFile( hFile, &nh, sizeof(nh), &cb, NULL );
        }

        ii->TimeStamp    = nh.FileHeader.TimeDateStamp;
        ii->CheckSum     = nh.OptionalHeader.CheckSum;
        ii->SizeOfImage  = nh.OptionalHeader.SizeOfImage;
        ii->BaseOfImage  = SE32To64(nh.OptionalHeader.ImageBase);

    } else {

        if (lpFoundName) {
            *lpFoundName = 0;
        }
        //
        // read in the pe/file headers from the DBG file
        //
        hFile = FindDebugInfoFile( lpImageName, lpPath, rgch );
        if (!hFile) {
            _splitpath( lpImageName, NULL, NULL, fname, NULL );
            sprintf( modname, "%s.dbg", fname );
            hFile = FindExecutableImage( modname, lpPath, rgch );
            if (!hFile) {
                return FALSE;
            }
        }

        SetFilePointer( hFile, 0, 0, FILE_BEGIN );
        ReadFile( hFile, &sdh, sizeof(IMAGE_SEPARATE_DEBUG_HEADER), &cb, NULL );

        nh.FileHeader.NumberOfSections = (USHORT)sdh.NumberOfSections;

        ii->CheckSum     = sdh.CheckSum;
        ii->TimeStamp    = sdh.TimeDateStamp;
        ii->SizeOfImage  = sdh.SizeOfImage;
        ii->BaseOfImage  = SE32To64(sdh.ImageBase);
    }
    
    cb = nh.FileHeader.NumberOfSections * IMAGE_SIZEOF_SECTION_HEADER;
    ii->NumberOfSections = nh.FileHeader.NumberOfSections;
    ii->Sections = MHAlloc( cb );
    ReadFile( hFile, ii->Sections, cb, &cb, NULL );

    CloseHandle( hFile );
    return TRUE;
}


BOOL
LookupImageByAddress(
    IN DWORD64 Address,
    OUT PSTR ImageName
    )
/*++

Routine Description:

    Look in rebase.log and coffbase.txt for an image which
    contains the address provided.

Arguments:

    Address - Supplies the address to look for.

    ImageName - Returns the name of the image if found.

Return Value:

    TRUE for success, FALSE for failure.  ImageName is not modified
    if the search fails.

--*/
{
    LPSTR RootPath;
    LPSTR pstr;
    char FileName[_MAX_PATH];
    char Buffer[_MAX_PATH];
    BOOL Replace;
    DWORD ImageAddress;
    DWORD Size;
    FILE *File;

    //
    // Locate rebase.log file
    //
    // SymbolPath or %SystemRoot%\Symbols
    //

    assert( Is64PtrSE(Address) );

    RootPath = pstr = (LPSTR)KdOptions[KDO_SYMBOLPATH].value;

    Replace = FALSE;
    File = NULL;

    while (File == NULL && *pstr) {

        while (*pstr) {
            if (*pstr == ';') {
                *pstr = 0;
                Replace = TRUE;
                break;
            }
            pstr++;
        }

        if (SearchTreeForFile(RootPath, "rebase.log", FileName)) {
            File = fopen(FileName, "r");
        }

        if (Replace) {
            *pstr = ';';
            RootPath = ++pstr;
        }
    }

    if (!File) {
        return FALSE;
    }

    //
    // Search file for image
    //
    while (fgets(Buffer, sizeof(Buffer), File)) {
        ImageAddress = 0xffffffff;
        Size = 0xffffffff;
        sscanf( Buffer, "%s %*s %*s 0x%p (size 0x%x)",
                FileName, 
                &ImageAddress, 
                &Size);
        if (Size == 0xffffffff) {
            continue;
        }
        if (Address >= ImageAddress && Address < ImageAddress + Size) {
            _tcscpy(ImageName, FileName);
            fclose(File);
            return TRUE;
        }
    }

    fclose(File);

    return FALSE;
}

VOID
ReloadModules(
    HTHDX hthd,
    LPSTR args
    )
{
    DEBUG_EVENT64               de = {0};
    ULONG                       len = 0;
    int                         i = 0;
    HPRCX                       hprc = 0;
    LPRTP                       rtp = 0;
    CHAR                        fname[_MAX_FNAME] = {0};
    CHAR                        ext[_MAX_EXT] = {0};
    CHAR                        drive[_MAX_DRIVE] = {0};
    CHAR                        dir[_MAX_DIR] = {0};
    CHAR                        modname[MAX_PATH] = {0};
    ULONG64                     Address = 0;
    ULONG64                     LoadAddress = 0;
    BOOL                        UnloadOnly = FALSE;
    PCHAR                       p = 0;
    BOOL                        LoadUsermodeModules = TRUE;
    BOOL                        ReallyVerbose = FALSE;
    BOOL                        UsedArgument = FALSE;
    DWORD                       dw = 0;


    //
    // this is to handle the ".reload foo.exe" command
    //
    // we search thru the module list and find the desired module.
    // the module is then unloaded and re-loaded.  the module is re-loaded
    // at its preferred load address.
    //
    while (args && *args) {

        //
        //  skip over any white space
        //
        while (*args == ' ' || *args == '\t') {
            args++;
        }

        if (args[0] == '/' || args[0] == '-') {

            args++;
            while (*args > ' ') {

                switch (*args++) {

                    case 'n':
                        LoadUsermodeModules = FALSE;
                        break;

                    case 'u':
                        UnloadOnly = TRUE;
                        break;

                    case 'v':
                        ReallyVerbose = TRUE;
                        break;

                    default:
                        DMPrintShellMsg("bangReload: unknown option '%c'\n", args[-1]);
                        DMPrintShellMsg("usage: !reload [flags] [module] ...\n");
                        DMPrintShellMsg("  flags:   /n  do not load from usermode list\n");
                        DMPrintShellMsg("           /u  unload symbols, no reload\n");
                        DMPrintShellMsg("           /v  verbose\n");
                        goto bail;
                }

            }

        } else if (*args) {

            UsedArgument = TRUE;

            LoadAddress = 0;
            if (p = _tcschr(args, '=')) {
                *p = 0;
                sscanf(p+1, "%I64x", &LoadAddress);

                if (!Is64PtrSE(LoadAddress)) {
                    //
                    // Let the user know he didn't sign extend
                    //
                    DMPrintShellMsg("DM RELOAD WARNING: Address was not properly sign extended. "
                        "Automatically sign extended from %016i64x to %016i64x.",
                        LoadAddress, SE32To64(LoadAddress));
                    LoadAddress = SE32To64(LoadAddress);
                }
            }

            _splitpath( args, drive, dir, fname, ext );

            if (p) {
                *p = '=';
            }

            _makepath( modname, NULL, NULL, fname, ext );

            if (isdigit(*args)) {
                sscanf(args, "%I64x", &Address);

                if (!Is64PtrSE(Address)) {
                    //
                    // Let the user know he didn't sign extend
                    //
                    DMPrintShellMsg("DM RELOAD WARNING: Address was not properly sign extended. "
                        "Automatically sign extended from %016i64x to %016i64x.",
                        Address, SE32To64(Address));
                    Address = SE32To64(Address);
                }

                if (LookupImageByAddress(Address, modname)) {
                    _splitpath( modname, drive, dir, fname, ext );
                }
            }

            while (*args && *args != ' ' && *args != '\t') {
                args++;
            }

            hprc = HPRCFromPID( KD_PROCESSID );

            for (i=0; i<hprc->cDllList; i++) {
                if ((hprc->rgDllList[i].fValidDll) &&
                    (_stricmp(hprc->rgDllList[i].szDllName, modname) == 0)) {           

                    UnloadModule( hprc->rgDllList[i].offBaseOfImage, modname );
                    break;

                }
            }
        }

        if (!UnloadOnly && UsedArgument) {

            _makepath( modname, drive, dir, fname, ext );

            if (!ReloadModulesFromList(hthd,
                                       vs.PsLoadedModuleList,
                                       FALSE,
                                       modname,
                                       LoadAddress)) {
                if (!LoadUsermodeModules ||
                    !ReloadModulesFromList(hthd,
                                           MmLoadedUserImageList,
                                           FALSE,
                                           modname,
                                           LoadAddress)) {
                    LPBYTE lpbPacket;
                    WORD   cbPacket;
                    de.dwDebugEventCode                = LOAD_DLL_DEBUG_EVENT;
                    de.dwProcessId                     = KD_PROCESSID;
                    de.dwThreadId                      = KD_THREADID;
                    de.u.LoadDll.hFile                 = NULL;
                    de.u.LoadDll.lpBaseOfDll           = LoadAddress;
                    de.u.LoadDll.lpImageName           = (UINT_PTR)modname;
                    de.u.LoadDll.fUnicode              = FALSE;
                    de.u.LoadDll.nDebugInfoSize        = 0;
                    de.u.LoadDll.dwDebugInfoFileOffset = 0;

                    assert( Is64PtrSE(de.u.LoadDll.lpBaseOfDll) );

                    if (LoadDll(&de, hthd, &cbPacket, &lpbPacket, FALSE) && (cbPacket != 0)) {
                        NotifyEM(&de, hthd, cbPacket, (UINT_PTR)lpbPacket);
                    }

                    //
                    // We may have just reloaded "ntkrnl".
                    //
                    // We may now get the correct addresses
                    //
                    GetKernelSymbolAddresses();
                    InitializeKiProcessor();

                }
            }
        }

    }

    if (!UsedArgument) {

        UnloadAllModules();

        ReloadModulesFromList( hthd, vs.PsLoadedModuleList, FALSE, NULL, 0 );

        if (LoadUsermodeModules) {
            ReloadModulesFromList( hthd, MmLoadedUserImageList, FALSE, NULL, 0 );
        }

        //ReloadCrashModules( hthd );

        //
        // We may now get the correct addresses
        //
        GetKernelSymbolAddresses();
        InitializeKiProcessor();
    }

bail:
    //
    // consume all of the continues from the modloads
    //
    ConsumeAllEvents();

    //
    // tell the shell that the !reload is finished
    //
    dw = 1;
    DMSendDebugPacket(dbcServiceDone,
                      hthd->hprc->hpid,
                      hthd->htid,
                      sizeof(DWORD),
                      &dw);

    return;
}

VOID
ClearBps(
    VOID
    )
{
    DBGKD_RESTORE_BREAKPOINT    bps[MAX_KD_BPS];
    DWORD                       i;

    //
    // clean out the kernel's bp list
    //
    for (i=0; i<MAX_KD_BPS; i++) {
        bps[i].BreakPointHandle = i + 1;
    }

    RestoreBreakPointEx( MAX_KD_BPS, bps );

    return;
}

void
AddQueue(
    DWORD   dwType,
    DWORD   dwProcessId,
    DWORD   dwThreadId,
    DWORD64 dwData,
    DWORD   dwLen
    )
{
    LPCQUEUE lpcq;


    EnterCriticalSection(&csContinueQueue);

    lpcq = lpcqFree;
    assert(lpcq);

    lpcqFree = lpcq->next;

    lpcq->next = NULL;
    if (lpcqLast) {
        lpcqLast->next = lpcq;
    }
    lpcqLast = lpcq;

    if (!lpcqFirst) {
        lpcqFirst = lpcq;
    }

    lpcq->pid  = dwProcessId;
    lpcq->tid  = dwThreadId;
    lpcq->typ  = dwType;
    lpcq->len  = dwLen;

    if (lpcq->typ == QT_RELOAD_MODULES || lpcq->typ == QT_DEBUGSTRING) {
        if (dwLen) {
            lpcq->data = (UINT_PTR) MHAlloc( dwLen );
            memcpy( (LPVOID)lpcq->data, (LPVOID)dwData, dwLen );
        }
        else {
            lpcq->data = 0;
        }

    }
    else {
        lpcq->data = dwData;
    }

    if (lpcq->typ == QT_CONTINUE_DEBUG_EVENT) {
        SetEvent( hEventContinue );
    }

    LeaveCriticalSection(&csContinueQueue);
    return;
}


BOOL
DequeueOneEvent(
    LPCQUEUE lpcqReturn
    )
{
    LPCQUEUE           lpcq;

    EnterCriticalSection(&csContinueQueue);

    if (!lpcqFirst) {
        LeaveCriticalSection(&csContinueQueue);
        return FALSE;
    }

    lpcq = lpcqFirst;

    lpcqFirst = lpcq->next;
    if (lpcqFirst == NULL) {
        lpcqLast = NULL;
    }

    lpcq->next = lpcqFree;
    lpcqFree   = lpcq;

    if (lpcq->pid == 0 || lpcq->tid == 0) {
        lpcq->pid = KD_PROCESSID;
        lpcq->tid = KD_THREADID;
    }

    *lpcqReturn = *lpcq;

    LeaveCriticalSection(&csContinueQueue);

    return TRUE;
}

#if DBG
LPSTR QtNames[] = {
"*** QT_INVALID_EVENT",
"QT_CONTINUE_DEBUG_EVENT",
"QT_RELOAD_MODULES",
"QT_TRACE_DEBUG_EVENT",
"QT_REBOOT",
"QT_RESYNC",
"QT_DEBUGSTRING",
"QT_CRASH"
};
#endif

BOOL
DequeueAllEvents(
    BOOL fForce,       // force a dequeue even if the dm isn't initialized
    BOOL fConsume      // delete all events from the queue with no action
    )
{
    CQUEUE             qitem;
    LPCQUEUE           lpcq = &qitem;
    BOOL               fDid = FALSE;
    HTHDX              hthd;
    DBGKD_CONTROL_SET  cs = {0};
    LPSTR              d;


    ResetEvent(hEventContinue);

    while ( DequeueOneEvent(&qitem) ) {

        if (fConsume) {
#if DBG
            //DMPrintShellMsg("DequeueAllEvents: consuming %s\n", QtNames[lpcq->typ]);
#endif

            switch (lpcq->typ) {

                case QT_CONTINUE_DEBUG_EVENT:
                    fDid = TRUE;
                    continue;

                case QT_TRACE_DEBUG_EVENT:
                case QT_REBOOT:
                case QT_CRASH:
                case QT_RESYNC:
                    continue;

                case QT_DEBUGSTRING:
                    // don't discard debug strings
                    break;

                case QT_RELOAD_MODULES:
                    // don't discard reloads
                    break;
            }

        }

        hthd = HTHDXFromPIDTID(lpcq->pid, lpcq->tid);
        if (hthd && hthd->fContextDirty) {
            DbgSetThreadContext( hthd, &hthd->context );
            hthd->fContextDirty = FALSE;
        }

        d = (LPSTR)lpcq->data;

        switch (lpcq->typ) {
            case QT_CONTINUE_DEBUG_EVENT:
                if (fCrashDump) {
                    break;
                }
                if (DmKdState >= S_READY || fForce) {
                    if (!fDid) {
                        fDid = TRUE;
                        ContinueTargetSystem( PtrToUlong(d), NULL );
                    }
                }
                break;

            case QT_TRACE_DEBUG_EVENT:
                if (fCrashDump) {
                    break;
                }
                if (DmKdState >= S_READY || fForce) {
                    if (!fDid) {
                        fDid = TRUE;
#ifdef TARGET_i386
                        cs.TraceFlag = 1;
                        cs.Dr7 = sc.ControlReport.Dr7;
                        cs.CurrentSymbolStart = 1;
                        cs.CurrentSymbolEnd = 1;
                        ContinueTargetSystem( PtrToUlong(d), &cs );
#else
                        ContinueTargetSystem( PtrToUlong(d), NULL );
#endif
                    }
                }
                break;

            case QT_RELOAD_MODULES:
                ReloadModules( hthd, d );
                MHFree( (LPVOID)d );
                break;

            case QT_REBOOT:
                if (fCrashDump) {
                    break;
                }
                DMPrintShellMsg( "Target system rebooting...\n" );
                DmKdPurgeCachedVirtualMemory( TRUE );
                UnloadAllModules();
                ZeroMemory( ContextCache, sizeof(ContextCache) );
                DmKdState = S_REBOOTED;
                // this is equivalent to a continue, so acquire the lock
                TakeApiLock();
                DmKdReboot();
                InitialBreak = (BOOL) KdOptions[KDO_INITIALBP].value;
                KdResync = TRUE;
                break;

            case QT_CRASH:
                if (fCrashDump) {
                    break;
                }
                DMPrintShellMsg( "Target system crashing...\n" );
                DmKdCrash( PtrToUlong(d) );
                InitialBreak = (BOOL) KdOptions[KDO_INITIALBP].value;
                KdResync = TRUE;
                fDid = TRUE;
                break;

            case QT_RESYNC:
                if (fCrashDump) {
                    break;
                }
                DMPrintShellMsg( "Host and target systems resynchronizing...\n" );
                KdResync = TRUE;
                break;

            case QT_DEBUGSTRING:
                DMPrintShellMsg( "%s", (LPSTR)d );
                MHFree( (LPVOID)d );
                break;

        }

    }

    return fDid;
}

void
InitEventQueue(
    void
    )
{
    int n;
    int i;
    //
    // initialize the queue variables
    //
    InitializeCriticalSection(&csContinueQueue);

    n = sizeof(cqueue) / sizeof(CQUEUE);
    for (i = 0; i < n-1; i++) {
        cqueue[i].next = &cqueue[i+1];
    }
    cqueue[n-1].next = NULL;
    lpcqFree = &cqueue[0];
    lpcqFirst = NULL;
    lpcqLast = NULL;
}

VOID
WriteKernBase(
    DWORD64 KernBase
    )
{
#if 0
    HKEY  hKeyKd;


    if ( RegOpenKey( HKEY_CURRENT_USER,
                     "software\\microsoft\\windbg\\0021\\programs\\ntoskrnl",
                     &hKeyKd ) == ERROR_SUCCESS ) {
        RegSetValueEx( hKeyKd, "KernBase", 0, REG_DWORD, (LPBYTE)&KernBase, sizeof(DWORD) );
        RegCloseKey( hKeyKd );
    }
#endif
    return;
}

DWORD64
ReadKernBase(
    VOID
    )
{
#if 0
    HKEY   hKeyKd;
    DWORD  dwType;
    DWORD  KernBase;
    DWORD  dwSize;


    if ( RegOpenKey( HKEY_CURRENT_USER,
                     "software\\microsoft\\windbg\\0021\\programs\\ntoskrnl",
                     &hKeyKd ) == ERROR_SUCCESS ) {
        dwSize = sizeof(DWORD);
        RegQueryValueEx( hKeyKd, "KernBase", NULL, &dwType, (LPBYTE)&KernBase, &dwSize );
        RegCloseKey( hKeyKd );
        return KernBase;
    }

    return KernBase;
#endif
    return 0;
}

DbgGetVersion(
    PDBGKD_GET_VERSION64 GetVersion
    )
/*++

Routine Description:

    Load the DBGKD_GET_VERSION packet.  If not debugging a crashdump, call
    DmKdGetVersion to do the work.  If it is a crashdump, conjure up the values.

Arguments:

    GetVersion -

Return Value:



--*/
{
    ULONG64 Addr;


    if (!fCrashDump) {
        return DmKdGetVersion(GetVersion);
    }

    //
    // the current build number
    //
    GetVersion->MinorVersion = (short)_DmpHeader->MinorVersion;
    GetVersion->MajorVersion = (short)_DmpHeader->MajorVersion;

#if defined(TARGET_i386)
    if (vs.MinorVersion < CONTEXT_SIZE_NT5_VERSION) {
        ContextSize = CONTEXT_SIZE_PRE_NT5;
    } else {
        ContextSize = sizeof(CONTEXT);
    }
#else
    ContextSize = sizeof(CONTEXT);
#endif

    //
    // kd protocol version number.  this should be incremented if the
    // protocol changes.
    //
    GetVersion->ProtocolVersion = 5;
    GetVersion->Flags = DBGKD_VERS_FLAG_DATA;

    //
    // This is wrong - what we really want to know is whether it is
    // a UP or MP kernel.
    //
    if (DUMPHEADER(NumberProcessors) > 1) {
        GetVersion->Flags |= DBGKD_VERS_FLAG_MP;
    }

#if defined(TARGET_i386)
    GetVersion->MachineType = IMAGE_FILE_MACHINE_I386;
#elif defined(TARGET_ALPHA)
    GetVersion->MachineType = IMAGE_FILE_MACHINE_ALPHA;
#elif defined(TARGET_AXP64)
    GetVersion->MachineType = IMAGE_FILE_MACHINE_ALPHA64;
#elif defined(TARGET_IA64)
    GetVersion->MachineType = IMAGE_FILE_MACHINE_IA64;
#else
#error( "unknown target machine" );
#endif

    //
    // address of the loader table
    //
    GetVersion->PsLoadedModuleList = SEPtrTo64(_DmpHeader->PsLoadedModuleList);

    //
    // If the debugger is being initialized during boot, PsNtosImageBase
    // and PsLoadedModuleList are not yet valid.  KdInitSystem got
    // the image base from the loader block.
    // On the other hand, if the debugger was initialized by a bugcheck,
    // it didn't get a loader block to look at, but the system was
    // running so the other variables are valid.
    //

    if ((Addr = GetSymbolAddress( "nt!KdpNtosImageBase" )) != 0 ||
        (Addr = GetSymbolAddress( "nt!KdpPsNtosImageBase" )) != 0) {

        if (DmKdPtr64) {
            DmpReadMemory( Addr, &GetVersion->KernBase, sizeof(GetVersion->KernBase) );
            // Better be sign extended.
            assert(Is64PtrSE(GetVersion->KernBase));
        } else {
            DmpReadMemory( Addr, &GetVersion->KernBase, sizeof(ULONG) );
            // Sign extend what we just read.
            GetVersion->KernBase = SEPtrTo64(GetVersion->KernBase);
        }
    }

    //
    // These values are not in the version packet in modern versions of
    // the system.  When debugging NT4 dumps, GetDebuggerDataBlock needs to
    // fill in these values.
    //

    //GetVersion->ThCallbackStack = FIELD_OFFSET(KTHREAD, CallbackStack);
    //GetVersion->NextCallback = FIELD_OFFSET(KCALLOUT_FRAME, CbStk);
#if defined(TARGET_i386)
    //GetVersion->FramePointer = FIELD_OFFSET(KCALLOUT_FRAME, Ebp);
#endif

    //GetVersion->BreakpointWithStatus = GetSymbolAddress("nt!RtlpBreakWithStatusInstruction");
    //GetVersion->KiCallUserMode = GetSymbolAddress("nt!KiCallUserMode");
    //GetVersion->KeUserCallbackDispatcher = GetSymbolAddress("nt!KeUserCallbackDispatcher");

    GetVersion->DebuggerDataList = GetSymbolAddress( "nt!KdpDebuggerDataListHead" );

    return 0;

}

VOID
GetVersionInfo(
    DWORD64 KernBase
    )
{
    CHAR buf[MAX_PATH];

    ZeroMemory( &vs, sizeof(vs) );
    if (DbgGetVersion( &vs ) == STATUS_SUCCESS) {
        if (!vs.KernBase) {
            vs.KernBase = KernBase;
        }
    }

    sprintf( buf, "Kernel Version %d", vs.MinorVersion  );
    if (vs.MajorVersion == 0xC) {
        _tcscat( buf, " Checked" );
    } else if (vs.MajorVersion == 0xF) {
        _tcscat( buf, " Free" );
    }
    sprintf( &buf[_tcslen(buf)], " loaded @ %016I64x", vs.KernBase  );

    DMPrintShellMsg( "%s\n", buf );

    return;
}

VOID
InitializeExtraProcessors(
    VOID
    )
{
    HTHDX               hthd;
    DWORD               i;
    DEBUG_EVENT64       de;


    CacheProcessors = sc.NumberProcessors;
    for (i = 1; i < sc.NumberProcessors; i++) {
        //
        // initialize the hthd
        //
        hthd = HTHDXFromPIDTID( KD_PROCESSID, i );

        //
        // refresh the context cache for this processor
        //
#if defined(TARGET_i386) || defined(TARGET_IA64)
        ContextCache[i].fSContextStale = TRUE;
        ContextCache[i].fSContextDirty = FALSE;
#endif
        ContextCache[i].fContextDirty = FALSE;
        ContextCache[i].fContextStale = TRUE;

        //
        // tell debugger to create the thread (processor)
        //
        de.dwDebugEventCode = CREATE_THREAD_DEBUG_EVENT;
        de.dwProcessId = KD_PROCESSID;
        de.dwThreadId  = i + 1;
        de.u.CreateThread.hThread = (HANDLE)(i + 1);
        de.u.CreateThread.lpThreadLocalBase = 0;
        de.u.CreateThread.lpStartAddress = 0;
        ProcessDebugEvent(&de, &sc);
        WaitForSingleObject(hEventContinue,INFINITE);
    }



    //
    // consume any continues that may have been queued
    //
    ConsumeAllEvents();

    //
    // get out of here
    //
    return;
}

DWORD
DmKdPollThread(
    LPSTR lpProgName
    )
{
    char                        buf[1024] = {0};
    DWORD                       st;
    DWORD                       i;
    DWORD                       j;
    BOOL                        fFirstSc = FALSE;
    DEBUG_EVENT64               de;
    char                        fname[_MAX_FNAME];
    char                        ext[_MAX_EXT];
    HTHDX                       hthd;
    DWORD                       n;
    IMAGEINFO                   ii;
    HPRCX                       hprc;


    PollThreadId = GetCurrentThreadId();


    TakeApiLock();

    DmKdSetMaxCacheSize( (ULONG)KdOptions[KDO_CACHE].value );
    InitialBreak = (BOOL) KdOptions[KDO_INITIALBP].value;

    InitializeListHead(&DebugDataCacheList);

    //
    // simulate a create process debug event
    //
    de.dwDebugEventCode = CREATE_PROCESS_DEBUG_EVENT;
    de.dwProcessId = KD_PROCESSID;
    de.dwThreadId  = KD_THREADID;
    de.u.CreateProcessInfo.hFile = NULL;
    de.u.CreateProcessInfo.hProcess = NULL;
    de.u.CreateProcessInfo.hThread = NULL;
    de.u.CreateProcessInfo.lpBaseOfImage = 0;
    de.u.CreateProcessInfo.dwDebugInfoFileOffset = 0;
    de.u.CreateProcessInfo.nDebugInfoSize = 0;
    de.u.CreateProcessInfo.lpStartAddress = 0;
    de.u.CreateProcessInfo.lpThreadLocalBase = 0;
    de.u.CreateProcessInfo.lpImageName = (UINT_PTR)lpProgName;
    de.u.CreateProcessInfo.fUnicode = 0;
    de.u.LoadDll.nDebugInfoSize = 0;
    ProcessDebugEvent(&de, &sc);
    WaitForSingleObject(hEventContinue,INFINITE);
    hprc = HPRCFromPID( KD_PROCESSID );
    ConsumeAllEvents();

    //
    // simulate a loader breakpoint event
    //
    de.dwDebugEventCode = BREAKPOINT_DEBUG_EVENT;
    de.dwProcessId = KD_PROCESSID;
    de.dwThreadId  = KD_THREADID;
    de.u.Exception.dwFirstChance = TRUE;
    de.u.Exception.ExceptionRecord.ExceptionCode = EXCEPTION_BREAKPOINT;
    de.u.Exception.ExceptionRecord.ExceptionFlags = 0;
    de.u.Exception.ExceptionRecord.ExceptionRecord = 0;
    de.u.Exception.ExceptionRecord.ExceptionAddress = 0;
    de.u.Exception.ExceptionRecord.NumberParameters = 0;
    ProcessDebugEvent( &de, &sc );
    ConsumeAllEvents();

    DMPrintShellMsg( "Kernel debugger waiting to connect on com%d @ %d baud\n",
                     KdOptions[KDO_PORT].value,
                     KdOptions[KDO_BAUDRATE].value
                   );

    setjmp( JumpBuffer );

    //
    // The first time thru we release the lock acquired and the top of the
    // function.
    //
    // Subsent passes thru here will release the lock acquired in the beginning 
    // of the while loop, but "jumped" out of thru a longjump in the DmKdWaitStateChange
    //
    ReleaseApiLock();

    while (TRUE) {

        if (DmKdExit) {
            goto cleanup;
        }

        TakeApiLock();

        st = DmKdWaitStateChange( &sc, buf, sizeof(buf) );

        LastStopTime = GetTickCount();

        if (st != STATUS_SUCCESS ) {
            DEBUG_PRINT_1( "DmKdWaitStateChange failed: %08lx\n", st );
            goto cleanup;
        }

        fFirstSc = FALSE;

        if (sc.NewState == DbgKdLoadSymbolsStateChange) {
            _splitpath( buf, NULL, NULL, fname, ext );
            _makepath( buf, NULL, NULL, fname, ext );
            if ((DmKdState == S_UNINITIALIZED) &&
                (_stricmp( buf, KERNEL_IMAGE_NAME ) == 0)) {
                WriteKernBase( sc.u.LoadSymbols.BaseOfDll );
                fFirstSc = TRUE;
            }
        }

        if ((DmKdState == S_UNINITIALIZED) ||
            (DmKdState == S_REBOOTED)) {
            hthd = HTHDXFromPIDTID( KD_PROCESSID, KD_THREADID );
            ContextCache[sc.Processor].fContextStale = TRUE;
            DbgGetThreadContext( hthd, &sc.Context );
#if defined(TARGET_i386) || defined(TARGET_IA64)
            ContextCache[sc.Processor].fSContextStale = TRUE;
#endif
        } else if (sc.NewState != DbgKdLoadSymbolsStateChange) {
#if defined(TARGET_i386) || defined(TARGET_IA64)
            ContextCache[sc.Processor].fSContextStale = TRUE;
#endif

            //
            // put the context record into the cache
            //
            memcpy( &ContextCache[sc.Processor].Context,
                    &sc.Context,
                    sizeof(sc.Context)
                  );

        }

        ContextCache[sc.Processor].fContextDirty = FALSE;
        ContextCache[sc.Processor].fContextStale = FALSE;

        //ReleaseApiLock();

        if (sc.NumberProcessors > 1 && CacheProcessors == 1) {
            InitializeExtraProcessors();
        }

        if (DmKdState == S_REBOOTED) {

            DmKdState = S_INITIALIZED;

            //
            // get the version/info packet from the target
            //
            if (fFirstSc) {
                GetVersionInfo( sc.u.LoadSymbols.BaseOfDll );
            } else {
                GetVersionInfo( 0 );
            }

            InitialBreak = (BOOL) KdOptions[KDO_INITIALBP].value;

        } else
        if (DmKdState == S_UNINITIALIZED) {

            DMPrintShellMsg( "Kernel Debugger connection established on com%d @ %d baud\n",
                             KdOptions[KDO_PORT].value,
                             KdOptions[KDO_BAUDRATE].value
                           );

            //
            // we're now initialized
            //
            DmKdState = S_INITIALIZED;

            //
            // get the version/info packet from the target
            //
            if (fFirstSc) {
                GetVersionInfo( sc.u.LoadSymbols.BaseOfDll );
            } else {
                GetVersionInfo( 0 );
            }

            //
            // clean out the kernel's bp list
            //
            ClearBps();

            if (sc.NewState != DbgKdLoadSymbolsStateChange) {
                //
                // generate a mod load for the kernel/osloader
                //
                GenerateKernelModLoad( hprc, lpProgName );
            }

            DisableEmCache();
        }

        if (fDisconnected) {
            if (sc.NewState == DbgKdLoadSymbolsStateChange) {

                //
                // we can process these debug events very carefully
                // while disconnected from the shell.  the only requirement
                // is that the dm doesn't call NotifyEM while disconnected.
                //

            } else {

                WaitForSingleObject( hEventRemoteQuit, INFINITE );
                ResetEvent( hEventRemoteQuit );

            }
        }

        if (sc.NewState == DbgKdExceptionStateChange) {
            DmKdInitVirtualCacheEntry( sc.ProgramCounter,
                                       (ULONG)sc.ControlReport.InstructionCount,
                                       sc.ControlReport.InstructionStream,
                                       TRUE
                                     );

            de.dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
            de.dwProcessId = KD_PROCESSID;
            de.dwThreadId  = KD_THREADID;
            de.u.Exception.ExceptionRecord = sc.u.Exception.ExceptionRecord;
            de.u.Exception.dwFirstChance = sc.u.Exception.FirstChance;

            //
            // HACK-HACK: this is here to handle the case where
            // the kernel delivers an exception during initialization
            // that is NOT a breakpoint exception.
            //
            if (DmKdState != S_READY) {
                de.u.Exception.ExceptionRecord.ExceptionCode = EXCEPTION_BREAKPOINT;
            }

            if (fDisconnected) {
                ReConnectDebugger( &de, DmKdState == S_INITIALIZED );
            }

            ProcessDebugEvent( &de, &sc );

            if (DmKdState == S_INITIALIZED) {
                DmKdState = S_READY;
            }
        }
        else
        if (sc.NewState == DbgKdLoadSymbolsStateChange) {
            if (sc.u.LoadSymbols.UnloadSymbols) {
                if (sc.u.LoadSymbols.PathNameLength == 0 &&
                    sc.u.LoadSymbols.BaseOfDll == (ULONG64)-1 &&
                    sc.u.LoadSymbols.ProcessId == 0
                   ) {
                    //
                    // the target system was just restarted
                    //
                    DMPrintShellMsg( "Target system restarted...\n" );
                    DmKdPurgeCachedVirtualMemory( TRUE );
                    UnloadAllModules();
                    ContinueTargetSystem( DBG_CONTINUE, NULL );
                    InitialBreak = (BOOL) KdOptions[KDO_INITIALBP].value;
                    KdResync = TRUE;
                    DmKdState = S_REBOOTED;
                    ReleaseApiLock();
                    continue;
                }
                de.dwDebugEventCode      = UNLOAD_DLL_DEBUG_EVENT;
                de.dwProcessId           = KD_PROCESSID;
                de.dwThreadId            = KD_THREADID;
                de.u.UnloadDll.lpBaseOfDll = sc.u.LoadSymbols.BaseOfDll;
                
                assert( Is64PtrSE(sc.u.LoadSymbols.BaseOfDll) );

                if (fDisconnected) {
                    ReConnectDebugger( &de, DmKdState == S_INITIALIZED );
                }
                
                ProcessDebugEvent( &de, &sc );
                ConsumeAllEvents();
                ContinueTargetSystem( DBG_CONTINUE, NULL );
                ReleaseApiLock();
                continue;
            } else {
                //
                // if the mod load is for the kernel image then we must
                // assume that the target system was rebooted while
                // the debugger was connected.  in this case we need to
                // unload all modules.  this will allow the mod loads that
                // are forthcoming to work correctly and cause the shell to
                // reinstanciate all of it's breakpoints.
                //
                if (_tcsicmp( buf, KERNEL_IMAGE_NAME ) == 0) {
                    UnloadAllModules();
                    DeleteAllBps();
                    ConsumeAllEvents();
                }

                assert( Is64PtrSE(sc.u.LoadSymbols.BaseOfDll) );

                de.dwDebugEventCode                 = LOAD_DLL_DEBUG_EVENT;
                de.dwProcessId                      = KD_PROCESSID;
                de.dwThreadId                       = KD_THREADID;
                de.u.LoadDll.hFile                  = (HANDLE)sc.u.LoadSymbols.CheckSum;
                de.u.LoadDll.lpBaseOfDll            = sc.u.LoadSymbols.BaseOfDll;
                de.u.LoadDll.lpImageName            = (UINT_PTR)buf;
                de.u.LoadDll.fUnicode               = FALSE;
                de.u.LoadDll.nDebugInfoSize         = 0;
                if (sc.u.LoadSymbols.SizeOfImage == 0) {
                    //
                    // this is likely a firmware image.  in such cases the boot
                    // loader on the target may not be able to deliver the size.
                    //
                    if (!ReadImageInfo(
                            buf,
                            NULL,
                            (LPSTR)KdOptions[KDO_SYMBOLPATH].value,
                            &ii )) {
                        //
                        // can't read the image correctly
                        //
                        DMPrintShellMsg( "Module load failed, missing size & image [%s]\n", buf );
                        ContinueTargetSystem( DBG_CONTINUE, NULL );
                        ReleaseApiLock();
                        continue;
                    }
                    de.u.LoadDll.dwDebugInfoFileOffset  = ii.SizeOfImage;
                } else {
                    de.u.LoadDll.dwDebugInfoFileOffset  = sc.u.LoadSymbols.SizeOfImage;
                }

                if (fDisconnected) {
                    ReConnectDebugger( &de, DmKdState == S_INITIALIZED );
                }

                //
                // HACK ALERT
                //
                // this code is here to allow the presence of the
                // mirrored disk drivers in a system that has crashdump
                // enabled.  if the modload is for a driver and the
                // image name for that driver is already present in the
                // dm's module table then we alias the driver.
                //
                _splitpath( buf, NULL, NULL, fname, ext );
                if (_tcsicmp( ext, ".sys" ) == 0) {
                    UnloadModule( sc.u.LoadSymbols.BaseOfDll, NULL );
                    for (i=0; i<(DWORD)hprc->cDllList; i++) {
                        if (hprc->rgDllList[i].fValidDll &&
                            _tcsicmp(hprc->rgDllList[i].szDllName,buf)==0) {
                            break;
                        }
                    }
                    if (i < (DWORD)hprc->cDllList) {
                        for (j=0; j<MAX_MODULEALIAS; j++) {
                            if (ModuleAlias[j].ModuleName[0] == 0) {
                                break;
                            }
                        }
                        if (j < MAX_MODULEALIAS) {
                            _tcscpy( ModuleAlias[j].Alias, buf );
                            ModuleAlias[j].ModuleName[0] = 'c';
                            _splitpath( buf, NULL, NULL, &ModuleAlias[j].ModuleName[1], NULL );
                            ModuleAlias[j].ModuleName[8] = 0;
                            ModuleAlias[j].Special = 2;     // One shot alias...
                        }
                    }
                } else {
                    UnloadModule( sc.u.LoadSymbols.BaseOfDll, buf );
                }

                ProcessDebugEvent( &de, &sc );
                if (DmKdState == S_INITIALIZED) {                    
                    DmKdState = S_READY;
                }

            }
        }

        if (DequeueAllEvents(FALSE,FALSE)) {
            ReleaseApiLock();
            continue;
        }

        ReleaseApiLock();

        //
        // this loop is executed while the target system is not running
        // the dm sits here and processes queue events and waits for a go
        //
        while (TRUE) {
            WaitForSingleObject( hEventContinue, 100 );
            ResetEvent( hEventContinue );

            if (WaitForSingleObject( hEventRemoteQuit, 0 ) == WAIT_OBJECT_0) {
                fDisconnected = TRUE;
                DmKdBreakIn = TRUE;
            }

            if (DmKdExit) {
                goto cleanup;
            }
            if (DmKdBreakIn || KdResync) {
                break;
            }
            if (DequeueAllEvents(FALSE,FALSE)) {
                break;
            }
        }
    }

cleanup:
    MHFree( lpProgName );
    return 0;
}


VOID
InitializeKiProcessor(
    VOID
    )
{
    if (!fCrashDump) {
        return;
    }

    //
    // get the address of the KiProcessorBlock
    //
    if (!KiProcessorBlockAddr) {
        DMPrintShellMsg( "Could not get address of KiProcessorBlock\n" );
    } else {

        //
        // read the contents of the KiProcessorBlock
        //
        DmpReadMemory( KiProcessorBlockAddr, &KiProcessors, sizeof(KiProcessors) );

    }
}


DWORD
DmKdPollThreadCrash(
    LPSTR lpProgName
    )
{
    DWORD                       i;
    BOOL                        fFirstSc = FALSE;
    DEBUG_EVENT64               de;
    DWORD                       n;
    EXCEPTION_RECORD64          Exception64;
    PEXCEPTION_RECORD           Exception;
    LIST_ENTRY64                List;
    ULONG64                     Next;
    ULONG64                     DataTable;
    LDR_DATA_TABLE_ENTRY64      DataTableBuffer;
    INT                         CurrProcessor;
    HPRCX                       hprc;
    CRASHDUMP_VERSION_INFO      VersionInfo;



    PollThreadId = GetCurrentThreadId();

    hprc = HPRCFromPID( KD_PROCESSID );

    DmKdExit = FALSE;

    //
    // initialize the queue variables
    //
    n = sizeof(cqueue) / sizeof(CQUEUE);
    for (i = 0; i < n-1; i++) {
        cqueue[i].next = &cqueue[i+1];
    }
    --n;
    cqueue[n].next = NULL;
    lpcqFree = &cqueue[0];
    lpcqFirst = NULL;
    lpcqLast = NULL;
    InitializeCriticalSection(&csContinueQueue);

    InitializeListHead(&DebugDataCacheList);

    DmKdSetMaxCacheSize( (ULONG)KdOptions[KDO_CACHE].value );
    InitialBreak = FALSE;

    //
    // initialize for crash debugging
    //
    if (!DmpInitialize( (LPSTR)KdOptions[KDO_CRASHDUMP].value,
                         &DmpContext,
                         &Exception,
                         &_DmpHeader
                       )) {
        DMPrintShellMsg( "Could not initialize crash dump file %s\n",
                         (LPSTR)KdOptions[KDO_CRASHDUMP].value );
        return 0;
    }

    //
    // fix up version bugs
    //
    DmpDetectVersionParameters( &VersionInfo );
    DmKdPtr64 = (VersionInfo.PointerSize == 64);

    if (DmKdPtr64) {
        Exception64 = *(PEXCEPTION_RECORD64)Exception;
    } else {
        ExceptionRecord32To64((PEXCEPTION_RECORD32)Exception, &Exception64);
    }

    vs.MajorVersion                     = (USHORT)DUMPHEADER(MajorVersion);
    vs.MinorVersion                     = (USHORT)DUMPHEADER(MinorVersion);
    vs.KernBase                         = 0;

#if defined(TARGET_i386)
    if (vs.MinorVersion < CONTEXT_SIZE_NT5_VERSION) {
        ContextSize = CONTEXT_SIZE_PRE_NT5;
    } else {
        ContextSize = sizeof(CONTEXT);
    }
#else
    ContextSize = sizeof(CONTEXT);
#endif

    memcpy( &sc.Context, DmpContext, ContextSize );
    memcpy( &ContextCache[0].Context, DmpContext, ContextSize );


    sc.NewState                         = DbgKdExceptionStateChange;
    sc.u.Exception.ExceptionRecord      = Exception64;
    sc.u.Exception.FirstChance          = FALSE;
    //
    // For the createprocess and loader bp, use cpu 0
    //
    CurrProcessor                       = 0;
    sc.Processor                        = 0;
    sc.NumberProcessors                 = DUMPHEADER(NumberProcessors);
    sc.ProgramCounter                   = Exception64.ExceptionAddress;
    sc.ControlReport.InstructionCount   = 0;


    //
    // Set Context flags
    //
    ContextCache[0].fContextDirty  = FALSE;
    ContextCache[0].fContextStale  = FALSE;

#if defined(TARGET_i386) || defined(TARGET_IA64)
    ContextCache[0].fSContextDirty = FALSE;
    ContextCache[0].fSContextStale = TRUE;
#endif

    vs.PsLoadedModuleList = (ULONG64)DUMPHEADER(PsLoadedModuleList);

    if (NT_SUCCESS(DmKdReadListEntry( vs.PsLoadedModuleList, &List ))) {
        Next = List.Flink;
        if (DmKdPtr64) {
            DataTable = CONTAINING_RECORD64( Next, LDR_DATA_TABLE_ENTRY64, InLoadOrderLinks );
        } else {
            DataTable = SE32To64( CONTAINING_RECORD32( Next, LDR_DATA_TABLE_ENTRY32, InLoadOrderLinks ));
        }
        if (NT_SUCCESS(DmKdReadLoaderEntry( DataTable, (PVOID)&DataTableBuffer ))) {
            vs.KernBase = DataTableBuffer.DllBase;
        }
    } else {
        DMPrintShellMsg( "Could not get base of kernel %08I64x\n",
                         vs.PsLoadedModuleList );
    }


#if defined(TARGET_i386)
    if ( DUMPHEADER(MachineImageType) != IMAGE_FILE_MACHINE_I386)
#elif defined(TARGET_ALPHA)
    if ( DUMPHEADER(MachineImageType) != IMAGE_FILE_MACHINE_ALPHA)
#elif defined(TARGET_AXP64)
    if ( DUMPHEADER(MachineImageType) != IMAGE_FILE_MACHINE_ALPHA64)
#elif defined(TARGET_IA64)
    if ( DUMPHEADER(MachineImageType) != IMAGE_FILE_MACHINE_IA64)
#else
#pragma error( "unknown target machine" );
#endif
    {
        DMPrintShellMsg( "Dumpfile is of an unknown machine type\n" );
    }

    //
    // simulate a create process debug event
    //
    de.dwDebugEventCode = CREATE_PROCESS_DEBUG_EVENT;
    de.dwProcessId = KD_PROCESSID;
    de.dwThreadId  = KD_THREADID;
    de.u.CreateProcessInfo.hFile = NULL;
    de.u.CreateProcessInfo.hProcess = NULL;
    de.u.CreateProcessInfo.hThread = NULL;
    de.u.CreateProcessInfo.lpBaseOfImage = 0;
    de.u.CreateProcessInfo.dwDebugInfoFileOffset = 0;
    de.u.CreateProcessInfo.nDebugInfoSize = 0;
    de.u.CreateProcessInfo.lpStartAddress = 0;
    de.u.CreateProcessInfo.lpThreadLocalBase = 0;
    de.u.CreateProcessInfo.lpImageName = (UINT_PTR)lpProgName;
    de.u.CreateProcessInfo.fUnicode = 0;
    ProcessDebugEvent(&de, &sc);
    WaitForSingleObject(hEventContinue,INFINITE);
    ConsumeAllEvents();

    //
    // LoadDll needs this to load the right kernel symbols:
    //
    CacheProcessors = DUMPHEADER(NumberProcessors);

    //
    // generate a mod load for the kernel/osloader
    //

    GenerateKernelModLoad( hprc, lpProgName );

    CurrProcessor                       = DmpGetCurrentProcessor();
    if (CurrProcessor == -1) {
        sc.Processor                    = 0;
    } else {
        sc.Processor                    = (USHORT)CurrProcessor;
    }

    //
    // initialize the other processors
    //
    InitializeKiProcessor();
    if (DUMPHEADER(NumberProcessors) > 1) {
        InitializeExtraProcessors();
    }

    //
    // simulate a loader breakpoint event
    //
    de.dwDebugEventCode = BREAKPOINT_DEBUG_EVENT;
    de.dwProcessId = KD_PROCESSID;
    de.dwThreadId  = KD_THREADID;
    de.u.Exception.dwFirstChance = TRUE;
    de.u.Exception.ExceptionRecord.ExceptionCode = EXCEPTION_BREAKPOINT;
    de.u.Exception.ExceptionRecord.ExceptionFlags = 0;
    de.u.Exception.ExceptionRecord.ExceptionRecord = 0;
    de.u.Exception.ExceptionRecord.ExceptionAddress = 0;
    de.u.Exception.ExceptionRecord.NumberParameters = 0;
    ProcessDebugEvent( &de, &sc );
    ConsumeAllEvents();

    DMPrintShellMsg( "Kernel Debugger connection established for %s\n",
                     (LPSTR)KdOptions[KDO_CRASHDUMP].value
                   );

    //
    // get the version/info packet from the target
    //

    if (DbgKdLoadSymbolsStateChange == sc.NewState) {
        // We have a new module address, let's process it.
        GetVersionInfo( (DWORD)sc.u.LoadSymbols.BaseOfDll );
    } else {
        // 'sc' contains exception data, not symbol information.
        // BUGBUG - Why do we even do this here? To force the symbol loads?
        //
        // Whatever value we have is probably a good guess anyway. In most
        // cases, the value of "vs.KernBase" is usually correct by the time
        // it gets here. So just pass it in.
        GetVersionInfo( vs.KernBase );
    }

    DMPrintShellMsg( "Bugcheck %08x : %08x %08x %08x %08x\n",
                     DUMPHEADER(BugCheckCode),
                     DUMPHEADER(BugCheckParameter1),
                     DUMPHEADER(BugCheckParameter2),
                     DUMPHEADER(BugCheckParameter3),
                     DUMPHEADER(BugCheckParameter4) );


    DisableEmCache();

    DmKdInitVirtualCacheEntry( sc.ProgramCounter,
                               (ULONG)sc.ControlReport.InstructionCount,
                               sc.ControlReport.InstructionStream,
                               TRUE
                             );

    de.dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
    de.dwProcessId = KD_PROCESSID;
    de.dwThreadId  = KD_THREADID;
    de.u.Exception.ExceptionRecord = sc.u.Exception.ExceptionRecord;
    de.u.Exception.dwFirstChance = sc.u.Exception.FirstChance;

    ProcessDebugEvent( &de, &sc );

    MHFree( lpProgName );

    while (!DmKdExit) {
        DequeueAllEvents(FALSE,FALSE);
        Sleep( 1000 );
    }

    return 0;
}

BOOL
DmKdConnectAndInitialize(
    LPSTR lpProgName
    )
{
    DWORD      dwThreadId;
    LPSTR      szProgName = MHAlloc( MAX_PATH );


    //
    // bail out if we're already initialized
    //
    if (DmKdState != S_UNINITIALIZED) {
        return TRUE;
    }


    szProgName[0] = '\0';
    if (lpProgName) {
        _tcscpy( szProgName, lpProgName );
    }

    fCrashDump = (BOOL) (KdOptions[KDO_CRASHDUMP].value != 0);

    if (fCrashDump) {
    
        hThreadDmPoll = CreateThread( NULL,
                                      16000,
                                      (LPTHREAD_START_ROUTINE)DmKdPollThreadCrash,
                                      (LPVOID)szProgName,
                                      THREAD_SET_INFORMATION,
                                      (LPDWORD)&dwThreadId
                                    );
    } else {

        //
        // initialize the com port
        //

        if (!DmKdInitComPort( (BOOL) KdOptions[KDO_USEMODEM].value )) {
            DMPrintShellMsg( "Could not initialize COM%d @ %d baud, error == 0x%x\n",
                             KdOptions[KDO_PORT].value,
                             KdOptions[KDO_BAUDRATE].value,
                             GetLastError()
                           );
            return FALSE;
        }

        hThreadDmPoll = CreateThread( NULL,
                                      16000,
                                      (LPTHREAD_START_ROUTINE)DmKdPollThread,
                                      (LPVOID)szProgName,
                                      THREAD_SET_INFORMATION,
                                      (LPDWORD)&dwThreadId
                                    );
    }


    if ( hThreadDmPoll == (HANDLE)NULL ) {
        return FALSE;
    }

    if (!SetThreadPriority(hThreadDmPoll, THREAD_PRIORITY_ABOVE_NORMAL)) {
        return FALSE;
    }

    KdResync = TRUE;
    return TRUE;
}

VOID
DmPollTerminate( VOID )
{
    extern HANDLE DmKdComPort;
    extern ULONG  MaxRetries;

    if (hThreadDmPoll) {
        DmKdExit = TRUE;
        MaxRetries = 1;
        WaitForSingleObject(hThreadDmPoll, INFINITE);

        DmKdState = S_UNINITIALIZED;
        DeleteCriticalSection(&csContinueQueue);
        ResetEvent( hEventContinue );
        if (fCrashDump) {
            DmpUnInitialize();
        } else {
            CloseHandle( DmKdComPort );
            MaxRetries = 5;
        }
        DmKdExit = FALSE;
    }

    return;
}

VOID
DisableEmCache( VOID )
{
    LPRTP       rtp;
    HTHDX       hthd;


    hthd = HTHDXFromPIDTID(1, 1);

    rtp = (LPRTP)MHAlloc(FIELD_OFFSET(RTP, rgbVar)+sizeof(DWORD));

    rtp->hpid    = hthd->hprc->hpid;
    rtp->htid    = hthd->htid;
    rtp->dbc     = dbceEnableCache;
    rtp->cb      = sizeof(DWORD);

    *(LPDWORD)rtp->rgbVar = 1;

    DmTlFunc( tlfRequest, rtp->hpid, FIELD_OFFSET(RTP, rgbVar)+rtp->cb, (UINT_PTR)rtp );

    MHFree( rtp );

    return;
}

ULONG64
GetSymbolAddress( 
    LPSTR sym 
    )
{
    extern char abEMReplyBuf[];
    LPRTP       rtp;
    HTHDX       hthd;
    DWORD64     offset;
    BOOL        fUseUnderBar = FALSE;


    __try {

try_underbar:
        hthd = HTHDXFromPIDTID(1, 1);

        rtp = (LPRTP)MHAlloc(FIELD_OFFSET(RTP, rgbVar)+_tcslen(sym)+16);

        rtp->hpid    = hthd->hprc->hpid;
        rtp->htid    = hthd->htid;
        rtp->dbc     = dbceGetOffsetFromSymbol;
        rtp->cb      = _tcslen(sym) + (fUseUnderBar ? 2 : 1);

        if (fUseUnderBar) {
            ((LPSTR)rtp->rgbVar)[0] = '_';
            memcpy( (LPSTR)rtp->rgbVar+1, sym, rtp->cb-1 );
        } else {
            memcpy( rtp->rgbVar, sym, rtp->cb );
        }

        DmTlFunc( tlfRequest, rtp->hpid, FIELD_OFFSET(RTP, rgbVar)+rtp->cb, (UINT_PTR)rtp );

        MHFree( rtp );

        offset = *(PDWORD64)abEMReplyBuf;
        if (!offset && !fUseUnderBar) {
            fUseUnderBar = TRUE;
            goto try_underbar;
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        offset = 0;

    }

    assert(Is64PtrSE(offset));

    return offset;
}

BOOL
UnloadModule(
    DWORD64 BaseOfDll,
    LPSTR   NameOfDll
    )
{
    HPRCX           hprc;
    HTHDX           hthd;
    DEBUG_EVENT64   de;
    DWORD           i;
    BOOL            fUnloaded = FALSE;

    assert( Is64PtrSE(BaseOfDll) );

    hprc = HPRCFromPID( KD_PROCESSID );
    hthd = HTHDXFromPIDTID( KD_PROCESSID, KD_THREADID );

    //
    // first lets look for the image by dll base
    //
    for (i=0; i<(DWORD)hprc->cDllList; i++) {
        if (hprc->rgDllList[i].fValidDll) {

            assert( Is64PtrSE(hprc->rgDllList[i].offBaseOfImage) );

            if (hprc->rgDllList[i].offBaseOfImage == BaseOfDll) {
                de.dwDebugEventCode        = UNLOAD_DLL_DEBUG_EVENT;
                de.dwProcessId             = KD_PROCESSID;
                de.dwThreadId              = KD_THREADID;
                de.u.UnloadDll.lpBaseOfDll = hprc->rgDllList[i].offBaseOfImage;
                NotifyEM( &de, hthd, 0, 0);
                DestroyDllLoadItem(&hprc->rgDllList[i]);
                fUnloaded = TRUE;
                break;
            }
        }
    }

    //
    // now we look by dll name
    //
    if (NameOfDll) {
        for (i=0; i<(DWORD)hprc->cDllList; i++) {
            if (hprc->rgDllList[i].fValidDll) {
                
                assert( Is64PtrSE(hprc->rgDllList[i].offBaseOfImage) );

                if (_tcsicmp(hprc->rgDllList[i].szDllName,NameOfDll)==0) {

                    de.dwDebugEventCode        = UNLOAD_DLL_DEBUG_EVENT;
                    de.dwProcessId             = KD_PROCESSID;
                    de.dwThreadId              = KD_THREADID;
                    de.u.UnloadDll.lpBaseOfDll = hprc->rgDllList[i].offBaseOfImage;
                    NotifyEM( &de, hthd, 0, 0);
                    fUnloaded = TRUE;
                    break;

                }
            }
        }
    }

    return fUnloaded;
}

VOID
UnloadAllModules(
    VOID
    )
{
    HPRCX           hprc;
    HTHDX           hthd;
    DEBUG_EVENT64   de;
    DWORD           i;


    hprc = HPRCFromPID( KD_PROCESSID );
    hthd = HTHDXFromPIDTID( KD_PROCESSID, KD_THREADID );

    for (i=0; i<(DWORD)hprc->cDllList; i++) {
        if (hprc->rgDllList[i].fValidDll) {

            assert( Is64PtrSE(hprc->rgDllList[i].offBaseOfImage) );

            de.dwDebugEventCode        = UNLOAD_DLL_DEBUG_EVENT;
            de.dwProcessId             = KD_PROCESSID;
            de.dwThreadId              = KD_THREADID;
            de.u.UnloadDll.lpBaseOfDll = hprc->rgDllList[i].offBaseOfImage;
            NotifyEM( &de, hthd, 0, 0);
            DestroyDllLoadItem(&hprc->rgDllList[i]);
        }
    }

    return;
}


BOOL
GenerateKernelModLoad(
    HPRCX hprc,
    LPSTR lpProgName
    )
{
    DEBUG_EVENT64               de;
    LIST_ENTRY64                List;
    ULONG64                     DataTable;
    LDR_DATA_TABLE_ENTRY64      DataTableBuffer;
    LPBYTE                      lpbPacket;
    WORD                        cbPacket;
    HTHDX                       hthd;

    hthd = HTHDXFromPIDTID( KD_PROCESSID, 1 );

    if (!NT_SUCCESS(DmKdReadListEntry( vs.PsLoadedModuleList, &List ))) {
        return FALSE;
    }

    if (DmKdPtr64) {
        DataTable = CONTAINING_RECORD64( List.Flink,
                                       LDR_DATA_TABLE_ENTRY64,
                                       InLoadOrderLinks
                                     );
    } else {
        DataTable = SE32To64( CONTAINING_RECORD32( List.Flink,
                                       LDR_DATA_TABLE_ENTRY32,
                                       InLoadOrderLinks
                                     ));
    }

    if (!NT_SUCCESS(DmKdReadLoaderEntry(DataTable, &DataTableBuffer ))) {
        return FALSE;
    }

    assert( Is64PtrSE(vs.KernBase) );

    de.dwDebugEventCode                = LOAD_DLL_DEBUG_EVENT;
    de.dwProcessId                     = KD_PROCESSID;
    de.dwThreadId                      = KD_THREADID;
    de.u.LoadDll.hFile                 = (HANDLE)DataTableBuffer.CheckSum;
    de.u.LoadDll.lpBaseOfDll           = vs.KernBase;
    de.u.LoadDll.lpImageName           = (UINT_PTR)lpProgName;
    de.u.LoadDll.fUnicode              = FALSE;
    de.u.LoadDll.nDebugInfoSize        = 0;
    de.u.LoadDll.dwDebugInfoFileOffset = DataTableBuffer.SizeOfImage;

    if (LoadDll(&de, hthd, &cbPacket, &lpbPacket, FALSE) && (cbPacket != 0)) {
        NotifyEM(&de, hthd, cbPacket, (ULONG64)lpbPacket);
    }
    GetKernelSymbolAddresses();

    ConsumeAllEvents();

    return TRUE;
}

VOID
GetKernelSymbolAddresses(
    VOID
    )
{
    if (fCrashDump) {
        vs.DebuggerDataList = GetSymbolAddress("nt!KdpDebuggerDataListHead");
    }
    //(DcbAddr = GetNtDebuggerData( IopDumpControlBlock )) ||
    if (!DcbAddr) {
        DcbAddr = GetSymbolAddress( "nt!IopDumpControlBlock" );
    }

    if (!MmLoadedUserImageList) {
        if (!LoadKdDataBlock()) {
            MmLoadedUserImageList = GetSymbolAddress("nt!MmLoadedUserImageList");
        }
    }

    //(KiProcessorBlockAddr = GetNtDebuggerData( KiProcessorBlock )) ||
    if (!KiProcessorBlockAddr) {
        KiProcessorBlockAddr = GetSymbolAddress( "nt!KiProcessorBlock" );
    }

#if defined(TARGET_ALPHA) || defined(TARGET_AXP64)
    if (!KiPcrBaseAddress) {
        KiPcrBaseAddress = GetSymbolAddress( "nt!KiPcrBaseAddress" );
    }
#endif
}

VOID
GetMachineType(
    LPPROCESSOR p
    )
{
#if defined(TARGET_i386)

    if (DmKdState != S_INITIALIZED) {
        p->Level = 3;
    } else {
        p->Level = sc.ProcessorLevel;
    }

    p->Type = mptix86;
    p->Endian = endLittle;

#elif defined(TARGET_ALPHA)

    p->Type = mptdaxp;
    p->Endian = endLittle;
    p->Level = 21064;

#elif defined(TARGET_AXP64)

    p->Type = mptdaxp;
    p->Endian = endLittle;
    p->Level = 21164;

#elif defined(TARGET_IA64)

    if (DmKdState != S_INITIALIZED) {
        p->Level = 7;
    } else {
        p->Level = sc.ProcessorLevel;
    }

    p->Type = mptia64;
    p->Endian = endLittle;

#else
#pragma error( "unknown target machine" );
#endif

}

#if defined(TARGET_i386) || defined(TARGET_IA64)
BOOL
GetExtendedContext(
    HTHDX               hthd,
    PKSPECIAL_REGISTERS pksr
    )
{
    DWORD  cb;
    DWORD  Status;
    USHORT processor;


    BOOL rc;

    NoApiForCrashDump();

    if (!hthd) {
        return FALSE;
    }

    processor = (USHORT)(hthd->tid - 1);
    if (ContextCache[processor].fSContextStale) {
        if (!ReadControlSpace( processor,
                               ContextSize,
                               (PVOID)pksr,
                               sizeof(KSPECIAL_REGISTERS),
                               &cb
                             )) {
            rc = FALSE;
        } else {
            memcpy( &ContextCache[processor].sregs,
                    pksr,
                    sizeof(KSPECIAL_REGISTERS)
                  );
            ContextCache[processor].fSContextStale = FALSE;
            ContextCache[processor].fSContextDirty = FALSE;
            rc = TRUE;
        }
    } else {
        memcpy( pksr,
                &ContextCache[processor].sregs,
                sizeof(KSPECIAL_REGISTERS)
              );
        rc = TRUE;
    }

    return rc;
}

BOOL
SetExtendedContext(
    HTHDX               hthd,
    PKSPECIAL_REGISTERS pksr
    )
{
    DWORD  cb;
    DWORD  Status;
    USHORT processor;


    BOOL rc;

    NoApiForCrashDump();

    processor = (USHORT)(hthd->tid - 1);
    Status = DmKdWriteControlSpace( processor,
                                    ContextSize,
                                    (PVOID)pksr,
                                    sizeof(KSPECIAL_REGISTERS),
                                    &cb
                                  );

    if (Status || cb != sizeof(KSPECIAL_REGISTERS)) {
        rc = FALSE;
    } else {
        memcpy( &ContextCache[processor].sregs, pksr, sizeof(KSPECIAL_REGISTERS) );
        ContextCache[processor].fSContextStale = FALSE;
        ContextCache[processor].fSContextDirty = FALSE;
        rc = TRUE;
    }

    return rc;
}
#endif // i386



DWORD
ProcessTerminateProcessCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    extern ULONG      MaxRetries;
    BREAKPOINT        *pbpT;
    BREAKPOINT        *pbp;
    HTHDX             hthdT;


    DEBUG_PRINT_2("ProcessTerminateProcessCmd called hprc=0x%p, hthd=0x%p\n",
                  hprc, hthd);

    MaxRetries = 1;

    if (hprc) {
        hprc->pstate |= ps_dead;
        hprc->dwExitCode = 0;
        ConsumeAllProcessEvents(hprc, TRUE);

        for (pbp = BPNextHprcPbp(hprc, NULL); pbp; pbp = pbpT) {
            pbpT = BPNextHprcPbp(hprc, pbp);
            RemoveBP(pbp);
        }

        for (hthdT = hprc->hthdChild; hthdT; hthdT = hthdT->nextSibling) {
            if ( !(hthdT->tstate & ts_dead) ) {
                hthdT->tstate |= ts_dead;
                hthdT->tstate &= ~ts_stopped;
            }
        }
    }

    return TRUE;
}


VOID
ProcessAllProgFreeCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    ProcessTerminateProcessCmd(hprc, hthd, lpdbb);
}



DWORD
ProcessAsyncGoCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    XOSD       xosd = xosdNone;

    DEBUG_PRINT("ProcessAsyncGoCmd called\n");

    hthd->tstate &= ~ts_frozen;
    Reply(0, &xosd, lpdbb->hpid);
    return(xosd);
}


VOID
ProcessAsyncStopCmd(
    HPRCX       hprc,
    HTHDX       hthd,
    LPDBB       lpdbb
    )
/*++

Routine Description:

    This function is called in response to a asynchronous stop request.
    In order to do this we will set breakpoints the current PC for
    every thread in the system and wait for the fireworks to start.

Arguments:

    hprc        - Supplies a process handle
    hthd        - Supplies a thread handle
    lpdbb       - Supplies the command information packet

Return Value:

    None.

--*/

{
    //
    // If we are debugging a dump file, don't
    // bother trying to stop it. Just reply to it as
    // if it were sucessful.
    //
    if (!fCrashDump) {
        //if ( !TryApiLock() ) {
        if ( !TryEnterCriticalSection(&csSynchronizeTargetInterlock) ) {

            DMPrintShellMsg("DMKD: Currently busy synchronizing target and host...\n");
            LpDmMsg->xosdRet = xosdUnknown;

        } else {

            DMPrintShellMsg("DMKD: Sending breakin packet...\n");
            DmKdBreakIn = TRUE;
            //ReleaseApiLock();
            LeaveCriticalSection(&csSynchronizeTargetInterlock);
            LpDmMsg->xosdRet = xosdNone;

        }
    }

    Reply(0, LpDmMsg, lpdbb->hpid);
    return;
}                            /* ProcessAsyncStopCmd() */


VOID
ProcessDebugActiveCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    if (!DmKdConnectAndInitialize( KERNEL_IMAGE_NAME )) {
        LpDmMsg->xosdRet = xosdFileNotFound;
    } else {
        LpDmMsg->xosdRet = xosdNone;
    }

    if (fDisconnected) {
        DmKdBreakIn = TRUE;
        SetEvent( hEventRemoteQuit );
    }

    LpDmMsg->xosdRet = xosdNone;
    Reply(0, LpDmMsg, lpdbb->hpid);
}


VOID
ProcessQueryTlsBaseCmd(
    HPRCX    hprcx,
    HTHDX    hthdx,
    LPDBB    lpdbb
    )

/*++

Routine Description:

    This function is called in response to an EM request to get the base
    of the thread local storage for a given thread and DLL.

Arguments:

    hprcx       - Supplies a process handle

    hthdx       - Supplies a thread handle

    lpdbb       - Supplies the command information packet

Return Value:

    None.

--*/

{
    LpDmMsg->xosdRet = xosdUnknown;
    Reply( sizeof(ADDR), LpDmMsg, lpdbb->hpid );
    return;
}                               /* ProcessQueryTlsBaseCmd() */


VOID
ProcessQuerySelectorCmd(
    HPRCX   hprcx,
    HTHDX   hthdx,
    LPDBB   lpdbb
    )

/*++

Routine Description:

    This command is send from the EM to fill-in a LDT_ENTRY structure
    for a given selector.

Arguments:

    hprcx  - Supplies the handle to the process

    hthdx  - Supplies the handle to the thread and is optional

    lpdbb  - Supplies the pointer to the full query packet

Return Value:

    None.

--*/

{
    XOSD               xosd;

    xosd = xosdUnsupported;
    Reply( sizeof(xosd), &xosd, lpdbb->hpid);

    return;
}


VOID
ProcessReloadModulesCmd(
    HPRCX   hprc,
    HTHDX   hthd,
    LPDBB   lpdbb
    )

/*++

Routine Description:

    This command is send from the EM to cause all modules to be reloaded.

Arguments:

    hprcx  - Supplies the handle to the process

    hthdx  - Supplies the handle to the thread and is optional

    lpdbb  - Supplies the pointer to the full query packet

Return Value:

    None.

--*/

{
    XOSD      xosd;
    LPSSS     lpsss = (LPSSS) lpdbb->rgbVar;

    AddQueue( QT_RELOAD_MODULES,
              hprc->pid,
              hthd->tid,
              *((PULONG)lpsss->rgbData),
              0
            );

    xosd = xosdNone;
    Reply( sizeof(xosd), &xosd, lpdbb->hpid);

    return;
}


VOID
ProcessVirtualQueryCmd(
    HPRCX hprc,
    LPDBB lpdbb
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
#define vaddr(va) ((hprc->fRomImage) ? d[iDll].offBaseOfImage+(va) : (va))

    ADDR                 addr;
    DWORD                cb;
    PDLLLOAD_ITEM        d = prcList->next->rgDllList;

    LPMEMINFO lpmi = (LPMEMINFO)LpDmMsg->rgb;
    XOSD xosd = xosdNone;

    static int                    iDll = 0;
    static PIMAGE_SECTION_HEADER  s    = NULL;

#ifdef DBG
    if (sizeof(s->VirtualAddress) == sizeof(DWORD64)) {
        assert("Remove sign extensions");
    }
#endif

    ZeroMemory(lpmi, sizeof(*lpmi));

    addr = *(LPADDR)(lpdbb->rgbVar);
    assert( Is64PtrSE(addr.addr.off) );

    lpmi->addr.addr.off = addr.addr.off & (PAGE_SIZE - 1);
    lpmi->uRegionSize = PAGE_SIZE;

    // first guess
    lpmi->addrAllocBase = lpmi->addr;

    lpmi->dwProtect = PAGE_READWRITE;
    lpmi->dwAllocationProtect = PAGE_READWRITE;
    lpmi->dwState = MEM_COMMIT;
    lpmi->dwType = MEM_PRIVATE;

    //
    // the following code is necessary to determine if the requested
    // base address is in a page that contains code.  if the base address
    // meets these conditions then reply that it is executable.
    //

    if (hprc->fRomImage) {
        assert( Is64PtrSE(d[iDll].offBaseOfImage) );
    }

    if ( !s ||
        addr.addr.off < SE32To64( vaddr(s->VirtualAddress) ) ||
        addr.addr.off >= SE32To64( vaddr(s->VirtualAddress + s->SizeOfRawData) ) 
        ) {

        for (iDll=0; iDll<prcList->next->cDllList; iDll++) {

            assert( Is64PtrSE(d[iDll].offBaseOfImage) );

            if (addr.addr.off >= d[iDll].offBaseOfImage &&
                addr.addr.off < d[iDll].offBaseOfImage + d[iDll].cbImage) {

                s = d[iDll].Sections;
                cb = d[iDll].NumberOfSections;
                while (cb) {
                    if (addr.addr.off >= SE32To64( vaddr(s->VirtualAddress) ) &&
                        addr.addr.off < SE32To64( vaddr(s->VirtualAddress+s->SizeOfRawData) )
                        ) {

                        break;

                    } else {
                        s++;
                        cb--;
                    }
                }
                if (cb == 0) {
                    s = NULL;
                }
                break;
            }
        }
    }

    if (s) {
        lpmi->addr.addr.off = SE32To64( vaddr(s->VirtualAddress) );
        lpmi->uRegionSize = SE32To64( vaddr(s->VirtualAddress) );

        switch ( s->Characteristics & (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE |
            IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE) ) {
            
        case  IMAGE_SCN_MEM_EXECUTE:
            lpmi->dwProtect =
                lpmi->dwAllocationProtect = PAGE_EXECUTE;
            break;
            
        case  IMAGE_SCN_CNT_CODE:
        case  (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE):
            lpmi->dwProtect =
                lpmi->dwAllocationProtect = PAGE_EXECUTE_READ;
            break;
            
            case  (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ |
                IMAGE_SCN_MEM_WRITE):
                lpmi->dwProtect =
                    lpmi->dwAllocationProtect = PAGE_EXECUTE_READWRITE;
                break;
                
                // This one probably never happens
        case  (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_WRITE):
            lpmi->dwProtect =
                lpmi->dwAllocationProtect = PAGE_EXECUTE_READWRITE;
            break;
            
        case  IMAGE_SCN_MEM_READ:
            lpmi->dwProtect =
                lpmi->dwAllocationProtect = PAGE_READONLY;
            break;
            
        case  (IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE):
            lpmi->dwProtect =
                lpmi->dwAllocationProtect = PAGE_READWRITE;
            break;
            
            // This one probably never happens
        case IMAGE_SCN_MEM_WRITE:
            lpmi->dwProtect =
                lpmi->dwAllocationProtect = PAGE_READWRITE;
            break;
            
        case 0:
            lpmi->dwProtect =
                lpmi->dwAllocationProtect = PAGE_NOACCESS;
            break;
            
        }
    }

    assert( Is64PtrSE( lpmi->addr.addr.off ) );
    assert( Is64PtrSE( lpmi->addrAllocBase.addr.off ) );

    LpDmMsg->xosdRet = xosd;
    Reply( sizeof(MEMINFO), LpDmMsg, lpdbb->hpid );

    return;
}

VOID
ProcessGetDmInfoCmd(
    HPRCX hprc,
    LPDBB lpdbb,
    DWORD cb
    )
{
    LPDMINFO lpi = (LPDMINFO)LpDmMsg->rgb;

    LpDmMsg->xosdRet = xosdNone;

    lpi->mAsync = asyncRun |
                  asyncStop;
    lpi->fHasThreads = 1;
    lpi->fReturnStep = 0;
    //lpi->fRemote = ???
    lpi->fAlwaysFlat = 0;
    lpi->fHasReload = 1;
    lpi->fNonLocalGoto = 0;
    lpi->fKernelMode = 1;

#ifdef HAS_DEBUG_REGS
    lpi->cbSpecialRegs = sizeof(KSPECIAL_REGISTERS);
#else
    lpi->cbSpecialRegs = 0;
#endif

    lpi->MajorVersion = vs.MajorVersion;
    lpi->MinorVersion = vs.MinorVersion;

    lpi->Breakpoints = bptsExec |
                       bptsDataC |
                       bptsDataW |
                       bptsDataR |
                       bptsDataExec;

    GetMachineType(&lpi->Processor);

    if (DmKdState != S_INITIALIZED ||
        vs.MajorVersion == 0) {
        lpi->fDMInfoCacheable = FALSE;
    } else {
        lpi->fDMInfoCacheable = TRUE;
    }

    //
    // hack so that TL can call tlfGetVersion before
    // reply buffer is initialized.
    //
    if ( cb >= (FIELD_OFFSET(DBB, rgbVar) + sizeof(DMINFO)) ) {
        memcpy(lpdbb->rgbVar, lpi, sizeof(DMINFO));
    }

    Reply( sizeof(DMINFO), LpDmMsg, lpdbb->hpid );
}

#ifdef HAS_DEBUG_REGS
VOID
ProcessGetExtendedContextCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    PKSPECIAL_REGISTERS pksr = (PKSPECIAL_REGISTERS)LpDmMsg->rgb;


    if (GetExtendedContext( hthd, pksr )) {
        LpDmMsg->xosdRet = xosdUnknown;
    } else {
        LpDmMsg->xosdRet = xosdNone;
    }

    Reply(sizeof(KSPECIAL_REGISTERS), LpDmMsg, lpdbb->hpid);
}

void
ProcessSetExtendedContextCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    PKSPECIAL_REGISTERS pksr = (PKSPECIAL_REGISTERS)lpdbb->rgbVar;


    if (SetExtendedContext( hthd, pksr )) {
        LpDmMsg->xosdRet = xosdUnknown;
    } else {
        LpDmMsg->xosdRet = xosdNone;
    }

    Reply(0, LpDmMsg, lpdbb->hpid);
}
#endif  // HAS_DEBUG_REGS

void
ProcessGetSectionsCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    DWORD64                     dwBaseOfDll = *((PDWORD64) lpdbb->rgbVar);
    LPOBJD                      rgobjd = (LPOBJD) LpDmMsg->rgb;
    IMAGE_DOS_HEADER            dh;
    IMAGE_NT_HEADERS            nh;
    PIMAGE_SECTION_HEADER       sec;
    IMAGE_ROM_OPTIONAL_HEADER   rom;
    DWORD64                     fpos;
    DWORD                       iobj;
    //
    // if offset is changed to anything other than a 
    // 32bit variable, modify SE32To64(offset) accordingly.
    //
    DWORD                       offset;
    DWORD                       cbObject;
    DWORD                       iDll;
    DWORD                       sig;
    IMAGEINFO                   ii;

    assert( Is64PtrSE(dwBaseOfDll) );

    //
    // find the module
    //
    for (iDll=0; iDll<(DWORD)hprc->cDllList; iDll++) {

        assert( Is64PtrSE(hprc->rgDllList[iDll].offBaseOfImage) );

        if (hprc->rgDllList[iDll].offBaseOfImage == dwBaseOfDll) {

            if (hprc->rgDllList[iDll].sec) {

                sec = hprc->rgDllList[iDll].sec;
                nh.FileHeader.NumberOfSections =
                                (USHORT)hprc->rgDllList[iDll].NumberOfSections;

            } else {
                fpos = dwBaseOfDll;

                if (!DbgReadMemory( hprc, fpos, &dh, sizeof(IMAGE_DOS_HEADER), NULL )) {
                    break;
                }

                if (dh.e_magic == IMAGE_DOS_SIGNATURE) {
                    fpos += dh.e_lfanew;
                } else {
                    fpos = dwBaseOfDll;
                }

                if (!DbgReadMemory( hprc, fpos, &sig, sizeof(sig), NULL )) {
                    break;
                }

                if (sig != IMAGE_NT_SIGNATURE) {
                    if (!DbgReadMemory( hprc, fpos, &nh.FileHeader, sizeof(IMAGE_FILE_HEADER), NULL )) {
                        break;
                    }
                    fpos += sizeof(IMAGE_FILE_HEADER);
                    if (nh.FileHeader.SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
                        if (!DbgReadMemory( hprc, fpos, &rom, sizeof(rom), NULL )) {
                            break;
                        }
                        ZeroMemory( &nh.OptionalHeader, sizeof(nh.OptionalHeader) );
                        nh.OptionalHeader.SizeOfImage      = rom.SizeOfCode;
                        nh.OptionalHeader.ImageBase        = rom.BaseOfCode;
                    } else {
                        //
                        // maybe its a firmware image?
                        //
                        if (! ReadImageInfo(
                                hprc->rgDllList[iDll].szDllName,
                                NULL,
                                (LPSTR)KdOptions[KDO_SYMBOLPATH].value,
                                &ii )) {
                            //
                            // can't read the image correctly
                            //
                            LpDmMsg->xosdRet = xosdUnknown;
                            Reply(0, LpDmMsg, lpdbb->hpid);
                            return;
                        }
                        sec = ii.Sections;
                        nh.FileHeader.NumberOfSections = (USHORT)ii.NumberOfSections;
                        nh.FileHeader.SizeOfOptionalHeader = IMAGE_SIZEOF_ROM_OPTIONAL_HEADER;
                    }
                } else {
                    if (!DbgReadMemory( hprc, fpos, &nh, sizeof(IMAGE_NT_HEADERS), NULL )) {
                        break;
                    }

                    fpos += sizeof(IMAGE_NT_HEADERS);

                    if (nh.Signature != IMAGE_NT_SIGNATURE) {
                        break;
                    }

                    if (hprc->rgDllList[iDll].TimeStamp == 0) {
                        hprc->rgDllList[iDll].TimeStamp = nh.FileHeader.TimeDateStamp;
                    }

                    if (hprc->rgDllList[iDll].CheckSum == 0) {
                        hprc->rgDllList[iDll].CheckSum = nh.OptionalHeader.CheckSum;
                    }

                    sec = MHAlloc( nh.FileHeader.NumberOfSections * IMAGE_SIZEOF_SECTION_HEADER );
                    if (!sec) {
                        break;
                    }

                    DbgReadMemory( hprc, fpos, sec, nh.FileHeader.NumberOfSections * IMAGE_SIZEOF_SECTION_HEADER, NULL );
                }
            }

            if (hprc->rgDllList[iDll].Sections == NULL) {
                hprc->rgDllList[iDll].Sections = sec;
                hprc->rgDllList[iDll].NumberOfSections =
                                                nh.FileHeader.NumberOfSections;

                if (nh.FileHeader.SizeOfOptionalHeader != IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
                    for (iobj=0; iobj<nh.FileHeader.NumberOfSections; iobj++) {
                        hprc->rgDllList[iDll].Sections[iobj].VirtualAddress += (DWORD)dwBaseOfDll;
                    }
                }
            }

            *((LPDWORD)LpDmMsg->rgb) = nh.FileHeader.NumberOfSections;
            rgobjd = (LPOBJD) (LpDmMsg->rgb + sizeof(DWORD));
            //
            //  Set up the descriptors for each of the section headers
            //  so that the EM can map between section numbers and flat
            //  addresses.
            //
            for (iobj=0; iobj<nh.FileHeader.NumberOfSections; iobj++) {
                offset = hprc->rgDllList[iDll].Sections[iobj].VirtualAddress;
                cbObject = hprc->rgDllList[iDll].Sections[iobj].Misc.VirtualSize;
                if (cbObject == 0) {
                    cbObject = hprc->rgDllList[iDll].Sections[iobj].SizeOfRawData;
                }

                //
                // See comment at variable declaration of offset
                //
                rgobjd[iobj].offset = SE32To64(offset);
                rgobjd[iobj].cb = cbObject;
                rgobjd[iobj].wPad = 1;
#ifdef TARGET_i386
                if (IMAGE_SCN_CNT_CODE &
                       hprc->rgDllList[iDll].Sections[iobj].Characteristics) {
                    rgobjd[iobj].wSel = (WORD) hprc->rgDllList[iDll].SegCs;
                } else {
                    rgobjd[iobj].wSel = (WORD) hprc->rgDllList[iDll].SegDs;
                }
#else
                rgobjd[iobj].wSel = 0;
#endif
            }

            LpDmMsg->xosdRet = xosdNone;
            Reply( sizeof(DWORD) + (hprc->rgDllList[iDll].NumberOfSections * sizeof(OBJD)),
                   LpDmMsg,
                   lpdbb->hpid);

            return;
        }
    }


    LpDmMsg->xosdRet = xosdUnknown;
    Reply(0, LpDmMsg, lpdbb->hpid);
}



BOOL
GetDebuggerDataBlock(
    IN HPRCX hprc,
    IN OUT PDBGKD_DEBUG_DATA_HEADER64 DataBlock
    )

/*++

Routine Description:

    This routine walks through the kernel's debugger data block list looking
    for a block with a particulat tag.  If that block is found, it will be
    read and returned to the caller.

    A single-entry cache is used to improve the performance of debugger
    extensions which query this interface frequently.

    Debugger data blocks are supplied by drivers and other components.  If a
    driver unloads without unlinking the debugger data, the list will be broken.
    If this routine encounters a broken list, it will try running through the
    list in reverse order.

Arguments:

    DataBlock - Supplies a pointer to block header with the OwnerTag and Size
                fields filled in.  The block must be Size bytes in length.
                If the block is found, it will be copied into this buffer.
                The size of the returned block is limited to the size specified
                by the caller; this routine will not write past the end of the
                supplied buffer.  However, the Size field will be modified to
                contain the actual size of the block in the debuggee system.

Return Value:

    TRUE if a valid data block is found, FALSE otherwise.

--*/

{
    DBGKD_DEBUG_DATA_HEADER64 Header;
    LIST_ENTRY64 List64;
    ULONG64 Next64;
    ULONG64 Address;
    BOOL Reverse = FALSE;
    ULONG SizeToRead;
    ULONG Result;

    PLIST_ENTRY pList;
    PDEBUG_DATA_CACHE CacheEntry = NULL;

    //
    // if block is cached, return it
    //

    pList = DebugDataCacheList.Flink;

    while (pList != &DebugDataCacheList) {
        CacheEntry = CONTAINING_RECORD(pList, DEBUG_DATA_CACHE, List);
        pList = pList->Flink;
        if (CacheEntry->Block.OwnerTag == DataBlock->OwnerTag) {
            //
            // got it - is it valid?
            //
            if (CacheEntry->Time != LastStopTime) {

                //
                // It is stale, and must be reloaded.
                //
                // if size is different, discard this block:
                //

                if (CacheEntry->Block.Size != DataBlock->Size) {
                    RemoveEntryList(&CacheEntry->List);
                    free(CacheEntry);
                    CacheEntry = NULL;
                }

                break;

            } else {

                //
                // return it
                //

                memcpy(DataBlock, &CacheEntry->Block, DataBlock->Size);
                return TRUE;
            }
        }
    }

    if (CacheEntry && (CacheEntry->Block.OwnerTag != DataBlock->OwnerTag)) {
        CacheEntry = NULL;
    }

    //
    // get list head
    //
    if (!vs.DebuggerDataList) {
        DbgGetVersion( &vs );
    }

    if (vs.MinorVersion <= 1381) {
        return FALSE;
    }

    if (!vs.DebuggerDataList) {
        DMPrintShellMsg("DMKD: Unable to get address of debugger data list\n");
        return FALSE;
    }

    //
    // walk list, look for tag
    //

    if (!NT_SUCCESS(DmKdReadListEntry(vs.DebuggerDataList, &List64))) {
        DMPrintShellMsg("DMKD: Unable to read debugger data list head\n");
        return FALSE;
    }

    Next64 = List64.Flink;
    if (Next64 == 0) {
        DMPrintShellMsg("DMKD: Debugger data list head is NULL!\n");
        return FALSE;
    }

SearchList:
    while (Next64 != vs.DebuggerDataList) {

        //if (CheckControlC()) {
            //return FALSE;
        //}

        if (DmKdPtr64) {
            Address = CONTAINING_RECORD64(Next64, DBGKD_DEBUG_DATA_HEADER64, List);
        } else {
            Address = SE32To64( CONTAINING_RECORD32(Next64, DBGKD_DEBUG_DATA_HEADER32, List));
        }
        if (!NT_SUCCESS(DmKdReadDebuggerDataHeader(Address, &Header))) {

            if (Reverse) {
                DMPrintShellMsg("DMKD: Debugger data list is corrupt.  Unable to locate block for %4.4s",
                        &DataBlock->OwnerTag);
                return FALSE;

            } else {
                //
                // List is broken; try it in reverse
                //
                Next64 = List64.Blink;
                Reverse = TRUE;
                goto SearchList;
            }

        }

        if (Header.OwnerTag == DataBlock->OwnerTag) {

            //
            // found it
            //

            //
            // if the requested size is different from the actual
            // size, read only the smaller of the two.
            // However, always return the actual size in the size field,
            // so the caller can see if it does not match.
            //

            SizeToRead = DataBlock->Size;
            if (Header.Size != SizeToRead) {
                DMPrintShellMsg("DMKD: Debugger data block sizes do not match for component %4.4s\n",
                        &Header.OwnerTag);
                DMPrintShellMsg("DMKD: Extension requested %d, component contains %d\n",
                        SizeToRead, Header.Size);
                if (Header.Size < SizeToRead) {
                    SizeToRead = Header.Size;
                }
            }

            if (!NT_SUCCESS(DmKdReadDebuggerDataBlock(Address, DataBlock, SizeToRead))) {
                DMPrintShellMsg("DMKD: Unable to read debugger data block\n");
                return FALSE;
            }

            if (!CacheEntry) {
                CacheEntry = malloc(SizeToRead - sizeof(DBGKD_DEBUG_DATA_HEADER64) + sizeof(DEBUG_DATA_CACHE));
            } else {
                RemoveEntryList(&CacheEntry->List);
            }
            CacheEntry->Time = LastStopTime;
            memcpy(&CacheEntry->Block, DataBlock, SizeToRead);
            InsertHeadList(&DebugDataCacheList, &CacheEntry->List);
            return TRUE;

        }

        if (Reverse) {
            Next64 = Header.List.Blink;
        } else {
            Next64 = Header.List.Flink;
        }

    }

    DMPrintShellMsg("DMKD: Debugger data block not found for %4.4s",
            &DataBlock->OwnerTag);

    return FALSE;

}

BOOL
LoadKdDataBlock(
    VOID
    )
{
    if (KdDebuggerData.Header.Size == 0) {
        KdDebuggerData.Header.OwnerTag = KDBG_TAG;
        KdDebuggerData.Header.Size = sizeof(KDDEBUGGER_DATA64);
        if (!GetDebuggerDataBlock( HPRCFromPID( KD_PROCESSID ), (PDBGKD_DEBUG_DATA_HEADER64)&KdDebuggerData.Header)) {
            KdDebuggerData.Header.Size = -1;
        }
    }
    return KdDebuggerData.Header.Size != -1;
}


VOID
LocalProcessSystemServiceCmd(
    HPRCX   hprc,
    HTHDX   hthd,
    LPDBB   lpdbb
    )

/*++

Routine Description:

    This function is called in response to a SystemService command from
    the shell.  It is used as a catch all to get and set strange information
    which is not covered elsewhere.  The set of SystemServices is OS and
    implemenation dependent.

Arguments:

    hprc        - Supplies a process handle

    hthd        - Supplies a thread handle

    lpdbb       - Supplies the command information packet

Return Value:

    None.

--*/

{
    LPSSS lpsss =  (LPSSS) lpdbb->rgbVar;

    switch( lpsss->ssvc ) {
        default:
            LpDmMsg->xosdRet = xosdUnsupported;
            Reply(0, LpDmMsg, lpdbb->hpid);
            return;
    }
}                               /* LocalProcessSystemServiceCmd() */



VOID
ProcessIoctlGenericCmd(
    HPRCX   hprc,
    HTHDX   hthd,
    LPDBB   lpdbb
    )
{

    LPSSS              lpsss  = (LPSSS)lpdbb->rgbVar;

    PIOCTLGENERIC      InputPig = (PIOCTLGENERIC)lpsss->rgbData;
    PVOID              InputBuffer = InputPig->data;
    DWORD              InputType = InputPig->ioctlSubType;

    DWORD              ReplyXosd;
    DWORD              ReplyLength;
    PVOID              ReplyBuffer;
    PIOCTLGENERIC      ReplyPig = (PIOCTLGENERIC)LpDmMsg->rgb;

    PPROCESSORINFO     pi;
    PREADCONTROLSPACE  prc;
    PIOSPACE           pis;
    PIOSPACE_EX        pisex;
    PPHYSICAL          phy;
    PKDHELP64          KdHelp;

    static ULONG64     SavedThread;
    static USHORT      Processor = (USHORT)-1;


    if (!TryApiLock()) {
        LpDmMsg->xosdRet = xosdUnknown;
        Reply( 0, LpDmMsg, lpdbb->hpid );
        return;
    }

    *ReplyPig = *InputPig;
    ReplyBuffer = ReplyPig->data;
    ReplyLength = ReplyPig->length;

    ReplyXosd = xosdNone;

    switch( InputPig->ioctlSubType ) {
        case IG_READ_CONTROL_SPACE:
            prc = (PREADCONTROLSPACE) ReplyPig->data;
            *prc = *(PREADCONTROLSPACE)InputPig->data;
            if ((SHORT)prc->Processor == -1) {
                if (Processor == (USHORT)-1) {
                    prc->Processor = sc.Processor;
                } else {
                    prc->Processor = Processor;
                }
            }
            if (!ReadControlSpace( (USHORT)prc->Processor,
                                   prc->Address,
                                   (PVOID)prc->Buf,
                                   prc->BufLen,
                                   &prc->BufLen
                                 )) {
                ReplyXosd = xosdUnknown;
                ReplyLength = 0;
            }
            break;

        case IG_WRITE_CONTROL_SPACE:
            ReplyLength = 0;
            break;

        case IG_READ_IO_SPACE:
            pis = (PIOSPACE) ReplyPig->data;
            *pis = *(PIOSPACE)InputPig->data;
            if (DmKdReadIoSpace( pis->Address,
                                 &pis->Data,
                                 pis->Length ) != STATUS_SUCCESS) {
                pis->Length = 0;
            }
            break;

        case IG_WRITE_IO_SPACE:
            pis = (PIOSPACE) ReplyPig->data;
            *pis = *(PIOSPACE)InputPig->data;
            if (DmKdWriteIoSpace( pis->Address,
                                  pis->Data,
                                  pis->Length ) != STATUS_SUCCESS) {
                pis->Length = 0;
            }
            break;

        case IG_READ_IO_SPACE_EX:
            pisex = (PIOSPACE_EX) ReplyPig->data;
            *pisex = *(PIOSPACE_EX)InputPig->data;
            if (DmKdReadIoSpaceEx(
                             pisex->Address,
                             &pisex->Data,
                             pisex->Length,
                             pisex->InterfaceType,
                             pisex->BusNumber,
                             pisex->AddressSpace
                             ) != STATUS_SUCCESS) {
                pisex->Length = 0;
            }
            break;

        case IG_WRITE_IO_SPACE_EX:
            pisex = (PIOSPACE_EX) ReplyPig->data;
            *pisex = *(PIOSPACE_EX)InputPig->data;
            if (DmKdWriteIoSpaceEx(
                             pisex->Address,
                             pisex->Data,
                             pisex->Length,
                             pisex->InterfaceType,
                             pisex->BusNumber,
                             pisex->AddressSpace
                             ) != STATUS_SUCCESS) {
                pisex->Length = 0;
            }
            break;

        case IG_READ_PHYSICAL:
            phy = (PPHYSICAL) ReplyPig->data;
            *phy = *(PPHYSICAL)InputPig->data;
            if (DmKdReadPhysicalMemory( phy->Address, phy->Buf, phy->BufLen, &phy->BufLen )) {
                phy->BufLen = 0;
            }
            break;

        case IG_WRITE_PHYSICAL:
            phy = (PPHYSICAL) ReplyPig->data;
            *phy = *(PPHYSICAL)InputPig->data;
            if (DmKdWritePhysicalMemory( phy->Address, phy->Buf, phy->BufLen, &phy->BufLen )) {
                phy->BufLen = 0;
            }
            break;

        case IG_DM_PARAMS:
            ParseDmParams( (LPSTR)InputPig->data );
            ReplyLength = 0;
            break;

        case IG_KD_CONTEXT:
            pi = (PPROCESSORINFO) ReplyPig->data;
            pi->Processor = sc.Processor;
            pi->NumberProcessors = (USHORT)sc.NumberProcessors;
            break;

        case IG_RELOAD:
            AddQueue( QT_RELOAD_MODULES,
                      0,
                      0,
                      (DWORD64)InputPig->data,
                      _tcslen((LPSTR)InputPig->data)+1 );
            ReplyLength = 0;
            break;

        case IG_CHANGE_PROC:
            Processor = (USHORT)((PULONG)InputPig->data)[0];
            ReplyLength = 0;
            break;

        case IG_KSTACK_HELP:
            KdHelp = (PKDHELP64)ReplyPig->data;
            KdHelp->Thread = SavedThread? SavedThread : (DWORD64)sc.Thread;
            SavedThread = 0;
            if (LoadKdDataBlock()) {
                KdHelp->KiCallUserMode = KdDebuggerData.KiCallUserMode;
                KdHelp->ThCallbackStack = KdDebuggerData.ThCallbackStack;
                KdHelp->NextCallback = KdDebuggerData.NextCallback;
                KdHelp->FramePointer = KdDebuggerData.FramePointer;
                KdHelp->KeUserCallbackDispatcher = KdDebuggerData.KeUserCallbackDispatcher;
            }
            ReplyLength = sizeof(KDHELP64);
            break;

        case IG_SET_THREAD:
            SavedThread = *(PULONG64)(InputPig->data);
            break;

        case IG_GET_DEBUGGER_DATA:
            *((PDBGKD_DEBUG_DATA_HEADER64)ReplyPig->data) = *((PDBGKD_DEBUG_DATA_HEADER64)InputPig->data);
            if (!GetDebuggerDataBlock(hprc, (PDBGKD_DEBUG_DATA_HEADER64)ReplyPig->data)) {
                ReplyXosd = xosdUnknown;
                ReplyLength = 0;
            }
            break;

        // BUGBUG add translation
        //case IG_GET_KERNEL_VERSION:
        //    *((PDBGKD_GET_VERSION)ReplyPig->data) = vs;
        //    ReplyLength = sizeof(DBGKD_GET_VERSION);
        //    break;

        case IG_RELOAD_SYMBOLS:
            ReloadModules(hthd, (LPSTR)InputPig->data);
            ReplyLength = 0;
            break;

        default:
            ReplyXosd = xosdUnknown;
            ReplyLength = 0;
            break;
    }

    ReleaseApiLock();

    ReplyPig->length = ReplyLength;
    LpDmMsg->xosdRet = ReplyXosd;
    Reply( ReplyLength + sizeof(IOCTLGENERIC), LpDmMsg, lpdbb->hpid );
}


VOID
ProcessSSVCCustomCmd(
    HPRCX   hprc,
    HTHDX   hthd,
    LPDBB   lpdbb
    )
{
    LPSSS   lpsss  = (LPSSS)lpdbb->rgbVar;
    LPSTR   p      = lpsss->rgbData;


    LpDmMsg->xosdRet = xosdUnsupported;

    //
    // parse the command
    //
    while (*p && !isspace(*p++));
    if (*p) {
        *(p-1) = '\0';
    }

    //
    // process the command
    //
    if (_tcsicmp( lpsss->rgbData, "resync" ) == 0) {
       if ( !TryEnterCriticalSection(&csSynchronizeTargetInterlock) ) {

            DMPrintShellMsg("DMKD: Currently busy synchronizing target and host...\n");
            LpDmMsg->xosdRet = xosdUnknown;

        } else {

            DMPrintShellMsg( "Host and target systems resynchronizing...\n" );
            KdResync = TRUE;
            LeaveCriticalSection(&csSynchronizeTargetInterlock);
            LpDmMsg->xosdRet = xosdNone;

        }
    } else
    if (_tcsicmp( lpsss->rgbData, "cache" ) == 0) {
        ProcessCacheCmd(p);
        LpDmMsg->xosdRet = xosdNone;
    } else
    if (_tcsicmp( lpsss->rgbData, "reboot" ) == 0) {
        if (TryApiLock()) {
            AddQueue( QT_REBOOT, 0, 0, 0, 0 );
            ReleaseApiLock();
            LpDmMsg->xosdRet = xosdNone;
        } else {
            LpDmMsg->xosdRet = xosdUnknown;
        }
    } else
    if (_tcsicmp( lpsss->rgbData, "crash" ) == 0) {
        if (TryApiLock()) {
            AddQueue( QT_CRASH, 0, 0, CRASH_BUGCHECK_CODE, 0 );
            ReleaseApiLock();
            LpDmMsg->xosdRet = xosdNone;
        } else {
            LpDmMsg->xosdRet = xosdUnknown;
        }
    } else
    if ( !_tcsicmp(lpsss->rgbData, "FastStep") ) {
        fSmartRangeStep = TRUE;
        LpDmMsg->xosdRet = xosdNone;
    } else
    if ( !_tcsicmp(lpsss->rgbData, "SlowStep") ) {
        fSmartRangeStep = FALSE;
        LpDmMsg->xosdRet = xosdNone;
    } else
    if ( !_tcsicmp(lpsss->rgbData, "trace") ) {
        fPacketTrace = !fPacketTrace;
        LpDmMsg->xosdRet = xosdNone;
    }

    //
    // send back our response
    //
    Reply(0, LpDmMsg, lpdbb->hpid);
}


void
ContinueThreadEx(
    HTHDX hthd,
    DWORD ContinueStatus,
    DWORD EventType,
    TSTATEX NewState
    )
{
    //
    // If NewState is ts_running, do the "usual magic" as a special
    // case.  If it is anything else, set the flag to NewState
    //
    if (NewState == ts_running) {
        hthd->tstate &= ~(ts_stopped | ts_first | ts_second);
        hthd->tstate |= ts_running;
    } else {
        hthd->tstate = NewState;
    }

    hthd->fExceptionHandled = FALSE;

    AddQueue (EventType,
              hthd->hprc->pid,
              hthd->tid,
              ContinueStatus,
              0);
}


void
ContinueProcess(
    HPRCX hprc
    )
{
    HTHDX hthd;
    for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
        if (hthd->tstate & ts_stopped) {
            ContinueThread(hthd);
        }
    }
}


void
ContinueThread(
    HTHDX hthd
    )
{
    DWORD ContinueStatus = DBG_CONTINUE;
    if ( (hthd->tstate & (ts_first | ts_second)) &&
         !hthd->fExceptionHandled) {
        ContinueStatus = (DWORD)DBG_EXCEPTION_NOT_HANDLED;
    }
    ContinueThreadEx(hthd,
                     ContinueStatus,
                     QT_CONTINUE_DEBUG_EVENT,
                     ts_running
                     );
}


BOOL
GetModnameFromImage(
    HPRCX                   hprc,
    LPLOAD_DLL_DEBUG_INFO64 ldd,
    LPTSTR                  lpName
    )
/*++

Routine Description:

    This routine attempts to get the name of the exe as placed
    in the debug section by the linker.

Arguments:

    hprc -

    ldd -

    lpName -

Return Value:

    TRUE if a name was found, FALSE if not.
    The exe name is returned as an ANSI string in lpName.

--*/
{
    #define ReadMem(b,s) DbgReadMemory( hprc, (address), (b), (s), NULL ); address += (s)

    IMAGE_DEBUG_DIRECTORY       DebugDir;
    PIMAGE_DEBUG_MISC           pMisc;
    PIMAGE_DEBUG_MISC           pT;
    DWORD                       rva;
    int                         nDebugDirs;
    int                         i;
    int                         j;
    int                         l;
    BOOL                        rVal = FALSE;
    PVOID                       pExeName;
    IMAGE_NT_HEADERS            nh;
    IMAGE_DOS_HEADER            dh;
    IMAGE_ROM_OPTIONAL_HEADER   rom;
    DWORD64                     address;
    DWORD                       sig;
    PIMAGE_SECTION_HEADER       pSH;
    DWORD                       cb;

    assert( Is64PtrSE(ldd->lpBaseOfDll) );

    lpName[0] = 0;

    address = ldd->lpBaseOfDll;

    ReadMem( &dh, sizeof(dh) );

    if (dh.e_magic == IMAGE_DOS_SIGNATURE) {
        address = ldd->lpBaseOfDll + dh.e_lfanew;
    } else {
        address = ldd->lpBaseOfDll;
    }

    ReadMem( &sig, sizeof(sig) );
    address -= sizeof(sig);

    if (sig == IMAGE_NT_SIGNATURE) {
        ReadMem( &nh, sizeof(nh) );
    } else {
        ReadMem( &nh.FileHeader, sizeof(IMAGE_FILE_HEADER) );
        if (nh.FileHeader.SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
            ReadMem( &rom, sizeof(rom) );
            ZeroMemory( &nh.OptionalHeader, sizeof(nh.OptionalHeader) );
            nh.OptionalHeader.SizeOfImage      = rom.SizeOfCode;
            nh.OptionalHeader.ImageBase        = rom.BaseOfCode;
        } else {
            return FALSE;
        }
    }

    cb = nh.FileHeader.NumberOfSections * IMAGE_SIZEOF_SECTION_HEADER;
    pSH = MHAlloc( cb );
    ReadMem( pSH, cb );

    nDebugDirs = nh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size /
                 sizeof(IMAGE_DEBUG_DIRECTORY);

    if (!nDebugDirs) {
        return FALSE;
    }

    rva = nh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;

    for(i = 0; i < nh.FileHeader.NumberOfSections; i++) {
        if (rva >= pSH[i].VirtualAddress &&
            rva < pSH[i].VirtualAddress + pSH[i].SizeOfRawData) {
            break;
        }
    }

    if (i >= nh.FileHeader.NumberOfSections) {
        return FALSE;
    }

    rva = ((rva - pSH[i].VirtualAddress) + pSH[i].VirtualAddress);

    for (j = 0; j < nDebugDirs; j++) {

        address = rva + (sizeof(DebugDir) * j) + ldd->lpBaseOfDll;
        assert( Is64PtrSE(address) );
        ReadMem( &DebugDir, sizeof(DebugDir) );

        if (DebugDir.Type == IMAGE_DEBUG_TYPE_MISC) {

            l = DebugDir.SizeOfData;
            pMisc = pT = MHAlloc(l);

            if ((ULONG)DebugDir.AddressOfRawData < pSH[i].VirtualAddress ||
                  (ULONG)DebugDir.AddressOfRawData >=
                                         pSH[i].VirtualAddress + pSH[i].SizeOfRawData) {
                //
                // the misc debug data MUST be in the .rdata section
                // otherwise windbg cannot access it as it is not mapped in
                //
                continue;
            }

            address = (ULONG64)DebugDir.AddressOfRawData + ldd->lpBaseOfDll;
            assert( Is64PtrSE(address) );
            ReadMem( pMisc, l );

            while (l > 0) {
                if (pMisc->DataType != IMAGE_DEBUG_MISC_EXENAME) {
                    l -= pMisc->Length;
                    pMisc = (PIMAGE_DEBUG_MISC)
                                (((LPSTR)pMisc) + pMisc->Length);
                } else {

                    pExeName = (PVOID)&pMisc->Data[ 0 ];

                    if (!pMisc->Unicode) {
                        _tcscpy(lpName, (LPSTR)pExeName);
                        rVal = TRUE;
                    } else {
                        WideCharToMultiByte(CP_ACP,
                                            0,
                                            (LPWSTR)pExeName,
                                            -1,
                                            lpName,
                                            MAX_PATH,
                                            NULL,
                                            NULL);
                        rVal = TRUE;
                    }

                    /*
                     *  Undo stevewo's error
                     */

                    if (_ftcsicmp(&lpName[_ftcslen(lpName)-4], ".DBG") == 0) {
                        TCHAR    rgchPath[_MAX_PATH];
                        TCHAR    rgchBase[_MAX_FNAME];

                        _splitpath(lpName, NULL, rgchPath, rgchBase, NULL);
                        if (_ftcslen(rgchPath)==4) {
                            rgchPath[_ftcslen(rgchPath)-1] = 0;
                            _ftcscpy(lpName, rgchBase);
                            _ftcscat(lpName, _T("."));
                            _ftcscat(lpName, rgchPath);
                        } else {
                            _ftcscpy(lpName, rgchBase);
                            _ftcscat(lpName, _T(".exe"));
                        }
                    }
                    break;
                }
            }

            MHFree(pT);

            break;

        }
    }

    return rVal;
}

LPMODULEALIAS
FindAddAliasByModule(
    LPSTR lpImageName,
    LPSTR lpModuleName
    )
/*++

Routine Description:

    Look for an alias entry by its "common" name, for example, look
    for "NT".  If it does not exist, and a new image name has been
    provided, add it.  If it does exist and a new image name has been
    provided, replace the image name.  Return the new or found record.

Arguments:


Return Value:


--*/
{
    int i;

    for (i=0; i<MAX_MODULEALIAS; i++) {
        if (ModuleAlias[i].ModuleName[0] == 0) {
            if (!lpImageName) {
                return NULL;
            } else {
                _tcscpy( ModuleAlias[i].Alias, lpImageName );
                _tcscpy( ModuleAlias[i].ModuleName, lpModuleName );
                ModuleAlias[i].Special = 1;
                return &ModuleAlias[i];
            }
        }
        if (_tcsicmp( ModuleAlias[i].ModuleName, lpModuleName ) == 0) {
            if (lpImageName) {
                _tcscpy( ModuleAlias[i].Alias, lpImageName);
            }
            return &ModuleAlias[i];
        }
    }

    // Should return before here
    assert( 0 );
    return NULL;
}

LPMODULEALIAS
FindAliasByImageName(
    LPSTR lpImageName
    )
{
    int i;

    for (i=0; i<MAX_MODULEALIAS; i++) {
        if (ModuleAlias[i].ModuleName[0] == 0) {
            return NULL;
        }
        if (_tcsicmp( ModuleAlias[i].Alias, lpImageName ) == 0) {
            return &ModuleAlias[i];
        }
    }

    // Should return before here
    assert( 0 );
    return NULL;
}

LPMODULEALIAS
CheckForRenamedImage(
    HPRCX hprc,
    LOAD_DLL_DEBUG_INFO64 *ldd,
    LPTSTR lpOrigImageName,
    LPTSTR lpModuleName
    )
{
    CHAR  ImageName[MAX_PATH];
    CHAR  fname[_MAX_FNAME];
    CHAR  ext[_MAX_EXT];
    DWORD i;


    if (_ftcsicmp( (LPTSTR)ldd->lpImageName, lpOrigImageName ) != 0) {
        return NULL;
    }

    if (GetModnameFromImage( hprc, ldd, ImageName ) && ImageName[0]) {
        _splitpath( ImageName, NULL, NULL, fname, ext );
        sprintf( ImageName, _T("%s%s"), fname, ext );
        return FindAddAliasByModule(ImageName, lpModuleName);
    }

    return NULL;
}


BOOL
LoadDll(
    LPDEBUG_EVENT64 de,
    HTHDX           hthd,
    LPWORD          lpcbPacket,
    LPBYTE *        lplpbPacket,
    BOOL            fThreadIsStopped
    )
/*++

Routine Description:

    This routine is used to load the signification information about
    a PE exe file.  This information consists of the name of the exe
    just loaded (hopefully this will be provided later by the OS) and
    a description of the sections in the exe file.

Arguments:

    de         - Supplies a pointer to the current debug event

    hthd       - Supplies a pointer to the current thread structure

    lpcbPacket - Returns the count of bytes in the created packet

    lplpbPacket - Returns the pointer to the created packet

    fThreadIsStopped - Supplies a flag to tell the shell whether to send a continue

Return Value:

    True on success and FALSE on failure

--*/

{
    LPLOAD_DLL_DEBUG_INFO64     ldd = &de->u.LoadDll;
    LPMODULELOAD                lpmdl;
    CHAR                        szModName[MAX_PATH];
    DWORD                       lenSz;
    INT                         iDll;
    HPRCX                       hprc = hthd->hprc;
    CHAR                        fname[_MAX_FNAME];
    CHAR                        ext[_MAX_EXT];
    CHAR                        szFoundName[_MAX_PATH];
    LPMODULEALIAS               Alias = NULL;
    DWORD                       i;
    IMAGEINFO                   ii;
    static int FakeDllNumber = 0;
    TCHAR FakeDllName[13];

    //
    // extern owned by kdapi.c:
    //
    extern DWORD CacheProcessors;
    static ULONG64 uMmSystemRangeStart = SE32To64(-1);


    ii.TimeStamp = -1;
    ii.SizeOfImage = -1;
    if ( hprc->pstate & (ps_killed | ps_dead) ) {
        //
        //  Process is dead, don't bother doing anything.
        //
        return FALSE;
    }


    assert( Is64PtrSE(ldd->lpBaseOfDll) );

    if ( *(LPTSTR)ldd->lpImageName == 0 ) {
        ldd->lpImageName = (UINT_PTR)FakeDllName;
        sprintf(FakeDllName, _T("DLL%05x"), FakeDllNumber++);
    }

    if (_ftcsicmp( (LPTSTR)ldd->lpImageName, HAL_IMAGE_NAME ) == 0) {
        Alias = CheckForRenamedImage( hprc, ldd, HAL_IMAGE_NAME, HAL_MODULE_NAME );
    }

    if (_ftcsicmp( (LPTSTR)ldd->lpImageName, KERNEL_IMAGE_NAME ) == 0) {
        Alias = CheckForRenamedImage( hprc, ldd, KERNEL_IMAGE_NAME, KERNEL_MODULE_NAME );
        if (!Alias && CacheProcessors > 1) {
            Alias = FindAddAliasByModule( KERNEL_IMAGE_NAME_MP, KERNEL_MODULE_NAME );
        }
    }

    if (!Alias) {
        Alias = FindAliasByImageName((LPTSTR)ldd->lpImageName);
    }

    //
    //  Create an entry in the DLL list and set the index to it so that
    //  we can have information about all DLLs for the current system.
    //
    for (iDll=0; iDll<hprc->cDllList; iDll+=1) {
        if (!hprc->rgDllList[iDll].fValidDll) {
            break;
        }
    }

    if (iDll == hprc->cDllList) {
        //
        // the dll list needs to be expanded
        //
        hprc->cDllList += 10;
        hprc->rgDllList = MHRealloc(hprc->rgDllList,
                                  hprc->cDllList * sizeof(DLLLOAD_ITEM));
        memset(&hprc->rgDllList[hprc->cDllList-10], 0, 10*sizeof(DLLLOAD_ITEM));
    } else {
        memset(&hprc->rgDllList[iDll], 0, sizeof(DLLLOAD_ITEM));
    }

    assert( Is64PtrSE(ldd->lpBaseOfDll) );
    hprc->rgDllList[iDll].fValidDll = TRUE;
    hprc->rgDllList[iDll].offBaseOfImage = (OFFSET) ldd->lpBaseOfDll;

    assert( Is64PtrSE(hprc->rgDllList[iDll].offBaseOfImage) );

    hprc->rgDllList[iDll].cbImage = ldd->dwDebugInfoFileOffset;
    if (Alias) {
        _splitpath( Alias->ModuleName, NULL, NULL, fname, ext );
    } else {
        _splitpath( (LPTSTR)ldd->lpImageName, NULL, NULL, fname, ext );
    }
    hprc->rgDllList[iDll].szDllName = MHAlloc(_ftcslen(fname)+_ftcslen(ext)+4);
    sprintf( hprc->rgDllList[iDll].szDllName, _T("%s%s"), fname, ext );
    hprc->rgDllList[iDll].NumberOfSections = 0;
    hprc->rgDllList[iDll].Sections = NULL;
    hprc->rgDllList[iDll].sec = NULL;

    *szFoundName = 0;

    LoadKdDataBlock();
    assert( Is64PtrSE(ldd->lpBaseOfDll) );

    if (uMmSystemRangeStart == SE32To64(-1)) {
        //
        // Has not yet been initialized
        //

        DWORD dwStatus;
        ULONG u32;
        ULONG64 u64;
            
        assert( Is64PtrSE(KdDebuggerData.MmSystemRangeStart) );
        
        dwStatus = DmKdReadMemoryWrapper(KdDebuggerData.MmSystemRangeStart,
                                         DmKdPtr64 ? (PVOID) &u64 : (PVOID) &u32,
                                         DmKdPtr64 ? sizeof(u64) : sizeof(u32),
                                         NULL
                                         );

        if (STATUS_SUCCESS == dwStatus) {
            if (DmKdPtr64) {
                uMmSystemRangeStart = u64;
            } else {
                uMmSystemRangeStart = SE32To64(u32);
            }
        }
    }

    //
    // Make sure uMmSystemRangeStart is not 0 and that it has been initialized
    //
    if (uMmSystemRangeStart
        && uMmSystemRangeStart != SE32To64(-1)
        && ldd->lpBaseOfDll < uMmSystemRangeStart) {
        //
        // must be a usermode module
        //
        if (ReadImageInfo( hprc->rgDllList[iDll].szDllName,
                           szFoundName,
                           (LPSTR)KdOptions[KDO_SYMBOLPATH].value,
                           &ii )) {
            //
            // we found the debug info, so now save the sections
            // this data is used by processgetsectionscmd()
            //
            hprc->rgDllList[iDll].NumberOfSections = ii.NumberOfSections;
            hprc->rgDllList[iDll].sec = ii.Sections;
            
            //
            // Always sign extend, no matter what.
            //
            assert( Is64PtrSE(ii.BaseOfImage) );
            hprc->rgDllList[iDll].offBaseOfImage = ii.BaseOfImage;
            hprc->rgDllList[iDll].cbImage = ii.SizeOfImage;
            ldd->hFile = (HANDLE)ii.CheckSum;
        }
    }

#ifdef TARGET_i386
    hprc->rgDllList[iDll].SegCs = (WORD) hthd->context.SegCs;
    hprc->rgDllList[iDll].SegDs = (WORD) hthd->context.SegDs;
#endif

    //
    //  Make up a record to send back from the name.
    //  Additionally send back:
    //          The file handle (if local)
    //          The load base of the dll
    //          The time and date stamp of the exe
    //          The checksum of the file
    //          ... and optionally the string "MP" to signal
    //              a multi-processor system to the symbol handler
    //
    *szModName = CMODULEDEMARCATOR;

    //
    // Send the name found by ReadImageInfo if it found an exe; if it
    // only found a dbg, send the default name.
    //
    _ftcscpy( szModName + 1,
            *szFoundName ? szFoundName : hprc->rgDllList[iDll].szDllName);
    lenSz=_ftcslen(szModName);
    szModName[lenSz] = CMODULEDEMARCATOR;
    
    if (ii.SizeOfImage == -1) {
        // Get the image size from LoadDll debug event
        ii.SizeOfImage = ldd->dwDebugInfoFileOffset;
    }
    if (ii.TimeStamp == -1) {
        IMAGE_NT_HEADERS64     NtHeaders;
        //
        // Get Image timestamp
        //
        
        ii.TimeStamp =  hprc->rgDllList[iDll].TimeStamp;

        if (!ii.TimeStamp) {
            ULONG SizeOfImage;

            //
            // Read from NT image header
            //
            DbgReadNtHeader(ldd->lpBaseOfDll, &NtHeaders);
            switch (NtHeaders.OptionalHeader.Magic) 
            {
            case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
                SizeOfImage = ((PIMAGE_NT_HEADERS32) &NtHeaders)->OptionalHeader.SizeOfImage;
                break;
            case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
                SizeOfImage = NtHeaders.OptionalHeader.SizeOfImage;
                break;
            }

            ii.TimeStamp = (hprc->rgDllList[iDll].TimeStamp = NtHeaders.FileHeader.TimeDateStamp);
            ii.SizeOfImage = SizeOfImage;
        }

    }
    
    sprintf( szModName+lenSz+1,_T("0x%08X%c0x%08X%c0x%016I64X%c0x%016I64X%c0x%08x%c"),
             ii.TimeStamp,                                 CMODULEDEMARCATOR,  // timestamp
             HandleToUlong(ldd->hFile),                    CMODULEDEMARCATOR,  // checksum
             (LONG64)-1,                                   CMODULEDEMARCATOR,  // File handle
             hprc->rgDllList[iDll].offBaseOfImage,         CMODULEDEMARCATOR,
             ii.SizeOfImage,                               CMODULEDEMARCATOR
           );

    if (Alias) {
        _ftcscat( szModName, Alias->Alias );
        lenSz = _ftcslen(szModName);
        szModName[lenSz] = CMODULEDEMARCATOR;
        szModName[lenSz+1] = 0;
        if (Alias->Special == 2) {
            // If it's a one-shot alias, nuke it.
            memset(Alias, 0, sizeof(MODULEALIAS));
        }
    }

    lenSz = _ftcslen(szModName);
    _ftcsupr(szModName);

    //
    // Allocate the packet which will be sent across to the EM.
    // The packet will consist of:
    //     The MDL structure                    sizeof(MDL) +
    //     The section description array        cobj*sizeof(OBJD) +
    //     The name of the DLL                  lenSz+1
    //
    *lpcbPacket = (WORD)(sizeof(MODULELOAD) + (lenSz+1));
    *lplpbPacket= (LPBYTE)(lpmdl=(LPMODULELOAD)MHAlloc(*lpcbPacket));
    ZeroMemory( lpmdl, *lpcbPacket );
    lpmdl->lpBaseOfDll = hprc->rgDllList[iDll].offBaseOfImage;

    assert( Is64PtrSE(lpmdl->lpBaseOfDll) );

    // mark the MDL packet as deferred:
    lpmdl->cobj = -1;
    lpmdl->mte = (WORD) -1;
#ifdef TARGET_i386
    lpmdl->CSSel    = (unsigned short)hthd->context.SegCs;
    lpmdl->DSSel    = (unsigned short)hthd->context.SegDs;
#else
    lpmdl->CSSel = lpmdl->DSSel = 0;
#endif

    lpmdl->fRealMode = 0;
    lpmdl->fFlatMode = 1;
    lpmdl->fOffset32 = 1;

    lpmdl->dwSizeOfDll = hprc->rgDllList[iDll].cbImage;

    lpmdl->fThreadIsStopped = fThreadIsStopped;

    //
    //  Copy the name of the dll to the end of the packet.
    //
    memcpy(((BYTE*)&lpmdl->rgobjd), szModName, lenSz+1);

    if (fDisconnected) {

        //
        // this will prevent the dm from sending a message up to
        // the shell.  the dm's data structures are setup just fine
        // so that when the debugger re-connects we can deliver the
        // mod loads correctly.
        //

        return FALSE;

    }

    return TRUE;
}                               /* LoadDll() */
