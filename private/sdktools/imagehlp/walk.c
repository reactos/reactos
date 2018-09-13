/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    walk.c

Abstract:

    This function implements the stack walking api.

Author:

    Wesley Witt (wesw) 1-Oct-1993

Environment:

    User Mode

--*/

#include <private.h>



BOOL
ReadMemoryRoutineLocal(
    HANDLE  hProcess,
    DWORD64 qwBaseAddress,
    LPVOID  lpBuffer,
    DWORD   nSize,
    LPDWORD lpNumberOfBytesRead
    );

LPVOID
FunctionTableAccessRoutineLocal(
    HANDLE  hProcess,
    DWORD64 AddrBase
    );

DWORD64
GetModuleBaseRoutineLocal(
    HANDLE  hProcess,
    DWORD64 ReturnAddress
    );

DWORD64
TranslateAddressRoutineLocal(
    HANDLE    hProcess,
    HANDLE    hThread,
    LPADDRESS64 lpaddr
    );

PREAD_PROCESS_MEMORY_ROUTINE    ImagepUserReadMemory32;
PFUNCTION_TABLE_ACCESS_ROUTINE  ImagepUserFunctionTableAccess32;
PGET_MODULE_BASE_ROUTINE        ImagepUserGetModuleBase32;
PTRANSLATE_ADDRESS_ROUTINE      ImagepUserTranslateAddress32;
IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY AlphaFunctionEntry64;

BOOL
ImagepReadMemoryThunk(
    HANDLE  hProcess,
    DWORD64 qwBaseAddress,
    LPVOID  lpBuffer,
    DWORD   nSize,
    LPDWORD lpNumberOfBytesRead
    )
{
    return ImagepUserReadMemory32(
                        hProcess,
                        (DWORD)qwBaseAddress,
                        lpBuffer,
                        nSize,
                        lpNumberOfBytesRead
                        );
}

LPVOID
ImagepFunctionTableAccessThunk(
    HANDLE  hProcess,
    DWORD64 AddrBase
    )
{
    return ImagepUserFunctionTableAccess32(
                hProcess,
                (DWORD)AddrBase
                );
}

PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY
AlphaFunctionTableAccessThunk(
    HANDLE  hProcess,
    DWORD64 AddrBase
    )
{
    PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY FunctionEntry32;
    
    FunctionEntry32 = (PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY)
                        ImagepUserFunctionTableAccess32(
                            hProcess,
                            (DWORD)AddrBase
                            );
    
    if (FunctionEntry32) {
        ConvertAlphaRf32To64( FunctionEntry32, &AlphaFunctionEntry64 );

        return &AlphaFunctionEntry64;
    }

    return NULL;
}

DWORD64
ImagepGetModuleBaseThunk(
    HANDLE  hProcess,
    DWORD64 ReturnAddress
    )
{
    return (ULONG64)(LONG64)(LONG)ImagepUserGetModuleBase32(
                hProcess,
                (DWORD)ReturnAddress
                );
}

DWORD64
ImagepTranslateAddressThunk(
    HANDLE    hProcess,
    HANDLE    hThread,
    LPADDRESS64 lpaddr
    )
{
    return 0;
}

void
StackFrame32To64(
    LPSTACKFRAME StackFrame32,
    LPSTACKFRAME64 StackFrame64
    )
{
    Address32To64(&StackFrame32->AddrPC, &StackFrame64->AddrPC );
    Address32To64(&StackFrame32->AddrReturn, &StackFrame64->AddrReturn );
    Address32To64(&StackFrame32->AddrFrame, &StackFrame64->AddrFrame );
    Address32To64(&StackFrame32->AddrStack, &StackFrame64->AddrStack );
    StackFrame64->FuncTableEntry = StackFrame32->FuncTableEntry;
    StackFrame64->Far = StackFrame32->Far;
    StackFrame64->Virtual = StackFrame32->Virtual;
    StackFrame64->Params[0] = StackFrame32->Params[0];
    StackFrame64->Params[1] = StackFrame32->Params[1];
    StackFrame64->Params[2] = StackFrame32->Params[2];
    StackFrame64->Params[3] = StackFrame32->Params[3];
    StackFrame64->Reserved[0] = StackFrame32->Reserved[0];
    StackFrame64->Reserved[1] = StackFrame32->Reserved[1];
    StackFrame64->Reserved[2] = StackFrame32->Reserved[2];
    KdHelp32To64(&StackFrame32->KdHelp, &StackFrame64->KdHelp);
}

