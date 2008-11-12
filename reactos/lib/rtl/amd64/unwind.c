/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         Exception related functions
 * PROGRAMMER:      Timo Kreuzer
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

#define UNWIND_HISTORY_TABLE_NONE 0
#define UNWIND_HISTORY_TABLE_GLOBAL 1
#define UNWIND_HISTORY_TABLE_LOCAL 2

PVOID
NTAPI
RtlpLookupModuleBase(
    PVOID Address);

/* FUNCTIONS *****************************************************************/

PRUNTIME_FUNCTION
NTAPI
RtlLookupFunctionTable(
    IN DWORD64 ControlPc,
    OUT PDWORD64 ImageBase,
    OUT PULONG Length)
{
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_DATA_DIRECTORY Directory;

    /* Find ModuleBase */
    DosHeader = RtlpLookupModuleBase((PVOID)ControlPc);
    if (!DosHeader)
    {
        return NULL;
    }

    /* Locate NT header and check number of directories */
    NtHeader = (PVOID)((ULONG64)DosHeader + DosHeader->e_lfanew);
    if (NtHeader->OptionalHeader.NumberOfRvaAndSizes 
         < IMAGE_DIRECTORY_ENTRY_EXCEPTION)
    {
        return NULL;
    }

    /* Locate the exception directory */
    Directory = &NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
    *Length = Directory->Size / sizeof(RUNTIME_FUNCTION);
    *ImageBase = (ULONG64)DosHeader;
    if (!Directory->VirtualAddress)
    {
        return NULL;
    }

    return (PVOID)((ULONG64)DosHeader + Directory->VirtualAddress);
}


// http://msdn.microsoft.com/en-us/library/ms680597(VS.85).aspx
PRUNTIME_FUNCTION
NTAPI
RtlLookupFunctionEntry(
    IN DWORD64 ControlPc,
    OUT PDWORD64 ImageBase,
    OUT PUNWIND_HISTORY_TABLE HistoryTable)
{
    PRUNTIME_FUNCTION FunctionTable, FunctionEntry;
    ULONG TableLength;
    ULONG IndexLo, IndexHi, IndexMid;

    /* Find the corresponding table */
    FunctionTable = RtlLookupFunctionTable(ControlPc,
                                           ImageBase,
                                           &TableLength);

    /* Fail, if no table is found */
    if (!FunctionTable)
    {
        return (PVOID)1;
    }

    /* Use relative virtual address */
    ControlPc -= *ImageBase;

    /* Do a binary search */
    IndexLo = 0;
    IndexHi = TableLength;

    while (IndexHi > IndexLo)
    {
        IndexMid = (IndexLo + IndexHi) / 2;
        FunctionEntry = &FunctionTable[IndexMid];

        if ( (ControlPc >= FunctionEntry->BeginAddress) &&
             (ControlPc < FunctionEntry->EndAddress) )
        {
            /* ControlPc is within limits, return entry */
            return FunctionEntry;
        }

        if (ControlPc < FunctionEntry->BeginAddress)
        {
            /* Continue search in lower half */
            IndexHi = IndexMid;
        }
        else
        {
            /* Continue search in upper half */
            IndexLo = IndexMid + 1;
        }
    }

    /* Nothing found, return NULL */
    return NULL;
}


PEXCEPTION_ROUTINE
NTAPI
RtlVirtualUnwind(
    IN ULONG HandlerType,
    IN ULONG64 ImageBase,
    IN ULONG64 ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN OUT PCONTEXT ContextRecord,
    OUT PVOID *HandlerData,
    OUT PULONG64 EstablisherFrame,
    IN OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
NTAPI
RtlUnwindEx(
   IN ULONG64 TargetFrame,
   IN ULONG64 TargetIp,
   IN PEXCEPTION_RECORD ExceptionRecord,
   IN PVOID ReturnValue,
   OUT PCONTEXT OriginalContext,
   IN PUNWIND_HISTORY_TABLE HistoryTable)
{
    UNIMPLEMENTED;
    return;
}


