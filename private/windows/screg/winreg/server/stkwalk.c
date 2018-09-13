/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    stkwalk.c

Abstract:

    This module contains memory debug routines for catching memory leaks and memory
    overwrites.

Author:
    Stolen from dbgmem.c
    Jim Stewart/Ramesh Pabbati    January 8, 1996

    Fixed up for regleaks
    UShaji                        Dec 11th,  1998

Revision History:

--*/

#ifdef LOCAL
#ifdef LEAK_TRACK

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdio.h>
#include<imagehlp.h>
#include "regleak.h"
#include "stkwalk.h"
DWORD   MachineType;            // the architecutre we are on
HANDLE  OurProcess;             // the process that we are running as a part of



// typedefs from imagehlp.dll

typedef BOOL (WINAPI * PFNSYMINITIALIZE)(HANDLE hProcess,
                                         PSTR UserSearchPath,
                                         BOOL fInvadeProcess);

typedef BOOL (WINAPI * PFNSYMCLEANUP)(HANDLE hProcess);

typedef BOOL (WINAPI * PFNSTACKWALK)(DWORD MachineType,
                                  HANDLE hProcess,
                                  HANDLE hThread,
                                  LPSTACKFRAME StackFrame,
                                  PVOID ContextRecord,
                                  PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,
                                  PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
                                  PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
                                  PTRANSLATE_ADDRESS_ROUTINE TranslateAddress);

typedef BOOL (WINAPI * PFNSYMGETSYMFROMADDR)(HANDLE hProcess,
                                             DWORD_PTR Address,
                                             PDWORD_PTR Displacement,
                                             PIMAGEHLP_SYMBOL Symbol);


typedef DWORD_PTR (WINAPI * PFNSYMGETMODULEBASE)(HANDLE hProcess,
                                          DWORD_PTR dwAddr);


typedef PVOID (WINAPI * PFNSYMFUNCTIONTABLEACCESS)(HANDLE hProcess,
                                                DWORD_PTR AddrBase);


// imagehlp function pointers

PFNSYMINITIALIZE            g_pfnSymInitialize=NULL;
PFNSYMCLEANUP               g_pfnSymCleanup=NULL;
PFNSTACKWALK                g_pfnStackWalk=NULL;
PFNSYMGETSYMFROMADDR        g_pfnSymGetSymFromAddr=NULL;
PFNSYMFUNCTIONTABLEACCESS   g_pfnSymFunctionTableAccess=NULL;
PFNSYMGETMODULEBASE         g_pfnSymGetModuleBase=NULL;

HINSTANCE                   g_hImagehlpInstance=NULL;


BOOL fDebugInitialised = FALSE;


BOOL
InitDebug(
    );


DWORD GetStack(
    IN EXCEPTION_POINTERS *exp,
    IN PCALLER_SYM   Caller,
    IN int           Skip,
    IN int           cFind,
    IN int           fResolveSymbols
    );

BOOL LoadImageHLP()
{

   g_hImagehlpInstance = LoadLibrary ("imagehlp.dll");

   if (!g_hImagehlpInstance) {
        return FALSE;
   }


   g_pfnSymInitialize = (PFNSYMINITIALIZE) GetProcAddress (g_hImagehlpInstance,
                                                           "SymInitialize");
   if (!g_pfnSymInitialize) {
        return FALSE;
   }

   g_pfnSymCleanup = (PFNSYMCLEANUP) GetProcAddress (g_hImagehlpInstance,
                                                           "SymCleanup");
   if (!g_pfnSymCleanup) {
        return FALSE;
   }


   g_pfnStackWalk = (PFNSTACKWALK) GetProcAddress (g_hImagehlpInstance,
                                                           "StackWalk");
   if (!g_pfnStackWalk) {
        return FALSE;
   }


   g_pfnSymGetSymFromAddr = (PFNSYMGETSYMFROMADDR) GetProcAddress (g_hImagehlpInstance,
                                                           "SymGetSymFromAddr");
   if (!g_pfnSymGetSymFromAddr) {
        return FALSE;
   }


   g_pfnSymFunctionTableAccess = (PFNSYMFUNCTIONTABLEACCESS) GetProcAddress (g_hImagehlpInstance,
                                                           "SymFunctionTableAccess");
   if (!g_pfnSymFunctionTableAccess) {
        return FALSE;
   }


   g_pfnSymGetModuleBase = (PFNSYMGETMODULEBASE) GetProcAddress (g_hImagehlpInstance,
                                                           "SymGetModuleBase");
   if (!g_pfnSymGetModuleBase) {
        return FALSE;
   }

   return TRUE;
}


