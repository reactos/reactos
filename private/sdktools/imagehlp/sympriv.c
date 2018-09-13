/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    symbolsp.c

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
#include <ntldr.h>
#include "private.h"
#include "symbols.h"
#include "tlhelp32.h"

typedef BOOL   (WINAPI *PMODULE32)(HANDLE, LPMODULEENTRY32);
typedef HANDLE (WINAPI *PCREATE32SNAPSHOT)(DWORD, DWORD);

typedef ULONG (NTAPI *PRTLQUERYPROCESSDEBUGINFORMATION)(HANDLE,ULONG,PRTL_DEBUG_INFORMATION);
typedef PRTL_DEBUG_INFORMATION (NTAPI *PRTLCREATEQUERYDEBUGBUFFER)(ULONG,BOOLEAN);
typedef NTSTATUS (NTAPI *PRTLDESTROYQUERYDEBUGBUFFER)(PRTL_DEBUG_INFORMATION);
typedef NTSTATUS (NTAPI *PNTQUERYSYSTEMINFORMATION)(SYSTEM_INFORMATION_CLASS,PVOID,ULONG,PULONG);
typedef ULONG (NTAPI *PRTLNTSTATUSTODOSERROR)(NTSTATUS);
//typedef NTSTATUS (NTAPI *PNTQUERYINFORMATIONPROCESS)(UINT_PTR,PROCESSINFOCLASS,UINT_PTR,ULONG,UINT_PTR);
typedef NTSTATUS (NTAPI *PNTQUERYINFORMATIONPROCESS)(HANDLE,PROCESSINFOCLASS,PVOID,ULONG,PULONG);

DWORD_PTR Win95GetProcessModules(HANDLE, PINTERNAL_GET_MODULE ,PVOID);
DWORD_PTR NTGetProcessModules(HANDLE, PINTERNAL_GET_MODULE ,PVOID);
DWORD64 miGetModuleBase(HANDLE hProcess, DWORD64 Address);

DWORD_PTR
NTGetPID(
    HANDLE hProcess
    )
{
    HMODULE hModule;
    PNTQUERYINFORMATIONPROCESS NtQueryInformationProcess;
    PROCESS_BASIC_INFORMATION pi;
    NTSTATUS status;

    hModule = GetModuleHandle( "ntdll.dll" );
    if (!hModule) {
        return ERROR_MOD_NOT_FOUND;
    }

    NtQueryInformationProcess = (PNTQUERYINFORMATIONPROCESS)GetProcAddress(
        hModule,
        "NtQueryInformationProcess"
        );

    if (!NtQueryInformationProcess) {
        return ERROR_INVALID_FUNCTION;
    }


    status = NtQueryInformationProcess(hProcess,
                                       ProcessBasicInformation,
                                       &pi,
                                       sizeof(pi),
                                       NULL);

    if (!NT_SUCCESS(status))
        return 0;

    return pi.UniqueProcessId;
}


//
// the block bounded by the #ifdef _X86_ statement
// contains the code for getting the PID from an
// HPROCESS when running under Win9X
//

#ifdef _X86_

#define HANDLE_INVALID  	       ((HANDLE)0xFFFFFFFF)
#define HANDLE_CURRENT_PROCESS	 ((HANDLE)0x7FFFFFFF)
#define HANDLE_CURRENT_THREAD    ((HANDLE)0xFFFFFFFE)
#define MAX_HANDLE_VALUE         ((HANDLE)0x00FFFFFF) 


// Thread Information Block.

typedef struct _TIB {

    DWORD     unknown[12];
    DWORD_PTR ppdb;

} TIB, *PTIB;

// Task Data Block

typedef struct _TDB {

    DWORD unknown[2];
    TIB		tib;
    
} TDB, *PTDB;

typedef struct _OBJ {

    BYTE    typObj;             // object type 
    BYTE    objFlags;           // object flags
    WORD    cntUses;            // count of this objects usage

} OBJ, *POBJ;
  
typedef struct _HTE {

    DWORD   flFlags;
    POBJ    pobj;
    
} HTE, *PHTE;

typedef struct _HTB {

    DWORD   chteMax;
    HTE     rghte[1];
    
} HTB, *PHTB;

typedef struct _W9XPDB {

    DWORD  unknown[17];
    PHTB   phtbHandles;

} W9XPDB, *PW9XPDB;

#pragma warning(disable:4035)

_inline struct _TIB * GetCurrentTib(void) { _asm mov eax, fs:[0x18] }

// stuff needed to convert local handle

#define IHTETOHANDLESHIFT  2
#define GLOBALHANDLEMASK  (0x453a4d3cLU)

#define IHTEFROMHANDLE(hnd) ((hnd) == HANDLE_INVALID ? (DWORD)(hnd) : (((DWORD)(hnd)) >> IHTETOHANDLESHIFT))

#define IHTEISGLOBAL(ihte) \
        (((ihte) >> (32 - 8 - IHTETOHANDLESHIFT)) == (((DWORD)GLOBALHANDLEMASK) >> 24))

#define IS_WIN32_PREDEFINED_HANDLE(hnd) \
        ((hnd == HANDLE_CURRENT_PROCESS)||(hnd == HANDLE_CURRENT_THREAD)||(hnd == HANDLE_INVALID))

DWORD
GetWin9xObsfucator(
  VOID
  )
/*++

Routine Description:

  GetWin9xObsfucator()


Arguments:

  none


Return Value:

  Obsfucator key used by Windows9x to hide Process and Thread Id's


Notes:

  The code has only been tested on Windows98SE and Millennium.
  

--*/
{
    DWORD ppdb       = 0;      // W9XPDB = Process Data Block
    DWORD processId  = (DWORD) GetCurrentProcessId();
  
    // get PDB pointer
  
    ppdb = GetCurrentTib()->ppdb;
    
    return ppdb ^ processId;
}


DWORD_PTR
GetPtrFromHandle(
  IN HANDLE Handle
  )
/*++

Routine Description:

  GetPtrFromHandle()


Arguments:

  Handle - handle from Process handle table


Return Value:

  Real Pointer to object


Notes:

  The code has only been tested on Windows98SE and Millennium.
  

--*/
{
    DWORD_PTR ptr  = 0;
    DWORD     ihte = 0;
    PW9XPDB   ppdb = 0;
  
    ppdb = (PW9XPDB) GetCurrentTib()->ppdb;
  
    // check for pre-defined handle values.
  
    if (Handle == HANDLE_CURRENT_PROCESS) {
        ptr = (DWORD_PTR) ppdb;
    } else if (Handle == HANDLE_CURRENT_THREAD) {
        ptr = (DWORD_PTR) CONTAINING_RECORD(GetCurrentTib(), TDB, tib);
    } else if (Handle == HANDLE_INVALID) {
        ptr = 0;
    } else {
        // not a special handle, we can perform our magic.
    
        ihte = IHTEFROMHANDLE(Handle);

        // if we have a global handle, it is only meaningful in the context
        // of the kernel process's handle table...we don't currently deal with
        // this type of handle
    
        if (!(IHTEISGLOBAL(ihte))) {
            ptr = (DWORD_PTR) ppdb->phtbHandles->rghte[ihte].pobj;
        }
    }

    return ptr;
}


DWORD_PTR
Win9xGetPID(
  IN HANDLE hProcess
  )
/*++

Routine Description:

  Win9xGetPid()


Arguments:

  hProcess - Process handle


Return Value:

  Process Id


Notes:

  The code has only been tested on Windows98SE and Millennium.
  

--*/
{
    static DWORD dwObsfucator = 0;

    // check to see that we have a predefined handle or an index into
    // our local handle table.

    if (IS_WIN32_PREDEFINED_HANDLE(hProcess) || (hProcess < MAX_HANDLE_VALUE)) {
        if (!dwObsfucator) {
            dwObsfucator = GetWin9xObsfucator();
            assert(dwObsfucator != 0);
        }
        return dwObsfucator ^ GetPtrFromHandle(hProcess);  
    }

    // don't know what we have here

    return 0;
}

#endif // _X86_


DWORD_PTR
GetPID(
    HANDLE hProcess
    )
{
    OSVERSIONINFO VerInfo;

    if (hProcess == GetCurrentProcess())
        return GetCurrentProcessId();

    VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&VerInfo);
    if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        return NTGetPID(hProcess);
    } else {
#ifdef _X86_
        return Win9xGetPID(hProcess);
#else
        return 0;
#endif
    }
}


DWORD
GetProcessModules(
    HANDLE                  hProcess,
    PINTERNAL_GET_MODULE    InternalGetModule,
    PVOID                   Context
    )
{
#ifdef _X86_
    OSVERSIONINFO VerInfo;

    VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&VerInfo);
    if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        return NTGetProcessModules(hProcess, InternalGetModule, Context);
    } else {
        return Win95GetProcessModules(hProcess, InternalGetModule, Context);
    }
}


DWORD
Win95GetProcessModules(
    HANDLE                  hProcess,
    PINTERNAL_GET_MODULE    InternalGetModule,
    PVOID                   Context
    )
{
    MODULEENTRY32 ModuleEntry;
    PMODULE32     pModule32Next, pModule32First;
    PCREATE32SNAPSHOT pCreateToolhelp32Snapshot;
    HANDLE hSnapshot;
    HMODULE hToolHelp;
    DWORD pid;

    // get the PID:
    // this hack supports old bug workaround, in which callers were passing
    // a pid, because an hprocess didn't work on W9X.

    pid = GetPID(hProcess);
    if (!pid)
        pid = (DWORD)hProcess;

    // get the module list from toolhelp apis

    hToolHelp = GetModuleHandle("kernel32.dll");
    if (!hToolHelp)
        return ERROR_MOD_NOT_FOUND;

    pModule32Next = (PMODULE32)GetProcAddress(hToolHelp, "Module32Next");
    pModule32First = (PMODULE32)GetProcAddress(hToolHelp, "Module32First");
    pCreateToolhelp32Snapshot = (PCREATE32SNAPSHOT)GetProcAddress(hToolHelp, "CreateToolhelp32Snapshot");
    if (!pModule32Next || !pModule32First || !pCreateToolhelp32Snapshot)
        return ERROR_MOD_NOT_FOUND;

    hSnapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid); 
    if (hSnapshot == (HANDLE)-1) {
        return ERROR_MOD_NOT_FOUND;
    }

    ModuleEntry.dwSize = sizeof(MODULEENTRY32);

    if (pModule32First(hSnapshot, &ModuleEntry)) {
        do
        {
            if (!InternalGetModule(
                    hProcess,
                    ModuleEntry.szModule,
                    (DWORD) ModuleEntry.modBaseAddr,
                    ModuleEntry.modBaseSize,
                    Context))
            {
                break;
            }

        } while ( pModule32Next(hSnapshot, &ModuleEntry) );
    }

    CloseHandle(hSnapshot);

    return(ERROR_SUCCESS);
}


DWORD
NTGetProcessModules(
    HANDLE                  hProcess,
    PINTERNAL_GET_MODULE    InternalGetModule,
    PVOID                   Context
    )
{

#endif      // _X86_

    PRTLQUERYPROCESSDEBUGINFORMATION    RtlQueryProcessDebugInformation;
    PRTLCREATEQUERYDEBUGBUFFER          RtlCreateQueryDebugBuffer;
    PRTLDESTROYQUERYDEBUGBUFFER         RtlDestroyQueryDebugBuffer;
    HMODULE                             hModule;
    NTSTATUS                            Status;
    PRTL_DEBUG_INFORMATION              Buffer;
    ULONG                               i;
    DWORD_PTR                           ProcessId;

    hModule = GetModuleHandle( "ntdll.dll" );
    if (!hModule) {
        return ERROR_MOD_NOT_FOUND;
    }

    RtlQueryProcessDebugInformation = (PRTLQUERYPROCESSDEBUGINFORMATION)GetProcAddress(
        hModule,
        "RtlQueryProcessDebugInformation"
        );

    if (!RtlQueryProcessDebugInformation) {
        return ERROR_INVALID_FUNCTION;
    }

    RtlCreateQueryDebugBuffer = (PRTLCREATEQUERYDEBUGBUFFER)GetProcAddress(
        hModule,
        "RtlCreateQueryDebugBuffer"
        );

    if (!RtlCreateQueryDebugBuffer) {
        return ERROR_INVALID_FUNCTION;
    }

    RtlDestroyQueryDebugBuffer = (PRTLDESTROYQUERYDEBUGBUFFER)GetProcAddress(
        hModule,
        "RtlDestroyQueryDebugBuffer"
        );

    if (!RtlDestroyQueryDebugBuffer) {
        return ERROR_INVALID_FUNCTION;
    }

    Buffer = RtlCreateQueryDebugBuffer( 0, FALSE );
    if (!Buffer) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ProcessId = GetPID(hProcess);

    // for backwards compatibility with an old bug
    if (!ProcessId)
        ProcessId = (DWORD_PTR)hProcess;

    Status = RtlQueryProcessDebugInformation(
        (HANDLE)ProcessId,
        RTL_QUERY_PROCESS_MODULES,
        Buffer
        );

    if (Status != STATUS_SUCCESS) {
        RtlDestroyQueryDebugBuffer( Buffer );
        return(ImagepSetLastErrorFromStatus(Status));
    }

    for (i=0; i<Buffer->Modules->NumberOfModules; i++) {
        PRTL_PROCESS_MODULE_INFORMATION Module = &Buffer->Modules->Modules[i];
        if (!InternalGetModule(
                hProcess,
                (LPSTR) &Module->FullPathName[Module->OffsetToFileName],
                (DWORD64)Module->ImageBase,
                (DWORD)Module->ImageSize,
                Context
                ))
        {
            break;
        }
    }

    RtlDestroyQueryDebugBuffer( Buffer );
    return ERROR_SUCCESS;
}


