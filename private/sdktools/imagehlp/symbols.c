/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    symbols.c

Abstract:

    This function implements a generic simple symbol handler.

Author:

    Wesley Witt (wesw) 1-Sep-1994

Environment:

    User Mode

--*/

#include "private.h"
#include "symbols.h"


BOOL
IMAGEAPI
SympGetSymNextPrev(
    IN     HANDLE              hProcess,
    IN OUT PIMAGEHLP_SYMBOL64  Symbol,
    IN     int                 Direction
    );

//
// globals
//
LIST_ENTRY      ProcessList;
BOOL            SymInitialized;
DWORD           SymOptions = SYMOPT_UNDNAME;

BOOL
IMAGEAPI
SymInitialize(
    IN HANDLE   hProcess,
    IN LPSTR    UserSearchPath,
    IN BOOL     InvadeProcess
    )

/*++

Routine Description:

    This function initializes the symbol handler for
    a process.  The process is identified by the
    process handle passed into this function.

Arguments:

    hProcess        - Process handle.  If InvadeProcess is FALSE
                      then this can be any unique value that identifies
                      the process to the symbol handler.

    UserSearchPath  - Pointer to a string of paths separated by semicolons.
                      These paths are used to search for symbol files.
                      The value NULL is acceptable.

    InvadeProcess   - If this is set to TRUE then the process identified
                      by the process handle is "invaded" and it's loaded
                      module list is enumerated.  Each module is added
                      to the symbol handler and symbols are attempted
                      to be loaded.

Return Value:

    TRUE            - The symbol handler was successfully initialized.

    FALSE           - The initialization failed.  Call GetLastError to
                      discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY  ProcessEntry;

    __try {

        if (!SymInitialized) {
            SymInitialized = TRUE;
            InitializeListHead( &ProcessList );
        }

        if (FindProcessEntry( hProcess )) {
            SetLastError( ERROR_INVALID_HANDLE );
            return TRUE;
        }

        ProcessEntry = (PPROCESS_ENTRY) MemAlloc( sizeof(PROCESS_ENTRY) );
        if (!ProcessEntry) {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }
        ZeroMemory( ProcessEntry, sizeof(PROCESS_ENTRY) );

        ProcessEntry->hProcess = hProcess;
        ProcessEntry->pid = (int) GetPID(hProcess);
        InitializeListHead( &ProcessEntry->ModuleList );
        InsertTailList( &ProcessList, &ProcessEntry->ListEntry );

        if (!SymSetSearchPath( hProcess, UserSearchPath )) {
            //
            // last error code was set by SymSetSearchPath, so just return
            //
            SymCleanup( hProcess );
            return FALSE;
        }

        if (InvadeProcess) {
            DWORD DosError = GetProcessModules(hProcess, InternalGetModule, NULL);
            if (DosError) {
                SymCleanup( hProcess );
                SetLastError( DosError );
                return FALSE;
            }
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}


BOOL
IMAGEAPI
SymCleanup(
    HANDLE hProcess
    )

/*++

Routine Description:

    This function cleans up the symbol handler's data structures
    for a previously initialized process.

Arguments:

    hProcess        - Process handle.

Return Value:

    TRUE            - The symbol handler was successfully cleaned up.

    FALSE           - The cleanup failed.  Call GetLastError to
                      discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY      ProcessEntry;
    PLIST_ENTRY         Next;
    PMODULE_ENTRY       ModuleEntry;


    __try {

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        Next = ProcessEntry->ModuleList.Flink;
        if (Next) {
            while (Next != &ProcessEntry->ModuleList) {
                ModuleEntry = CONTAINING_RECORD( Next, MODULE_ENTRY, ListEntry );
                Next = ModuleEntry->ListEntry.Flink;
                FreeModuleEntry( ModuleEntry );
            }
        }

        CloseSymbolServer();

        if (ProcessEntry->SymbolSearchPath) {
            MemFree( ProcessEntry->SymbolSearchPath );
        }

        RemoveEntryList( &ProcessEntry->ListEntry );
        MemFree( ProcessEntry );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}


DWORD
IMAGEAPI
SymSetOptions(
    DWORD   UserOptions
    )

/*++

Routine Description:

    This function changes the symbol handler's option mask.

Arguments:

    UserOptions     - The new options mask.

Return Value:

    The new mask is returned.

--*/

{
    SymOptions = UserOptions;
    return SymOptions;
}


DWORD
IMAGEAPI
SymGetOptions(
    VOID
    )

/*++

Routine Description:

    This function queries the symbol handler's option mask.

Arguments:

    None.

Return Value:

    The current options mask is returned.

--*/

{
    return SymOptions;
}


BOOL
SympEnumerateModules(
    IN HANDLE   hProcess,
    IN PROC     EnumModulesCallback,
    IN PVOID    UserContext,
    IN BOOL     Use64
    )

/*++

Routine Description:

    This is the worker function for the 32 and 64 bit versions.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    EnumModulesCallback - Callback pointer that is called once for each
                          module that is enumerated.  If the enum callback
                          returns FALSE then the enumeration is terminated.

    UserContext         - This data is simply passed on to the callback function
                          and is completly user defined.

    Use64               - Supplies flag which determines whether to use the 32 bit
                          or 64 bit callback prototype.

Return Value:

    TRUE                - The modules were successfully enumerated.

    FALSE               - The enumeration failed.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY  ProcessEntry;
    PMODULE_ENTRY   ModuleEntry;
    PLIST_ENTRY     Next;


    __try {

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        Next = ProcessEntry->ModuleList.Flink;
        if (Next) {
            while (Next != &ProcessEntry->ModuleList) {
                ModuleEntry = CONTAINING_RECORD( Next, MODULE_ENTRY, ListEntry );
                Next = ModuleEntry->ListEntry.Flink;
                if (Use64) {
                    if ( !(*(PSYM_ENUMMODULES_CALLBACK64)EnumModulesCallback) (
                            ModuleEntry->ModuleName,
                            ModuleEntry->BaseOfDll,
                            UserContext
                            )) {
                        break;
                    }
                } else {
                    if ( !(*(PSYM_ENUMMODULES_CALLBACK)EnumModulesCallback) (
                            ModuleEntry->ModuleName,
                            (DWORD)ModuleEntry->BaseOfDll,
                            UserContext
                            )) {
                        break;
                    }
                }
            }
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}

BOOL
IMAGEAPI
SymEnumerateModules(
    IN HANDLE                      hProcess,
    IN PSYM_ENUMMODULES_CALLBACK EnumModulesCallback,
    IN PVOID                       UserContext
    )

/*++

Routine Description:

    This function enumerates all of the modules that are currently
    loaded into the symbol handler.  This is the 32 bit wrapper.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    EnumModulesCallback - Callback pointer that is called once for each
                          module that is enumerated.  If the enum callback
                          returns FALSE then the enumeration is terminated.

    UserContext         - This data is simply passed on to the callback function
                          and is completly user defined.

Return Value:

    TRUE                - The modules were successfully enumerated.

    FALSE               - The enumeration failed.  Call GetLastError to
                          discover the cause of the failure.

--*/
{
    return SympEnumerateModules(hProcess, (PROC)EnumModulesCallback, UserContext, FALSE);
}


BOOL
IMAGEAPI
SymEnumerateModules64(
    IN HANDLE   hProcess,
    IN PSYM_ENUMMODULES_CALLBACK64 EnumModulesCallback,
    IN PVOID    UserContext
    )

/*++

Routine Description:

    This function enumerates all of the modules that are currently
    loaded into the symbol handler.  This is the 64 bit wrapper.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    EnumModulesCallback - Callback pointer that is called once for each
                          module that is enumerated.  If the enum callback
                          returns FALSE then the enumeration is terminated.

    UserContext         - This data is simply passed on to the callback function
                          and is completly user defined.

Return Value:

    TRUE                - The modules were successfully enumerated.

    FALSE               - The enumeration failed.  Call GetLastError to
                          discover the cause of the failure.

--*/
{
    return SympEnumerateModules(hProcess, (PROC)EnumModulesCallback, UserContext, TRUE);
}

int __cdecl
CompareAddrs (const void *addr1, const void *addr2) {
    LONGLONG Diff = *(DWORD64 *)addr1 - *(DWORD64 *)addr2;

    if (Diff < 0) {
        return -1;
    } else if (Diff > 0) {
        return 1;
    } else {
        return 0;
    }
}

BOOL
SympEnumerateSymbols(
    IN HANDLE  hProcess,
    IN ULONG64 BaseOfDll,
    IN PROC    EnumSymbolsCallback,
    IN PVOID   UserContext,
    IN BOOL    Use64,
    IN BOOL    CallBackUsesUnicode
    )