BOOL
InitDebug(
    )
/*++

Description:

    This routine initializes the debug memory functionality.

Arguments:

    none

Return Value:

    BOOL - pass or fail

--*/
{
    BOOL        status;
    SYSTEM_INFO SysInfo;

    if (fDebugInitialised)
        return TRUE;

    status = RtlEnterCriticalSection(&(g_RegLeakTraceInfo.StackInitCriticalSection));
    ASSERT( NT_SUCCESS( status ) );

    if (fDebugInitialised)
        return TRUE;

    OurProcess = GetCurrentProcess();



    g_RegLeakTraceInfo.szSymPath = (LPTSTR) RtlAllocateHeap(
                                                            RtlProcessHeap(),
                                                            0,
                                                            SYM_PATH_MAX_SIZE*sizeof(TCHAR));


    if (!g_RegLeakTraceInfo.szSymPath) {
        // looks like machine already doesn't have enough memory
        // disable leak tracking
        g_RegLeakTraceInfo.bEnableLeakTrack = 0;
        return FALSE;
    }

    g_RegLeakTraceInfo.dwMaxStackDepth = GetProfileInt(TEXT("RegistryLeak"), TEXT("StackDepth"), MAX_LEAK_STACK_DEPTH);
    GetProfileString(TEXT("RegistryLeak"), TEXT("SymbolPath"), TEXT(""), g_RegLeakTraceInfo.szSymPath, SYM_PATH_MAX_SIZE);


    if (!(*g_RegLeakTraceInfo.szSymPath)) {

        RtlFreeHeap(
            RtlProcessHeap(),
            0,
            g_RegLeakTraceInfo.szSymPath);

            g_RegLeakTraceInfo.szSymPath = NULL;
    }


    if (!LoadImageHLP()) {
        g_RegLeakTraceInfo.bEnableLeakTrack = FALSE;
        status = RtlLeaveCriticalSection(&(g_RegLeakTraceInfo.StackInitCriticalSection));
        return FALSE;
    }

    GetSystemInfo( &SysInfo );
    switch (SysInfo.wProcessorArchitecture) {

    default:
    case PROCESSOR_ARCHITECTURE_INTEL:
        MachineType = IMAGE_FILE_MACHINE_I386;
        break;

    case PROCESSOR_ARCHITECTURE_MIPS:
        //
        // note this may not detect R10000 machines correctly
        //
        MachineType = IMAGE_FILE_MACHINE_R4000;
        break;

    case PROCESSOR_ARCHITECTURE_ALPHA:
        MachineType = IMAGE_FILE_MACHINE_ALPHA;
        break;

    case PROCESSOR_ARCHITECTURE_PPC:
        MachineType = IMAGE_FILE_MACHINE_POWERPC;
        break;

    }


    // symbols from Current directory/Environment variable _NT_SYMBOL_PATH
    // Environment variable _NT_ALTERNATE_SYMBOL_PATH or Environment variable SYSTEMROOT

    status = g_pfnSymInitialize ( OurProcess, g_RegLeakTraceInfo.szSymPath, FALSE );

    fDebugInitialised = TRUE;

    status = RtlLeaveCriticalSection(&(g_RegLeakTraceInfo.StackInitCriticalSection));
    return( TRUE );
}

BOOL
StopDebug()
{
    if (fDebugInitialised) {

        BOOL fSuccess;

        fSuccess = g_pfnSymCleanup(OurProcess);

        fDebugInitialised = FALSE;

        FreeLibrary(g_hImagehlpInstance);

        if (g_RegLeakTraceInfo.szSymPath) {
            RtlFreeHeap(
                RtlProcessHeap(),
                0,
                g_RegLeakTraceInfo.szSymPath);
        }

        return fSuccess;
    }
    return TRUE;
}

BOOL
ReadMem(
    IN HANDLE   hProcess,
    IN LPCVOID  BaseAddr,
    IN LPVOID   Buffer,
    IN DWORD    Size,
    IN LPDWORD  NumBytes )
/*++

Description:

    This is a callback routine that StackWalk uses - it just calls teh system ReadProcessMemory
    routine with this process's handle

Arguments:


Return Value:

    none

--*/