VOID
FreeModuleEntry(
    PMODULE_ENTRY ModuleEntry
    )
{
    if (ModuleEntry->symbolTable) {
        MemFree( ModuleEntry->symbolTable  );
    }
    if (ModuleEntry->SymType == SymPdb) {
        TPI *tpi;
        if ( ModuleEntry->ptpi ) {
            TypesClose(ModuleEntry->ptpi);
        } else if (PDBOpenTpi(ModuleEntry->pdb, pdbRead, &tpi)){
            TypesClose(tpi);
        }
        GSIClose( ModuleEntry->globals );
        GSIClose( ModuleEntry->gsi );
        DBIClose( ModuleEntry->dbi );
        PDBClose( ModuleEntry->pdb );
    }
    if (ModuleEntry->SectionHdrs) {
        MemFree( ModuleEntry->SectionHdrs );
    }
    if (ModuleEntry->OriginalSectionHdrs) {
        MemFree( ModuleEntry->OriginalSectionHdrs );
    }
    if (ModuleEntry->pFpoData) {
        VirtualFree( ModuleEntry->pFpoData, 0, MEM_RELEASE );
    }
    if (ModuleEntry->pExceptionData) {
        VirtualFree( ModuleEntry->pExceptionData, 0, MEM_RELEASE );
    }
    if (ModuleEntry->TmpSym.Name) {
        MemFree( ModuleEntry->TmpSym.Name );
    }
    if (ModuleEntry->ImageName) {
        MemFree( ModuleEntry->ImageName );
    }
    if (ModuleEntry->LoadedImageName) {
        MemFree( ModuleEntry->LoadedImageName );
    }
    if (ModuleEntry->pOmapTo) {
        MemFree( ModuleEntry->pOmapTo );
    }
    if (ModuleEntry->pOmapFrom) {
        MemFree( ModuleEntry->pOmapFrom );
    }
    if (ModuleEntry->SourceFiles) {
        PSOURCE_ENTRY Src, SrcNext;

        for (Src = ModuleEntry->SourceFiles; Src != NULL; Src = SrcNext) {
            SrcNext = Src->Next;
            MemFree(Src);
        }
    }
    MemFree( ModuleEntry );
}