void
StackFrame64To32(
    LPSTACKFRAME64 StackFrame64,
    LPSTACKFRAME StackFrame32
    )
{
    Address64To32(&StackFrame64->AddrPC, &StackFrame32->AddrPC );
    Address64To32(&StackFrame64->AddrReturn, &StackFrame32->AddrReturn );
    Address64To32(&StackFrame64->AddrFrame, &StackFrame32->AddrFrame );
    Address64To32(&StackFrame64->AddrStack, &StackFrame32->AddrStack );
    StackFrame32->FuncTableEntry = StackFrame64->FuncTableEntry;
    StackFrame32->Far = StackFrame64->Far;
    StackFrame32->Virtual = StackFrame64->Virtual;
    StackFrame32->Params[0] = (ULONG)StackFrame64->Params[0];
    StackFrame32->Params[1] = (ULONG)StackFrame64->Params[1];
    StackFrame32->Params[2] = (ULONG)StackFrame64->Params[2];
    StackFrame32->Params[3] = (ULONG)StackFrame64->Params[3];
    StackFrame32->Reserved[0] = (ULONG)StackFrame64->Reserved[0];
    StackFrame32->Reserved[1] = (ULONG)StackFrame64->Reserved[1];
    StackFrame32->Reserved[2] = (ULONG)StackFrame64->Reserved[2];
}

BOOL
StackWalk(
    DWORD                             MachineType,
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME                    StackFrame32,
    LPVOID                            ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE    ReadMemory32,
    PFUNCTION_TABLE_ACCESS_ROUTINE  FunctionTableAccess32,
    PGET_MODULE_BASE_ROUTINE        GetModuleBase32,
    PTRANSLATE_ADDRESS_ROUTINE      TranslateAddress32
    )
{
    BOOL rval;
    BOOL UseSym = FALSE;
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory;
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess;
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase;
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress;
    STACKFRAME64                      StackFrame;

    // Alpha stack walking no longer requires the FunctionTableAccess callback
    // except for backward compatability with debuggers that didn't specify
    // a GetModuleBase callback. If the GetModuleBase routine is provided
    // then set FunctionTableAccess to NULL to prevent a mixture of the
    // callback and read-from-image methods of accessing function table entries.

    if (MachineType == IMAGE_FILE_MACHINE_ALPHA) {
        if (GetModuleBase32 == NULL && FunctionTableAccess32) {
            FunctionTableAccess = AlphaFunctionTableAccessThunk;
            ImagepUserFunctionTableAccess32 = FunctionTableAccess32;
        } else {
            FunctionTableAccess = NULL;
        }
    } else {
        if (FunctionTableAccess32) {
            ImagepUserFunctionTableAccess32 = FunctionTableAccess32;
            FunctionTableAccess = ImagepFunctionTableAccessThunk;
        } else {
            FunctionTableAccess = FunctionTableAccessRoutineLocal;
            UseSym = TRUE;
        }
    }

    if (GetModuleBase32) {
        ImagepUserGetModuleBase32 = GetModuleBase32;
        GetModuleBase = ImagepGetModuleBaseThunk;
    } else {
        GetModuleBase = GetModuleBaseRoutineLocal;
        UseSym = TRUE;
    }

    if (ReadMemory32) {
        ImagepUserReadMemory32 = ReadMemory32;
        ReadMemory = ImagepReadMemoryThunk;
    } else {
        ReadMemory = ReadMemoryRoutineLocal;
    }

    if (TranslateAddress32) {
        ImagepUserTranslateAddress32 = TranslateAddress32;
        TranslateAddress = ImagepTranslateAddressThunk;
    } else {
        TranslateAddress = TranslateAddressRoutineLocal;
    }

    if (UseSym) {
        //
        // We are using the code in symbols.c
        // hProcess better be a real valid process handle
        //

        //
        // Always call syminitialize.  It's a nop if process
        // is already loaded.
        //
        if (!SymInitialize( hProcess, NULL, FALSE )) {
            return FALSE;
        }

    }

    StackFrame32To64(StackFrame32, &StackFrame);

    switch (MachineType) {
        case IMAGE_FILE_MACHINE_I386:
            rval = WalkX86( hProcess,
                            hThread,
                            &StackFrame,
                            ReadMemory,
                            FunctionTableAccess,
                            GetModuleBase,
                            TranslateAddress
                            );
            break;

        case IMAGE_FILE_MACHINE_ALPHA:
            rval = WalkAlpha( hProcess,
                              &StackFrame,
                              (PCONTEXT) ContextRecord,
                              ReadMemory,
                              GetModuleBase,
                              FunctionTableAccess,
                              FALSE
                              );
            break;

        case IMAGE_FILE_MACHINE_IA64:
        case IMAGE_FILE_MACHINE_ALPHA64:
        default:
            rval = FALSE;
            break;
    }
    if (rval) {
        StackFrame64To32(&StackFrame, StackFrame32);
    }

    return rval;
}


