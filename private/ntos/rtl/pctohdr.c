/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    pctohdr.c

Abstract:

    This module implements code to locate the file header for an image or
    dll given a PC value that lies within the image.

    N.B. This routine is conditionalized for user mode and kernel mode.

Author:

    Steve Wood (stevewo) 18-Aug-1989

Environment:

    User Mode or Kernel Mode

Revision History:

--*/

#if defined(NTOS_KERNEL_RUNTIME)
#include "ntos.h"
#else
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#if !defined(NTOS_KERNEL_RUNTIME)
extern PVOID NtDllBase;             // defined in ntos\dll\ldrinit.c
#endif


PVOID
RtlPcToFileHeader(
    IN PVOID PcValue,
    OUT PVOID *BaseOfImage
    )

/*++

Routine Description:

    This function returns the base of an image that contains the
    specified PcValue. An image contains the PcValue if the PcValue
    is within the ImageBase, and the ImageBase plus the size of the
    virtual image.

Arguments:

    PcValue - Supplies a PcValue.  All of the modules mapped into the
        calling processes address space are scanned to compute which
        module contains the PcValue.

    BaseOfImage - Returns the base address for the image containing the
        PcValue.  This value must be added to any relative addresses in
        the headers to locate portions of the image.

Return Value:

    NULL - No image was found that contains the PcValue.

    NON-NULL - Returns the base address of the image that contain the
        PcValue.

--*/

{

#if defined(NTOS_KERNEL_RUNTIME)

    extern LIST_ENTRY PsLoadedModuleList;
    extern KSPIN_LOCK PsLoadedModuleSpinLock;

    PVOID Base;
    ULONG_PTR Bounds;
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY Next;
    KIRQL OldIrql;

    //
    // Acquire the loaded module list spinlock and scan the list for the
    // specified PC value if the list has been initialized.
    //

    ExAcquireSpinLock(&PsLoadedModuleSpinLock, &OldIrql);
    Next = PsLoadedModuleList.Flink;
    if (Next != NULL) {
        while (Next != &PsLoadedModuleList) {
            Entry = CONTAINING_RECORD(Next,
                                      LDR_DATA_TABLE_ENTRY,
                                      InLoadOrderLinks);

            Next = Next->Flink;
            Base = Entry->DllBase;
            Bounds = (ULONG_PTR)Base + Entry->SizeOfImage;
            if (((ULONG_PTR)PcValue >= (ULONG_PTR)Base) && ((ULONG_PTR)PcValue < Bounds)) {
                ExReleaseSpinLock(&PsLoadedModuleSpinLock, OldIrql);
                *BaseOfImage = Base;
                return Base;
            }
        }
    }

    //
    // Release the loaded module list spin lock and return NULL.
    //

    ExReleaseSpinLock(&PsLoadedModuleSpinLock, OldIrql);
    *BaseOfImage = NULL;
    return NULL;

#else

    PVOID Base;
    ULONG_PTR Bounds;
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY ModuleListHead;
    PLIST_ENTRY Next;
    PIMAGE_NT_HEADERS NtHeaders;
    PPEB Peb;
    PTEB Teb;
    MEMORY_BASIC_INFORMATION MemInfo;
    NTSTATUS st;

    //
    // Acquire the Loader lock for the current process and scan the loaded
    // module list for the specified PC value if all the data structures
    // have been initialized.
    //

    if ( !RtlTryEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock) ) {

        //
        // We could not get the loader lock, so call the system to find the image that
        // contains this pc
        //

        st = NtQueryVirtualMemory(
                NtCurrentProcess(),
                PcValue,
                MemoryBasicInformation,
                (PVOID)&MemInfo,
                sizeof(MemInfo),
                NULL
                );
        if ( !NT_SUCCESS(st) ) {
            MemInfo.AllocationBase = NULL;;
            }
        else {
            if ( MemInfo.Type == MEM_IMAGE ) {
                try {
                    *BaseOfImage = MemInfo.AllocationBase;
                    }
                except (EXCEPTION_EXECUTE_HANDLER) {
                    MemInfo.AllocationBase = NULL;
                    }
                }
            else {
                MemInfo.AllocationBase = NULL;;
                }
            }
        return MemInfo.AllocationBase;
        }

    try {
        Teb = NtCurrentTeb();
        if (Teb != NULL) {
            Peb = Teb->ProcessEnvironmentBlock;
            if (Peb->Ldr != NULL) {
                ModuleListHead = &Peb->Ldr->InLoadOrderModuleList;
                Next = ModuleListHead->Flink;
                if (Next != NULL) {
                    while (Next != ModuleListHead) {
                        Entry = CONTAINING_RECORD(Next,
                                                  LDR_DATA_TABLE_ENTRY,
                                                  InLoadOrderLinks);

                        Next = Next->Flink;
                        Base = Entry->DllBase;
                        NtHeaders = RtlImageNtHeader(Base);
                        Bounds = (ULONG_PTR)Base + NtHeaders->OptionalHeader.SizeOfImage;
                        if (((ULONG_PTR)PcValue >= (ULONG_PTR)Base) && ((ULONG_PTR)PcValue < Bounds)) {
                            RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
                            *BaseOfImage = Base;
                            return Base;
                        }
                    }
                }

            } else {

                //
                //  ( Peb->Ldr == NULL )
                //
                //  If called during process intialization before the Ldr
                //  module list has been setup, code executing must be in
                //  NTDLL module.  If NtDllBase is non-NULL and the PcValue
                //  falls into the NTDLL range, return a valid Base.  This
                //  allows DbgPrint's during LdrpInitializeProcess to work
                //  on RISC machines.
                //

                if ( NtDllBase != NULL ) {
                    Base = NtDllBase;
                    NtHeaders = RtlImageNtHeader( Base );
                    Bounds = (ULONG_PTR)Base + NtHeaders->OptionalHeader.SizeOfImage;
                    if (((ULONG_PTR)PcValue >= (ULONG_PTR)Base) && ((ULONG_PTR)PcValue < Bounds)) {
                        RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
                        *BaseOfImage = Base;
                        return Base;
                    }
                }
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }

    //
    // Release the Loader lock for the current process a return NULL.
    //

    RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
    *BaseOfImage = NULL;
    return NULL;

#endif

}