BOOL __inline
MatchSymbolName(
    PSYMBOL_ENTRY       sym,
    LPSTR               SymName
    )
{
    if (SymOptions & SYMOPT_CASE_INSENSITIVE) {
        if (_stricmp( sym->Name, SymName ) == 0) {
            return TRUE;
        }
    } else {
        if (strcmp( sym->Name, SymName ) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}


PSYMBOL_ENTRY
HandleDuplicateSymbols(
    PPROCESS_ENTRY  ProcessEntry,
    PMODULE_ENTRY   mi,
    PSYMBOL_ENTRY   sym
    )
{
    DWORD                       i;
    DWORD                       Dups;
    DWORD                       NameSize;
    PIMAGEHLP_SYMBOL64          Syms64 = NULL;
    PIMAGEHLP_SYMBOL          Syms32 = NULL;
    PIMAGEHLP_DUPLICATE_SYMBOL64 DupSym64 = NULL;
    PIMAGEHLP_DUPLICATE_SYMBOL DupSym32 = NULL;
    PULONG                      SymSave;


    if (!ProcessEntry->pCallbackFunction32 && !ProcessEntry->pCallbackFunction64) {
        return sym;
    }

    if (!(sym->Flags & SYMF_DUPLICATE)) {
        return sym;
    }

    Dups = 0;
    NameSize = 0;
    for (i = 0; i < mi->numsyms; i++) {
        if ((mi->symbolTable[i].NameLength == sym->NameLength) &&
            (strcmp( mi->symbolTable[i].Name, sym->Name ) == 0)) {
                Dups += 1;
                NameSize += (mi->symbolTable[i].NameLength + 1);
        }
    }

    if (ProcessEntry->pCallbackFunction32) {
        DupSym32 = (PIMAGEHLP_DUPLICATE_SYMBOL) MemAlloc( sizeof(IMAGEHLP_DUPLICATE_SYMBOL) );
        if (!DupSym32) {
            return sym;
        }

        Syms32 = (PIMAGEHLP_SYMBOL) MemAlloc( (sizeof(IMAGEHLP_SYMBOL) * Dups) + NameSize );
        if (!Syms32) {
            MemFree( DupSym32 );
            return sym;
        }

        SymSave = (PULONG) MemAlloc( sizeof(ULONG) * Dups );
        if (!SymSave) {
            MemFree( Syms32 );
            MemFree( DupSym32 );
            return sym;
        }

        DupSym32->SizeOfStruct    = sizeof(IMAGEHLP_DUPLICATE_SYMBOL);
        DupSym32->NumberOfDups    = Dups;
        DupSym32->Symbol          = Syms32;
        DupSym32->SelectedSymbol  = (ULONG) -1;

        Dups = 0;
        for (i = 0; i < mi->numsyms; i++) {
            if ((mi->symbolTable[i].NameLength == sym->NameLength) &&
                (strcmp( mi->symbolTable[i].Name, sym->Name ) == 0)) {
                    symcpy32( Syms32, &mi->symbolTable[i] );
                    Syms32 += (sizeof(IMAGEHLP_SYMBOL) + mi->symbolTable[i].NameLength + 1);
                    SymSave[Dups] = i;
                    Dups += 1;
            }
        }

    } else {
        DupSym64 = (PIMAGEHLP_DUPLICATE_SYMBOL64) MemAlloc( sizeof(IMAGEHLP_DUPLICATE_SYMBOL64) );
        if (!DupSym64) {
            return sym;
        }

        Syms64 = (PIMAGEHLP_SYMBOL64) MemAlloc( (sizeof(IMAGEHLP_SYMBOL64) * Dups) + NameSize );
        if (!Syms64) {
            MemFree( DupSym64 );
            return sym;
        }

        SymSave = (PULONG) MemAlloc( sizeof(ULONG) * Dups );
        if (!SymSave) {
            MemFree( Syms64 );
            MemFree( DupSym64 );
            return sym;
        }

        DupSym64->SizeOfStruct    = sizeof(IMAGEHLP_DUPLICATE_SYMBOL64);
        DupSym64->NumberOfDups    = Dups;
        DupSym64->Symbol          = Syms64;
        DupSym64->SelectedSymbol  = (ULONG) -1;

        Dups = 0;
        for (i = 0; i < mi->numsyms; i++) {
            if ((mi->symbolTable[i].NameLength == sym->NameLength) &&
                (strcmp( mi->symbolTable[i].Name, sym->Name ) == 0)) {
                    symcpy64( Syms64, &mi->symbolTable[i] );
                    Syms64 += (sizeof(IMAGEHLP_SYMBOL64) + mi->symbolTable[i].NameLength + 1);
                    SymSave[Dups] = i;
                    Dups += 1;
            }
        }

    }

    sym = NULL;

    __try {

        if (ProcessEntry->pCallbackFunction32) {
            ProcessEntry->pCallbackFunction32(
                ProcessEntry->hProcess,
                CBA_DUPLICATE_SYMBOL,
                (PVOID) DupSym32,
                (PVOID) ProcessEntry->CallbackUserContext
                );

            if (DupSym32->SelectedSymbol != (ULONG) -1) {
                if (DupSym32->SelectedSymbol < DupSym32->NumberOfDups) {
                    sym = &mi->symbolTable[SymSave[DupSym32->SelectedSymbol]];
                }
            }
        } else {
            ProcessEntry->pCallbackFunction64(
                ProcessEntry->hProcess,
                CBA_DUPLICATE_SYMBOL,
                (ULONG64) &DupSym64,
                ProcessEntry->CallbackUserContext
                );

            if (DupSym64->SelectedSymbol != (ULONG) -1) {
                if (DupSym64->SelectedSymbol < DupSym64->NumberOfDups) {
                    sym = &mi->symbolTable[SymSave[DupSym64->SelectedSymbol]];
                }
            }
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ;
    }

    if (DupSym32) {
        MemFree( DupSym32 );
    }
    if (DupSym64) {
        MemFree( DupSym64 );
    }
    if (Syms32) {
        MemFree( Syms32 );
    }
    if (Syms64) {
        MemFree( Syms64 );
    }
    MemFree( SymSave );

    return sym;
}


PSYMBOL_ENTRY
FindSymbolByName(
    PPROCESS_ENTRY  ProcessEntry,
    PMODULE_ENTRY   mi,
    LPSTR           SymName
    )
{
    DWORD               Hash;
    PSYMBOL_ENTRY       sym;
    DWORD               i;
    LPSTR               name;
    LPSTR               p;
    int                 rslt;

    if (mi->SymType == SymPdb) {
        DATASYM32 *dataSym = (DATASYM32*)GSINextSym( mi->gsi, NULL );
        PIMAGE_SECTION_HEADER sh;
        DWORD64 addr;
        ULONG k;
        LPSTR PdbSymbolName;
        UCHAR PdbSymbolLen;
        while( dataSym ) {
            PdbSymbolName = DataSymNameStart(dataSym);
            PdbSymbolLen  = DataSymNameLength(dataSym);
            addr = 0;
            k = DataSymSeg(dataSym);

            if ((k <= mi->OriginalNumSections)) {
                addr = mi->OriginalSectionHdrs[k-1].VirtualAddress + DataSymOffset(dataSym) + mi->BaseOfDll;

                if (SymOptions & SYMOPT_UNDNAME) {
                    SymUnDNameInternal( mi->TmpSym.Name, TMP_SYM_LEN-sizeof(mi->TmpSym), PdbSymbolName, PdbSymbolLen );
                } else {
                    mi->TmpSym.Name[0] = 0;
                    strncat( mi->TmpSym.Name, PdbSymbolName, __min(PdbSymbolLen, TMP_SYM_LEN-sizeof(mi->TmpSym)) );
                }
                if (SymOptions & SYMOPT_CASE_INSENSITIVE) {
                    rslt = _stricmp( mi->TmpSym.Name, SymName );
                } else {
                    rslt = strcmp( mi->TmpSym.Name, SymName );
                }
                if (rslt == 0) {
                    DWORD Bias;

                    addr = ConvertOmapFromSrc( mi, addr, &Bias );

                    if (addr != 0) {
                        addr += Bias;
                    }

                    mi->TmpSym.Size = 0;
                    mi->TmpSym.Flags = 0;
                    mi->TmpSym.Address = addr;
                    mi->TmpSym.NameLength = 0;

                    return &mi->TmpSym;
                }
            }
            dataSym = (DATASYM32*)GSINextSym( mi->gsi, (PUCHAR)dataSym );
        }
        return NULL;
    }

    Hash = ComputeHash( SymName, strlen(SymName) );
    sym = mi->NameHashTable[Hash];

    if (sym) {
        //
        // there are collision(s) so lets walk the
        // collision list and match the names
        //
        while( sym ) {
            if (MatchSymbolName( sym, SymName )) {
                sym = HandleDuplicateSymbols( ProcessEntry, mi, sym );
                return sym;
            }
            sym = sym->Next;
        }
    }

    //
    // the symbol did not hash to anything valid
    // this is possible if the caller passed an undecorated name
    // now we must look linearly thru the list
    //
    for (i=0; i<mi->numsyms; i++) {
        sym = &mi->symbolTable[i];
        if (MatchSymbolName( sym, SymName )) {
            sym = HandleDuplicateSymbols( ProcessEntry, mi, sym );
            return sym;
        }
    }

    return NULL;
}


IMGHLP_RVA_FUNCTION_DATA *
SearchRvaFunctionTable(
    IMGHLP_RVA_FUNCTION_DATA *FunctionTable,
    LONG High,
    LONG Low,
    DWORD dwPC
    )
{
    LONG    Middle;
    IMGHLP_RVA_FUNCTION_DATA *FunctionEntry;

    // Perform binary search on the function table for a function table
    // entry that subsumes the specified PC.

    while (High >= Low) {

        // Compute next probe index and test entry. If the specified PC
        // is greater than of equal to the beginning address and less
        // than the ending address of the function table entry, then
        // return the address of the function table entry. Otherwise,
        // continue the search.

        Middle = (Low + High) >> 1;
        FunctionEntry = &FunctionTable[Middle];
        if (dwPC < FunctionEntry->rvaBeginAddress) {
            High = Middle - 1;

        } else if (dwPC >= FunctionEntry->rvaEndAddress) {
            Low = Middle + 1;

        } else {
            return FunctionEntry;
        }
    }
    return NULL;
}

PIMAGE_FUNCTION_ENTRY
LookupFunctionEntryAxp32 (
    PMODULE_ENTRY mi,
    DWORD         ControlPc
    )
{
    IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY *FunctionEntry;
    static IMAGE_FUNCTION_ENTRY ife;

    // Don't specify the function table access callback or it will cause recursion. 

    FunctionEntry = LookupFunctionEntry(mi->hProcess, ControlPc, ReadInProcMemory, miGetModuleBase, NULL, FALSE );
    
    if (!FunctionEntry)
        return NULL;

    ife.StartingAddress = (ULONG)FunctionEntry->BeginAddress;
    ife.EndingAddress   = (ULONG)FunctionEntry->EndAddress;
    ife.EndOfPrologue   = (ULONG)FunctionEntry->PrologEndAddress;

    return &ife;
} // LookupFunctionEntryAxp32()


PIMAGE_FUNCTION_ENTRY64
LookupFunctionEntryAxp64 (
    PMODULE_ENTRY mi,
    DWORD64       ControlPc
    )
{
    IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY *FunctionEntry;
    static IMAGE_FUNCTION_ENTRY64 ife64;

    // Don't specify the function table access callback or it will cause recursion. 

    FunctionEntry = LookupFunctionEntry(mi->hProcess, ControlPc, ReadInProcMemory, miGetModuleBase, NULL, TRUE );
    
    if (!FunctionEntry == 0)
        return NULL;

    ife64.StartingAddress = FunctionEntry->BeginAddress;
    ife64.EndingAddress   = FunctionEntry->EndAddress;
    ife64.EndOfPrologue   = FunctionEntry->PrologEndAddress;

    return &ife64;

} // LookupFunctionEntryAxp64()

PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY
GetAlphaFunctionEntryFromDebugInfo (
    PPROCESS_ENTRY  ProcessEntry,
    DWORD64         ControlPc
    )
{
    PMODULE_ENTRY mi;
    IMGHLP_RVA_FUNCTION_DATA   *FunctionTable;
    IMGHLP_RVA_FUNCTION_DATA   *FunctionEntry;
    static IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY rfe64;

    mi = GetModuleForPC( ProcessEntry, ControlPc, FALSE );
    if (mi == NULL) {
        return NULL;
    }

    if (!GetPData(ProcessEntry->hProcess, mi))
        return NULL;

    FunctionTable = (IMGHLP_RVA_FUNCTION_DATA *)mi->pExceptionData;
    FunctionEntry = SearchRvaFunctionTable(FunctionTable, mi->dwEntries - 1, 0, (ULONG)(ControlPc - mi->BaseOfDll));
    
    if (!FunctionEntry) {
        return NULL;
    }
    
    rfe64.BeginAddress      = mi->BaseOfDll + FunctionEntry->rvaBeginAddress;
    rfe64.EndAddress        = mi->BaseOfDll + FunctionEntry->rvaEndAddress;
    rfe64.ExceptionHandler  = 0;
    rfe64.HandlerData       = 0;
    rfe64.PrologEndAddress  = mi->BaseOfDll + FunctionEntry->rvaPrologEndAddress;

    return &rfe64;
}

PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY
LookupFunctionEntryIA64 (
    PMODULE_ENTRY mi,
    DWORD         ControlPc
    )
{
    IMGHLP_RVA_FUNCTION_DATA   *FunctionTable;
    IMGHLP_RVA_FUNCTION_DATA   *FunctionEntry;
    static IMAGE_IA64_RUNTIME_FUNCTION_ENTRY irfe;

// TF-FIXFIX
if (!GetPData(NULL, mi))
   return NULL;

    FunctionTable = (IMGHLP_RVA_FUNCTION_DATA *)mi->pExceptionData;
    FunctionEntry = SearchRvaFunctionTable(FunctionTable, mi->dwEntries - 1, 0, ControlPc);

    if (!FunctionEntry) {
        return NULL;
    }

    irfe.BeginAddress      = FunctionEntry->rvaBeginAddress;
    irfe.EndAddress        = FunctionEntry->rvaEndAddress;
    irfe.UnwindInfoAddress = FunctionEntry->rvaPrologEndAddress;

    return &irfe;

} // LookupFunctionEntryIA64()

PFPO_DATA
SwSearchFpoData(
    DWORD     key,
    PFPO_DATA base,
    DWORD     num
    )
{
    PFPO_DATA  lo = base;
    PFPO_DATA  hi = base + (num - 1);
    PFPO_DATA  mid;
    DWORD      half;

    while (lo <= hi) {
        if (half = num / 2) {
            mid = lo + ((num & 1) ? half : (half - 1));
            if ((key >= mid->ulOffStart)&&(key < (mid->ulOffStart+mid->cbProcSize))) {
                return mid;
            }
            if (key < mid->ulOffStart) {
                hi = mid - 1;
                num = (num & 1) ? half : half-1;
            } else {
                lo = mid + 1;
                num = half;
            }
        } else
        if (num) {
            if ((key >= lo->ulOffStart)&&(key < (lo->ulOffStart+lo->cbProcSize))) {
                return lo;
            } else {
                break;
            }
        } else {
            break;
        }
    }
    return(NULL);
}

BOOL
DoSymbolCallback (
    PPROCESS_ENTRY                  ProcessEntry,
    ULONG                           CallbackType,
    IN  PMODULE_ENTRY               mi,
    PIMAGEHLP_DEFERRED_SYMBOL_LOAD64 idsl64,
    LPSTR                           FileName
    )
{
    BOOL Status;
    IMAGEHLP_DEFERRED_SYMBOL_LOAD idsl32;

    Status = FALSE;
    if (ProcessEntry->pCallbackFunction32) {
        idsl32.SizeOfStruct  = sizeof(IMAGEHLP_DEFERRED_SYMBOL_LOAD);
        idsl32.BaseOfImage   = (ULONG)mi->BaseOfDll;
        idsl32.CheckSum      = mi->CheckSum;
        idsl32.TimeDateStamp = mi->TimeDateStamp;
        idsl32.Reparse       = FALSE;
        idsl32.FileName[0] = 0;
        if (FileName) {
            strncat( idsl32.FileName, FileName, MAX_PATH );
        }

        __try {

            Status = ProcessEntry->pCallbackFunction32(
                        ProcessEntry->hProcess,
                        CallbackType,
                        (PVOID)&idsl32,
                        (PVOID)ProcessEntry->CallbackUserContext
                        );
            idsl64->SizeOfStruct = sizeof(IMAGEHLP_DEFERRED_SYMBOL_LOAD64);
            idsl64->BaseOfImage = idsl32.BaseOfImage;
            idsl64->CheckSum = idsl32.CheckSum;
            idsl64->TimeDateStamp = idsl32.TimeDateStamp;
            idsl64->Reparse = idsl32.Reparse;
            if (idsl32.FileName) {
                strncpy( idsl64->FileName, idsl32.FileName, MAX_PATH );
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    } else
    if (ProcessEntry->pCallbackFunction64) {
        idsl64->SizeOfStruct  = sizeof(IMAGEHLP_DEFERRED_SYMBOL_LOAD64);
        idsl64->BaseOfImage   = mi->BaseOfDll;
        idsl64->CheckSum      = mi->CheckSum;
        idsl64->TimeDateStamp = mi->TimeDateStamp;
        idsl64->Reparse       = FALSE;
        idsl64->FileName[0] = 0;
        if (FileName) {
            strncat( idsl64->FileName, FileName, MAX_PATH );
        }

        __try {

            Status = ProcessEntry->pCallbackFunction64(
                        ProcessEntry->hProcess,
                        CallbackType,
                        (ULONG64)(ULONG_PTR)idsl64,
                        ProcessEntry->CallbackUserContext
                        );

        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    return Status;
}

VOID
SympSendDebugString(
    PPROCESS_ENTRY ProcessEntry,
    LPSTR          String
    )
{
    DWORD Status;
    __try {
        if (!ProcessEntry) 
            ProcessEntry = FindFirstProcessEntry();
        if (!ProcessEntry)
            return;
        
        if (ProcessEntry->pCallbackFunction32) {
            Status = ProcessEntry->pCallbackFunction32(
                        ProcessEntry->hProcess,
                        CBA_DEBUG_INFO,
                        (PVOID)String,
                        (PVOID)ProcessEntry->CallbackUserContext
                        );
        } else
        if (ProcessEntry->pCallbackFunction64) {
            Status = ProcessEntry->pCallbackFunction64(
                        ProcessEntry->hProcess,
                        CBA_DEBUG_INFO,
                        (ULONG64)String,
                        ProcessEntry->CallbackUserContext
                        );

        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

int
WINAPIV
SympDprintf(
    PPROCESS_ENTRY ProcessEntry,
    LPSTR Format,
    ...
    )
{
    static char buf[1000] = "DBGHELP: ";
    va_list args;

    va_start(args, Format);
    _vsnprintf(buf+9, sizeof(buf)-9, Format, args);
    va_end(args);
    SympSendDebugString(ProcessEntry, buf);
    return 1;
}

int
WINAPIV
SympEprintf(
    PPROCESS_ENTRY ProcessEntry,
    LPSTR Format,
    ...
    )
{
    static char buf[1000] = "";
    va_list args;

    va_start(args, Format);
    _vsnprintf(buf, sizeof(buf), Format, args);
    va_end(args);
    SympSendDebugString(ProcessEntry, buf);
    return 1;
}


BOOL
CompleteDeferredSymbolLoad(
    IN  HANDLE          hProcess,
    IN  PMODULE_ENTRY   mi
    )
{
    IMAGEHLP_DEFERRED_SYMBOL_LOAD64 idsl;
    PPROCESS_ENTRY              ProcessEntry;
    ULONG                       i;
    PIMGHLP_DEBUG_DATA          pIDD;
    ULONG                       bias;
    PIMAGE_SYMBOL               lpSymbolEntry;
    PUCHAR                      lpStringTable;
    PUCHAR                      p;
    BOOL                        SymbolsLoaded = FALSE;
    PUCHAR                      CallbackFileName, ImageName;
    ULONG                       Size;

    ProcessEntry = FindProcessEntry( hProcess );
    if (!ProcessEntry) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    CallbackFileName   = mi->LoadedImageName ? mi->LoadedImageName :
                                mi->ImageName ? mi->ImageName : mi->ModuleName;

    DoSymbolCallback(
        ProcessEntry,
        CBA_DEFERRED_SYMBOL_LOAD_START,
        mi,
        &idsl,
        CallbackFileName
        );

    ImageName = mi->ImageName;
    for (; ;) {
        pIDD = ImgHlpFindDebugInfo(
            hProcess,
            mi->hFile,
            ImageName,
            ProcessEntry->SymbolSearchPath,
            mi->BaseOfDll,
            0
            );

        if (pIDD) {
            break;
        }

        DPRINTF(ProcessEntry, "ImgHlpFindDebugInfo(%p, %s, %s, %I64x, 0) failed\n",
            mi->hFile,
            ImageName,
            ProcessEntry->SymbolSearchPath,
            mi->BaseOfDll
            );

        if (!DoSymbolCallback(
                 ProcessEntry,
                 CBA_DEFERRED_SYMBOL_LOAD_FAILURE,
                 mi,
                 &idsl,
                 CallbackFileName
                 ) || !idsl.Reparse) {
            mi->SymType = SymNone;
            mi->Flags |= MIF_NO_SYMBOLS;
            return FALSE;
        }

        ImageName = idsl.FileName;
        CallbackFileName = idsl.FileName;
    }

    // The following code ONLY works if the dll wasn't rebased
    // during install.  Is it really useful?

    if (!mi->BaseOfDll) {
        //
        // This case occurs when modules are loaded multiple times by
        // name with no explicit base address.
        //
        if (GetModuleForPC( ProcessEntry, pIDD->ImageBaseFromImage, TRUE )) {
            if (pIDD->ImageBaseFromImage) {
                DPRINTF(ProcessEntry, "GetModuleForPC(%p, %I64x, TRUE) failed\n",
                    ProcessEntry,
                    pIDD->ImageBaseFromImage,
                    TRUE
                    );
            } else {
                DPRINTF(ProcessEntry, "No base address for %s:  Please specify\n", ImageName);
            }
            return FALSE;
        }
        mi->BaseOfDll    = pIDD->ImageBaseFromImage;
    }

    if (!mi->DllSize) {
        mi->DllSize      = pIDD->SizeOfImage;
    }

    mi->hProcess         = pIDD->hProcess;
    mi->InProcImageBase  = pIDD->InProcImageBase;

    mi->CheckSum         = pIDD->CheckSum;
    mi->TimeDateStamp    = pIDD->TimeDateStamp;
    mi->MachineType      = pIDD->Machine;
    if (pIDD->DbgFileMap) {
        mi->LoadedImageName = StringDup(pIDD->DbgFilePath);
    } else if (*pIDD->ImageFilePath) {
        mi->LoadedImageName = StringDup(pIDD->ImageFilePath);
    } else if (pIDD->pPdb) {
        mi->LoadedImageName = StringDup(pIDD->PdbFileName);
    } else {
        mi->LoadedImageName = StringDup("");
    }

    if (pIDD->fROM) {
        mi->Flags |= MIF_ROM_IMAGE;
    }

    if (!mi->ImageName) {
        mi->ImageName = StringDup(pIDD->OriginalImageFileName);
        _splitpath( mi->ImageName, NULL, NULL, mi->ModuleName, NULL );
        mi->AliasName[0] = 0;
    }

    mi->dsExceptions = pIDD->dsExceptions;

    if (pIDD->cFpo) {
        //
        // use virtualalloc() because the rtf search function
        // return a pointer into this memory.  we want to make
        // all of this memory read only so that callers cannot
        // stomp on imagehlp's data
        //
        mi->pFpoData = VirtualAlloc(
            NULL,
            sizeof(FPO_DATA) * pIDD->cFpo,
            MEM_COMMIT,
            PAGE_READWRITE
            );
        if (mi->pFpoData) {
            mi->dwEntries = pIDD->cFpo;
            CopyMemory(
                mi->pFpoData,
                pIDD->pFpo,
                sizeof(FPO_DATA) * mi->dwEntries
                );
            VirtualProtect(
                mi->pFpoData,
                sizeof(FPO_DATA) * mi->dwEntries,
                PAGE_READONLY,
                &i
                );
        }
    }

    mi->NumSections = pIDD->cCurrentSections;
    if (pIDD->fCurrentSectionsMapped) {
        mi->SectionHdrs = (PIMAGE_SECTION_HEADER) MemAlloc(
            sizeof(IMAGE_SECTION_HEADER) * mi->NumSections
            );
        if (mi->SectionHdrs) {
            CopyMemory(
                mi->SectionHdrs,
                pIDD->pCurrentSections,
                sizeof(IMAGE_SECTION_HEADER) * mi->NumSections
                );
        }
    } else {
        mi->SectionHdrs = pIDD->pCurrentSections;
    }

    if (pIDD->pOriginalSections) {
        mi->OriginalNumSections = pIDD->cOriginalSections;
        mi->OriginalSectionHdrs = pIDD->pOriginalSections;
    } else {
        mi->OriginalNumSections = mi->NumSections;
        mi->OriginalSectionHdrs = (PIMAGE_SECTION_HEADER) MemAlloc(
            sizeof(IMAGE_SECTION_HEADER) * mi->NumSections
            );
        if (mi->OriginalSectionHdrs) {
            CopyMemory(
                mi->OriginalSectionHdrs,
                pIDD->pCurrentSections,
                sizeof(IMAGE_SECTION_HEADER) * mi->NumSections
                );
        }
    }

    mi->TmpSym.Name = (LPSTR) MemAlloc( TMP_SYM_LEN );

    if (pIDD->pPdb) {
        mi->pdb = pIDD->pPdb;
        mi->dbi = pIDD->pDbi;
        mi->gsi = pIDD->pGsi;
        PDBOpenTpi(pIDD->pPdb, pdbRead, (TPI **) &mi->ptpi);
        if (mi->dbi) 
           DBIOpenGlobals((DBI *) mi->dbi, (GSI **) &mi->globals);
        else
           mi->globals = NULL;
        mi->SymType = SymPdb;
        SymbolsLoaded = TRUE;
    } else {
        if (pIDD->pMappedCv) {
            SymbolsLoaded = LoadCodeViewSymbols(
                hProcess,
                mi,
                pIDD
                );
            DPRINTF(ProcessEntry, "codeview symbols %sloaded\n", SymbolsLoaded?"":"not ");
        }
        if (!SymbolsLoaded && pIDD->pMappedCoff) {
            SymbolsLoaded = LoadCoffSymbols(hProcess, mi, pIDD);
            DPRINTF(ProcessEntry, "coff symbols %sloaded\n", SymbolsLoaded?"":"not ");
        }

        if (!SymbolsLoaded && pIDD->pMappedExportDirectory) {
            SymbolsLoaded = LoadExportSymbols( mi, pIDD );
            DPRINTF(ProcessEntry, "export symbols %sloaded\n", SymbolsLoaded?"":"not ");
        }

        if (!SymbolsLoaded) {
            mi->SymType = SymNone;
            DPRINTF(ProcessEntry, "no symbols loaded\n");
        }
    }

    ProcessOmapForModule( mi, pIDD );

    ImgHlpReleaseDebugInfo( pIDD, IMGHLP_FREE_FPO | IMGHLP_FREE_SYMPATH );
    mi->Flags &= ~MIF_DEFERRED_LOAD;

    DoSymbolCallback(
        ProcessEntry,
        CBA_DEFERRED_SYMBOL_LOAD_COMPLETE,
        mi,
        &idsl,
        CallbackFileName
        );

    return TRUE;
}


DWORD64
InternalLoadModule(
    IN  HANDLE          hProcess,
    IN  PSTR            ImageName,
    IN  PSTR            ModuleName,
    IN  DWORD64         BaseOfDll,
    IN  DWORD           DllSize,
    IN  HANDLE          hFile
    )
{
    IMAGEHLP_DEFERRED_SYMBOL_LOAD64 idsl;
    PPROCESS_ENTRY              ProcessEntry;
    PMODULE_ENTRY               mi;
    LPSTR                       p;

    ProcessEntry = FindProcessEntry( hProcess );
    if (!ProcessEntry) {
        return 0;
    }

    if (BaseOfDll) {
        mi = GetModuleForPC( ProcessEntry, BaseOfDll, TRUE );
    } else {
        mi = NULL;
    }

    if (mi) {
        //
        // in this case the symbols are already loaded
        // so the caller really wants the deferred
        // symbols to be loaded
        //
        if ( (mi->Flags & MIF_DEFERRED_LOAD) &&
                CompleteDeferredSymbolLoad( hProcess, mi )) {

            return mi->BaseOfDll;
        } else {
            return 0;
        }
    }

    //
    // look to see if there is an overlapping module entry
    //
    if (BaseOfDll) {
        do {
            mi = GetModuleForPC( ProcessEntry, BaseOfDll, FALSE );
            if (mi) {
                RemoveEntryList( &mi->ListEntry );

                DoSymbolCallback(
                    ProcessEntry,
                    CBA_SYMBOLS_UNLOADED,
                    mi,
                    &idsl,
                    mi->LoadedImageName ? mi->LoadedImageName : mi->ImageName ? mi->ImageName : mi->ModuleName
                    );

                FreeModuleEntry( mi );
            }
        } while( mi );
    }

    mi = (PMODULE_ENTRY) MemAlloc( sizeof(MODULE_ENTRY) );
    if (!mi) {
        return 0;
    }
    ZeroMemory( mi, sizeof(MODULE_ENTRY) );

    mi->BaseOfDll = BaseOfDll;
    mi->DllSize = DllSize;
    mi->hFile = hFile;
    if (ImageName) {
        mi->ImageName = StringDup(ImageName);
        _splitpath( ImageName, NULL, NULL, mi->ModuleName, NULL );
        if (ModuleName && _stricmp( ModuleName, mi->ModuleName ) != 0) {
            strcpy( mi->AliasName, ModuleName );
        } else {
            mi->AliasName[0] = 0;
        }
    } else {

        if (ModuleName) {
            strcpy( mi->AliasName, ModuleName );
        }

    }

    if ((SymOptions & SYMOPT_DEFERRED_LOADS) && BaseOfDll) {
        mi->Flags |= MIF_DEFERRED_LOAD;
        mi->SymType = SymDeferred;
    } else if (!CompleteDeferredSymbolLoad( hProcess, mi )) {
        FreeModuleEntry( mi );
        return 0;
    }

    ProcessEntry->Count += 1;

    InsertTailList( &ProcessEntry->ModuleList, &mi->ListEntry);

    return mi->BaseOfDll;
}

PPROCESS_ENTRY
FindProcessEntry(
    HANDLE  hProcess
    )
{
    PLIST_ENTRY                 Next;
    PPROCESS_ENTRY              ProcessEntry;

    Next = ProcessList.Flink;
    if (!Next) {
        return NULL;
    }

    while ((PVOID)Next != (PVOID)&ProcessList) {
        ProcessEntry = CONTAINING_RECORD( Next, PROCESS_ENTRY, ListEntry );
        Next = ProcessEntry->ListEntry.Flink;
        if (ProcessEntry->hProcess == hProcess) {
            return ProcessEntry;
        }
    }

    return NULL;
}

PPROCESS_ENTRY
FindFirstProcessEntry(
    )
{
    return CONTAINING_RECORD(ProcessList.Flink, PROCESS_ENTRY, ListEntry);
}


PMODULE_ENTRY
FindModule(
    HANDLE hProcess,
    PPROCESS_ENTRY ProcessEntry,
    LPSTR ModuleName,
    BOOL LoadSymbols
    )
{
    PLIST_ENTRY Next;
    PMODULE_ENTRY mi;

    Next = ProcessEntry->ModuleList.Flink;
    if (Next) {
        while ((PVOID)Next != (PVOID)&ProcessEntry->ModuleList) {
            mi = CONTAINING_RECORD( Next, MODULE_ENTRY, ListEntry );
            Next = mi->ListEntry.Flink;

            if ((_stricmp( mi->ModuleName, ModuleName ) == 0) ||
                (mi->AliasName[0] &&
                 _stricmp( mi->AliasName, ModuleName ) == 0))
            {
                if (LoadSymbols && !ENSURE_SYMBOLS(hProcess, mi)) {
                    return NULL;
                }

                return mi;
            }
        }
    }

    return NULL;
}


#ifdef FORLATER
        
BOOL
GetSizeofPDBSym(
    
    PMODULE_ENTRY   mi
        
    IN     HANDLE               hProcess,
    IN OUT PIMAGEHLP_SYMBOL64   Symbol,
    IN     int                  Direction
    )

/*++

Routine Description:

    Common code for SymGetSymNext and SymGetSymPrev.


Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Symbol              - Starting symbol.

    Dir                 - Supplies direction to search

Return Value:

    Non NULL pointer    - The symbol was located.

    NULL pointer        - The symbol was not found.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY      ProcessEntry;
    PMODULE_ENTRY       mi;
    ULONG64             Displacement;
    PSYMBOL_ENTRY       sym;

    DATASYM32 *nextSym;
    DATASYM32 *bestSym;
    DWORD64 bestAddr;
    PIMAGE_SECTION_HEADER sh;
    DWORD64 addr;
    ULONG k;
    LPSTR SymbolName;
    UCHAR SymbolLen;
    DWORD Bias;


    __try {

        nextSym = (DATASYM32*)GSINextSym( mi->gsi, NULL );
        bestSym = NULL;
        bestAddr = 0xffffffff;

        while( nextSym ) {
            addr = 0;
            k = DataSymSeg(nextSym);

            if ((k <= mi->OriginalNumSections)) {
                addr = mi->OriginalSectionHdrs[k-1].VirtualAddress + DataSymOffset(nextSym) + mi->BaseOfDll;

                if ((addr > sym->Address) && (addr < bestAddr)) {

                    SymbolName = DataSymNameStart(nextSym);
                    // ignore strings
                    if (*(ULONG *)SymbolName != 0x435f3f3f) { /* starts with ??_C */
                        bestAddr = addr;
                        bestSym = nextSym;
                    }
                }
            }
            nextSym = (DATASYM32*)GSINextSym( mi->gsi, (PUCHAR)nextSym );
        }

        if (!bestSym) {
            SetLastError( ERROR_INVALID_ADDRESS );
            return FALSE;
        }

        SymbolName = DataSymNameStart(bestSym);
        SymbolLen  = DataSymNameLength(bestSym);

        sym = &mi->TmpSym;

        if (SymOptions & SYMOPT_UNDNAME) {
            SymUnDNameInternal( sym->Name, TMP_SYM_LEN-sizeof(*sym), SymbolName, SymbolLen);
        } else {
            // use strncat to always get a \0 at the end
            sym->Name[0] = 0;
            strncat( sym->Name, SymbolName, TMP_SYM_LEN-sizeof(*sym) );
        }
        sym->NameLength = strlen(sym->Name);

        addr = ConvertOmapFromSrc( mi, bestAddr, &Bias );

        if (addr != 0) {
            bestAddr = addr + Bias;
        }

        sym->Size = 0;
        sym->Flags = 0;
        sym->Address = bestAddr;

        symcpy64( Symbol, sym );

        return TRUE;
    
    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return FALSE;
}



#endif FORLATER


PSYMBOL_ENTRY
GetSymFromAddr(
    DWORD64         dwAddr,
    PDWORD64        pqwDisplacement,
    PMODULE_ENTRY   mi
    )
{
    PSYMBOL_ENTRY           sym;
    DWORD                   i;
    LONG                    High;
    LONG                    Low;
    LONG                    Middle;
    DWORD                   Displacement32;

    if (mi == NULL) {
        return NULL;
    }

    if (mi->SymType == SymPdb) {
        DWORD       Bias;
        DWORD64     SectionBase;
        DATASYM32  *dataSym;
        DATASYM32  *nextSym;
        DWORD64     srcBlkAddr;
        DWORD64     srcIPAddr;
        DWORD64     symAddr;
        DWORD64     nextAddr;
        LONG        size;           // presume no symbol can be larger than this
        BOOL        omap = FALSE;

        srcBlkAddr = ConvertOmapToSrc( mi,
                                   dwAddr,
                                   &Bias,
                                   (SymOptions & SYMOPT_OMAP_FIND_NEAREST) != 0
                                   );

        if (srcBlkAddr == 0) {
            //
            // No equivalent address
            //
            return NULL;
        }

        //
        // We have successfully converted
        //
        srcIPAddr = srcBlkAddr + Bias;

        if (srcIPAddr != dwAddr) {
            omap = TRUE;
        }

        //
        // locate the section that the address resides in
        //

        {
            PIMAGE_SECTION_HEADER   sh;

            for (i=0, sh=mi->OriginalSectionHdrs; i < mi->OriginalNumSections; i++, sh++) {
                if (srcIPAddr >= (mi->BaseOfDll + sh->VirtualAddress) &&
                    srcIPAddr <  (mi->BaseOfDll + sh->VirtualAddress + sh->Misc.VirtualSize))
                {
                    SectionBase = sh->VirtualAddress;
                    //
                    // found the section
                    //
                    break;
                }
            }
        }

        if (i == mi->OriginalNumSections) {
            return NULL;
        }

        dataSym = (DATASYM32*)GSINearestSym(
            mi->gsi,
            (USHORT)(i+1),
            (ULONG)(srcIPAddr - mi->BaseOfDll - SectionBase),
            &Displacement32
            );

        if (pqwDisplacement) {
            *pqwDisplacement = Displacement32;
        }

        if (dataSym == NULL) {
            return NULL;
        }

        // start to build symbol: address

        mi->TmpSym.Flags = 0;
        mi->TmpSym.Address  = srcIPAddr - Displacement32;

        // setup the name

        if (SymOptions & SYMOPT_UNDNAME) {
            SymUnDNameInternal( mi->TmpSym.Name, TMP_SYM_LEN,
                    DataSymNameStart(dataSym), DataSymNameLength(dataSym));
        } else {
            mi->TmpSym.NameLength = DataSymNameLength(dataSym);
            memcpy( mi->TmpSym.Name, DataSymNameStart(dataSym), mi->TmpSym.NameLength );
            mi->TmpSym.Name[mi->TmpSym.NameLength] = 0;
        }

        // get the address

        i = DataSymSeg(dataSym) - 1;
        symAddr = mi->OriginalSectionHdrs[i].VirtualAddress + mi->BaseOfDll + DataSymOffset(dataSym);

#  if 0 //BUGBUG - develop a system for representing relocated symbols
        // if the block is relocated, change symbol name

        if (omap) {
    
            if (symAddr < srcBlkAddr) {
                char sz[10];
                sprintf(sz, "_%d", Bias);
                strcat(mi->TmpSym.Name, sz);
                mi->TmpSym.NameLength = strlen(mi->TmpSym.Name);
            } 
        }
#endif

        // calculate size of symbol

        mi->TmpSym.Size = 0;

        // done

        return &mi->TmpSym;
    }

    //
    // do a binary search to locate the symbol
    //
    Low = 0;
    High = mi->numsyms - 1;

    while (High >= Low) {
        Middle = (Low + High) >> 1;
        sym = &mi->symbolTable[Middle];
        if (dwAddr < sym->Address) {

            High = Middle - 1;

        } else if (dwAddr >= sym->Address + sym->Size) {

            Low = Middle + 1;

        } else {

            if (pqwDisplacement) {
                *pqwDisplacement = dwAddr - sym->Address;
            }
            return sym;

        }
    }

    return NULL;
}


PMODULE_ENTRY
GetModuleForPC(
    PPROCESS_ENTRY  ProcessEntry,
    DWORD64         dwPcAddr,
    BOOL            ExactMatch
    )
{
    static PLIST_ENTRY          Next = NULL;
    PMODULE_ENTRY               ModuleEntry;


    if (dwPcAddr == (DWORD64)-1) {
        if ((PVOID)Next == (PVOID)&ProcessEntry->ModuleList) {
            // Reset to NULL so the list can be re-walked
            Next = NULL;
            return NULL;
        }
        ModuleEntry = CONTAINING_RECORD( Next, MODULE_ENTRY, ListEntry );
        Next = ModuleEntry->ListEntry.Flink;
        return ModuleEntry;
    }

    Next = ProcessEntry->ModuleList.Flink;
    if (!Next) {
        return NULL;
    }

    while ((PVOID)Next != (PVOID)&ProcessEntry->ModuleList) {
        ModuleEntry = CONTAINING_RECORD( Next, MODULE_ENTRY, ListEntry );
        Next = ModuleEntry->ListEntry.Flink;
        if (dwPcAddr == 0) {
            return ModuleEntry;
        }
        if (ExactMatch) {
            if (dwPcAddr == ModuleEntry->BaseOfDll) {
               return ModuleEntry;
            }
        } else
        if ((dwPcAddr == ModuleEntry->BaseOfDll && ModuleEntry->DllSize == 0) ||
            ((dwPcAddr >= ModuleEntry->BaseOfDll) &&
                (dwPcAddr  < ModuleEntry->BaseOfDll + ModuleEntry->DllSize))) {
               return ModuleEntry;
        }
    }

    return NULL;
}

PSYMBOL_ENTRY
GetSymFromAddrAllContexts(
    DWORD64         dwAddr,
    PDWORD64        pqwDisplacement,
    PPROCESS_ENTRY  ProcessEntry
    )
{
    PMODULE_ENTRY mi = GetModuleForPC( ProcessEntry, dwAddr, FALSE );
    if (mi == NULL) {
        return NULL;
    }
    return GetSymFromAddr( dwAddr, pqwDisplacement, mi );
}

DWORD
ComputeHash(
    LPSTR   lpbName,
    ULONG   cb
    )
{
    ULONG UNALIGNED *   lpulName;
    ULONG               ulEnd = 0;
    int                 cul;
    int                 iul;
    ULONG               ulSum = 0;

    while (cb & 3) {
        ulEnd |= (lpbName[cb - 1] & 0xdf);
        ulEnd <<= 8;
        cb -= 1;
    }

    cul = cb / 4;
    lpulName = (ULONG UNALIGNED *) lpbName;
    for (iul =0; iul < cul; iul++) {
        ulSum ^= (lpulName[iul] & 0xdfdfdfdf);
        ulSum = _lrotl( ulSum, 4);
    }
    ulSum ^= ulEnd;
    return ulSum % HASH_MODULO;
}

PSYMBOL_ENTRY
AllocSym(
    PMODULE_ENTRY   mi,
    DWORD64         addr,
    LPSTR           name
    )
{
    PSYMBOL_ENTRY       sym;
    ULONG               Length;


    if (mi->numsyms == mi->MaxSyms) {
        return NULL;
    }

    if (!mi->StringSize) {
        return NULL;
    }

    Length = strlen(name);

    if ((Length + 1) > mi->StringSize) {
        return NULL;
    }

    sym = &mi->symbolTable[mi->numsyms];

    mi->numsyms += 1;
    sym->Name = mi->SymStrings;
    mi->SymStrings += (Length + 2);
    mi->StringSize -= (Length + 2);

    strcpy( sym->Name, name );
    sym->Address = addr;
    sym->Size = 0;
    sym->Flags = 0;
    sym->Next = NULL;
    sym->NameLength = Length;

    return sym;
}

int __cdecl
SymbolTableAddressCompare(
    const void *e1,
    const void *e2
    )
{
    PSYMBOL_ENTRY    sym1 = (PSYMBOL_ENTRY) e1;
    PSYMBOL_ENTRY    sym2 = (PSYMBOL_ENTRY) e2;
    LONG64 diff;

    if ( sym1 && sym2 ) {
        diff = (sym1->Address - sym2->Address);
        return (diff < 0) ? -1 : (diff == 0) ? 0 : 1;
    } else {
        return 1;
    }
}

int __cdecl
SymbolTableNameCompare(
    const void *e1,
    const void *e2
    )
{
    PSYMBOL_ENTRY    sym1 = (PSYMBOL_ENTRY) e1;
    PSYMBOL_ENTRY    sym2 = (PSYMBOL_ENTRY) e2;

    return strcmp( sym1->Name, sym2->Name );
}

VOID
CompleteSymbolTable(
    PMODULE_ENTRY   mi
    )
{
    PSYMBOL_ENTRY       sym;
    PSYMBOL_ENTRY       symH;
    ULONG               Hash;
    ULONG               i;
    ULONG               dups;
    ULONG               seq;


    //
    // sort the symbols by name
    //
    qsort(
        mi->symbolTable,
        mi->numsyms,
        sizeof(SYMBOL_ENTRY),
        SymbolTableNameCompare
        );

    //
    // mark duplicate names
    //
    seq = 0;
    for (i=0; i<mi->numsyms; i++) {
        dups = 0;
        while ((mi->symbolTable[i+dups].NameLength == mi->symbolTable[i+dups+1].NameLength) &&
               (strcmp( mi->symbolTable[i+dups].Name, mi->symbolTable[i+dups+1].Name ) == 0)) {
                   mi->symbolTable[i+dups].Flags |= SYMF_DUPLICATE;
                   mi->symbolTable[i+dups+1].Flags |= SYMF_DUPLICATE;
                   dups += 1;
        }
        i += dups;
    }

    //
    // sort the symbols by address
    //
    qsort(
        mi->symbolTable,
        mi->numsyms,
        sizeof(SYMBOL_ENTRY),
        SymbolTableAddressCompare
        );

    //
    // calculate the size of each symbol
    //
    for (i=0; i<mi->numsyms; i++) {
        mi->symbolTable[i].Next = NULL;
        if (i+1 < mi->numsyms) {
            mi->symbolTable[i].Size = (ULONG)(mi->symbolTable[i+1].Address - mi->symbolTable[i].Address);
        }
    }

    //
    // compute the hash for each symbol
    //
    ZeroMemory( mi->NameHashTable, sizeof(mi->NameHashTable) );
    for (i=0; i<mi->numsyms; i++) {
        sym = &mi->symbolTable[i];

        Hash = ComputeHash( sym->Name, sym->NameLength );

        if (mi->NameHashTable[Hash]) {

            //
            // we have a collision
            //
            symH = mi->NameHashTable[Hash];
            while( symH->Next ) {
                symH = symH->Next;
            }
            symH->Next = sym;

        } else {

            mi->NameHashTable[Hash] = sym;

        }
    }
}

BOOL
CreateSymbolTable(
    PMODULE_ENTRY   mi,
    DWORD           SymbolCount,
    SYM_TYPE        SymType,
    DWORD           NameSize
    )
{
    //
    // allocate the symbol table
    //
    NameSize += OMAP_SYM_STRINGS;
    mi->symbolTable = (PSYMBOL_ENTRY) MemAlloc(
        (sizeof(SYMBOL_ENTRY) * (SymbolCount + OMAP_SYM_EXTRA)) + NameSize + (SymbolCount * CPP_EXTRA)
        );
    if (!mi->symbolTable) {
        return FALSE;
    }

    //
    // initialize the relevant fields
    //
    mi->numsyms    = 0;
    mi->MaxSyms    = SymbolCount + OMAP_SYM_EXTRA;
    mi->SymType    = SymType;
    mi->StringSize = NameSize + (SymbolCount * CPP_EXTRA);
    mi->SymStrings = (LPSTR)(mi->symbolTable + SymbolCount + OMAP_SYM_EXTRA);

    return TRUE;
}

PIMAGE_SECTION_HEADER 
FindSection(
    PIMAGE_SECTION_HEADER   sh,
    ULONG                   NumSections,
    ULONG                   Address
    )
{
    ULONG i;
    for (i=0; i<NumSections; i++) {
        if (Address >= sh[i].VirtualAddress &&
            Address <  (sh[i].VirtualAddress + sh[i].Misc.VirtualSize)) {
                    return &sh[i];
        }
    }
    return NULL;
}

PVOID 
GetSectionPhysical(
    HANDLE             hp,
    ULONG64            base,
    PIMGHLP_DEBUG_DATA pIDD,
    ULONG              Address
    )
{
    PIMAGE_SECTION_HEADER   sh;

    sh = FindSection( pIDD->pCurrentSections, pIDD->cCurrentSections, Address );
    if (!sh) {
        return 0;
    }

    return (PCHAR)pIDD->ImageMap + sh->PointerToRawData + (Address - sh->VirtualAddress);
}

BOOL
ReadSectionInfo(
    HANDLE             hp,
    ULONG64            base,
    PIMGHLP_DEBUG_DATA pIDD,
    ULONG              address,
    PVOID              buf,
    DWORD              size
    )
{
    PIMAGE_SECTION_HEADER   sh;
    DWORD_PTR status = TRUE;

    sh = FindSection( pIDD->pCurrentSections, pIDD->cCurrentSections, address );
    if (!sh) 
        return FALSE;

    if (!hp) {
        status = (DWORD_PTR)memcpy((PCHAR)buf, 
                               (PCHAR)base + sh->PointerToRawData + (address - sh->VirtualAddress), 
                               size);
    } else {
        status = ReadImageData(hp, base, address, buf, size);
    }
    if (!status)
        return FALSE;

    return TRUE;
}


ULONG
LoadExportSymbols(
    PMODULE_ENTRY      mi,
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    PULONG                  names;
    PULONG                  addrs;
    PUSHORT                 ordinals;
    PUSHORT                 ordidx;
    ULONG                   cnt;
    ULONG                   idx;
    PIMAGE_EXPORT_DIRECTORY expdir;
    ULONG                   i;
    PSYMBOL_ENTRY           sym;
    ULONG                   NameSize;
    HANDLE                  hp;
    ULONG64                 base;
    CHAR                    name[2048];
    BOOL                    rc;

    // setup pointers for grabing data

    switch (pIDD->dsExports) {
    case dsInProc:
        hp = pIDD->hProcess;
        base = pIDD->InProcImageBase;
        break;
    case dsImage:
        hp = NULL;
        // BUGBUG: localize this!
        if (!pIDD->ImageMap)
            pIDD->ImageMap = MapItRO(pIDD->ImageFileHandle);
        base = (ULONG64)pIDD->ImageMap;
        break;
    default:
        return 0;
    }

    expdir = &pIDD->expdir;

    names = (PULONG) MemAlloc( expdir->NumberOfNames * sizeof(ULONG) );
    addrs = (PULONG) MemAlloc( expdir->NumberOfFunctions * sizeof(ULONG) );
    ordinals = (PUSHORT) MemAlloc( expdir->NumberOfNames * sizeof(USHORT) );
    ordidx = (PUSHORT) MemAlloc( expdir->NumberOfFunctions * sizeof(USHORT) );
    
    if (!names || !addrs || !ordinals || !ordidx)
        goto cleanup;

    rc = ReadSectionInfo(hp,
                         base,
                         pIDD,
                         expdir->AddressOfNames,
                         (PCHAR)names,
                         expdir->NumberOfNames * sizeof(ULONG));
    if (!rc)
        goto cleanup;
                              
    rc = ReadSectionInfo(hp,
                        base,
                        pIDD,
                        expdir->AddressOfFunctions,
                        (PCHAR)addrs,
                        expdir->NumberOfFunctions * sizeof(ULONG));
    if (!rc)
        goto cleanup;
                    
    rc = ReadSectionInfo(hp,
                         base,
                         pIDD,
                         expdir->AddressOfNameOrdinals,
                         (PCHAR)ordinals,
                         expdir->NumberOfNames * sizeof(USHORT));
    if (!rc)
        goto cleanup;
    
    cnt = 0;
    NameSize = 0;

    //
    // count the symbols
    //

    for (i=0; i<expdir->NumberOfNames; i++) {
        *name = 0;
        ReadSectionInfo(hp,
                        base,
                        pIDD,
                        names[i],
                        name,
                        sizeof(name));
        if (!*name) {
            continue;
        }
        if (SymOptions & SYMOPT_UNDNAME) {
            SymUnDNameInternal( mi->TmpSym.Name, TMP_SYM_LEN, name, strlen(name) );
            NameSize += strlen(mi->TmpSym.Name);
            cnt += 1;
        } else {
            NameSize += (strlen(name) + 2);
            cnt += 1;
        }
    }

    for (i=0,idx=expdir->NumberOfNames; i<expdir->NumberOfFunctions; i++) {
        if (!ordidx[i]) {
            NameSize += 16;
            cnt += 1;
        }
    }

    //
    // allocate the symbol table
    //
    if (!CreateSymbolTable( mi, cnt, SymExport, NameSize )) {
        cnt = 0;
        goto cleanup;
    }

    //
    // allocate the symbols
    //

    cnt = 0;

    for (i=0; i<expdir->NumberOfNames; i++) {
        idx = ordinals[i];
        ordidx[idx] = TRUE;
        *name = 0;
        ReadSectionInfo(hp,
                        base,
                        pIDD,
                        names[i],
                        name,
                        sizeof(name));
        if (!*name) {
            continue;
        }
        if (SymOptions & SYMOPT_UNDNAME) {
            SymUnDNameInternal( mi->TmpSym.Name, TMP_SYM_LEN, (LPSTR)name, strlen(name) );
            sym = AllocSym( mi, addrs[idx] + mi->BaseOfDll, mi->TmpSym.Name);
        } else {
            sym = AllocSym( mi, addrs[idx] + mi->BaseOfDll, name);
        }
        if (sym) {
            cnt += 1;
        }
    }

    for (i=0,idx=expdir->NumberOfNames; i<expdir->NumberOfFunctions; i++) {
        if (!ordidx[i]) {
            CHAR NameBuf[sizeof("Ordinal99999") + 1];       // Ordinals are only 64k max.
            strcpy( NameBuf, "Ordinal" );
            _itoa( i+expdir->Base, &NameBuf[7], 10 );
            sym = AllocSym( mi, addrs[i] + mi->BaseOfDll, NameBuf);
            if (sym) {
                cnt += 1;
            }
            idx += 1;
        }
    }

    CompleteSymbolTable( mi );

cleanup:
    if (names)    MemFree(names);
    if (addrs)    MemFree(addrs);
    if (ordinals) MemFree(ordinals);
    if (ordidx)   MemFree( ordidx );

    return cnt;
}


BOOL
LoadCoffSymbols(
    HANDLE             hProcess,
    PMODULE_ENTRY      mi,
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    PIMAGE_COFF_SYMBOLS_HEADER pCoffHeader = (PIMAGE_COFF_SYMBOLS_HEADER)(pIDD->pMappedCoff);
    PUCHAR              stringTable;
    PIMAGE_SYMBOL       allSymbols;
    DWORD               numberOfSymbols;
    PIMAGE_LINENUMBER   LineNumbers;
    PIMAGE_SYMBOL       NextSymbol;
    PIMAGE_SYMBOL       Symbol;
    PSYMBOL_ENTRY       sym;
    CHAR                szSymName[256];
    DWORD               i;
    DWORD64             addr;
    DWORD               CoffSymbols = 0;
    DWORD               NameSize = 0;
    DWORD64             Bias;

    allSymbols = (PIMAGE_SYMBOL)((PCHAR)pCoffHeader +
                 pCoffHeader->LvaToFirstSymbol);

    stringTable = (PUCHAR)pCoffHeader +
                  pCoffHeader->LvaToFirstSymbol +
                  (pCoffHeader->NumberOfSymbols * IMAGE_SIZEOF_SYMBOL);

    numberOfSymbols = pCoffHeader->NumberOfSymbols;
    LineNumbers = (PIMAGE_LINENUMBER)(PCHAR)pCoffHeader +
                        pCoffHeader->LvaToFirstLinenumber;

    //
    // count the number of actual symbols
    //
    NextSymbol = allSymbols;
    for (i= 0; i < numberOfSymbols; i++) {
        Symbol = NextSymbol++;
        if (Symbol->StorageClass == IMAGE_SYM_CLASS_EXTERNAL &&
            Symbol->SectionNumber > 0) {
            GetSymName( Symbol, stringTable, szSymName, sizeof(szSymName) );
            if (szSymName[0] == '?' && szSymName[1] == '?' &&
                szSymName[2] == '_' && szSymName[3] == 'C'    ) {
                //
                // ignore strings
                //
            } else if (SymOptions & SYMOPT_UNDNAME) {
                SymUnDNameInternal( mi->TmpSym.Name, TMP_SYM_LEN, szSymName,
                                    strlen(szSymName) );
                NameSize += strlen(mi->TmpSym.Name);
                CoffSymbols += 1;
            } else {
                CoffSymbols += 1;
                NameSize += (strlen(szSymName) + 1);
            }
        }

        NextSymbol += Symbol->NumberOfAuxSymbols;
        i += Symbol->NumberOfAuxSymbols;
    }

    //
    // allocate the symbol table
    //
    if (!CreateSymbolTable( mi, CoffSymbols, SymCoff, NameSize )) {
        return FALSE;
    }

    //
    // populate the symbol table
    //

    if (mi->Flags & MIF_ROM_IMAGE) {
        Bias = mi->BaseOfDll & 0xffffffff00000000;
    } else {
        Bias = mi->BaseOfDll;
    }

    NextSymbol = allSymbols;
    for (i= 0; i < numberOfSymbols; i++) {
        Symbol = NextSymbol++;
        if (Symbol->StorageClass == IMAGE_SYM_CLASS_EXTERNAL &&
            Symbol->SectionNumber > 0) {
            GetSymName( Symbol, stringTable, szSymName, sizeof(szSymName) );
            addr = Symbol->Value + Bias;
            if (szSymName[0] == '?' && szSymName[1] == '?' &&
                szSymName[2] == '_' && szSymName[3] == 'C'    ) {
                //
                // ignore strings
                //
            } else if (SymOptions & SYMOPT_UNDNAME) {
                SymUnDNameInternal( mi->TmpSym.Name, TMP_SYM_LEN, szSymName,
                                    strlen(szSymName) );
                AllocSym( mi, addr, mi->TmpSym.Name);
            } else {
                AllocSym( mi, addr, szSymName );
            }
        }

        NextSymbol += Symbol->NumberOfAuxSymbols;
        i += Symbol->NumberOfAuxSymbols;
    }

    CompleteSymbolTable( mi );

    if (SymOptions & SYMOPT_LOAD_LINES) {
        AddLinesForCoff(mi, allSymbols, numberOfSymbols, LineNumbers);
    }

    return TRUE;
}

BOOL
LoadCodeViewSymbols(
    HANDLE             hProcess,
    PMODULE_ENTRY      mi,
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    DWORD                   i, j;
    PPROCESS_ENTRY          ProcessEntry;
    OMFSignature           *omfSig;
    OMFDirHeader           *omfDirHdr;
    OMFDirEntry            *omfDirEntry;
    OMFSymHash             *omfSymHash;
    DATASYM32              *dataSym;
    DWORD64                 addr;
    DWORD                   CvSymbols;
    DWORD                   NameSize;

    ProcessEntry = FindProcessEntry( hProcess );
    if (!ProcessEntry) {
        return FALSE;
    }

    DPRINTF(ProcessEntry, "LoadCodeViewSymbols:\n"
            " hProcess   %p\n"
            " mi         %p\n"
            " pCvData    %p\n"
            " dwSize     %x\n",
            hProcess,
            mi,
            pIDD->pMappedCv,
            pIDD->cMappedCv
            );

    omfSig = (OMFSignature*) pIDD->pMappedCv;
    if ((*(DWORD *)(omfSig->Signature) != '80BN') &&
        (*(DWORD *)(omfSig->Signature) != '90BN') &&
        (*(DWORD *)(omfSig->Signature) != '11BN')) {
        DPRINTF(ProcessEntry, "unrecognized OMF sig: %x\n",
                *(DWORD *)(omfSig->Signature));
        return FALSE;
    }

    //
    // count the number of actual symbols
    //
    omfDirHdr = (OMFDirHeader*) ((ULONG_PTR)omfSig + (DWORD)omfSig->filepos);
    omfDirEntry = (OMFDirEntry*) ((ULONG_PTR)omfDirHdr + sizeof(OMFDirHeader));

    NameSize = 0;
    CvSymbols = 0;

    for (i=0; i<omfDirHdr->cDir; i++,omfDirEntry++) {
        LPSTR SymbolName;
        UCHAR SymbolLen;
        if (omfDirEntry->SubSection == sstGlobalPub) {
            omfSymHash = (OMFSymHash*) ((ULONG_PTR)omfSig + omfDirEntry->lfo);
            dataSym = (DATASYM32*) ((ULONG_PTR)omfSig + omfDirEntry->lfo + sizeof(OMFSymHash));
            for (j=sizeof(OMFSymHash); j<=omfSymHash->cbSymbol; ) {
                addr = 0;
                if (DataSymSeg(dataSym) && (DataSymSeg(dataSym) <= mi->OriginalNumSections))
                {
                    addr = mi->OriginalSectionHdrs[DataSymSeg(dataSym)-1].VirtualAddress + DataSymOffset(dataSym) + mi->BaseOfDll;
                    SymbolName = DataSymNameStart(dataSym);
                    SymbolLen = DataSymNameLength(dataSym);
                    if (SymbolName[0] == '?' &&
                        SymbolName[1] == '?' &&
                        SymbolName[2] == '_' &&
                        SymbolName[3] == 'C' )
                    {
                        //
                        // ignore strings
                        //
                    } else if (SymOptions & SYMOPT_UNDNAME) {
                        SymUnDNameInternal( mi->TmpSym.Name, TMP_SYM_LEN, SymbolName, SymbolLen );
                        NameSize += strlen(mi->TmpSym.Name);
                        CvSymbols += 1;
                    } else {
                        CvSymbols += 1;
                        NameSize += SymbolLen + 1;
                    }
                }
                j += dataSym->reclen + 2;
                dataSym = (DATASYM32*) ((ULONG_PTR)dataSym + dataSym->reclen + 2);
            }
            break;
        }
    }

    //
    // allocate the symbol table
    //
    if (!CreateSymbolTable( mi, CvSymbols, SymCv, NameSize )) {
        DPRINTF(ProcessEntry, "CreateSymbolTable failed\n");
        return FALSE;
    }

    //
    // populate the symbol table
    //
    omfDirHdr = (OMFDirHeader*) ((ULONG_PTR)omfSig + (DWORD)omfSig->filepos);
    omfDirEntry = (OMFDirEntry*) ((ULONG_PTR)omfDirHdr + sizeof(OMFDirHeader));
    for (i=0; i<omfDirHdr->cDir; i++,omfDirEntry++) {
        LPSTR SymbolName;
        if (omfDirEntry->SubSection == sstGlobalPub) {
            omfSymHash = (OMFSymHash*) ((ULONG_PTR)omfSig + omfDirEntry->lfo);
            dataSym = (DATASYM32*) ((ULONG_PTR)omfSig + omfDirEntry->lfo + sizeof(OMFSymHash));
            for (j=sizeof(OMFSymHash); j<=omfSymHash->cbSymbol; ) {
                addr = 0;
                if (DataSymSeg(dataSym) && (DataSymSeg(dataSym) <= mi->OriginalNumSections))
                {
                    addr = mi->OriginalSectionHdrs[DataSymSeg(dataSym)-1].VirtualAddress + DataSymOffset(dataSym) + mi->BaseOfDll;
                    SymbolName = DataSymNameStart(dataSym);
                    if (SymbolName[0] == '?' &&
                        SymbolName[1] == '?' &&
                        SymbolName[2] == '_' &&
                        SymbolName[3] == 'C' )
                    {
                        //
                        // ignore strings
                        //
                    } else if (SymOptions & SYMOPT_UNDNAME) {
                        SymUnDNameInternal( mi->TmpSym.Name, TMP_SYM_LEN, SymbolName, DataSymNameLength(dataSym) );

                        AllocSym( mi, addr, (LPSTR) mi->TmpSym.Name);
                    } else {
                        mi->TmpSym.NameLength = DataSymNameLength(dataSym);
                        memcpy( mi->TmpSym.Name, SymbolName, mi->TmpSym.NameLength );
                        mi->TmpSym.Name[mi->TmpSym.NameLength] = 0;
                        AllocSym( mi, addr, mi->TmpSym.Name);
                    }
                }
                j += dataSym->reclen + 2;
                dataSym = (DATASYM32*) ((ULONG_PTR)dataSym + dataSym->reclen + 2);
            }
            break;
        }
        else if (omfDirEntry->SubSection == sstSrcModule &&
                 (SymOptions & SYMOPT_LOAD_LINES)) {
            AddLinesForOmfSourceModule(mi, (PCHAR)(pIDD->pMappedCv)+omfDirEntry->lfo,
                                       (OMFSourceModule *)
                                       ((PCHAR)(pIDD->pMappedCv)+omfDirEntry->lfo),
                                       NULL);
        }
    }

    CompleteSymbolTable( mi );

    return TRUE;
}

VOID
GetSymName(
    PIMAGE_SYMBOL Symbol,
    PUCHAR        StringTable,
    LPSTR         s,
    DWORD         size
    )
{
    DWORD i;

    if (Symbol->n_zeroes) {
        for (i=0; i<8; i++) {
            if ((Symbol->n_name[i]>0x1f) && (Symbol->n_name[i]<0x7f)) {
                *s++ = Symbol->n_name[i];
            }
        }
        *s = 0;
    }
    else {
        strncpy( s, (char *) &StringTable[Symbol->n_offset], size );
    }
}


VOID
ProcessOmapForModule(
    PMODULE_ENTRY      mi,
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    PSYMBOL_ENTRY       sym;
    PSYMBOL_ENTRY       symN;
    DWORD               i;

    if (pIDD->pOmapTo) {
        if (pIDD->fOmapToMapped) {
            mi->pOmapTo = MemAlloc(pIDD->cOmapTo * sizeof(OMAP));
            if (mi->pOmapTo) {
                CopyMemory(
                    mi->pOmapTo,
                    pIDD->pOmapTo,
                    pIDD->cOmapTo * sizeof(OMAP)
                    );
            }
        } else {
            mi->pOmapTo = pIDD->pOmapTo;
        }
        mi->cOmapTo = pIDD->cOmapTo;
    }

    if (pIDD->pOmapFrom) {
        if (pIDD->fOmapFromMapped) {
            mi->pOmapFrom = MemAlloc(pIDD->cOmapFrom * sizeof(OMAP));
            if (mi->pOmapFrom) {
                CopyMemory(
                    mi->pOmapFrom,
                    pIDD->pOmapFrom,
                    pIDD->cOmapFrom * sizeof(OMAP)
                    );
            }
        } else {
            mi->pOmapFrom = pIDD->pOmapFrom;
        }

        mi->cOmapFrom = pIDD->cOmapFrom;
    }

    if (!mi->pOmapFrom ||
        !mi->symbolTable ||
        ((mi->SymType != SymCoff) && (mi->SymType != SymCv))
       )
    {
        return;
    }

    for (i=0; i<mi->numsyms; i++) {
        ProcessOmapSymbol( mi, &mi->symbolTable[i] );
    }

    CompleteSymbolTable( mi );
}


BOOL
ProcessOmapSymbol(
    PMODULE_ENTRY       mi,
    PSYMBOL_ENTRY       sym
    )
{
    DWORD           bias;
    DWORD64         OptimizedSymAddr;
    DWORD           rvaSym;
    POMAPLIST       pomaplistHead;
    DWORD64         SymbolValue;
    DWORD64         OrgSymAddr;
    POMAPLIST       pomaplistNew;
    POMAPLIST       pomaplistPrev;
    POMAPLIST       pomaplistCur;
    POMAPLIST       pomaplistNext;
    DWORD           rva;
    DWORD           rvaTo;
    DWORD           cb;
    DWORD           end;
    DWORD           rvaToNext;
    LPSTR           NewSymName;
    CHAR            Suffix[32];
    DWORD64         addrNew;
    POMAP           pomap;
    PSYMBOL_ENTRY   symOmap;

    if ((sym->Flags & SYMF_OMAP_GENERATED) || (sym->Flags & SYMF_OMAP_MODIFIED)) {
        return FALSE;
    }

    OrgSymAddr = SymbolValue = sym->Address;

    OptimizedSymAddr = ConvertOmapFromSrc( mi, SymbolValue, &bias );

    if (OptimizedSymAddr == 0) {
        //
        // No equivalent address
        //
        sym->Address = 0;
        return FALSE;

    }

    //
    // We have successfully converted
    //
    sym->Address = OptimizedSymAddr + bias;

    rvaSym = (ULONG)(SymbolValue - mi->BaseOfDll);
    SymbolValue = sym->Address;

    pomap = GetOmapFromSrcEntry( mi, OrgSymAddr );
    if (!pomap) {
        goto exit;
    }

    pomaplistHead = NULL;

    //
    // Look for all OMAP entries belonging to SymbolEntry
    //

    end = (ULONG)(OrgSymAddr - mi->BaseOfDll + sym->Size);

    while (pomap && (pomap->rva < end)) {

        if (pomap->rvaTo == 0) {
            pomap++;
            continue;
        }

        //
        // Allocate and initialize a new entry
        //
        pomaplistNew = (POMAPLIST) MemAlloc( sizeof(OMAPLIST) );
        if (!pomaplistNew) {
            return FALSE;
        }

        pomaplistNew->omap = *pomap;
        pomaplistNew->cb = pomap[1].rva - pomap->rva;

        pomaplistPrev = NULL;
        pomaplistCur = pomaplistHead;

        while (pomaplistCur != NULL) {
            if (pomap->rvaTo < pomaplistCur->omap.rvaTo) {
                //
                // Insert between Prev and Cur
                //
                break;
            }
            pomaplistPrev = pomaplistCur;
            pomaplistCur = pomaplistCur->next;
        }

        if (pomaplistPrev == NULL) {
            //
            // Insert in head position
            //
            pomaplistHead = pomaplistNew;
        } else {
            pomaplistPrev->next = pomaplistNew;
        }

        pomaplistNew->next = pomaplistCur;

        pomap++;
    }

    if (pomaplistHead == NULL) {
        goto exit;
    }

    pomaplistCur = pomaplistHead;
    pomaplistNext = pomaplistHead->next;

    //
    // we do have a list
    //
    while (pomaplistNext != NULL) {
        rva = pomaplistCur->omap.rva;
        rvaTo  = pomaplistCur->omap.rvaTo;
        cb = pomaplistCur->cb;
        rvaToNext = pomaplistNext->omap.rvaTo;

        if (rvaToNext == sym->Address - mi->BaseOfDll) {
            //
            // Already inserted above
            //
        } else if (rvaToNext < (rvaTo + cb + 8)) {
            //
            // Adjacent to previous range
            //
        } else {
            addrNew = mi->BaseOfDll + rvaToNext;
            Suffix[0] = '_';
            _ltoa( pomaplistNext->omap.rva - rvaSym, &Suffix[1], 10 );
            memcpy( mi->TmpSym.Name, sym->Name, sym->NameLength );
            strncpy( &mi->TmpSym.Name[sym->NameLength], Suffix, strlen(Suffix) + 1 );
            symOmap = AllocSym( mi, addrNew, mi->TmpSym.Name);
            if (symOmap) {
                symOmap->Flags |= SYMF_OMAP_GENERATED;
            }
        }

        MemFree(pomaplistCur);

        pomaplistCur = pomaplistNext;
        pomaplistNext = pomaplistNext->next;
    }

    MemFree(pomaplistCur);

exit:
    if (sym->Address != OrgSymAddr) {
        sym->Flags |= SYMF_OMAP_MODIFIED;
    }

    return TRUE;
}


DWORD64
ConvertOmapFromSrc(
    PMODULE_ENTRY  mi,
    DWORD64        addr,
    LPDWORD        bias
    )
{
    DWORD   rva;
    DWORD   comap;
    POMAP   pomapLow;
    POMAP   pomapHigh;
    DWORD   comapHalf;
    POMAP   pomapMid;


    *bias = 0;

    if (!mi->pOmapFrom) {
        return addr;
    }

    rva = (DWORD)(addr - mi->BaseOfDll);

    comap = mi->cOmapFrom;
    pomapLow = mi->pOmapFrom;
    pomapHigh = pomapLow + comap;

    while (pomapLow < pomapHigh) {

        comapHalf = comap / 2;

        pomapMid = pomapLow + ((comap & 1) ? comapHalf : (comapHalf - 1));

        if (rva == pomapMid->rva) {
            if (pomapMid->rvaTo) {
                return mi->BaseOfDll + pomapMid->rvaTo;
            } else {
                return(0);      // No need adding the base.  This address was discarded...
            }
        }

        if (rva < pomapMid->rva) {
            pomapHigh = pomapMid;
            comap = (comap & 1) ? comapHalf : (comapHalf - 1);
        } else {
            pomapLow = pomapMid + 1;
            comap = comapHalf;
        }
    }

    //
    // If no exact match, pomapLow points to the next higher address
    //
    if (pomapLow == mi->pOmapFrom) {
        //
        // This address was not found
        //
        return 0;
    }

    if (pomapLow[-1].rvaTo == 0) {
        //
        // This address is in a discarded block
        //
        return 0;
    }

    //
    // Return the closest address plus the bias
    //
    *bias = rva - pomapLow[-1].rva;

    return mi->BaseOfDll + pomapLow[-1].rvaTo;
}


DWORD64
ConvertOmapToSrc(
    PMODULE_ENTRY  mi,
    DWORD64        addr,
    LPDWORD        bias,
    BOOL           fBackup
    )
{
    DWORD   rva;
    DWORD   comap;
    POMAP   pomapLow;
    POMAP   pomapHigh;
    DWORD   comapHalf;
    POMAP   pomapMid;

    *bias = 0;

    if (!mi->pOmapTo) {
        return addr;
    }

    rva = (DWORD)(addr - mi->BaseOfDll);

    comap = mi->cOmapTo;
    pomapLow = mi->pOmapTo;
    pomapHigh = pomapLow + comap;

    while (pomapLow < pomapHigh) {

        comapHalf = comap / 2;

        pomapMid = pomapLow + ((comap & 1) ? comapHalf : (comapHalf - 1));

        if (rva == pomapMid->rva) {
            if (pomapMid->rvaTo == 0) {
                //
                // We may be at the start of an inserted branch instruction
                //

                if (fBackup) {
                    //
                    // Return information about the next lower address
                    //

                    rva--;
                    pomapLow = pomapMid;
                    break;
                }

                return 0;
            }

            return mi->BaseOfDll + pomapMid->rvaTo;
        }

        if (rva < pomapMid->rva) {
            pomapHigh = pomapMid;
            comap = (comap & 1) ? comapHalf : (comapHalf - 1);
        } else {
            pomapLow = pomapMid + 1;
            comap = comapHalf;
        }
    }

    //
    // If no exact match, pomapLow points to the next higher address
    //
    
    if (pomapLow == mi->pOmapTo) {
        //
        // This address was not found
        //
        return 0;
    }

    // find the previous valid item in the omap

    do {
        pomapLow--;
        if (pomapLow->rvaTo)
            break;
    } while (pomapLow > mi->pOmapTo);

    // should never occur

    assert(pomapLow->rvaTo);
    if (pomapLow->rvaTo == 0) {
        return 0;   
    }

    //
    // Return the new address plus the bias
    //
    *bias = rva - pomapLow->rva;

    return mi->BaseOfDll + pomapLow->rvaTo;
}

POMAP
GetOmapFromSrcEntry(
    PMODULE_ENTRY  mi,
    DWORD64        addr
    )
{
    DWORD   rva;
    DWORD   comap;
    POMAP   pomapLow;
    POMAP   pomapHigh;
    DWORD   comapHalf;
    POMAP   pomapMid;


    if (mi->pOmapFrom == NULL) {
        return NULL;
    }

    rva = (DWORD)(addr - mi->BaseOfDll);

    comap = mi->cOmapFrom;
    pomapLow = mi->pOmapFrom;
    pomapHigh = pomapLow + comap;

    while (pomapLow < pomapHigh) {

        comapHalf = comap / 2;

        pomapMid = pomapLow + ((comap & 1) ? comapHalf : (comapHalf - 1));

        if (rva == pomapMid->rva) {
            return pomapMid;
        }

        if (rva < pomapMid->rva) {
            pomapHigh = pomapMid;
            comap = (comap & 1) ? comapHalf : (comapHalf - 1);
        } else {
            pomapLow = pomapMid + 1;
            comap = comapHalf;
        }
    }

    return NULL;
}

LPSTR
StringDup(
    LPSTR str
    )
{
    LPSTR ds = (LPSTR) MemAlloc( strlen(str) + 1 );
    if (ds) {
        strcpy( ds, str );
    }
    return ds;
}


BOOL
InternalGetModule(
    HANDLE  hProcess,
    LPSTR   ModuleName,
    DWORD64 ImageBase,
    DWORD   ImageSize,
    PVOID   Context
    )
{
    InternalLoadModule(
            hProcess,
            ModuleName,
            NULL,
            ImageBase,
            ImageSize,
            NULL
            );

    return TRUE;
}


BOOL
LoadedModuleEnumerator(
    HANDLE         hProcess,
    LPSTR          ModuleName,
    DWORD64        ImageBase,
    DWORD          ImageSize,
    PLOADED_MODULE lm
    )
{
    if (lm->EnumLoadedModulesCallback64) {
        return lm->EnumLoadedModulesCallback64( ModuleName, ImageBase, ImageSize, lm->Context );
    } else {
        return lm->EnumLoadedModulesCallback32( ModuleName, (DWORD)ImageBase, ImageSize, lm->Context );
    }
}


LPSTR
SymUnDNameInternal(
    LPSTR UnDecName,
    DWORD UnDecNameLength,
    LPSTR DecName,
    DWORD DecNameLength
    )
{
    LPSTR p;
    ULONG Suffix;
    ULONG i;
    LPSTR TmpDecName;


    UnDecName[0] = 0;

    if ((DecName[0] == '?') || (DecName[0] == '.' && DecName[1] == '.' && DecName[2] == '?')) {

        __try {

            if (DecName[0] == '.' && DecName[1] == '.') {
                Suffix = 2;
                UnDecName[0] = '.';
                UnDecName[1] = '.';
            } else {
                Suffix = 0;
            }

            TmpDecName = MemAlloc( 4096 );
            if (!TmpDecName) {
                strncat( UnDecName, DecName, min(DecNameLength,UnDecNameLength) );
                return UnDecName;
            }
            TmpDecName[0] = 0;
            strncat( TmpDecName, DecName+Suffix, DecNameLength );

            if(UnDecorateSymbolName( TmpDecName,
                                     UnDecName+Suffix,
                                     UnDecNameLength-Suffix,
                                     UNDNAME_NAME_ONLY ) == 0 ) {
                strncat( UnDecName, DecName, min(DecNameLength,UnDecNameLength) );
            }

            MemFree( TmpDecName );

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            strncat( UnDecName, DecName, min(DecNameLength,UnDecNameLength) );

        }

        if (SymOptions & SYMOPT_NO_CPP) {
            while (p = strstr( UnDecName, "::" )) {
                p[0] = '_';
                p[1] = '_';
            }
        }

    } else {

        __try {

            if (DecName[0] == '_' || DecName[0] == '@') {
                DecName += 1;
                DecNameLength -= 1;
            }
            p = 0;
            for (i = 0; i < DecNameLength; i++) {
                if (DecName [i] == '@') {
                    p = &DecName [i];
                    break;
                }
            }
            if (p) {
                i = (int)(p - DecName);
            } else {
                i = min(DecNameLength,UnDecNameLength);
            }
            strncat( UnDecName, DecName, i );

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            strncat( UnDecName, DecName, min(DecNameLength,UnDecNameLength) );

        }

    }

    return UnDecName;
}



PIMAGEHLP_SYMBOL
symcpy32(
    PIMAGEHLP_SYMBOL  External,
    PSYMBOL_ENTRY       Internal
    )
{
    External->Address      = (ULONG)Internal->Address;
    External->Size         = Internal->Size;
    External->Flags        = Internal->Flags;

    External->Name[0] = 0;
    strncat( External->Name, Internal->Name, External->MaxNameLength );

    return External;
}

PIMAGEHLP_SYMBOL64
symcpy64(
    PIMAGEHLP_SYMBOL64  External,
    PSYMBOL_ENTRY       Internal
    )
{
    External->Address      = Internal->Address;
    External->Size         = Internal->Size;
    External->Flags        = Internal->Flags;

    External->Name[0] = 0;
    strncat( External->Name, Internal->Name, External->MaxNameLength );

    return External;
}

BOOL
SympConvertSymbol64To32(
    PIMAGEHLP_SYMBOL64 Symbol64,
    PIMAGEHLP_SYMBOL Symbol32
    )
{
    Symbol32->Address = (DWORD)Symbol64->Address;
    Symbol32->Size = Symbol64->Size;
    Symbol32->Flags = Symbol64->Flags;
    Symbol32->MaxNameLength = Symbol64->MaxNameLength;
    Symbol32->Name[0] = 0;
    strncat( Symbol32->Name, Symbol64->Name, Symbol32->MaxNameLength );

    return (Symbol64->Address >> 32) == 0;
}

BOOL
SympConvertSymbol32To64(
    PIMAGEHLP_SYMBOL Symbol32,
    PIMAGEHLP_SYMBOL64 Symbol64
    )
{
    Symbol64->Address = Symbol32->Address;
    Symbol64->Size = Symbol32->Size;
    Symbol64->Flags = Symbol32->Flags;
    Symbol64->MaxNameLength = Symbol32->MaxNameLength;
    Symbol64->Name[0] = 0;
    strncat( Symbol64->Name, Symbol32->Name, Symbol64->MaxNameLength );

    return TRUE;
}

BOOL
SympConvertLine64To32(
    PIMAGEHLP_LINE64 Line64,
    PIMAGEHLP_LINE Line32
    )
{
    Line32->Key = Line64->Key;
    Line32->LineNumber = Line64->LineNumber;
    Line32->FileName = Line64->FileName;
    Line32->Address = (DWORD)Line64->Address;

    return (Line64->Address >> 32) == 0;
}

BOOL
SympConvertLine32To64(
    PIMAGEHLP_LINE Line32,
    PIMAGEHLP_LINE64 Line64
    )
{
    Line64->Key = Line32->Key;
    Line64->LineNumber = Line32->LineNumber;
    Line64->FileName = Line32->FileName;
    Line64->Address = Line32->Address;

    return TRUE;
}

DWORD
ReadInProcMemory(
    HANDLE    hProcess,
    DWORD64   addr,
    PVOID     buf,
    DWORD     bytes,
    DWORD    *bytesread
    )
{
    DWORD                    rc;
    PPROCESS_ENTRY           pe;
    IMAGEHLP_CBA_READ_MEMORY rm;

    rm.addr      = addr;
    rm.buf       = buf;
    rm.bytes     = bytes;
    rm.bytesread = bytesread;

    rc = FALSE;
    
    __try {
        pe = FindProcessEntry(hProcess);
        if (!pe) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        if (pe->pCallbackFunction32) {
            rc = pe->pCallbackFunction32(pe->hProcess,
                                         CBA_READ_MEMORY,
                                         (PVOID)&rm,
                                         (PVOID)pe->CallbackUserContext);
        
        } else if (pe->pCallbackFunction64) {
            rc = pe->pCallbackFunction64(pe->hProcess,
                                         CBA_READ_MEMORY,
                                         (ULONG64)&rm,
                                         pe->CallbackUserContext);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        rc = FALSE;
    }

    return (rc != FALSE);
}

__inline
BOOL
miReadInProcMemory(
    PMODULE_ENTRY mi,
    ULONG_PTR     addr,
    PVOID         buf,
    DWORD         bytes,
    DWORD        *bytesread
    )
{
    return ReadInProcMemory(mi->hProcess, /*(DWORD64)(PCHAR)*/mi->InProcImageBase + addr, buf, bytes, bytesread);
}

DWORD64
miGetModuleBase(
    HANDLE  hProcess,
    DWORD64 Address
    )
{
    IMAGEHLP_MODULE64 ModuleInfo = {0};
    ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);

    if (SymGetModuleInfo64(hProcess, Address, &ModuleInfo)) {
        return ModuleInfo.BaseOfImage;
    } else {
        return 0;
    }
}

BOOL
GetPData(
    HANDLE        hp,
    PMODULE_ENTRY mi
    )
{
    BOOL status;
    ULONG cb;
    PCHAR pc;
    BOOL  fROM = FALSE;
    IMAGE_DOS_HEADER DosHeader;
    IMAGE_NT_HEADERS ImageNtHeaders;
    PIMAGE_FILE_HEADER ImageFileHdr;
    PIMAGE_OPTIONAL_HEADER ImageOptionalHdr;
    PIMAGE_OPTIONAL_HEADER32 OptionalHeader32 = NULL;
    PIMAGE_OPTIONAL_HEADER64 OptionalHeader64 = NULL;
    ULONG feCount = 0;
    ULONG i;

    HANDLE fh = 0;
    PCHAR  base = NULL;
    USHORT                       filetype;
    PIMAGE_SEPARATE_DEBUG_HEADER  sdh;
    PIMAGE_DOS_HEADER dh;
    PIMAGE_NT_HEADERS inth;
    PIMAGE_OPTIONAL_HEADER32 ioh32;
    PIMAGE_OPTIONAL_HEADER64 ioh64;
    ULONG cdd;
    PCHAR p;
    PIMAGE_DEBUG_DIRECTORY dd;
    ULONG cexp = 0;
    ULONG tsize;
    ULONG csize;

    // IA64 is not ready to roll with on-demand pdata.
    // Call the original code.

    if (mi->MachineType == IMAGE_FILE_MACHINE_IA64)
        goto ia64;

    // if the pdata is already loaded, return

    if (mi->pExceptionData) 
        return TRUE;

    if (!ENSURE_SYMBOLS(hp, mi)) 
        return FALSE;

    if (!mi->dsExceptions)
        return FALSE;

    // open the file and get the file typ

    fh = CreateFile(mi->LoadedImageName,
                    GENERIC_READ,
                    OSVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ? (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE) : (FILE_SHARE_READ | FILE_SHARE_WRITE),
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

    if (fh == INVALID_HANDLE_VALUE) 
        return FALSE;

    base = MapItRO(fh);
    if (!base)
        goto cleanup;
    p = base;

    filetype = *(USHORT *)p;
    if (filetype == IMAGE_DOS_SIGNATURE)
        goto image;
    if (filetype == IMAGE_SEPARATE_DEBUG_SIGNATURE)
        goto dbg;
    goto cleanup;

image:

    // process disk-based image

    dh = (PIMAGE_DOS_HEADER)p;
    p  += dh->e_lfanew;
    inth = (PIMAGE_NT_HEADERS)p;

    if (inth->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        ioh32 = (PIMAGE_OPTIONAL_HEADER32)&inth->OptionalHeader;
        p = base + ioh32->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress;
        csize = ioh32->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size;
    } 
    else if (inth->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        ioh64 = (PIMAGE_OPTIONAL_HEADER64)&inth->OptionalHeader;
        p = base + ioh64->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress;
        csize = ioh64->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size;
    }
    
    if (!csize)
        goto cleanup;

    switch (mi->MachineType) 
    {
    case IMAGE_FILE_MACHINE_ALPHA:
        cexp = csize / sizeof(IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY);
        break;
    case IMAGE_FILE_MACHINE_ALPHA64:
        cexp = csize / sizeof(IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY);
        break;
    case IMAGE_FILE_MACHINE_IA64:
        cexp = csize / sizeof(IMAGE_IA64_RUNTIME_FUNCTION_ENTRY);
        break;
    default:
        goto cleanup;
    }
    
    goto table;

dbg:

    // process dbg file

    sdh = (PIMAGE_SEPARATE_DEBUG_HEADER)p;
    cdd = sdh->DebugDirectorySize / sizeof(IMAGE_DEBUG_DIRECTORY);
    p +=  sizeof(IMAGE_SEPARATE_DEBUG_HEADER) +
          (sdh->NumberOfSections * sizeof(IMAGE_SECTION_HEADER)) +
          sdh->ExportedNamesSize;
    dd = (PIMAGE_DEBUG_DIRECTORY)p;

    for (i = 0; i < cdd; i++, dd++) {
        if (dd->Type == IMAGE_DEBUG_TYPE_EXCEPTION) {
            p = base + dd->PointerToRawData;
            cexp = dd->SizeOfData / sizeof(IMAGE_FUNCTION_ENTRY);
            break;
        }
    }
    
table:
    
    // parse the pdata into a table

    if (!cexp) 
        goto cleanup;

    tsize = cexp * sizeof(IMGHLP_RVA_FUNCTION_DATA);

    mi->pExceptionData = VirtualAlloc( NULL, tsize, MEM_COMMIT, PAGE_READWRITE );

    if (mi->pExceptionData) {
        IMGHLP_RVA_FUNCTION_DATA *pIRFD = mi->pExceptionData;
        switch (mi->MachineType) {

        case IMAGE_FILE_MACHINE_ALPHA:
            if (filetype == IMAGE_SEPARATE_DEBUG_SIGNATURE) {
                // easy case.  The addresses are already in rva format.
                PIMAGE_FUNCTION_ENTRY pFE = (PIMAGE_FUNCTION_ENTRY)p;
                for (i = 0; i < cexp; i++) {
                    pIRFD[i].rvaBeginAddress     = pFE[i].StartingAddress;
                    pIRFD[i].rvaEndAddress       = pFE[i].EndingAddress;
                    pIRFD[i].rvaPrologEndAddress = pFE[i].EndOfPrologue;
                    pIRFD[i].rvaExceptionHandler = 0;
                    pIRFD[i].rvaHandlerData      = 0;
                }
            } else {
                PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY pRFE = (PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY)p;
                for (i = 0; i < cexp; i++) {
                    pIRFD[i].rvaBeginAddress     = pRFE[i].BeginAddress - (ULONG)mi->BaseOfDll;
                    pIRFD[i].rvaEndAddress       = pRFE[i].EndAddress - (ULONG)mi->BaseOfDll;
                    pIRFD[i].rvaPrologEndAddress = pRFE[i].PrologEndAddress - (ULONG)mi->BaseOfDll;
                    pIRFD[i].rvaExceptionHandler = pRFE[i].ExceptionHandler - (ULONG)mi->BaseOfDll;
                    pIRFD[i].rvaHandlerData      = pRFE[i].HandlerData - (ULONG)mi->BaseOfDll;
                }
            }
            break;

        case IMAGE_FILE_MACHINE_ALPHA64:
            {
                PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY pRFE = (PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY)p;
                for (i = 0; i < cexp; i++) {
                    pIRFD[i].rvaBeginAddress     = (DWORD)(pRFE[i].BeginAddress - mi->BaseOfDll);
                    pIRFD[i].rvaEndAddress       = (DWORD)(pRFE[i].EndAddress - mi->BaseOfDll);
                    pIRFD[i].rvaPrologEndAddress = (DWORD)(pRFE[i].PrologEndAddress - mi->BaseOfDll);
                    pIRFD[i].rvaExceptionHandler = (DWORD)(pRFE[i].ExceptionHandler - mi->BaseOfDll);
                    pIRFD[i].rvaHandlerData      = (DWORD)(pRFE[i].HandlerData - mi->BaseOfDll);
                }
            }
            break;

        case IMAGE_FILE_MACHINE_IA64:
            {
                PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY pRFE = (PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY)p; // mi->pImageFunction
                for (i = 0; i < cexp; i++) {
                    pIRFD[i].rvaBeginAddress     = pRFE[i].BeginAddress;
                    pIRFD[i].rvaEndAddress       = pRFE[i].EndAddress;
                    pIRFD[i].rvaPrologEndAddress = pRFE[i].UnwindInfoAddress;
                    pIRFD[i].rvaExceptionHandler = 0;
                    pIRFD[i].rvaHandlerData      = 0;
                }
            }
            break;

        default:
            break;
        }
        
        VirtualProtect( mi->pExceptionData, tsize, PAGE_READONLY, &i );

        mi->dwEntries     = cexp;
    }
                                   
cleanup:
    
    if (base)
        UnmapViewOfFile(base);

    if (fh)
        CloseHandle(fh);

    return (cexp) ? TRUE : FALSE;




// The is the original GetPData, preserved for IA64 support which still relies 
// on this working by calling from in-proc memory.

ia64:
    
// if the pdata is already loaded, return

    if (mi->pExceptionData) 
        return TRUE;

    // get in-process header pointers

    status = miReadInProcMemory(mi, 0, &DosHeader, sizeof(DosHeader), &cb);
    if (!status || cb != sizeof(DosHeader) || DosHeader.e_magic != IMAGE_DOS_SIGNATURE) 
        return FALSE;

    status = miReadInProcMemory(mi, DosHeader.e_lfanew, &ImageNtHeaders, sizeof(ImageNtHeaders), &cb);
    if (!status || cb != sizeof(ImageNtHeaders))
        return FALSE;

    ImageFileHdr = &ImageNtHeaders.FileHeader;
    if (ImageNtHeaders.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        OptionalHeader32 = (PIMAGE_OPTIONAL_HEADER32)&ImageNtHeaders.OptionalHeader;
        OptionalHeader64 = NULL; 
    } else
    if (ImageNtHeaders.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        OptionalHeader64 = (PIMAGE_OPTIONAL_HEADER64)&ImageNtHeaders.OptionalHeader;
        OptionalHeader32 = NULL;
    }
    
    // this doesn't work with ROM images

    if (ImageFileHdr->SizeOfOptionalHeader == sizeof(IMAGE_ROM_OPTIONAL_HEADER)) 
        return FALSE;

    // copy pdata from in-process memory

    if (pc = MemAlloc(OPTIONALHEADER(DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size))) {
        status = miReadInProcMemory(mi, 
                                    OPTIONALHEADER(DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress),
                                    pc, 
                                    OPTIONALHEADER(DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size),
                                    &cb);

        if (status && cb == OPTIONALHEADER(DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size)) {
            switch (ImageFileHdr->Machine) {
                case IMAGE_FILE_MACHINE_ALPHA:
                    feCount = OPTIONALHEADER(DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size) /
                              sizeof(IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY);
                    break;

                case IMAGE_FILE_MACHINE_ALPHA64:
                    feCount = OPTIONALHEADER(DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size) /
                              sizeof(IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY);
                    break;

                case IMAGE_FILE_MACHINE_IA64:
                    feCount = OPTIONALHEADER(DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size) /
                              sizeof(IMAGE_IA64_RUNTIME_FUNCTION_ENTRY);
                    break;
            }
        } 
    }

    // parse the pdata into a table

    if (feCount) {

        ULONG TableSize = feCount * sizeof(IMGHLP_RVA_FUNCTION_DATA);

        mi->pExceptionData = VirtualAlloc( NULL, TableSize, MEM_COMMIT, PAGE_READWRITE );

        if (mi->pExceptionData) {
            IMGHLP_RVA_FUNCTION_DATA *pIRFD = mi->pExceptionData;

            switch (mi->MachineType) {
                case IMAGE_FILE_MACHINE_ALPHA:
                    {
                        PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY pRFE = (PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY)pc; // mi->pImageFunction
                        DWORD ImageBase = (DWORD)mi->InProcImageBase;
                        for (i = 0; i < feCount; i++) {
                            pIRFD[i].rvaBeginAddress     = pRFE[i].BeginAddress - ImageBase;
                            pIRFD[i].rvaEndAddress       = pRFE[i].EndAddress - ImageBase;
                            pIRFD[i].rvaPrologEndAddress = pRFE[i].PrologEndAddress - ImageBase;
                            pIRFD[i].rvaExceptionHandler = pRFE[i].ExceptionHandler - ImageBase;
                            pIRFD[i].rvaHandlerData      = pRFE[i].HandlerData - ImageBase;
                        }
                    }
                    break;

                case IMAGE_FILE_MACHINE_ALPHA64:
                    {
                        PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY pRFE = (PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY)pc; // mi->pImageFunction
                        DWORD ImageBase = (DWORD)mi->InProcImageBase;
                        for (i = 0; i < feCount; i++) {
                            pIRFD[i].rvaBeginAddress     = (DWORD)(pRFE[i].BeginAddress - ImageBase);
                            pIRFD[i].rvaEndAddress       = (DWORD)(pRFE[i].EndAddress - ImageBase);
                            pIRFD[i].rvaPrologEndAddress = (DWORD)(pRFE[i].PrologEndAddress - ImageBase);
                            pIRFD[i].rvaExceptionHandler = (DWORD)(pRFE[i].ExceptionHandler - ImageBase);
                            pIRFD[i].rvaHandlerData      = (DWORD)(pRFE[i].HandlerData - ImageBase);
                        }
                    }
                    break;

                case IMAGE_FILE_MACHINE_IA64:
                    {
                        PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY pRFE = (PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY)pc; // mi->pImageFunction
                        for (i = 0; i < feCount; i++) {
                            pIRFD[i].rvaBeginAddress     = pRFE[i].BeginAddress;
                            pIRFD[i].rvaEndAddress       = pRFE[i].EndAddress;
                            pIRFD[i].rvaPrologEndAddress = pRFE[i].UnwindInfoAddress;
                            pIRFD[i].rvaExceptionHandler = 0;
                            pIRFD[i].rvaHandlerData      = 0;
                        }
                    }
                    break;
                default:
                    break;
           }
            
            VirtualProtect( mi->pExceptionData, TableSize, PAGE_READONLY, &i );

            mi->dwEntries     = feCount;
        }
    }
                                   
    // clean up

    if (pc)
        MemFree(pc);

    return TRUE;
}