{
    BOOL    status;

    status = ReadProcessMemory( GetCurrentProcess(),BaseAddr,Buffer,Size,NumBytes );

    return( status );
}


VOID
GetCallStack(
    IN PCALLER_SYM   Caller,
    IN int           Skip,
    IN int           cFind,
    IN int           fResolveSymbols
    )
/*++

Description:

    This routine walks te stack to find the return address of caller. The number of callers
    and the number of callers on top to be skipped can be specified.

Arguments:

    pdwCaller       array of DWORD to return callers
                    return addresses
    Skip            no. of callers to skip
    cFInd           no. of callers to find

Return Value:

    none

--*/
{

    if (!g_RegLeakTraceInfo.bEnableLeakTrack) {
        return;
    }

    if (!InitDebug()) {
        return;
    }

    __try {
        memset(Caller, 0, cFind * sizeof(CALLER_SYM));
        RaiseException(MY_DBG_EXCEPTION, 0, 0, NULL);
        // raise an exception to get the exception record to start the stack walk
        //
    }
    __except(GetStack(GetExceptionInformation(), Caller, Skip, cFind, fResolveSymbols)) {
    }
}

DWORD GetStack(
    IN EXCEPTION_POINTERS *exp,
    IN PCALLER_SYM   Caller,
    IN int           Skip,
    IN int           cFind,
    IN int           fResolveSymbols
    )
{
    BOOL             status;
    CONTEXT          ContextRecord;
    PUCHAR           Buffer[sizeof(IMAGEHLP_SYMBOL)-1 + MAX_FUNCTION_INFO_SIZE]; // symbol info
    PIMAGEHLP_SYMBOL Symbol = (PIMAGEHLP_SYMBOL)Buffer;
    STACKFRAME       StackFrame;
    INT              i;
    DWORD            Count;

    memcpy(&ContextRecord, exp->ContextRecord, sizeof(CONTEXT));

    ZeroMemory( &StackFrame,sizeof(STACKFRAME) );
    StackFrame.AddrPC.Segment = 0;
    StackFrame.AddrPC.Mode = AddrModeFlat;

#ifdef _M_IX86
    StackFrame.AddrFrame.Offset = ContextRecord.Ebp;
    StackFrame.AddrFrame.Mode = AddrModeFlat;

    StackFrame.AddrStack.Offset = ContextRecord.Esp;
    StackFrame.AddrStack.Mode = AddrModeFlat;

    StackFrame.AddrPC.Offset = (DWORD)ContextRecord.Eip;
#elif defined(_M_MRX000)
    StackFrame.AddrPC.Offset = (DWORD)ContextRecord.Fir;
#elif defined(_M_ALPHA)
    StackFrame.AddrPC.Offset = (DWORD)ContextRecord.Fir;
#elif defined(_M_PPC)
    StackFrame.AddrPC.Offset = (DWORD)ContextRecord.Iar;
#endif

    Count = 0;
    for (i=0;i<cFind+Skip ;i++ ) {
        status = g_pfnStackWalk( MachineType,
            OurProcess,
            GetCurrentThread(),
            &StackFrame,
            (PVOID)&ContextRecord,
            (PREAD_PROCESS_MEMORY_ROUTINE)ReadMem,
            g_pfnSymFunctionTableAccess,
            g_pfnSymGetModuleBase,
            NULL );


        if (status) {
            if ( i >= Skip) {
                DWORD   Displacement;

                ZeroMemory( Symbol,sizeof(IMAGEHLP_SYMBOL)-1 + MAX_FUNCTION_INFO_SIZE );
                Symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
                Symbol->Address = StackFrame.AddrPC.Offset;
                Symbol->MaxNameLength = MAX_FUNCTION_INFO_SIZE-1;
                Symbol->Flags = SYMF_OMAP_GENERATED;

                if (fResolveSymbols)
                    status = g_pfnSymGetSymFromAddr( OurProcess,StackFrame.AddrPC.Offset,(DWORD_PTR*)&Displacement,Symbol );

                //
                // save the name of the function and the displacement into it for later printing
                //

                Caller[Count].Addr = (PVOID)StackFrame.AddrPC.Offset;

                if (status) {
                    strcpy( Caller[Count].Buff,Symbol->Name );
                    Caller[Count].Displacement = Displacement;
                }
                Count++;
            }

        } else {
            break;
        }
    }

    return EXCEPTION_CONTINUE_EXECUTION;
    // done with exceptions
}

#endif // LEAK_TRACK
#endif // LOCAL