BOOL
StackWalk64(
    DWORD                             MachineType,
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME64                    StackFrame,
    LPVOID                            ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    BOOL rval;
    BOOL UseSym = FALSE;

    if (!FunctionTableAccess) {
        FunctionTableAccess = FunctionTableAccessRoutineLocal;
        UseSym = TRUE;
    }

    if (!GetModuleBase) {
        GetModuleBase = GetModuleBaseRoutineLocal;
        UseSym = TRUE;
    }

    if (!ReadMemory) {
        ReadMemory = ReadMemoryRoutineLocal;
    }

    if (!TranslateAddress) {
        TranslateAddress = TranslateAddressRoutineLocal;
    }

    if (UseSym) {
        //
        // We are using the code in symbols.c
        // hProcess better be a real valid process handle
        //

        //
        // Always call syminitialize.  It's a nop if process
        // is already loaded.
        //
        if (!SymInitialize( hProcess, NULL, FALSE )) {
            return FALSE;
        }

    }

    switch (MachineType) {
        case IMAGE_FILE_MACHINE_I386:
            rval = WalkX86( hProcess,
                            hThread,
                            StackFrame,
                            ReadMemory,
                            FunctionTableAccess,
                            GetModuleBase,
                            TranslateAddress
                            );
            break;

        case IMAGE_FILE_MACHINE_IA64:
            rval = WalkIa64( hProcess,
                             StackFrame,
                             (PCONTEXT) ContextRecord,
                             ReadMemory,
                             FunctionTableAccess,
                             GetModuleBase
                             );
            break;

        case IMAGE_FILE_MACHINE_ALPHA:
            rval = WalkAlpha( hProcess,
                              StackFrame,
                              (PCONTEXT) ContextRecord,
                              ReadMemory,
                              GetModuleBase,
                              FunctionTableAccess,
                              FALSE
                              );
            break;

        case IMAGE_FILE_MACHINE_ALPHA64:
            rval = WalkAlpha( hProcess,
                              StackFrame,
                              (PCONTEXT) ContextRecord,
                              ReadMemory,
                              GetModuleBase,
                              FunctionTableAccess,
                              TRUE
                              );
            break;

        default:
            rval = FALSE;
            break;
    }

    return rval;
}


BOOL
ReadMemoryRoutineLocal(
    HANDLE  hProcess,
    DWORD64 qwBaseAddress,
    LPVOID  lpBuffer,
    DWORD   nSize,
    LPDWORD lpNumberOfBytesRead
    )
{
    return ReadProcessMemory( hProcess,
                              (LPVOID)(ULONG_PTR)qwBaseAddress,
                              lpBuffer,
                              nSize,
                              lpNumberOfBytesRead );
}


LPVOID
FunctionTableAccessRoutineLocal(
    HANDLE  hProcess,
    DWORD64 AddrBase
    )
{
    return SymFunctionTableAccess64(hProcess, AddrBase);
}

DWORD64
GetModuleBaseRoutineLocal(
    HANDLE  hProcess,
    DWORD64 ReturnAddress
    )
{
    IMAGEHLP_MODULE64 ModuleInfo = {0};
    ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);

    if (SymGetModuleInfo64(hProcess, ReturnAddress, &ModuleInfo)) {
        return ModuleInfo.BaseOfImage;
    } else {
        return 0;
    }
}


DWORD64
TranslateAddressRoutineLocal(
    HANDLE    hProcess,
    HANDLE    hThread,
    LPADDRESS64 paddr
    )
{
    return 0;
}
