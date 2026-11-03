/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     C specific exception and unwind handler for ARM64
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <precomp.h>
#include <winnt.h>

LONG
__C_specific_handler_call(
    _In_ PVOID Argument,
    _In_ PVOID EstablisherFrame,
    _In_ PVOID Funclet,
    _In_ PUCHAR NonVolatileRegisters);

/* The scope table lives elsewhere in the image when the top bit of its first word is set */
static
PSCOPE_TABLE_ARM64
GetScopeTable(
    _In_ ULONG64 ImageBase,
    _In_ PVOID HandlerData)
{
    ULONG First = *(PULONG)HandlerData;

    if (First & 0x80000000)
        return (PSCOPE_TABLE_ARM64)(ImageBase + (First & 0x7FFFFFFF));

    return HandlerData;
}

/* Tells whether the unwind target sits in a scope equivalent to the one at Index */
static
BOOLEAN
IsTargetInScope(
    _In_ PSCOPE_TABLE_ARM64 ScopeTable,
    _In_ ULONG Index,
    _In_ ULONG64 TargetPc)
{
    ULONG i;

    for (i = 0; i < ScopeTable->Count; i++)
    {
        if (TargetPc >= ScopeTable->ScopeRecord[i].BeginAddress &&
            TargetPc < ScopeTable->ScopeRecord[i].EndAddress &&
            ScopeTable->ScopeRecord[i].JumpTarget == ScopeTable->ScopeRecord[Index].JumpTarget &&
            ScopeTable->ScopeRecord[i].HandlerAddress == ScopeTable->ScopeRecord[Index].HandlerAddress)
        {
            return TRUE;
        }
    }

    return FALSE;
}

_CRTIMP
EXCEPTION_DISPOSITION
__cdecl
__C_specific_handler(
    _In_ struct _EXCEPTION_RECORD *ExceptionRecord,
    _In_ void *EstablisherFrame,
    _Inout_ struct _CONTEXT *ContextRecord,
    _Inout_ struct _DISPATCHER_CONTEXT *DispatcherContext)
{
    ULONG64 ImageBase = DispatcherContext->ImageBase;
    PSCOPE_TABLE_ARM64 ScopeTable = GetScopeTable(ImageBase, DispatcherContext->HandlerData);
    ULONG64 ControlPc = DispatcherContext->ControlPc - ImageBase;
    EXCEPTION_POINTERS ExceptionPointers;
    LONG FilterResult;
    ULONG i;

    /* After a call the address is the return address, the scope check needs the call */
    if (DispatcherContext->ControlPcIsUnwound)
        ControlPc -= 4;

    if (IS_UNWINDING(ExceptionRecord->ExceptionFlags))
    {
        ULONG64 TargetPc = DispatcherContext->TargetPc - ImageBase;

        for (i = DispatcherContext->ScopeIndex; i < ScopeTable->Count; i++)
        {
            if (ControlPc < ScopeTable->ScopeRecord[i].BeginAddress ||
                ControlPc >= ScopeTable->ScopeRecord[i].EndAddress)
            {
                continue;
            }

            /* Scopes around the unwind target stay active */
            if ((ExceptionRecord->ExceptionFlags & EXCEPTION_TARGET_UNWIND) &&
                IsTargetInScope(ScopeTable, i, TargetPc))
            {
                return ExceptionContinueSearch;
            }

            /* An __except block, the unwind ends when it targets it */
            if (ScopeTable->ScopeRecord[i].JumpTarget != 0)
            {
                if (TargetPc == ScopeTable->ScopeRecord[i].JumpTarget)
                    return ExceptionContinueSearch;
                continue;
            }

            /* A __finally block, skip it if an unwind passes here again */
            DispatcherContext->ScopeIndex = i + 1;
            __C_specific_handler_call((PVOID)TRUE,
                                      EstablisherFrame,
                                      (PVOID)(ImageBase + ScopeTable->ScopeRecord[i].HandlerAddress),
                                      DispatcherContext->NonVolatileRegisters);
        }

        return ExceptionContinueSearch;
    }

    ExceptionPointers.ExceptionRecord = ExceptionRecord;
    ExceptionPointers.ContextRecord = ContextRecord;

    for (i = DispatcherContext->ScopeIndex; i < ScopeTable->Count; i++)
    {
        if (ControlPc < ScopeTable->ScopeRecord[i].BeginAddress ||
            ControlPc >= ScopeTable->ScopeRecord[i].EndAddress ||
            ScopeTable->ScopeRecord[i].JumpTarget == 0)
        {
            continue;
        }

        /* A filter of EXCEPTION_EXECUTE_HANDLER is stored as the value itself */
        if (ScopeTable->ScopeRecord[i].HandlerAddress != EXCEPTION_EXECUTE_HANDLER)
        {
            FilterResult = __C_specific_handler_call(&ExceptionPointers,
                                                     EstablisherFrame,
                                                     (PVOID)(ImageBase + ScopeTable->ScopeRecord[i].HandlerAddress),
                                                     DispatcherContext->NonVolatileRegisters);
            if (FilterResult < 0)
                return ExceptionContinueExecution;
            if (FilterResult == 0)
                continue;
        }

        /* Continue in the __except block, this does not return */
        RtlUnwindEx(EstablisherFrame,
                    (PVOID)(ImageBase + ScopeTable->ScopeRecord[i].JumpTarget),
                    ExceptionRecord,
                    (PVOID)(LONG_PTR)(LONG)ExceptionRecord->ExceptionCode,
                    DispatcherContext->ContextRecord,
                    DispatcherContext->HistoryTable);
    }

    return ExceptionContinueSearch;
}

void __cdecl _local_unwind(void* frame, void* target)
{
    RtlUnwind(frame, target, NULL, 0);
}