/*++

Routine Description:

    This function enumerates all of the symbols contained the module
    specified by the BaseOfDll argument.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize

    BaseOfDll           - Base address of the DLL that symbols are to be
                          enumerated for

    EnumSymbolsCallback - User specified callback routine for enumeration
                          notification

    UserContext         - Pass thru variable, this is simply passed thru to the
                          callback function

    Use64               - Supplies flag which determines whether to use the 32 bit
                          or 64 bit callback prototype.

Return Value:

    TRUE                - The symbols were successfully enumerated.

    FALSE               - The enumeration failed.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY      ProcessEntry;
    PLIST_ENTRY         Next;
    PMODULE_ENTRY       ModuleEntry;
    DWORD               i;
    PSYMBOL_ENTRY       sym;
    LPSTR               szSymName;

    __try {

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        Next = ProcessEntry->ModuleList.Flink;
        if (Next) {
            while (Next != &ProcessEntry->ModuleList) {
                ModuleEntry = CONTAINING_RECORD( Next, MODULE_ENTRY, ListEntry );
                Next = ModuleEntry->ListEntry.Flink;
                if (ModuleEntry->BaseOfDll == BaseOfDll) {
                    if (ModuleEntry->Flags & MIF_DEFERRED_LOAD) {
                        if ((ModuleEntry->Flags & MIF_NO_SYMBOLS) ||
                                !CompleteDeferredSymbolLoad( hProcess, ModuleEntry )) {
                            continue;
                        }
                    }

                    if (ModuleEntry->SymType == SymPdb) {

                        // In order to set the size field in the callback, we need to read
                        // all the addresses, sort the array, then calculate the size based
                        // on the delta between each.

                        DATASYM32 *dataSym = NULL;
                        DWORD64    *pPdbAddrs;
                        DWORD       cPdbAddrs = 4096;   // Arbitrary - initial 32k allocation for addr array
                        DWORD       SymCount = 0;
                        DWORD64     addr;
                        DWORD       size;
                        DWORD       Bias;
                        LPSTR       SymbolName;
                        UCHAR       SymbolLen;
                        unsigned short k;

                        pPdbAddrs = MemAlloc(cPdbAddrs * sizeof(addr));
                        if (!pPdbAddrs)
                            return FALSE;

                        // Init the addr array with an end ptr
                        *pPdbAddrs = ModuleEntry->DllSize + ModuleEntry->BaseOfDll;
                        SymCount = 1;

                        for (;;) {

                            dataSym = (DATASYM32 *) GSINextSym( ModuleEntry->gsi, (PUCHAR)dataSym );

                            if (dataSym == NULL)
                                break;          // Hit the terminator

                            k = DataSymSeg(dataSym) - 1;

                            if (k < ModuleEntry->OriginalNumSections) {
                                SymbolName = DataSymNameStart(dataSym);

                                if (*(ULONG UNALIGNED *)SymbolName == 0x435f3f3f) /* starts with ??_C */
                                    continue;           // ignore strings

                                addr = ModuleEntry->BaseOfDll + DataSymOffset(dataSym);
                                addr += ModuleEntry->OriginalSectionHdrs[k].VirtualAddress;

                                addr = ConvertOmapFromSrc( ModuleEntry, addr, &Bias );

                                if (addr != 0)
                                    addr += Bias;

                                *(pPdbAddrs+SymCount) = addr;

                                SymCount++;

                                if (SymCount >= cPdbAddrs) {
                                    PVOID pTmp = MemReAlloc(pPdbAddrs, (cPdbAddrs + 1024) * sizeof(addr));
                                    if (!pTmp)
                                        return FALSE;
                                    pPdbAddrs = pTmp;
                                    cPdbAddrs += 1024;
                                }
                            }
                        }

                        // Sort the addr array
                        qsort(pPdbAddrs, SymCount, sizeof(addr), CompareAddrs);

                        // Now go through again, this time sending it off

                        dataSym = NULL;

                        for (;;) {
                            unsigned short k;

                            dataSym = (DATASYM32 *) GSINextSym( ModuleEntry->gsi, (PUCHAR)dataSym );

                            if (dataSym == NULL)
                                break;

                            k = DataSymSeg(dataSym) - 1;

                            if (k < ModuleEntry->OriginalNumSections) {
                                SymbolName = DataSymNameStart(dataSym);

                                if (*(ULONG UNALIGNED *)SymbolName == 0x435f3f3f) /* starts with ??_C */
                                    continue;           // ignore strings

                                SymbolLen  = DataSymNameLength(dataSym);
                                if (SymOptions & SYMOPT_UNDNAME) {
                                    SymUnDNameInternal( ModuleEntry->TmpSym.Name, TMP_SYM_LEN-sizeof(ModuleEntry->TmpSym), SymbolName, SymbolLen);
                                } else {
                                    ModuleEntry->TmpSym.Name[0] = 0;
                                    strncat( ModuleEntry->TmpSym.Name, SymbolName, __min(SymbolLen, TMP_SYM_LEN-sizeof(ModuleEntry->TmpSym)) );
                                }

                                addr = ModuleEntry->BaseOfDll + DataSymOffset(dataSym);
                                addr += ModuleEntry->OriginalSectionHdrs[k].VirtualAddress;
                                addr = ConvertOmapFromSrc( ModuleEntry, addr, &Bias );

                                if (addr != 0)
                                    addr += Bias;

                                {
                                    // Find the addr in the array
                                    DWORD High;
                                    DWORD Low;
                                    DWORD Middle;
                                    LONG Result;

                                    Low = 0;
                                    High = SymCount - 1;
                                    while (High >= Low) {
                                        Middle = (Low + High) >> 1;
                                        Result = CompareAddrs(&addr, &pPdbAddrs[Middle]);

                                        if (Result < 0)
                                            High = Middle - 1;
                                        else if (Result > 0)
                                            Low = Middle + 1;
                                        else
                                            break;
                                    }

                                    if (High < Low)
                                        size = 0;
                                    else
                                        size = (DWORD)(pPdbAddrs[Middle+1] - pPdbAddrs[Middle]);
                                }

                                if (Use64) {
                                    if ( !(*(PSYM_ENUMSYMBOLS_CALLBACK64)EnumSymbolsCallback) (
                                                ModuleEntry->TmpSym.Name,
                                                addr,
                                                size,
                                                UserContext )) {
                                        break;
                                    }
                                } else {
                                    if (CallBackUsesUnicode) {
                                        PWSTR pszTmp = AnsiToUnicode(ModuleEntry->TmpSym.Name);
                                        if ( !(*(PSYM_ENUMSYMBOLS_CALLBACKW)EnumSymbolsCallback) (
                                                    pszTmp,
                                                    (DWORD)addr,
                                                    size,
                                                    UserContext )) {
                                            free(pszTmp);
                                            break;
                                        }
                                        free(pszTmp);
                                    } else {
                                        if ( !(*(PSYM_ENUMSYMBOLS_CALLBACK)EnumSymbolsCallback) (
                                                    ModuleEntry->TmpSym.Name,
                                                    (DWORD)addr,
                                                    size,
                                                    UserContext )) {
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        MemFree(pPdbAddrs);
                        return TRUE;
                    }

                    for (i = 0; i < ModuleEntry->numsyms; i++) {
                        sym = &ModuleEntry->symbolTable[i];
                        ModuleEntry->TmpSym.Name[0] = 0;
                        strncat( ModuleEntry->TmpSym.Name, sym->Name, TMP_SYM_LEN );
                        if (Use64) {
                            if ( !(*(PSYM_ENUMSYMBOLS_CALLBACK64)EnumSymbolsCallback) (
                                        ModuleEntry->TmpSym.Name,
                                        sym->Address,
                                        sym->Size,
                                        UserContext )) {
                                break;
                            }
                        } else {
                            if (CallBackUsesUnicode) {
                                PWSTR pszTmp = AnsiToUnicode(ModuleEntry->TmpSym.Name);
                                if ( !(*(PSYM_ENUMSYMBOLS_CALLBACKW)EnumSymbolsCallback) (
                                            pszTmp,
                                            (DWORD)sym->Address,
                                            sym->Size,
                                            UserContext )) {
                                    free(pszTmp);
                                    break;
                                }
                                free(pszTmp);
                            } else {
                                if ( !(*(PSYM_ENUMSYMBOLS_CALLBACK)EnumSymbolsCallback) (
                                            ModuleEntry->TmpSym.Name,
                                            (DWORD)sym->Address,
                                            sym->Size,
                                            UserContext )) {
                                    break;
                                }
                            }
                        }
                    }
                    break;
                }
            }
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}

BOOL
IMAGEAPI
SymEnumerateSymbols(
    IN HANDLE                       hProcess,
    IN ULONG                        BaseOfDll,
    IN PSYM_ENUMSYMBOLS_CALLBACK    EnumSymbolsCallback,
    IN PVOID                        UserContext
    )

/*++

Routine Description:

    This function enumerates all of the symbols contained the module
    specified by the BaseOfDll argument.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize

    BaseOfDll           - Base address of the DLL that symbols are to be
                          enumerated for

    EnumSymbolsCallback - User specified callback routine for enumeration
                          notification

    UserContext         - Pass thru variable, this is simply passed thru to the
                          callback function

Return Value:

    TRUE                - The symbols were successfully enumerated.

    FALSE               - The enumeration failed.  Call GetLastError to
                          discover the cause of the failure.

--*/
{
    return SympEnumerateSymbols(hProcess, BaseOfDll, (PROC)EnumSymbolsCallback, UserContext, FALSE, FALSE);
}

BOOL
IMAGEAPI
SymEnumerateSymbolsW(
    IN HANDLE                       hProcess,
    IN ULONG                        BaseOfDll,
    IN PSYM_ENUMSYMBOLS_CALLBACKW   EnumSymbolsCallback,
    IN PVOID                        UserContext
    )
{
    return SympEnumerateSymbols(hProcess, BaseOfDll, (PROC)EnumSymbolsCallback, UserContext, FALSE, TRUE);
}

BOOL
IMAGEAPI
SymEnumerateSymbols64(
    IN HANDLE                       hProcess,
    IN ULONG64                      BaseOfDll,
    IN PSYM_ENUMSYMBOLS_CALLBACK64  EnumSymbolsCallback,
    IN PVOID                        UserContext
    )
{
    return SympEnumerateSymbols(hProcess, BaseOfDll, (PROC)EnumSymbolsCallback, UserContext, TRUE, FALSE);
}

BOOL
IMAGEAPI
SymEnumerateSymbolsW64(
    IN HANDLE                       hProcess,
    IN ULONG64                      BaseOfDll,
    IN PSYM_ENUMSYMBOLS_CALLBACK64W EnumSymbolsCallback,
    IN PVOID                        UserContext
    )
{
    return SympEnumerateSymbols(hProcess, BaseOfDll, (PROC)EnumSymbolsCallback, UserContext, TRUE, TRUE);
}

BOOL
SympGetSymFromAddr(
    IN  HANDLE              hProcess,
    IN  DWORD64             Address,
    OUT PDWORD64            Displacement,
    OUT PSYMBOL_ENTRY       SymRet
    )

/*++

Routine Description:

    This function finds an entry in the symbol table based on an address.
    This is the common worker function for the 32 and 64 bit API.


Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Address             - Address of the desired symbol.


    Displacement        - This value is set to the offset from the beginning
                          of the symbol.

    sym                 - Returns the found symbol

Return Value:

    TRUE - The symbol was located.

    FALSE - The symbol was not found.  Call GetLastError to
              discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY      ProcessEntry;
    PMODULE_ENTRY       mi;
    PSYMBOL_ENTRY       psym;

    __try {

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        mi = GetModuleForPC( ProcessEntry, Address, FALSE );
        if (mi == NULL) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        if (!ENSURE_SYMBOLS(hProcess, mi)) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }
        
        psym = GetSymFromAddr( Address, Displacement, mi );
        if (psym) {
            *SymRet = *psym;
        } else {
            SetLastError( ERROR_INVALID_ADDRESS );
            return FALSE;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}

BOOL
IMAGEAPI
SymGetSymFromAddr64(
    IN  HANDLE              hProcess,
    IN  DWORD64             Address,
    OUT PDWORD64            Displacement,
    OUT PIMAGEHLP_SYMBOL64  Symbol
    )

/*++

Routine Description:

    This function finds an entry in the symbol table based on an address.


Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Address             - Address of the desired symbol.


    Displacement        - This value is set to the offset from the beginning
                          of the symbol.

    Symbol              - Returns the found symbol

Return Value:

    TRUE - The symbol was located.

    FALSE - The symbol was not found.  Call GetLastError to
              discover the cause of the failure.

--*/
{
    SYMBOL_ENTRY sym;

    if (SympGetSymFromAddr(hProcess, Address, Displacement, &sym)) {
        symcpy64(Symbol, &sym);
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
IMAGEAPI
SymGetSymFromAddr(
    IN  HANDLE              hProcess,
    IN  DWORD               Address,
    OUT PDWORD              Displacement,
    OUT PIMAGEHLP_SYMBOL    Symbol
    )

/*++

Routine Description:

    This function finds an entry in the symbol table based on an address.


Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Address             - Address of the desired symbol.


    Displacement        - This value is set to the offset from the beginning
                          of the symbol.

    Symbol              - Returns the found symbol

Return Value:

    TRUE - The symbol was located.

    FALSE - The symbol was not found.  Call GetLastError to
              discover the cause of the failure.

--*/
{
    SYMBOL_ENTRY sym;
    DWORD64 qDisplacement;

    if (SympGetSymFromAddr(hProcess, Address, &qDisplacement, &sym)) {
        symcpy32(Symbol, &sym);
        if (Displacement) {
            *Displacement = (DWORD)qDisplacement;
        }
        return TRUE;
    } else {
        return FALSE;
    }
}


BOOL
SympGetSymFromName(
    IN  HANDLE          hProcess,
    IN  LPSTR           Name,
    OUT PSYMBOL_ENTRY   SymRet
    )

/*++

Routine Description:

    This function finds an entry in the symbol table based on a name.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    SymName             - A string containing the symbol name.

    sym                 - Returns the located symbol

Return Value:

    TRUE - The symbol was located.

    FALSE - The symbol was not found.  Call GetLastError to
              discover the cause of the failure.

--*/

{
    LPSTR               p;
    PPROCESS_ENTRY      ProcessEntry;
    PMODULE_ENTRY       mi = NULL;
    PLIST_ENTRY         Next;
    PSYMBOL_ENTRY       psym;


    __try {

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        p = strchr( Name, '!' );
        if (p > Name) {

            LPSTR ModName = MemAlloc(p - Name + 1);
            memcpy(ModName, Name, (int)(p - Name));
            ModName[p-Name] = 0;

            //
            // the caller wants to look in a specific module
            //

            mi = FindModule(hProcess, ProcessEntry, ModName, TRUE);

            MemFree(ModName);

            if (mi != NULL) {
                psym = FindSymbolByName( ProcessEntry, mi, p+1 );
                if (psym) {
                    *SymRet = *psym;
                    return TRUE;
                }
            }

            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        Next = ProcessEntry->ModuleList.Flink;
        if (!Next) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        while (Next != &ProcessEntry->ModuleList) {

            mi = CONTAINING_RECORD( Next, MODULE_ENTRY, ListEntry );
            Next = mi->ListEntry.Flink;

            if (!ENSURE_SYMBOLS(hProcess, mi)) {
                continue;
            }

            psym = FindSymbolByName( ProcessEntry, mi, Name );
            if (psym) {
                *SymRet = *psym;
                return TRUE;
            }
        }

        SetLastError( ERROR_MOD_NOT_FOUND );
        return FALSE;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    SetLastError( ERROR_INVALID_FUNCTION );
    return FALSE;
}

BOOL
IMAGEAPI
SymGetSymFromName64(
    IN  HANDLE              hProcess,
    IN  LPSTR               Name,
    OUT PIMAGEHLP_SYMBOL64  Symbol
    )

/*++

Routine Description:

    This function finds an entry in the symbol table based on a name.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    SymName             - A string containing the symbol name.

    Symbol              - Returns found symbol

Return Value:

    TRUE - The symbol was located.

    FALSE - The symbol was not found.  Call GetLastError to
              discover the cause of the failure.

--*/
{
    SYMBOL_ENTRY sym;

    if (SympGetSymFromName(hProcess, Name, &sym)) {
        symcpy64(Symbol, &sym);
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
IMAGEAPI
SymGetSymFromName(
    IN  HANDLE              hProcess,
    IN  LPSTR               Name,
    OUT PIMAGEHLP_SYMBOL  Symbol
    )

/*++

Routine Description:

    This function finds an entry in the symbol table based on a name.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    SymName             - A string containing the symbol name.

    Symbol              - Returns found symbol

Return Value:

    TRUE - The symbol was located.

    FALSE - The symbol was not found.  Call GetLastError to
              discover the cause of the failure.

--*/
{
    SYMBOL_ENTRY sym;

    if (SympGetSymFromName(hProcess, Name, &sym)) {
        symcpy32(Symbol, &sym);
        return TRUE;
    } else {
        return FALSE;
    }
}


BOOL
IMAGEAPI
SymGetSymNext(
    IN     HANDLE              hProcess,
    IN OUT PIMAGEHLP_SYMBOL  Symbol32
    )

/*++

Routine Description:

    This function finds the next symbol in the symbol table that falls
    sequentially after the symbol passed in.


Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Symbol              - Starting symbol.

Return Value:

    Non NULL pointer    - The symbol was located.

    NULL pointer        - The symbol was not found.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PIMAGEHLP_SYMBOL64 Symbol64;
    BOOL r;

    Symbol64 = MemAlloc(sizeof(IMAGEHLP_SYMBOL64) + Symbol32->MaxNameLength);

    if (Symbol64) {
        SympConvertSymbol32To64(Symbol32, Symbol64);
        if (SympGetSymNextPrev(hProcess, Symbol64, 1)) {
            SympConvertSymbol64To32(Symbol64, Symbol32);
            r = TRUE;
        }

        MemFree(Symbol64);
    }
    return r;
}


BOOL
IMAGEAPI
SymGetSymNext64(
    IN     HANDLE              hProcess,
    IN OUT PIMAGEHLP_SYMBOL64  Symbol
    )

/*++

Routine Description:

    This function finds the next symbol in the symbol table that falls
    sequentially after the symbol passed in.


Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Symbol              - Starting symbol.

Return Value:

    Non NULL pointer    - The symbol was located.

    NULL pointer        - The symbol was not found.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    return SympGetSymNextPrev(hProcess, Symbol, 1);
}

BOOL
IMAGEAPI
SymGetSymPrev(
    IN     HANDLE              hProcess,
    IN OUT PIMAGEHLP_SYMBOL  Symbol32
    )

/*++

Routine Description:

    This function finds the next symbol in the symbol table that falls
    sequentially after the symbol passed in.


Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Symbol              - Starting symbol.

Return Value:

    Non NULL pointer    - The symbol was located.

    NULL pointer        - The symbol was not found.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PIMAGEHLP_SYMBOL64 Symbol64;
    BOOL r = FALSE;

    Symbol64 = MemAlloc(sizeof(IMAGEHLP_SYMBOL64) + Symbol32->MaxNameLength);

    if (Symbol64) {
        SympConvertSymbol32To64(Symbol32, Symbol64);
        if (SympGetSymNextPrev(hProcess, Symbol64, -1)) {
            SympConvertSymbol64To32(Symbol64, Symbol32);
            r = TRUE;
        }
        MemFree(Symbol64);
    }
    return r;
}

BOOL
IMAGEAPI
SymGetSymPrev64(
    IN     HANDLE              hProcess,
    IN OUT PIMAGEHLP_SYMBOL64  Symbol
    )

/*++

Routine Description:

    This function finds the next symbol in the symbol table that falls
    sequentially after the symbol passed in.


Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Symbol              - Starting symbol.

Return Value:

    Non NULL pointer    - The symbol was located.

    NULL pointer        - The symbol was not found.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    return SympGetSymNextPrev(hProcess, Symbol, -1);
}

BOOL
SympGetSymNextPrev(
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

    __try {

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        mi = GetModuleForPC( ProcessEntry, Symbol->Address, FALSE );
        if (mi == NULL) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        if (!ENSURE_SYMBOLS(hProcess, mi)) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        sym = GetSymFromAddr( Symbol->Address, &Displacement, mi );
        if (!sym) {
            SetLastError( ERROR_INVALID_ADDRESS );
            return FALSE;
        }

        if (mi->SymType == SymPdb && Direction < 0) {

            sym = GetSymFromAddr( sym->Address, &Displacement, mi );
            if (sym) {
                symcpy64( Symbol, sym );
            } else {
                SetLastError( ERROR_INVALID_ADDRESS );
                return FALSE;
            }

        } else if (mi->SymType == SymPdb) {

            DATASYM32 *nextSym;
            DATASYM32 *bestSym;
            DWORD64 bestAddr;
            PIMAGE_SECTION_HEADER sh;
            DWORD64 addr;
            ULONG k;
            LPSTR SymbolName;
            UCHAR SymbolLen;
            DWORD Bias;

            nextSym = (DATASYM32*)GSINextSym( mi->gsi, NULL );
            bestSym = NULL;
            bestAddr = (DWORD64)-1;

            while( nextSym ) {
                addr = 0;
                k = DataSymSeg(nextSym);

                if ((k <= mi->OriginalNumSections)) {
                    addr = mi->OriginalSectionHdrs[k-1].VirtualAddress + DataSymOffset(nextSym) + mi->BaseOfDll;

                    if ((addr > sym->Address) && (addr < bestAddr)) {

                        SymbolName = DataSymNameStart(nextSym);
                        // ignore strings
                        if (*(ULONG UNALIGNED *)SymbolName != 0x435f3f3f) { /* starts with ??_C */
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

        } else {

            if (Direction > 0 && sym+1 >= mi->symbolTable+mi->numsyms) {
                SetLastError( ERROR_INVALID_ADDRESS );
                return FALSE;
            } else if (Direction < 0 && sym-1 < mi->symbolTable) {
                SetLastError( ERROR_INVALID_ADDRESS );
                return FALSE;
            }

            symcpy64( Symbol, sym + Direction);
        }


        return TRUE;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return FALSE;
}

BOOL
IMAGEAPI
SymGetLineFromAddr64(
    IN  HANDLE                  hProcess,
    IN  DWORD64                 dwAddr,
    OUT PDWORD                  pdwDisplacement,
    OUT PIMAGEHLP_LINE64        Line
    )

/*++

Routine Description:

    This function finds a source file and line number entry for the
    line closest to the given address.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    dwAddr              - Supplies an address for which a line is to be
                          located.

    pdwDisplacement     - Returns the offset between the given address
                          and the first instruction of the line.

    Line                - Returns the line and file information.

Return Value:

    TRUE                - A line was located.

    FALSE               - The line was not found.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY      ProcessEntry;
    PMODULE_ENTRY       mi;

    __try {
        if (Line->SizeOfStruct != sizeof(IMAGEHLP_LINE64)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        mi = GetModuleForPC( ProcessEntry, dwAddr, FALSE );
        if (mi == NULL) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        if (!ENSURE_SYMBOLS(hProcess, mi)) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        if (!GetLineFromAddr(mi, dwAddr, pdwDisplacement, Line)) {
            SetLastError( ERROR_INVALID_ADDRESS );
            return FALSE;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}

BOOL
IMAGEAPI
SymGetLineFromAddr(
    IN  HANDLE                  hProcess,
    IN  DWORD                   dwAddr,
    OUT PDWORD                  pdwDisplacement,
    OUT PIMAGEHLP_LINE        Line32
    )
{
    IMAGEHLP_LINE64 Line64;
    Line64.SizeOfStruct = sizeof(Line64);
    if (SymGetLineFromAddr64(hProcess, dwAddr, pdwDisplacement, &Line64)) {
        SympConvertLine64To32(&Line64, Line32);
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
IMAGEAPI
SymGetLineFromName64(
    IN     HANDLE               hProcess,
    IN     LPSTR                ModuleName,
    IN     LPSTR                FileName,
    IN     DWORD                dwLineNumber,
       OUT PLONG                plDisplacement,
    IN OUT PIMAGEHLP_LINE64     Line
    )

/*++

Routine Description:

    This function finds an entry in the source file and line-number
    information based on a particular filename and line number.

    A module name can be given if the search is to be restricted to
    a specific module.

    The filename can be omitted if a pure line number search is desired,
    in which case Line must be a previously filled out line number
    struct.  The module and file that Line->Address lies in is used
    to look up the new line number.  This cannot be used when a module
    name is given.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    ModuleName          - Module name or NULL.

    FileName            - File name or NULL.

    dwLineNumber        - Line number of interest.

    plDisplacement      - Difference between requested line number and
                          returned line number.

    Line                - Line information input and return.

Return Value:

    TRUE                - A line was located.

    FALSE               - A line was not found.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY      ProcessEntry;
    PMODULE_ENTRY       mi = NULL;
    PLIST_ENTRY         Next;

    __try {
        if (Line->SizeOfStruct != sizeof(IMAGEHLP_LINE64)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        if (ModuleName != NULL) {

            //
            // The caller wants to look in a specific module.
            // A filename must be given in this case because it doesn't
            // make sense to do an address-driven search when a module
            // is explicitly specified since the address also specifies
            // a module.
            //

            if (FileName == NULL) {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }

            mi = FindModule(hProcess, ProcessEntry, ModuleName, TRUE);
            if (mi != NULL &&
                FindLineByName( mi, FileName, dwLineNumber, plDisplacement, Line )) {
                return TRUE;
            }

            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        if (FileName == NULL) {
            // Only a line number has been given, implying that
            // it's a line in the same file as the given line is currently in.

            mi = GetModuleForPC( ProcessEntry, Line->Address, FALSE );
            if (mi == NULL) {
                SetLastError( ERROR_MOD_NOT_FOUND );
                return FALSE;
            }

            if (!ENSURE_SYMBOLS(hProcess, mi)) {
                SetLastError( ERROR_MOD_NOT_FOUND );
                return FALSE;
            }

            if (FindLineByName( mi, FileName, dwLineNumber,
                                plDisplacement, Line )) {
                return TRUE;
            }

            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        Next = ProcessEntry->ModuleList.Flink;
        if (!Next) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        while (Next != &ProcessEntry->ModuleList) {

            mi = CONTAINING_RECORD( Next, MODULE_ENTRY, ListEntry );
            Next = mi->ListEntry.Flink;

            if (!ENSURE_SYMBOLS(hProcess, mi)) {
                continue;
            }

            if (FindLineByName( mi, FileName, dwLineNumber,
                                plDisplacement, Line )) {
                return TRUE;
            }
        }

        SetLastError( ERROR_MOD_NOT_FOUND );
        return FALSE;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    SetLastError( ERROR_INVALID_FUNCTION );
    return FALSE;
}

BOOL
IMAGEAPI
SymGetLineFromName(
    IN     HANDLE               hProcess,
    IN     LPSTR                ModuleName,
    IN     LPSTR                FileName,
    IN     DWORD                dwLineNumber,
       OUT PLONG                plDisplacement,
    IN OUT PIMAGEHLP_LINE     Line32
    )
{
    IMAGEHLP_LINE64 Line64;
    Line64.SizeOfStruct = sizeof(Line64);
    SympConvertLine32To64(Line32, &Line64);
    if (SymGetLineFromName64(hProcess,
                             ModuleName,
                             FileName,
                             dwLineNumber,
                             plDisplacement,
                             &Line64)) {
        return SympConvertLine64To32(&Line64, Line32);
    } else {
        return FALSE;
    }
}


BOOL
IMAGEAPI
SymGetLineNext64(
    IN     HANDLE               hProcess,
    IN OUT PIMAGEHLP_LINE64     Line
    )

/*++

Routine Description:

    This function returns line address information for the line immediately
    following the line given.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Line                - Supplies line number information for the line
                          prior to the one being located.

Return Value:

    TRUE                - A line was located.  The Key, LineNumber and Address
                          of Line are updated.

    FALSE               - No such line exists.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY      ProcessEntry;
    PMODULE_ENTRY       mi;
    PSOURCE_LINE        SrcLine;
    PSOURCE_ENTRY       Src;

    __try {
        if (Line->SizeOfStruct != sizeof(IMAGEHLP_LINE64)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        // Use existing information to look up module and then
        // locate the file information.  The key could be extended
        // to make this unnecessary but it's done as a validation step
        // more than as a way to save a DWORD.

        SrcLine = (PSOURCE_LINE)Line->Key;

        mi = GetModuleForPC( ProcessEntry, SrcLine->Addr, FALSE );
        if (mi == NULL) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        for (Src = mi->SourceFiles; Src != NULL; Src = Src->Next) {
            if (SrcLine >= Src->LineInfo &&
                SrcLine < Src->LineInfo+Src->Lines) {
                break;
            }
        }

        if (Src == NULL) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if (SrcLine == Src->LineInfo+Src->Lines-1) {
            SetLastError(ERROR_NO_MORE_ITEMS);
            return FALSE;
        }

        SrcLine++;
        Line->Key = SrcLine;
        Line->LineNumber = SrcLine->Line;
        Line->Address = SrcLine->Addr;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}

BOOL
IMAGEAPI
SymGetLineNext(
    IN     HANDLE               hProcess,
    IN OUT PIMAGEHLP_LINE     Line32
    )
{
    IMAGEHLP_LINE64 Line64;
    Line64.SizeOfStruct = sizeof(Line64);
    SympConvertLine32To64(Line32, &Line64);
    if (SymGetLineNext64(hProcess, &Line64)) {
        return SympConvertLine64To32(&Line64, Line32);
    } else {
        return FALSE;
    }
}


BOOL
IMAGEAPI
SymGetLinePrev64(
    IN     HANDLE               hProcess,
    IN OUT PIMAGEHLP_LINE64     Line
    )

/*++

Routine Description:

    This function returns line address information for the line immediately
    before the line given.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Line                - Supplies line number information for the line
                          after the one being located.

Return Value:

    TRUE                - A line was located.  The Key, LineNumber and Address
                          of Line are updated.

    FALSE               - No such line exists.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY      ProcessEntry;
    PMODULE_ENTRY       mi;
    PSOURCE_LINE        SrcLine;
    PSOURCE_ENTRY       Src;

    __try {
        if (Line->SizeOfStruct != sizeof(IMAGEHLP_LINE64)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        // Use existing information to look up module and then
        // locate the file information.  The key could be extended
        // to make this unnecessary but it's done as a validation step
        // more than as a way to save a DWORD.

        SrcLine = (PSOURCE_LINE)Line->Key;

        mi = GetModuleForPC( ProcessEntry, SrcLine->Addr, FALSE );
        if (mi == NULL) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        for (Src = mi->SourceFiles; Src != NULL; Src = Src->Next) {
            if (SrcLine >= Src->LineInfo &&
                SrcLine < Src->LineInfo+Src->Lines) {
                break;
            }
        }

        if (Src == NULL) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if (SrcLine == Src->LineInfo) {
            SetLastError(ERROR_NO_MORE_ITEMS);
            return FALSE;
        }

        SrcLine--;
        Line->Key = SrcLine;
        Line->LineNumber = SrcLine->Line;
        Line->Address = SrcLine->Addr;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}

BOOL
IMAGEAPI
SymGetLinePrev(
    IN     HANDLE               hProcess,
    IN OUT PIMAGEHLP_LINE     Line32
    )
{
    IMAGEHLP_LINE64 Line64;
    Line64.SizeOfStruct = sizeof(Line64);
    SympConvertLine32To64(Line32, &Line64);
    if (SymGetLinePrev64(hProcess, &Line64)) {
        return SympConvertLine64To32(&Line64, Line32);
    } else {
        return FALSE;
    }
}


BOOL
IMAGEAPI
SymMatchFileName(
    IN  LPSTR  FileName,
    IN  LPSTR  Match,
    OUT LPSTR *FileNameStop,
    OUT LPSTR *MatchStop
    )

/*++

Routine Description:

    This function attempts to match a string against a filename and path.
    The match string is allowed to be a suffix of the complete filename,
    so this function is useful for matching a plain filename against
    a fully qualified filename.

    Matching begins from the end of both strings and proceeds backwards.
    Matching is case-insensitive and equates \ with /.

Arguments:

    FileName            - Filename to match against.

    Match               - String to match against filename.

    FileNameStop        - Returns pointer into FileName where matching stopped.
                          May be one before FileName for full matches.
                          May be NULL.

    MatchStop           - Returns pointer info Match where matching stopped.
                          May be one before Match for full matches.
                          May be NULL.        

Return Value:

    TRUE                - Match is a matching suffix of FileName.

    FALSE               - Mismatch.

--*/

{
    LPSTR pF, pM;

    pF = FileName+strlen(FileName)-1;
    pM = Match+strlen(Match)-1;

    while (pF >= FileName && pM >= Match) {
        int chF, chM;

        chF = tolower(*pF);
        chF = chF == '\\' ? '/' : chF;
        chM = tolower(*pM);
        chM = chM == '\\' ? '/' : chM;

        if (chF != chM) {
            break;
        }

        pF--;
        pM--;
    }

    if (FileNameStop != NULL) {
        *FileNameStop = pF;
    }
    if (MatchStop != NULL) {
        *MatchStop = pM;
    }

    return pM < Match;
}


BOOL
IMAGEAPI
SymRegisterFunctionEntryCallback(
    IN HANDLE                     hProcess,
    IN PSYMBOL_FUNCENTRY_CALLBACK CallbackFunction,
    IN PVOID                      UserContext
    )
/*++

Routine Description:

    Set the address of a callback routine to access extended function
    table entries directly. This function is useful when debugging
    Alpha processes where RUNTIME_FUNCTION_ENTRYs are available from
    sources other than in the image. Two existing examples are:
    
    1) Access to dynamic function tables for run-time code
    2) Access to function tables for ROM images
    
Arguments:

    hProcess    - Process handle, must have been previously registered
                  with SymInitialize.
                          
                                
    DirectFunctionTableRoutine - Address of direct function table callback routine.
                  On alpha this routine must return a pointer to the
                  RUNTIME_FUNCTION_ENTRY containing the specified address.
                  If no such entry is available, it must return NULL.
    
Return Value:

    TRUE        - The callback was successfully registered

    FALSE       - The initialization failed. Most likely failure is that
                  the hProcess parameter is invalid. Call GetLastError()
                  for specific error codes.
--*/
{
    PPROCESS_ENTRY  ProcessEntry = NULL;

    __try {

        if (!CallbackFunction) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        ProcessEntry->pFunctionEntryCallback32 = CallbackFunction;
        ProcessEntry->pFunctionEntryCallback64 = NULL;
        ProcessEntry->FunctionEntryUserContext = (ULONG64)UserContext;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }
    return TRUE;
}


BOOL
IMAGEAPI
SymRegisterFunctionEntryCallback64(
    IN HANDLE                       hProcess,
    IN PSYMBOL_FUNCENTRY_CALLBACK64 CallbackFunction,
    IN ULONG64                      UserContext
    )
/*++

Routine Description:

    See SymRegisterFunctionEntryCallback64
--*/    
{
    PPROCESS_ENTRY  ProcessEntry = NULL;

    __try {

        if (!CallbackFunction) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        ProcessEntry->pFunctionEntryCallback32 = NULL;
        ProcessEntry->pFunctionEntryCallback64 = CallbackFunction;
        ProcessEntry->FunctionEntryUserContext = (ULONG64)UserContext;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }
    return TRUE;
}

LPVOID
IMAGEAPI
SymFunctionTableAccess(
    HANDLE  hProcess,
    DWORD   AddrBase
    )
{
    return SymFunctionTableAccess64(hProcess, AddrBase);
}

LPVOID
IMAGEAPI
SymFunctionTableAccess64(
    HANDLE  hProcess,
    DWORD64 AddrBase
    )

/*++

Routine Description:

    This function finds a function table entry or FPO record for an address.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    AddrBase            - Supplies an address for which a function table entry
                          or FPO entry is to be located.

Return Value:

    Non NULL pointer    - The symbol was located.

    NULL pointer        - The symbol was not found.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY  ProcessEntry;
    PMODULE_ENTRY   mi;
    PVOID           rtf;

    __try {

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            SetLastError( ERROR_INVALID_HANDLE );
            return NULL;
        }

        mi = GetModuleForPC( ProcessEntry, AddrBase, FALSE );
        if (mi == NULL) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return NULL;
        }

        if (!ENSURE_SYMBOLS(hProcess, mi)) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return NULL;
        }

        switch (mi->MachineType) {
            default:
                rtf = NULL;
                break;

            case IMAGE_FILE_MACHINE_I386:
                if (!mi->pFpoData) {
                    rtf = NULL;
                } else {
                    DWORD Bias;
                    AddrBase = ConvertOmapToSrc( mi, AddrBase, &Bias, TRUE );
                    if (AddrBase == 0) {
                        rtf = NULL;
                    } else {
                        AddrBase += Bias;
                        rtf = SwSearchFpoData( (ULONG)(AddrBase - mi->BaseOfDll), mi->pFpoData, mi->dwEntries );
                    }
                }
                break;

            case IMAGE_FILE_MACHINE_ALPHA:             // Alpha_AXP
                rtf = LookupFunctionEntryAxp32( mi,
                                                (DWORD)AddrBase
                                                );
                break;

            case IMAGE_FILE_MACHINE_IA64:              // Intel 64
                rtf = LookupFunctionEntryIA64( mi,
                                               (ULONG)(AddrBase - mi->BaseOfDll)
                                               );
                break;

            case IMAGE_FILE_MACHINE_AXP64:             // AXP64

                rtf = LookupFunctionEntryAxp64( mi,
                                                AddrBase
                                                );
                break;
        }

        if (!rtf) {
            SetLastError( ERROR_INVALID_ADDRESS );
            return NULL;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return NULL;

    }

    return rtf;
}


BOOL
IMAGEAPI
SymGetModuleInfo64(
    IN  HANDLE              hProcess,
    IN  DWORD64             dwAddr,
    OUT PIMAGEHLP_MODULE64  ModuleInfo
    )
{
    PPROCESS_ENTRY          ProcessEntry;
    PMODULE_ENTRY           mi;
    DWORD                   SizeOfStruct;

    __try {

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        mi = GetModuleForPC( ProcessEntry, dwAddr, FALSE );
        if (mi == NULL) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        SizeOfStruct = ModuleInfo->SizeOfStruct;
        if (SizeOfStruct > sizeof(IMAGEHLP_MODULE64)) {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        ZeroMemory( ModuleInfo, SizeOfStruct);
        ModuleInfo->SizeOfStruct = SizeOfStruct;

        ModuleInfo->BaseOfImage = mi->BaseOfDll;
        ModuleInfo->ImageSize = mi->DllSize;
        ModuleInfo->NumSyms = mi->numsyms;
        ModuleInfo->CheckSum = mi->CheckSum;
        ModuleInfo->TimeDateStamp = mi->TimeDateStamp;
        ModuleInfo->SymType = mi->SymType;
        strcpy( ModuleInfo->ModuleName, mi->ModuleName );
        if (mi->ImageName) {
            strcpy( ModuleInfo->ImageName, mi->ImageName );
        }
        if (mi->LoadedImageName) {
            strcpy( ModuleInfo->LoadedImageName, mi->LoadedImageName );
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}



/*++

Routine Description:

    This routine is used to get type information from a type index or a type-name
    in the module. 
    
   TypeIn      -  Specifies the type of the input
   
   DataIn      -  Pointer to Input data         
   
   TypeOut     -  Specifies type of output data and what kind of processing to 
                  be done.
   
   SizeOut     -  Size of the buffer allocated to the DataOut pointer
   
   DataOut     -  Value/Record to be returned is copied here.
   
++*/

BOOL
IMAGEAPI
SymGetModuleInfoEx64(
    IN  HANDLE          hProcess,
    IN  DWORD64         Address,
    IN  IMAGEHLP_TYPES  TypeIn,
    IN  PBYTE           DataIn,
    IN  IMAGEHLP_TYPES  TypeOut,
    IN  OUT PULONG      SizeOut,
    IN  OUT PBYTE       DataOut
    ){
    PPROCESS_ENTRY      ProcessEntry;
    PLIST_ENTRY         Next;
    PMODULE_ENTRY       ModuleEntry;
    PIMGHLP_DEBUG_DATA  pIDD;
    PMODULE_TYPE_INFO   pModTypes;
    BOOL res=0;

    __try {

       if (!SizeOut || !DataOut || !DataIn) {
          return FALSE;
       }

       ProcessEntry = FindProcessEntry( hProcess );
       if (!ProcessEntry) {
          SetLastError( ERROR_INVALID_HANDLE );
          return FALSE;
       }

       Next = ProcessEntry->ModuleList.Flink;
       if (Next) {
          while (Next != &ProcessEntry->ModuleList) {
             ModuleEntry = CONTAINING_RECORD( Next, MODULE_ENTRY, ListEntry );
             Next = ModuleEntry->ListEntry.Flink;
             if (ModuleEntry->BaseOfDll == Address) {
                break;
             }
          }
       }

       if (!ModuleEntry) {
          return FALSE;
       }

       switch (TypeOut) {
       case IMAGEHLP_TYPEID_MODULE_TYPE_INFO: {
          PBYTE buffer; 
          PDWORD plen;
          CV_typ_t typIndex;

          if (TypeIn == IMAGEHLP_TYPEID_INDEX) {
             typIndex = *((PULONG) DataIn);
          } else {
             break;
          }

          if (ModuleEntry->ptpi) {
             if (*SizeOut) {
                plen = SizeOut; 
                buffer = DataOut;
                return(TypesQueryCVRecordForTi(ModuleEntry->ptpi, typIndex, 
                                               buffer, plen));
             }
          }
          return FALSE;
       } 

       case IMAGEHLP_TYPEID_INDEX: {      
          LPSTR name;
          CV_typ_t *typIndex;

          if (TypeIn == IMAGEHLP_TYPEID_NAME) {
             name = ((LPSTR) DataIn);
          } else {
             break;
          }
          if (*SizeOut >= 4) {
             if (ModuleEntry->ptpi) {
                if (TypesQueryTiForUDT((TPI *) ModuleEntry->ptpi, name, FALSE, (CV_typ_t *) DataOut)) {
                   return TRUE;
                }
             }
             // Try to get something from globals
             return GetPdbTypeInfo((DBI *) ModuleEntry->dbi, 
                                   (GSI *) ModuleEntry->globals, 
                                   name, 
                                   FALSE, 
                                   TypeOut,
                                   DataOut);

          }
          break;
       }

       case IMAGEHLP_TYPEID_TYPE_ENUM_INFO : {
          LPSTR  nameIn=NULL;
          CHAR name[MAX_SYM_NAME + 1];
          ULONG len, res;
          BOOL partial=FALSE;
          CV_typ_t *typeIndex;
          GSI *Globals;

          switch (TypeIn) {
          case IMAGEHLP_TYPEID_IMAGEHLP_SYMBOL64: {
             PIMAGEHLP_SYMBOL64 pTypeName = (PIMAGEHLP_SYMBOL64) DataIn;
             if (pTypeName->SizeOfStruct == sizeof(IMAGEHLP_SYMBOL64)) {
                nameIn = &pTypeName->Name[0];
             }
             break;
          }
          case IMAGEHLP_TYPEID_IMAGEHLP_SYMBOL: {
             PIMAGEHLP_SYMBOL pTypeName = (PIMAGEHLP_SYMBOL) DataIn;
             if (pTypeName->SizeOfStruct == sizeof(IMAGEHLP_SYMBOL)) {
                nameIn = &pTypeName->Name[0];
             }
             break;
          }

          case IMAGEHLP_TYPEID_NAME: {
             nameIn = (LPSTR) DataIn;
             break;
          }
          default:
             break;
          }

          if (!nameIn) {
             break;
          }

          ZeroMemory(name, sizeof(name)); len = strlen (nameIn);
          strncpy(name, nameIn, ( len > MAX_SYM_NAME ? MAX_SYM_NAME : len));

          return GetPdbTypeInfo((DBI *) ModuleEntry->dbi, 
                                (GSI *) ModuleEntry->globals, 
                                name, 
                                TRUE, 
                                TypeOut,
                                DataOut);
       }

       default:
          break;
       } /* switch */

    } __except (EXCEPTION_EXECUTE_HANDLER) {

         ImagepSetLastErrorFromStatus( GetExceptionCode() );
         return FALSE;

    }

    return FALSE;
}


BOOL
IMAGEAPI
SymGetModuleInfoEx(
    IN  HANDLE          hProcess,
    IN  DWORD           Address,
    IN  IMAGEHLP_TYPES  TypeIn,
    IN  PBYTE           DataIn,
    IN  IMAGEHLP_TYPES  TypeOut,
    IN  OUT PULONG      SizeOut,
    IN  OUT PBYTE       DataOut
    ) {
   return SymGetModuleInfoEx64(hProcess, Address, TypeIn, DataIn, TypeOut, SizeOut, DataOut);
}

BOOL
IMAGEAPI
SymGetModuleInfoW(
    IN  HANDLE              hProcess,
    IN  DWORD               dwAddr,
    OUT PIMAGEHLP_MODULEW   W_ModuleInfo
    )
{
    IMAGEHLP_MODULE A_ModuleInfo;

    if (W_ModuleInfo->SizeOfStruct != sizeof(IMAGEHLP_MODULEW)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    ZeroMemory(W_ModuleInfo, sizeof(IMAGEHLP_MODULEW));
    W_ModuleInfo->SizeOfStruct = sizeof(IMAGEHLP_MODULEW);
    
    if (!SympConvertUnicodeModule32ToAnsiModule32(
        W_ModuleInfo, &A_ModuleInfo)) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!SymGetModuleInfo(hProcess, dwAddr, &A_ModuleInfo)) {
        return FALSE;
    }

    if (!SympConvertAnsiModule32ToUnicodeModule32(
        &A_ModuleInfo, W_ModuleInfo)) {

        return FALSE;
    }
    return TRUE;
}

BOOL
IMAGEAPI
SymGetModuleInfoW64(
    IN  HANDLE              hProcess,
    IN  DWORD64             dwAddr,
    OUT PIMAGEHLP_MODULEW64 W_ModuleInfo
    )
{

    IMAGEHLP_MODULE64 A_ModuleInfo;

    if (!SympConvertUnicodeModule64ToAnsiModule64(
        W_ModuleInfo, &A_ModuleInfo)) {

        return FALSE;
    }

    if (!SymGetModuleInfo64(hProcess, dwAddr, &A_ModuleInfo)) {
        return FALSE;
    }

    if (!SympConvertAnsiModule64ToUnicodeModule64(
        &A_ModuleInfo, W_ModuleInfo)) {

        return FALSE;
    }
    return TRUE;
}

BOOL
IMAGEAPI
SymGetModuleInfo(
    IN  HANDLE              hProcess,
    IN  DWORD               dwAddr,
    OUT PIMAGEHLP_MODULE   ModuleInfo
    )
{
    PPROCESS_ENTRY          ProcessEntry;
    PMODULE_ENTRY           mi;
    DWORD                   SizeOfStruct;

    __try {

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        mi = GetModuleForPC( ProcessEntry,
            dwAddr == (DWORD)-1 ? (DWORD64)-1 : dwAddr, FALSE );
        if (mi == NULL) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        SizeOfStruct = ModuleInfo->SizeOfStruct;
        if (SizeOfStruct > sizeof(IMAGEHLP_MODULE)) {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        ZeroMemory( ModuleInfo, SizeOfStruct);
        ModuleInfo->SizeOfStruct = SizeOfStruct;

        ModuleInfo->BaseOfImage = (DWORD)mi->BaseOfDll;
        ModuleInfo->ImageSize = mi->DllSize;
        ModuleInfo->NumSyms = mi->numsyms;
        ModuleInfo->CheckSum = mi->CheckSum;
        ModuleInfo->TimeDateStamp = mi->TimeDateStamp;
        ModuleInfo->SymType = mi->SymType;
        strcpy( ModuleInfo->ModuleName, mi->ModuleName );
        if (mi->ImageName) {
            strcpy( ModuleInfo->ImageName, mi->ImageName );
        }
        if (mi->LoadedImageName) {
            strcpy( ModuleInfo->LoadedImageName, mi->LoadedImageName );
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}

DWORD64
IMAGEAPI
SymGetModuleBase64(
    IN  HANDLE  hProcess,
    IN  DWORD64 dwAddr
    )
{
    PPROCESS_ENTRY          ProcessEntry;
    PMODULE_ENTRY           mi;


    __try {

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            return 0;
        }

        mi = GetModuleForPC( ProcessEntry, dwAddr, FALSE );
        if (mi == NULL) {
            return 0;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return mi->BaseOfDll;
}

DWORD
IMAGEAPI
SymGetModuleBase(
    IN  HANDLE hProcess,
    IN  DWORD  dwAddr
    )
{
    return (ULONG)SymGetModuleBase64(hProcess, dwAddr);
}

BOOL
IMAGEAPI
SymUnloadModule64(
    IN  HANDLE      hProcess,
    IN  DWORD64     BaseOfDll
    )

/*++

Routine Description:

    Remove the symbols for an image from a process' symbol table.

Arguments:

    hProcess - Supplies the token which refers to the process

    BaseOfDll - Supplies the offset to the image as supplies by the
        LOAD_DLL_DEBUG_EVENT and UNLOAD_DLL_DEBUG_EVENT.

Return Value:

    Returns TRUE if the module's symbols were successfully unloaded.
    Returns FALSE if the symbol handler does not recognize hProcess or
    no image was loaded at the given offset.

--*/

{
    PPROCESS_ENTRY  ProcessEntry;
    PLIST_ENTRY     Next;
    PMODULE_ENTRY   ModuleEntry;


    __try {

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            return FALSE;
        }

        Next = ProcessEntry->ModuleList.Flink;
        if (Next) {
            while (Next != &ProcessEntry->ModuleList) {
                ModuleEntry = CONTAINING_RECORD( Next, MODULE_ENTRY, ListEntry );
                if (ModuleEntry->BaseOfDll == BaseOfDll) {
                    RemoveEntryList(Next);
                    FreeModuleEntry(ModuleEntry);
                    return TRUE;
                }
                Next = ModuleEntry->ListEntry.Flink;
            }
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return FALSE;
}

BOOL
IMAGEAPI
SymUnloadModule(
    IN  HANDLE      hProcess,
    IN  DWORD       BaseOfDll
    )
{
    return SymUnloadModule64(hProcess, BaseOfDll);
}

DWORD64
IMAGEAPI
SymLoadModule64(
    IN  HANDLE          hProcess,
    IN  HANDLE          hFile,
    IN  PSTR            ImageName,
    IN  PSTR            ModuleName,
    IN  DWORD64         BaseOfDll,
    IN  DWORD           DllSize
    )

/*++

Routine Description:

    Loads the symbols for an image for use by the other Sym functions.

Arguments:

    hProcess - Supplies unique process identifier.

    hFile -

    ImageName - Supplies the name of the image file.

    ModuleName - ???? Supplies the module name that will be returned by
            enumeration functions ????

    BaseOfDll - Supplies loaded base address of image.

    DllSize


Return Value:


--*/

{
    __try {

        return InternalLoadModule( hProcess, ImageName, ModuleName, BaseOfDll, DllSize, hFile );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return 0;

    }
}

DWORD
IMAGEAPI
SymLoadModule(
    IN  HANDLE          hProcess,
    IN  HANDLE          hFile,
    IN  PSTR            ImageName,
    IN  PSTR            ModuleName,
    IN  DWORD           BaseOfDll,
    IN  DWORD           DllSize
    )
{
    return (DWORD)SymLoadModule64( hProcess, hFile, ImageName, ModuleName, BaseOfDll, DllSize );
}

BOOL
IMAGEAPI
SymUnDName(
    IN  PIMAGEHLP_SYMBOL  sym,
    OUT LPSTR               UnDecName,
    OUT DWORD               UnDecNameLength
    )
{
    __try {

        if (SymUnDNameInternal( UnDecName, UnDecNameLength-1, sym->Name, strlen(sym->Name) )) {
            return TRUE;
        } else {
            return FALSE;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }
}

BOOL
IMAGEAPI
SymUnDName64(
    IN  PIMAGEHLP_SYMBOL64  sym,
    OUT LPSTR               UnDecName,
    OUT DWORD               UnDecNameLength
    )
{
    __try {

        if (SymUnDNameInternal( UnDecName, UnDecNameLength-1, sym->Name, strlen(sym->Name) )) {
            return TRUE;
        } else {
            return FALSE;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }
}


BOOL
IMAGEAPI
SymGetSearchPath(
    IN  HANDLE          hProcess,
    OUT LPSTR           SearchPath,
    IN  DWORD           SearchPathLength
    )

/*++

Routine Description:

    This function looks up the symbol search path associated with a process.

Arguments:

    hProcess - Supplies the token associated with a process.

Return Value:

    A pointer to the search path.  Returns NULL if the process is not
    know to the symbol handler.

--*/

{
    PPROCESS_ENTRY ProcessEntry;


    __try {

        ProcessEntry = FindProcessEntry( hProcess );

        if (!ProcessEntry) {
            return FALSE;
        }

        SearchPath[0] = 0;
        strncat( SearchPath, ProcessEntry->SymbolSearchPath, SearchPathLength );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}

BOOL
IMAGEAPI
SymSetSearchPath(
    HANDLE      hProcess,
    LPSTR       UserSearchPath
    )

/*++

Routine Description:

    This functions sets the searh path to be used by the symbol loader
    for the given process.  If UserSearchPath is not supplied, a default
    path will be used.

Arguments:

    hProcess - Supplies the process token associated with a symbol table.

    UserSearchPath - Supplies the new search path to associate with the
        process. If this argument is NULL, the following path is generated:

        .;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%;%WINDIR%

        It is ok if any or all of the environment variables is missing.

Return Value:

    A pointer to the new search path.  The user should not modify this string.
    Returns NULL if the process is not known to the symbol handler.

--*/

{
    PPROCESS_ENTRY  ProcessEntry;
    LPSTR           p;
    DWORD           cbSymPath;
    DWORD           cb;
    char            ExpandedSearchPath[MAX_PATH];

    __try {

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            return FALSE;
        }

        if (ProcessEntry->SymbolSearchPath) {
            MemFree(ProcessEntry->SymbolSearchPath);
        }

        if (UserSearchPath) {
            cbSymPath = ExpandEnvironmentStrings(UserSearchPath, 
                                     ExpandedSearchPath, 
                                     sizeof(ExpandedSearchPath) / sizeof(ExpandedSearchPath[0]));
            if (cbSymPath < sizeof(ExpandedSearchPath)/sizeof(ExpandedSearchPath[0])) {
            ProcessEntry->SymbolSearchPath = StringDup(ExpandedSearchPath);
            } else {
                ProcessEntry->SymbolSearchPath = (LPSTR)MemAlloc( cbSymPath );
                ExpandEnvironmentStrings(UserSearchPath, 
                                         ProcessEntry->SymbolSearchPath,
                                         cbSymPath );
            }
        } else {

            //
            // ".;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%;%WINDIR%"
            //

            cbSymPath = 2;     // ".;"

            //
            // GetEnvironmentVariable returns the size of the string
            // INCLUDING the '\0' in this case.
            //
            cbSymPath += GetEnvironmentVariable( SYMBOL_PATH, NULL, 0 );
            cbSymPath += GetEnvironmentVariable( ALTERNATE_SYMBOL_PATH, NULL, 0 );
            cbSymPath += GetEnvironmentVariable( WINDIR, NULL, 0 );


            p = ProcessEntry->SymbolSearchPath = (LPSTR) MemAlloc( cbSymPath );
            if (!p) {
                return FALSE;
            }

            *p++ = '.';
            --cbSymPath;

            cb = GetEnvironmentVariable(SYMBOL_PATH, p+1, cbSymPath);
            if (cb) {
                *p = ';';
                p += cb+1;
                cbSymPath -= cb+1;
            }

            cb = GetEnvironmentVariable(ALTERNATE_SYMBOL_PATH, p+1, cbSymPath);
            if (cb) {
                *p = ';';
                p += cb+1;
                cbSymPath -= cb+1;
            }

            cb = GetEnvironmentVariable(WINDIR, p+1, cbSymPath);
            if (cb) {
                *p = ';';
                p += cb+1;
            }

            *p = 0;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}


BOOL
IMAGEAPI
EnumerateLoadedModules(
    IN HANDLE                           hProcess,
    IN PENUMLOADED_MODULES_CALLBACK     EnumLoadedModulesCallback,
    IN PVOID                            UserContext
    )
{
    LOADED_MODULE lm;
    DWORD status = NO_ERROR;

    __try {

        lm.EnumLoadedModulesCallback32 = EnumLoadedModulesCallback;
        lm.EnumLoadedModulesCallback64 = NULL;
        lm.Context = UserContext;

        status = GetProcessModules( hProcess, (PINTERNAL_GET_MODULE)LoadedModuleEnumerator, (PVOID)&lm );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return (status == NO_ERROR);
}


BOOL
IMAGEAPI
EnumerateLoadedModules64(
    IN HANDLE                           hProcess,
    IN PENUMLOADED_MODULES_CALLBACK64   EnumLoadedModulesCallback,
    IN PVOID                            UserContext
    )
{
    LOADED_MODULE lm;
    DWORD status = NO_ERROR;

    __try {

        lm.EnumLoadedModulesCallback64 = EnumLoadedModulesCallback;
        lm.EnumLoadedModulesCallback32 = NULL;
        lm.Context = UserContext;

        status = GetProcessModules( hProcess, LoadedModuleEnumerator, (PVOID)&lm );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return (status == NO_ERROR);
}

BOOL
IMAGEAPI
SymRegisterCallback(
    IN HANDLE                        hProcess,
    IN PSYMBOL_REGISTERED_CALLBACK   CallbackFunction,
    IN PVOID                         UserContext
    )
{
    PPROCESS_ENTRY  ProcessEntry = NULL;

    __try {

        if (!CallbackFunction) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        ProcessEntry->pCallbackFunction32 = CallbackFunction;
        ProcessEntry->pCallbackFunction64 = NULL;
        ProcessEntry->CallbackUserContext = (ULONG64)UserContext;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}


BOOL
IMAGEAPI
SymRegisterCallback64(
    IN HANDLE                        hProcess,
    IN PSYMBOL_REGISTERED_CALLBACK64 CallbackFunction,
    IN ULONG64                       UserContext
    )
{
    PPROCESS_ENTRY  ProcessEntry = NULL;

    __try {

        if (!CallbackFunction) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        ProcessEntry->pCallbackFunction32 = NULL;
        ProcessEntry->pCallbackFunction64 = CallbackFunction;
        ProcessEntry->CallbackUserContext = UserContext;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}

BOOL
SympConvertAnsiModule32ToUnicodeModule32(
    PIMAGEHLP_MODULE  A_Module32,
    PIMAGEHLP_MODULEW W_Module32
    )
{
    // BUGBUG - kcarlos - Better sizeof validation

    ZeroMemory(W_Module32, sizeof(*W_Module32));
    W_Module32->SizeOfStruct = sizeof(*W_Module32);

    W_Module32->BaseOfImage = A_Module32->BaseOfImage;
    W_Module32->ImageSize = A_Module32->ImageSize;
    W_Module32->TimeDateStamp = A_Module32->TimeDateStamp;
    W_Module32->CheckSum = A_Module32->CheckSum;
    W_Module32->NumSyms = A_Module32->NumSyms;
    W_Module32->SymType = A_Module32->SymType;

    if (!CopyAnsiToUnicode(W_Module32->ModuleName, A_Module32->ModuleName,
        sizeof(A_Module32->ModuleName) / sizeof(A_Module32->ModuleName[0])) ) {

        return FALSE;
    }

    if (!CopyAnsiToUnicode(W_Module32->ImageName, A_Module32->ImageName,
        sizeof(A_Module32->ImageName) / sizeof(A_Module32->ImageName[0])) ) {

        return FALSE;
    }

    if (!CopyAnsiToUnicode(W_Module32->LoadedImageName, A_Module32->LoadedImageName,
        sizeof(A_Module32->LoadedImageName) / sizeof(A_Module32->LoadedImageName[0])) ) {

        return FALSE;
    }

    return TRUE;
}

BOOL
SympConvertUnicodeModule32ToAnsiModule32(
    PIMAGEHLP_MODULEW W_Module32,
    PIMAGEHLP_MODULE  A_Module32
    )
{
    ZeroMemory(A_Module32, sizeof(*A_Module32));
    A_Module32->SizeOfStruct = sizeof(*A_Module32);

    A_Module32->BaseOfImage = W_Module32->BaseOfImage;
    A_Module32->ImageSize = W_Module32->ImageSize;
    A_Module32->TimeDateStamp = W_Module32->TimeDateStamp;
    A_Module32->CheckSum = W_Module32->CheckSum;
    A_Module32->NumSyms = W_Module32->NumSyms;
    A_Module32->SymType = W_Module32->SymType;

    if (!CopyUnicodeToAnsi(A_Module32->ModuleName, W_Module32->ModuleName,
        sizeof(W_Module32->ModuleName) / sizeof(W_Module32->ModuleName[0])) ) {

        return FALSE;
    }

    if (!CopyUnicodeToAnsi(A_Module32->ImageName, W_Module32->ImageName,
        sizeof(W_Module32->ImageName) / sizeof(W_Module32->ImageName[0])) ) {

        return FALSE;
    }

    if (!CopyUnicodeToAnsi(A_Module32->LoadedImageName, W_Module32->LoadedImageName,
        sizeof(W_Module32->LoadedImageName) / sizeof(W_Module32->LoadedImageName[0])) ) {

        return FALSE;
    }

    return TRUE;
}


BOOL
SympConvertAnsiModule64ToUnicodeModule64(
    PIMAGEHLP_MODULE64  A_Module64,
    PIMAGEHLP_MODULEW64 W_Module64
    )
{
    // BUGBUG - kcarlos - Better sizeof validation

    ZeroMemory(W_Module64, sizeof(*W_Module64));
    W_Module64->SizeOfStruct = sizeof(*W_Module64);

    W_Module64->BaseOfImage = A_Module64->BaseOfImage;
    W_Module64->ImageSize = A_Module64->ImageSize;
    W_Module64->TimeDateStamp = A_Module64->TimeDateStamp;
    W_Module64->CheckSum = A_Module64->CheckSum;
    W_Module64->NumSyms = A_Module64->NumSyms;
    W_Module64->SymType = A_Module64->SymType;

    if (!CopyAnsiToUnicode(W_Module64->ModuleName, A_Module64->ModuleName,
        sizeof(A_Module64->ModuleName) / sizeof(A_Module64->ModuleName[0])) ) {

        return FALSE;
    }

    if (!CopyAnsiToUnicode(W_Module64->ImageName, A_Module64->ImageName,
        sizeof(A_Module64->ImageName) / sizeof(A_Module64->ImageName[0])) ) {

        return FALSE;
    }

    if (!CopyAnsiToUnicode(W_Module64->LoadedImageName, A_Module64->LoadedImageName,
        sizeof(A_Module64->LoadedImageName) / sizeof(A_Module64->LoadedImageName[0])) ) {

        return FALSE;
    }

    return TRUE;
}

BOOL
SympConvertUnicodeModule64ToAnsiModule64(
    PIMAGEHLP_MODULEW64 W_Module64,
    PIMAGEHLP_MODULE64  A_Module64
    )
{
    ZeroMemory(A_Module64, sizeof(*A_Module64));
    A_Module64->SizeOfStruct = sizeof(*A_Module64);

    A_Module64->BaseOfImage = W_Module64->BaseOfImage;
    A_Module64->ImageSize = W_Module64->ImageSize;
    A_Module64->TimeDateStamp = W_Module64->TimeDateStamp;
    A_Module64->CheckSum = W_Module64->CheckSum;
    A_Module64->NumSyms = W_Module64->NumSyms;
    A_Module64->SymType = W_Module64->SymType;

    if (!CopyUnicodeToAnsi(A_Module64->ModuleName, W_Module64->ModuleName,
        sizeof(W_Module64->ModuleName) / sizeof(W_Module64->ModuleName[0])) ) {

        return FALSE;
    }

    if (!CopyUnicodeToAnsi(A_Module64->ImageName, W_Module64->ImageName,
        sizeof(W_Module64->ImageName) / sizeof(W_Module64->ImageName[0])) ) {

        return FALSE;
    }

    if (!CopyUnicodeToAnsi(A_Module64->LoadedImageName, W_Module64->LoadedImageName,
        sizeof(W_Module64->LoadedImageName) / sizeof(W_Module64->LoadedImageName[0])) ) {

        return FALSE;
    }

    return TRUE;
}

/*++

Routine Description:

    This routine looks up a global variable and returns the index for its type.
    
   TypeIn      -  Specifies the type of the input
   
   DataIn      -  Pointer to Input data         
   
   TypeOut     -  Specifies type of output data and what kind of processing to 
                  be done.
   
   SizeOut     -  Size of the buffer allocated to the DataOut pointer
   
   DataOut     -  Value/Record to be returned is copied here.
   
++*/

BOOL 
IMAGEAPI
SymGetSymbolInfo64(
    IN  HANDLE          hProcess,
    IN  DWORD64         Address,
    IN  IMAGEHLP_TYPES  TypeIn,
    IN  PBYTE           DataIn,
    IN  IMAGEHLP_TYPES  TypeOut,
    IN  OUT PULONG      SizeOut,
    IN  OUT PBYTE       DataOut
    ){   
    PMODULE_ENTRY me, ModuleEntry;
    PPROCESS_ENTRY ProcessEntry;
    PLIST_ENTRY Next;
    BOOL partial=TRUE;

    __try {
       if (!SizeOut || !DataOut || !DataIn) {
          return FALSE;
       }

       ProcessEntry = FindProcessEntry( hProcess );
       if (!ProcessEntry) {
          SetLastError(ERROR_INVALID_HANDLE);
          return FALSE;
       }

       me = NULL;
       Next = ProcessEntry->ModuleList.Flink;
       if (Next) {
          while (Next != &ProcessEntry->ModuleList) {
             ModuleEntry = CONTAINING_RECORD( Next, MODULE_ENTRY, ListEntry );
             Next = ModuleEntry->ListEntry.Flink;
             if (ModuleEntry->BaseOfDll == Address) {
                me = ModuleEntry;
             }
          }
       }

       if (!me) {
          return FALSE;
       }
       ModuleEntry = me;

       if ((TypeOut == IMAGEHLP_TYPEID_INDEX) ||
           (TypeOut == IMAGEHLP_TYPEID_TYPE_ENUM_INFO)) {
          LPSTR  nameIn=NULL;
          CHAR name[MAX_SYM_NAME + 1];
          ULONG len;
          CV_typ_t *typeIndex;

          switch (TypeIn) {
          case IMAGEHLP_TYPEID_IMAGEHLP_SYMBOL64: {
             PIMAGEHLP_SYMBOL64 pTypeName = (PIMAGEHLP_SYMBOL64) DataIn;
             if (pTypeName->SizeOfStruct == sizeof(IMAGEHLP_SYMBOL64)) {
                nameIn = &pTypeName->Name[0];
             }
             break;
          }
          case IMAGEHLP_TYPEID_IMAGEHLP_SYMBOL: {
             PIMAGEHLP_SYMBOL pTypeName = (PIMAGEHLP_SYMBOL) DataIn;
             if (pTypeName->SizeOfStruct == sizeof(IMAGEHLP_SYMBOL)) {
                nameIn = &pTypeName->Name[0];
             }
             break;
          }
          case IMAGEHLP_TYPEID_NAME: {
             nameIn = (LPSTR) DataIn;
             break;
          }
          default:
             break;
          }

          if (!nameIn) {
             return FALSE;
          }

          ZeroMemory(name, sizeof(name)); len = strlen (nameIn);
          strncpy(name, nameIn, ( len > MAX_SYM_NAME ? MAX_SYM_NAME : len));

          return GetPdbTypeInfo((DBI *) ModuleEntry->dbi, 
                                (GSI *) ModuleEntry->gsi, 
                                name, 
                                TypeOut == IMAGEHLP_TYPEID_TYPE_ENUM_INFO,
                                TypeOut, 
                                DataOut);
       }
       return FALSE;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
         ImagepSetLastErrorFromStatus( GetExceptionCode() );
         return FALSE;
    }
    return FALSE;
}

BOOL 
IMAGEAPI
SymGetSymbolInfo(
    IN  HANDLE          hProcess,
    IN  DWORD           Address,
    IN  IMAGEHLP_TYPES  TypeIn,
    IN  PBYTE           DataIn,
    IN  IMAGEHLP_TYPES  TypeOut,
    IN  OUT PULONG      SizeOut,
    IN  OUT PBYTE       DataOut
    ) {
   return SymGetSymbolInfo64(hProcess, Address, TypeIn, DataIn, TypeOut, SizeOut, DataOut);
}


//#ifdef _WIN64
#if 0
BOOL  __cdecl  PDBOpenTpi(PDB* ppdb, const char* szMode,  TPI** pptpi) {return FALSE;}
BOOL  __cdecl  PDBCopyTo(PDB* ppdb, const char* szTargetPdb, DWORD dwCopyFilter, DWORD dwReserved){return FALSE;}
BOOL  __cdecl  PDBClose(PDB* ppdb) {return FALSE;}
BOOL  __cdecl  ModQueryImod(Mod* pmod,  USHORT* pimod) {return FALSE;}
BOOL  __cdecl  ModQueryLines(Mod* pmod, BYTE* pbLines, long* pcb) {return FALSE;}
BOOL  __cdecl  DBIQueryModFromAddr(DBI* pdbi, USHORT isect, long off,  Mod** ppmod,  USHORT* pisect,  long* poff,  long* pcb){return FALSE;}
BOOL  __cdecl  ModClose(Mod* pmod){return FALSE;}
BOOL  __cdecl  DBIQueryNextMod(DBI* pdbi, Mod* pmod, Mod** ppmodNext) {return FALSE;}
BYTE* __cdecl  GSINextSym (GSI* pgsi, BYTE* pbSym) {return NULL;}
BOOL  __cdecl  PDBOpen(char* szPDB,char* szMode,SIG sigInitial,EC* pec,char szError[cbErrMax],PDB** pppdb) {return FALSE;}
BOOL  __cdecl  TypesClose(TPI* ptpi){return FALSE;}
BOOL  __cdecl  GSIClose(GSI* pgsi){return FALSE;}
BOOL  __cdecl  DBIClose(DBI* pdbi){return FALSE;}
BYTE* __cdecl  GSINearestSym (GSI* pgsi, USHORT isect, long off, long* pdisp){return NULL;}
BOOL  __cdecl  PDBOpenValidate(char* szPDB,char* szPath,char* szMode,SIG sig,AGE age,EC* pec,char szError[cbErrMax],PDB** pppdb){return FALSE;}
BOOL  __cdecl  PDBOpenDBI(PDB* ppdb, const char* szMode, const char* szTarget,  DBI** ppdbi){return FALSE;}
BOOL  __cdecl  DBIOpenPublics(DBI* pdbi,  GSI **ppgsi){return FALSE;}
BOOL  __cdecl  DBIQuerySecMap(DBI* pdbi,  BYTE* pb, long* pcb){return FALSE;}
#endif
