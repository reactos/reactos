/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ldrsnap.c

Abstract:

    This module implements the guts of the Ldr Dll Snap Routine.
    This code is system code that is part of the executive, but
    is executed from both user and system space.

Author:

    Mike O'Leary (mikeol) 23-Mar-1990

Revision History:

--*/

#define LDRDBG 0

#include "ntos.h"
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <heap.h>
#include "ldrp.h"

#if DBG // DBG
PUCHAR MonthOfYear[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
PUCHAR DaysOfWeek[] =  { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
LARGE_INTEGER MapBeginTime, MapEndTime, MapElapsedTime;
#endif // DBG

#if defined (_ALPHA_)
VOID
AlphaFindArchitectureFixups(
    PIMAGE_NT_HEADERS NtHeaders,
    PVOID ViewBase,
    BOOLEAN StaticLink
    );
#endif


#if defined (_X86_)
extern PVOID LdrpLockPrefixTable;

//
// Specify address of kernel32 lock prefixes
//
IMAGE_LOAD_CONFIG_DIRECTORY _load_config_used = {
    0,                             // Reserved
    0,                             // Reserved
    0,                             // Reserved
    0,                             // Reserved
    0,                             // GlobalFlagsClear
    0,                             // GlobalFlagsSet
    0,                             // CriticalSectionTimeout (milliseconds)
    0,                             // DeCommitFreeBlockThreshold
    0,                             // DeCommitTotalFreeThreshold
    (ULONG_PTR) &LdrpLockPrefixTable,  // LockPrefixTable
    0, 0, 0, 0, 0, 0, 0            // Reserved
};

void
LdrpValidateImageForMp(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    )
{
    PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigData;
    ULONG i;
    PUCHAR *pb;
    ULONG ErrorParameters;
    ULONG ErrorResponse;

    //
    // If we are on an MP system and the DLL has image config info, check to see
    // if it has a lock prefix table and make sure the locks have not been converted
    // to NOPs
    //

    ImageConfigData = RtlImageDirectoryEntryToData( LdrDataTableEntry->DllBase,
                                                    TRUE,
                                                    IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                                    &i
                                                  );

    if (ImageConfigData != NULL &&
        i == sizeof( *ImageConfigData ) &&
        ImageConfigData->LockPrefixTable ) {
            pb = (PUCHAR *)ImageConfigData->LockPrefixTable;
            while ( *pb ) {
                if ( **pb == (UCHAR)0x90 ) {

                    if ( LdrpNumberOfProcessors > 1 ) {

                        //
                        // Hard error time. One of the know DLL's is corrupt !
                        //

                        ErrorParameters = (ULONG)&LdrDataTableEntry->BaseDllName;

                        NtRaiseHardError(
                            STATUS_IMAGE_MP_UP_MISMATCH,
                            1,
                            1,
                            &ErrorParameters,
                            OptionOk,
                            &ErrorResponse
                            );

                        if ( LdrpInLdrInit ) {
                            LdrpFatalHardErrorCount++;
                            }

                        }
                    }
                pb++;
                }
        }
}
#endif

NTSTATUS
LdrpWalkImportDescriptor (
    IN PWSTR DllPath OPTIONAL,
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    );

VOID
LdrpDphInitializeTargetDll (
    PLDR_DATA_TABLE_ENTRY LoadedDllData
    );

NTSTATUS
LdrpLoadImportModule(
    IN PWSTR DllPath OPTIONAL,
    IN LPSTR ImportName,
    IN PVOID DllBaseImporter,
    OUT PLDR_DATA_TABLE_ENTRY *DataTableEntry,
    OUT PBOOLEAN AlreadyLoaded
    )
{
    NTSTATUS st;
    ANSI_STRING AnsiString;
    PUNICODE_STRING ImportDescriptorName_U;
    BOOLEAN Wx86KnownDll = FALSE;

    ImportDescriptorName_U = &NtCurrentTeb()->StaticUnicodeString;
    RtlInitAnsiString(&AnsiString, ImportName);
    st = RtlAnsiStringToUnicodeString(ImportDescriptorName_U, &AnsiString, FALSE);
    if (!NT_SUCCESS(st)) {
        return st;
    }


#if defined (WX86)
    if (Wx86ProcessInit) {
        Wx86KnownDll = RtlImageNtHeader(DllBaseImporter)->FileHeader.Machine == IMAGE_FILE_MACHINE_I386;
        }
#endif


    //
    // BUGBUG If a DLL refers to itself we will recurse and blow up
    //

    //
    // Check the LdrTable to see if Dll has already been mapped
    // into this image. If not, map it.
    //

    if (LdrpCheckForLoadedDll( DllPath,
                               ImportDescriptorName_U,
                               TRUE,
                               Wx86KnownDll,
                               DataTableEntry
                             )
       ) {
        *AlreadyLoaded = TRUE;
    } else {
        *AlreadyLoaded = FALSE;

        st = LdrpMapDll(DllPath,
                        ImportDescriptorName_U->Buffer,
                        NULL,
                        TRUE,
                        Wx86KnownDll,
                        DataTableEntry
                        );

        if (!NT_SUCCESS(st)) {
            return st;
        }
        st = LdrpWalkImportDescriptor(
                DllPath,
                *DataTableEntry
                );
        if (!NT_SUCCESS(st)) {
            InsertTailList(&NtCurrentPeb()->Ldr->InInitializationOrderModuleList,
                           &(*DataTableEntry)->InInitializationOrderLinks);
        }
    }

    return st;
}


NTSTATUS
LdrpWalkImportDescriptor (
    IN PWSTR DllPath OPTIONAL,
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    )

/*++

Routine Description:

    This is a recursive routine which walks the Import Descriptor
    Table and loads each DLL that is referenced.

Arguments:

    DllPath - Supplies an optional search path to be used to locate
        the DLL.

    LdrDataTableEntry - Supplies the address of the data table entry
        to initialize.

Return Value:

    Status value.

--*/

{
    ULONG i, ImportSize, NewImportSize;
    BOOLEAN AlreadyLoaded, SnapForwardersOnly, StaleBinding;
    PLDR_DATA_TABLE_ENTRY DataTableEntry, FwdDataTableEntry;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
    PIMAGE_BOUND_IMPORT_DESCRIPTOR NewImportDescriptor;
    PIMAGE_BOUND_FORWARDER_REF NewImportForwarder;
    PSZ ImportName, NewImportName, NewFwdImportName, NewImportStringBase;
    NTSTATUS st;
    PIMAGE_NT_HEADERS ExportNtHeaders;
    PVOID ImportBase;
    ULONG RegionSize;
    PIMAGE_THUNK_DATA Thunk,Name;
    PIMAGE_THUNK_DATA FirstThunk;
    PPEB Peb = NtCurrentPeb();

    //
    // See if there is a bound import table.  If so, walk that to
    // verify if the binding is good.  If so, then succeed with
    // having touched the .idata section, as all the information
    // in the bound imports table is stored in the header.  If any
    // are stale, then fall out into the unbound case.
    //
    NewImportDescriptor = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(
                           LdrDataTableEntry->DllBase,
                           TRUE,
                           IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT,
                           &NewImportSize
                           );

    ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(
                        LdrDataTableEntry->DllBase,
                        TRUE,
                        IMAGE_DIRECTORY_ENTRY_IMPORT,
                        &ImportSize
                        );


    if (NewImportDescriptor) {
        NewImportStringBase = (LPSTR)NewImportDescriptor;
        while (NewImportDescriptor->OffsetModuleName) {
            NewImportName = NewImportStringBase +
                         NewImportDescriptor->OffsetModuleName;

            if (ShowSnaps) {
                DbgPrint("LDR: %wZ bound to %s\n",
                    &LdrDataTableEntry->BaseDllName,
                    NewImportName
                    );
            }

            st = LdrpLoadImportModule( DllPath,
                                       NewImportName,
                                       LdrDataTableEntry->DllBase,
                                       &DataTableEntry,
                                       &AlreadyLoaded
                                       );
            if ( !NT_SUCCESS(st) ) {
                return st;
            }

            //
            // Add to initialization list.
            //

            if (!AlreadyLoaded) {
                InsertTailList(&Peb->Ldr->InInitializationOrderModuleList,
                               &DataTableEntry->InInitializationOrderLinks);
            }

#if defined (_ALPHA_) && defined (WX86)
            ExportNtHeaders = RtlImageNtHeader(LdrDataTableEntry->DllBase);

            if ((ExportNtHeaders->OptionalHeader.SectionAlignment < PAGE_SIZE) &&
                (ExportNtHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)) {
                if (LdrpWx86DllHasRelocatedSharedSection(LdrDataTableEntry->DllBase)) {
                   DataTableEntry->Flags |= LDRP_IMAGE_NOT_AT_BASE;
                }
            }
#endif // defined (_ALPHA_) && defined (WX86)

            if ( NewImportDescriptor->TimeDateStamp != DataTableEntry->TimeDateStamp ||
                 (DataTableEntry->Flags & LDRP_IMAGE_NOT_AT_BASE) ) {
                if (ShowSnaps) {
                    DbgPrint("LDR: %wZ has stale binding to %s\n",
                            &LdrDataTableEntry->BaseDllName,
                            NewImportName
                            );
                }
                StaleBinding = TRUE;
            } else {
#if DBG
                LdrpSnapBypass++;
#endif
                if (ShowSnaps) {
                    DbgPrint("LDR: %wZ has correct binding to %s\n",
                            &LdrDataTableEntry->BaseDllName,
                            NewImportName
                            );
                }
                StaleBinding = FALSE;
            }

            NewImportForwarder = (PIMAGE_BOUND_FORWARDER_REF)(NewImportDescriptor+1);
            for (i=0; i<NewImportDescriptor->NumberOfModuleForwarderRefs; i++) {
                NewFwdImportName = NewImportStringBase +
                                NewImportForwarder->OffsetModuleName;
                if (ShowSnaps) {
                    DbgPrint("LDR: %wZ bound to %s via forwarder(s) from %wZ\n",
                        &LdrDataTableEntry->BaseDllName,
                        NewFwdImportName,
                        &DataTableEntry->BaseDllName
                        );
                }

                st = LdrpLoadImportModule( DllPath,
                                           NewFwdImportName,
                                           LdrDataTableEntry->DllBase,
                                           &FwdDataTableEntry,
                                           &AlreadyLoaded
                                           );
                if ( NT_SUCCESS(st) ) {
                    if (!AlreadyLoaded) {
                        InsertTailList(&Peb->Ldr->InInitializationOrderModuleList,
                                       &FwdDataTableEntry->InInitializationOrderLinks);
                        }
                    }

                if ( !NT_SUCCESS(st) ||
                     NewImportForwarder->TimeDateStamp != FwdDataTableEntry->TimeDateStamp ||
                     (FwdDataTableEntry->Flags & LDRP_IMAGE_NOT_AT_BASE ) ) {
                    if (ShowSnaps) {
                        DbgPrint("LDR: %wZ has stale binding to %s\n",
                                &LdrDataTableEntry->BaseDllName,
                                NewFwdImportName
                                );
                    }
                    StaleBinding = TRUE;
                } else {
#if DBG
                    LdrpSnapBypass++;
#endif
                    if (ShowSnaps) {
                        DbgPrint("LDR: %wZ has correct binding to %s\n",
                                &LdrDataTableEntry->BaseDllName,
                                NewFwdImportName
                                );
                    }
                }

                NewImportForwarder += 1;
            }
            NewImportDescriptor = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)NewImportForwarder;

            if (StaleBinding) {
#if DBG
                LdrpNormalSnap++;
#endif
                //
                // Find the unbound import descriptor that matches this bound
                // import descriptor
                //

                ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(
                                    LdrDataTableEntry->DllBase,
                                    TRUE,
                                    IMAGE_DIRECTORY_ENTRY_IMPORT,
                                    &ImportSize
                                    );

                while (ImportDescriptor->Name) {
                    ImportName = (PSZ)((ULONG_PTR)LdrDataTableEntry->DllBase + ImportDescriptor->Name);
                    if (!_stricmp(ImportName, NewImportName)) {
                        break;
                    }

                    ImportDescriptor += 1;
                }

                if (!ImportDescriptor->Name) {
                    return STATUS_OBJECT_NAME_INVALID;
                }

                if (ShowSnaps) {
                    DbgPrint("LDR: Stale Bind %s from %wZ\n",ImportName,&LdrDataTableEntry->BaseDllName);
                }

                st = LdrpSnapIAT(
                        DataTableEntry,
                        LdrDataTableEntry,
                        ImportDescriptor,
                        FALSE
                        );

                if (!NT_SUCCESS(st)) {
                    return st;
                }
            }
        }
    }
    else
    if (ImportDescriptor) {
        //
        // For each DLL used by this DLL, load the dll. Then snap
        // the IAT, and call the DLL's init routine.
        //

        while (ImportDescriptor->Name && ImportDescriptor->FirstThunk) {
            ImportName = (PSZ)((ULONG_PTR)LdrDataTableEntry->DllBase + ImportDescriptor->Name);

            //
            // check for import that has no references
            //
            FirstThunk = (PIMAGE_THUNK_DATA)((ULONG_PTR)LdrDataTableEntry->DllBase + ImportDescriptor->FirstThunk);
            if ( !FirstThunk->u1.Function ) {
                goto skipimport;
                }

            if (ShowSnaps) {
                DbgPrint("LDR: %s used by %wZ\n",
                    ImportName,
                    &LdrDataTableEntry->BaseDllName
                    );
            }


            st = LdrpLoadImportModule( DllPath,
                                       ImportName,
                                       LdrDataTableEntry->DllBase,
                                       &DataTableEntry,
                                       &AlreadyLoaded
                                       );
            if ( !NT_SUCCESS(st) ) {
                return st;
            }

            if (ShowSnaps) {
                DbgPrint("LDR: Snapping imports for %wZ from %s\n",
                        &LdrDataTableEntry->BaseDllName,
                        ImportName
                        );
            }

            //
            // If the image has been bound and the import date stamp
            // matches the date time stamp in the export modules header,
            // and the image was mapped at it's prefered base address,
            // then we are done.
            //

            SnapForwardersOnly = FALSE;

            if ( ImportDescriptor->OriginalFirstThunk ) {
                if ( ImportDescriptor->TimeDateStamp &&
                     ImportDescriptor->TimeDateStamp == DataTableEntry->TimeDateStamp &&
                     (! DataTableEntry->Flags & LDRP_IMAGE_NOT_AT_BASE) ) {
#if DBG
                    LdrpSnapBypass++;
#endif
                    if (ShowSnaps) {
                        DbgPrint("LDR: Snap bypass %s from %wZ\n",
                            ImportName,
                            &LdrDataTableEntry->BaseDllName
                            );
                    }

                    if (ImportDescriptor->ForwarderChain == -1) {
                        goto bypass_snap;
                        }

                    SnapForwardersOnly = TRUE;

                    }
            }
#if DBG
            LdrpNormalSnap++;
#endif
            //
            // Add to initialization list.
            //

            if (!AlreadyLoaded) {
                InsertTailList(&Peb->Ldr->InInitializationOrderModuleList,
                               &DataTableEntry->InInitializationOrderLinks);
            }
            st = LdrpSnapIAT(
                    DataTableEntry,
                    LdrDataTableEntry,
                    ImportDescriptor,
                    SnapForwardersOnly
                    );

            if (!NT_SUCCESS(st)) {
                return st;
            }
            AlreadyLoaded = TRUE;
bypass_snap:
            //
            // Add to initialization list.
            //

            if (!AlreadyLoaded) {
                InsertTailList(&Peb->Ldr->InInitializationOrderModuleList,
                               &DataTableEntry->InInitializationOrderLinks);
            }

skipimport:
            ++ImportDescriptor;
        }
    }

    if (Peb->NtGlobalFlag & FLG_HEAP_ENABLE_TAG_BY_DLL) {
        PVOID IATBase;
        SIZE_T BigIATSize;
        ULONG  LittleIATSize;
        PVOID *ProcAddresses;
        ULONG NumberOfProcAddresses;
        ULONG OldProtect;
        USHORT TagIndex;

        //
        // Determine the location and size of the IAT.  If found, scan the
        // IAT address to see if any are pointing to RtlAllocateHeap.  If so
        // replace when with a pointer to a unique thunk function that will
        // replace the tag with a unique tag for this image.
        //

        IATBase = RtlImageDirectoryEntryToData( LdrDataTableEntry->DllBase,
                                                TRUE,
                                                IMAGE_DIRECTORY_ENTRY_IAT,
                                                &LittleIATSize
                                              );
        BigIATSize = LittleIATSize;
        if (IATBase != NULL) {
            st = NtProtectVirtualMemory( NtCurrentProcess(),
                                         &IATBase,
                                         &BigIATSize,
                                         PAGE_READWRITE,
                                         &OldProtect
                                       );
            if (!NT_SUCCESS(st)) {
                DbgPrint( "LDR: Unable to unprotect IAT to enable tagging by DLL.\n" );
            }
            else {
                ProcAddresses = (PVOID *)IATBase;
                NumberOfProcAddresses = (ULONG)(BigIATSize / sizeof(PVOID));
                while (NumberOfProcAddresses--) {
                    if (*ProcAddresses == RtlAllocateHeap) {
                        *ProcAddresses = LdrpDefineDllTag( LdrDataTableEntry->BaseDllName.Buffer, &TagIndex );
                        if (*ProcAddresses == NULL) {
                            *ProcAddresses = RtlAllocateHeap;
                        }
                    }

                    ProcAddresses += 1;
                }

                NtProtectVirtualMemory( NtCurrentProcess(),
                                        &IATBase,
                                        &BigIATSize,
                                        OldProtect,
                                        &OldProtect
                                      );
            }
        }
    }

    //
    // Check if we need to redirect some imports in case page heap
    // is enabled and we target specific dlls.
    //

    if (Peb->NtGlobalFlag & FLG_HEAP_PAGE_ALLOCS) {
        
        LdrpDphInitializeTargetDll (LdrDataTableEntry);
    }

    return STATUS_SUCCESS;
}


ULONG
LdrpClearLoadInProgress(
    VOID
    )
{
    PLIST_ENTRY Head, Next;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    ULONG i;

    Head = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
    Next = Head->Flink;
    i = 0;
    while ( Next != Head ) {
        LdrDataTableEntry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);
        LdrDataTableEntry->Flags &= ~LDRP_LOAD_IN_PROGRESS;

        //
        // return the number of entries that have not been processed, but
        // have init routines
        //

        if ( !(LdrDataTableEntry->Flags & LDRP_ENTRY_PROCESSED) && LdrDataTableEntry->EntryPoint) {
            i++;
            }

        Next = Next->Flink;
        }
    return i;
}

NTSTATUS
LdrpRunInitializeRoutines(
    IN PCONTEXT Context OPTIONAL
    )
{
    PLIST_ENTRY Head, Next;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PLDR_DATA_TABLE_ENTRY *LdrDataTableBase;
    PDLL_INIT_ROUTINE InitRoutine;
    BOOLEAN InitStatus;
    ULONG NumberOfRoutines;
    ULONG i;
    NTSTATUS Status;
    ULONG BreakOnDllLoad;

    //
    // Run the Init routines
    //

    //
    // capture the entries that have init routines
    //

    NumberOfRoutines = LdrpClearLoadInProgress();
    if ( NumberOfRoutines ) {
        LdrDataTableBase = RtlAllocateHeap(RtlProcessHeap(),MAKE_TAG( TEMP_TAG ),NumberOfRoutines*sizeof(LdrDataTableBase));
        if ( !LdrDataTableBase ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        LdrDataTableBase = NULL;
        }

    Head = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
    Next = Head->Flink;
    if (ShowSnaps) {
        DbgPrint("LDR: Real INIT LIST\n");
        }

    i = 0;
    while ( Next != Head ) {
        LdrDataTableEntry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);

#if defined (WX86)
        if (Wx86ProcessInit && !(LdrDataTableEntry->Flags & LDRP_ENTRY_PROCESSED)) {
            PIMAGE_NT_HEADERS NtHeaders;

            NtHeaders = RtlImageNtHeader(LdrDataTableEntry->DllBase);
            if (NtHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_I386) {
                LdrpWx86DllMapNotify(LdrDataTableEntry->DllBase, TRUE);
            }
        }
#endif

        if ( !(LdrDataTableEntry->Flags & LDRP_ENTRY_PROCESSED) && LdrDataTableEntry->EntryPoint) {
            LdrDataTableBase[i] = LdrDataTableEntry;
            if (ShowSnaps) {
                DbgPrint("     %wZ init routine %x\n",
                        &LdrDataTableEntry->FullDllName,
                        LdrDataTableEntry->EntryPoint
                        );
                }

            i++;
            }
        LdrDataTableEntry->Flags |= LDRP_ENTRY_PROCESSED;

        Next = Next->Flink;
        }
    if ( !LdrDataTableBase ) {
        return STATUS_SUCCESS;
        }

    try {
        i = 0;
        while ( i < NumberOfRoutines ) {
            LdrDataTableEntry = LdrDataTableBase[i];
            i++;
            InitRoutine = (PDLL_INIT_ROUTINE)LdrDataTableEntry->EntryPoint;

            //
            // Walk through the entire list looking for un-processed
            // entries. For each entry, set the processed flag
            // and optionally call it's init routine
            //

            BreakOnDllLoad = 0;
#if DBG
            if (TRUE) {
#else
            if (NtCurrentPeb()->BeingDebugged || NtCurrentPeb()->ReadImageFileExecOptions) {
#endif
                Status = LdrQueryImageFileExecutionOptions( &LdrDataTableEntry->BaseDllName,
                                                            L"BreakOnDllLoad",
                                                            REG_DWORD,
                                                            &BreakOnDllLoad,
                                                            sizeof( BreakOnDllLoad ),
                                                            NULL
                                                          );
                if (!NT_SUCCESS( Status )) {
                    BreakOnDllLoad = 0;
                    }
                }

            if (BreakOnDllLoad) {
                if (ShowSnaps) {
                    DbgPrint( "LDR: %wZ loaded.", &LdrDataTableEntry->BaseDllName );
                    DbgPrint( " - About to call init routine at %lx\n", InitRoutine );
                    }
                DbgBreakPoint();

                }
            else if (ShowSnaps) {
                if ( InitRoutine ) {
                    DbgPrint( "LDR: %wZ loaded.", &LdrDataTableEntry->BaseDllName );
                    DbgPrint(" - Calling init routine at %lx\n", InitRoutine);
                    }
                }

            if ( InitRoutine ) {

                //
                // If the DLL has TLS data, then call the optional initializers
                //



                if ( LdrDataTableEntry->TlsIndex && Context) {
                    LdrpCallTlsInitializers(LdrDataTableEntry->DllBase,DLL_PROCESS_ATTACH);
                    }

#if defined (WX86)
                if (!Wx86ProcessInit ||
                    LdrpRunWx86DllEntryPoint(InitRoutine,
                                            &InitStatus,
                                            LdrDataTableEntry->DllBase,
                                            DLL_PROCESS_ATTACH,
                                            Context
                                            ) ==  STATUS_IMAGE_MACHINE_TYPE_MISMATCH)
                    {
                    InitStatus = LdrpCallInitRoutine(InitRoutine,
                                                     LdrDataTableEntry->DllBase,
                                                     DLL_PROCESS_ATTACH,
                                                     Context
                                                    );
                    }

#else
                InitStatus = LdrpCallInitRoutine(InitRoutine,
                                                 LdrDataTableEntry->DllBase,
                                                 DLL_PROCESS_ATTACH,
                                                 Context
                                                );
#endif

                LdrDataTableEntry->Flags |= LDRP_PROCESS_ATTACH_CALLED;

                if ( !InitStatus ) {
                    return STATUS_DLL_INIT_FAILED;
                    }
                }
            }

        //
        // If the image has tls than call its initializers
        //

        if ( LdrpImageHasTls && Context ) {
            LdrpCallTlsInitializers(NtCurrentPeb()->ImageBaseAddress,DLL_PROCESS_ATTACH);
            }

        }
    finally {
        RtlFreeHeap(RtlProcessHeap(),0,LdrDataTableBase);
        }

    return STATUS_SUCCESS;
}

BOOLEAN
LdrpCheckForLoadedDll (
    IN PWSTR DllPath OPTIONAL,
    IN PUNICODE_STRING DllName,
    IN BOOLEAN StaticLink,
    IN BOOLEAN Wx86KnownDll,
    OUT PLDR_DATA_TABLE_ENTRY *LdrDataTableEntry
    )

/*++

Routine Description:

    This function scans the loader data table looking to see if
    the specified DLL has already been mapped into the image. If
    the dll has been loaded, the address of its data table entry
    is returned.

Arguments:

    DllPath - Supplies an optional search path used to locate the DLL.

    DllName - Supplies the name to search for.

    StaticLink - TRUE if performing a static link.

    Wx86KnownDll - TRUE, treat Importer as x86

    LdrDataTableEntry - Returns the address of the loader data table
        entry that describes the first dll section that implements the
        dll.

Return Value:

    TRUE- The dll is already loaded.  The address of the data table
        entries that implement the dll, and the number of data table
        entries are returned.

    FALSE - The dll is not already mapped.

--*/

{
    BOOLEAN Result;
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY Head, Next;
    UNICODE_STRING FullDllName;
    HANDLE DllFile;
    BOOLEAN HardCodedPath;
    PWCH p;
    WCHAR wch;
    ULONG i;
    CHAR FullDllNameBuffer[530+sizeof(UNICODE_NULL)];
    ULONG Length = 0;
    PWCH src,dest;



    if (!DllName->Buffer || !DllName->Buffer[0]) {
        return FALSE;
        }

#if defined (WX86)
    if (Wx86ProcessInit) {

        FullDllName.Buffer = (PWCHAR)FullDllNameBuffer;
        FullDllName.MaximumLength = sizeof(FullDllNameBuffer);
        FullDllName.Length = 0;

        Entry = LdrpWx86CheckForLoadedDll(DllPath,
                                          DllName,
                                          Wx86KnownDll,
                                          &FullDllName
                                          );

        if (Entry) {
            RtlCopyUnicodeString(DllName, &FullDllName);
            *LdrDataTableEntry = Entry;
            return TRUE;
            }
        else {
            return FALSE;
            }
        }

#endif



    //
    // for static links, just go to the hash table
    //
staticlink:
    if ( StaticLink ) {

        i = LDRP_COMPUTE_HASH_INDEX(DllName->Buffer[0]);
        Head = &LdrpHashTable[i];
        Next = Head->Flink;
        while ( Next != Head ) {
            Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, HashLinks);
#if DBG
            LdrpCompareCount++;
#endif
            if (RtlEqualUnicodeString(DllName,
                        &Entry->BaseDllName,
                        TRUE
                        )) {

                *LdrDataTableEntry = Entry;
                return TRUE;
                }
            Next = Next->Flink;
            }
        }

    if ( StaticLink ) {
        return FALSE;
        }


    //
    // If the dll name contained a hard coded path
    // (dynamic link only), then the fully qualified
    // name needs to be compared to make sure we
    // have the correct dll.
    //

    p = DllName->Buffer;
    HardCodedPath = FALSE;
    while (*p) {
        wch = *p++;
        if (wch == (WCHAR)'\\' || wch == (WCHAR)'/' ) {

            HardCodedPath = TRUE;

            //
            // We have a hard coded path, so we have to search path
            // for the DLL. We need the full DLL name so we can

            FullDllName.Buffer = (WCHAR *)FullDllNameBuffer;

            Length = RtlDosSearchPath_U(
                        ARGUMENT_PRESENT(DllPath) ? DllPath : LdrpDefaultPath.Buffer,
                        DllName->Buffer,
                        NULL,
                        sizeof(FullDllNameBuffer)-sizeof(UNICODE_NULL),
                        FullDllName.Buffer,
                        NULL
                        );
            if ( !Length || Length > sizeof(FullDllNameBuffer)-sizeof(UNICODE_NULL) ) {

                if (ShowSnaps) {
                    DbgPrint("LDR: LdrpCheckForLoadedDll - Unable To Locate ");
                    DbgPrint("%ws from %ws\n",
                        DllName->Buffer,
                        ARGUMENT_PRESENT(DllPath) ? DllPath : LdrpDefaultPath.Buffer
                        );
                    }

                return FALSE;
                }

            FullDllName.Length = (USHORT)Length;
            FullDllName.MaximumLength = FullDllName.Length + (USHORT)sizeof(UNICODE_NULL);
            break;
            }
        }

    //
    // if this is a dynamic load lib, and there is not a hard
    // coded path, then go to the static lib hash table for resolution
    //

    if ( !HardCodedPath ) {

        StaticLink = TRUE;

        goto staticlink;
        }


    Result = FALSE;
    Head = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    Next = Head->Flink;

    while ( Next != Head ) {
        Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        Next = Next->Flink;

        //
        // when we unload, the memory order links flink field is nulled.
        // this is used to skip the entry pending list removal.
        //

        if ( !Entry->InMemoryOrderLinks.Flink ) {
            continue;
        }


        if (RtlEqualUnicodeString(
                &FullDllName,
                &Entry->FullDllName,
                TRUE
                ) ) {

                Result = TRUE;
                *LdrDataTableEntry = Entry;
                break;
            }
        }

    if ( !Result ) {

        //
        // no names matched. This might be a long short name mismatch or
        // any kind of alias pathname. Deal with this by opening and mapping
        // full dll name and then repeat the scan this time checking for
        // timedatestamp matches
        //

        HANDLE File;
        HANDLE Section;
        NTSTATUS st;
        OBJECT_ATTRIBUTES ObjectAttributes;
        IO_STATUS_BLOCK IoStatus;
        PVOID ViewBase;
        SIZE_T ViewSize;
        PIMAGE_NT_HEADERS NtHeadersSrc,NtHeadersE;
        UNICODE_STRING NtFileName;


        if (!RtlDosPathNameToNtPathName_U( FullDllName.Buffer,
                                           &NtFileName,
                                           NULL,
                                           NULL
                                         )
           ) {
            goto alldone;
            }

        InitializeObjectAttributes(
            &ObjectAttributes,
            &NtFileName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        st = NtOpenFile(
                &File,
                SYNCHRONIZE | FILE_EXECUTE,
                &ObjectAttributes,
                &IoStatus,
                FILE_SHARE_READ | FILE_SHARE_DELETE,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                );
        RtlFreeHeap(RtlProcessHeap(), 0, NtFileName.Buffer);

        if (!NT_SUCCESS(st)) {
            goto alldone;
            }

        st = NtCreateSection(
                &Section,
                SECTION_MAP_READ | SECTION_MAP_EXECUTE | SECTION_MAP_WRITE,
                NULL,
                NULL,
                PAGE_EXECUTE,
                SEC_COMMIT,
                File
                );
        NtClose( File );

        if (!NT_SUCCESS(st)) {
            goto alldone;
            }

        ViewBase = NULL;
        ViewSize = 0;
        st = NtMapViewOfSection(
                Section,
                NtCurrentProcess(),
                (PVOID *)&ViewBase,
                0L,
                0L,
                NULL,
                &ViewSize,
                ViewShare,
                0L,
                PAGE_EXECUTE
                );
        NtClose(Section);
        if (!NT_SUCCESS(st)) {
            goto alldone;
            }

        //
        // The section is mapped. Now find the headers
        //
        NtHeadersSrc = RtlImageNtHeader(ViewBase);
        if ( !NtHeadersSrc ) {
            NtUnmapViewOfSection(NtCurrentProcess(),ViewBase);
            goto alldone;
            }
        Head = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
        Next = Head->Flink;

        while ( Next != Head ) {
            Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
            Next = Next->Flink;

            //
            // when we unload, the memory order links flink field is nulled.
            // this is used to skip the entry pending list removal.
            //

            if ( !Entry->InMemoryOrderLinks.Flink ) {
                continue;
            }

            try {
                if ( Entry->TimeDateStamp == NtHeadersSrc->FileHeader.TimeDateStamp &&
                     Entry->SizeOfImage == NtHeadersSrc->OptionalHeader.SizeOfImage ) {

                    //
                    // there is a very good chance we have an image match. Check the
                    // entire file header and optional header. If they match, declare
                    // this a match
                    //

                    NtHeadersE = RtlImageNtHeader(Entry->DllBase);

                    if ( RtlCompareMemory(NtHeadersE,NtHeadersSrc,sizeof(*NtHeadersE)) ) {

                        //
                        // Now that it looks like we have a match, compare
                        // volume serial number's and file index's
                        //

                        st = NtAreMappedFilesTheSame(Entry->DllBase,ViewBase);

                        if ( !NT_SUCCESS(st) ) {
                            continue;
                            }
                        else {
                            Result = TRUE;
                            *LdrDataTableEntry = Entry;
                            break;
                            }
                        }
                    }
                }
            except (EXCEPTION_EXECUTE_HANDLER) {
                break;
                }
            }
        NtUnmapViewOfSection(NtCurrentProcess(),ViewBase);
        }

alldone:
    return Result;
}


BOOLEAN
LdrpCheckForLoadedDllHandle (
    IN PVOID DllHandle,
    OUT PLDR_DATA_TABLE_ENTRY *LdrDataTableEntry
    )

/*++

Routine Description:

    This function scans the loader data table looking to see if
    the specified DLL has already been mapped into the image address
    space. If the dll has been loaded, the address of its data table
    entry that describes the dll is returned.

Arguments:

    DllHandle - Supplies the DllHandle of the DLL being searched for.

    LdrDataTableEntry - Returns the address of the loader data table
        entry that describes the dll.

Return Value:

    TRUE- The dll is loaded.  The address of the data table entry is
        returned.

    FALSE - The dll is not loaded.

--*/

{
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY Head,Next;

    if ( LdrpLoadedDllHandleCache &&
        (PVOID) LdrpLoadedDllHandleCache->DllBase == DllHandle ) {
        *LdrDataTableEntry = LdrpLoadedDllHandleCache;
        return TRUE;
        }

    Head = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    Next = Head->Flink;

    while ( Next != Head ) {
        Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        Next = Next->Flink;
        //
        // when we unload, the memory order links flink field is nulled.
        // this is used to skip the entry pending list removal.
        //

        if ( !Entry->InMemoryOrderLinks.Flink ) {
            continue;
        }

        if (DllHandle == (PVOID)Entry->DllBase ){
            LdrpLoadedDllHandleCache = Entry;
            *LdrDataTableEntry = Entry;
            return TRUE;
        }
    }
    return FALSE;
}

NTSTATUS
LdrpMapDll (
    IN PWSTR DllPath OPTIONAL,
    IN PWSTR DllName,
    IN PULONG DllCharacteristics OPTIONAL,
    IN BOOLEAN StaticLink,
    IN BOOLEAN Wx86KnownDll,
    OUT PLDR_DATA_TABLE_ENTRY *LdrDataTableEntry
    )

/*++

Routine Description:

    This routine maps the DLL into the users address space.

Arguments:

    DllPath - Supplies an optional search path to be used to locate the DLL.

    DllName - Supplies the name of the DLL to load.

    StaticLink - TRUE if this DLL has a static link to it.

    Wx86KnownDll - TRUE, treat Importer as x86

    LdrDataTableEntry - Supplies the address of the data table entry.

Return Value:

    Status value.

--*/

{
    NTSTATUS st;
    PVOID ViewBase;
    PTEB Teb = NtCurrentTeb();
    SIZE_T ViewSize;
    HANDLE Section, DllFile;
    UNICODE_STRING FullDllName, BaseDllName;
    UNICODE_STRING NtFileName;
    PLDR_DATA_TABLE_ENTRY Entry;
    PIMAGE_NT_HEADERS NtHeaders;
    PVOID ArbitraryUserPointer;
    BOOLEAN KnownDll;
    UNICODE_STRING CollidingDll;
    PUCHAR ImageBase, ImageBounds, ScanBase, ScanTop;
    PLDR_DATA_TABLE_ENTRY ScanEntry;
    PLIST_ENTRY ScanHead,ScanNext;
    BOOLEAN CollidingDllFound;
    NTSTATUS ErrorStatus;
    ULONG_PTR ErrorParameters[2];
    ULONG ErrorResponse;
#if defined (WX86)
    BOOLEAN Wx86Plugin = FALSE;
#endif


    //
    // Get section handle of DLL being snapped
    //

#if LDRDBG
    if (ShowSnaps) {
        DbgPrint("LDR: LdrpMapDll: Image Name %ws, Search Path %ws\n",
                DllName,
                ARGUMENT_PRESENT(DllPath) ? DllPath : L""
                );
    }
#endif

    Entry = NULL;
    KnownDll = FALSE;
    Section = NULL;

    if ( LdrpKnownDllObjectDirectory && !Wx86KnownDll) {

        PWCH p = DllName;
        WCHAR wch;

        //
        // Skip the KnownDll check if this is an explicit path.
        //

        while (*p) {
            wch = *p++;
            if (wch == (WCHAR)'\\' || wch == (WCHAR)'/' ) {
                goto SkipKnownDllCheck;
                }
            }
        Section = LdrpCheckForKnownDll(
                        DllName,
                        &FullDllName,
                        &BaseDllName
                        );

        }

SkipKnownDllCheck:

    if ( !Section ) {

#if defined (WX86)
        if (Wx86ProcessInit) {
            RtlInitUnicodeString(&BaseDllName, DllName);
            st = LdrpWx86MapDll(DllPath,
                                DllCharacteristics,
                                Wx86KnownDll,
                                StaticLink,
                                &BaseDllName,
                                &Entry,
                                &ViewSize,
                                &Section
                                );

            if (st == STATUS_DLL_NOT_FOUND) {
                goto Wx86MapDllNotFound;
                }

            if (!NT_SUCCESS(st)) {
                return st;
                }


            ViewBase    = Entry->DllBase;
            FullDllName = Entry->FullDllName;
            BaseDllName = Entry->BaseDllName;
            NtHeaders = RtlImageNtHeader(ViewBase);

            goto Wx86MapComplete;

        } else
#endif

        if (LdrpResolveDllName( DllPath,
                                DllName,
                                &FullDllName,
                                &BaseDllName,
                                &DllFile
                              )
           ) {
            if (ShowSnaps) {
                PSZ type;
                type = StaticLink ? "STATIC" : "DYNAMIC";
                DbgPrint("LDR: Loading (%s) %wZ\n",
                         type,
                         &FullDllName
                         );
            }

            if (!RtlDosPathNameToNtPathName_U( FullDllName.Buffer,
                                               &NtFileName,
                                               NULL,
                                               NULL
                                             )
               ) {
                return STATUS_OBJECT_PATH_SYNTAX_BAD;
                }

            st = LdrpCreateDllSection(&NtFileName,
                                      DllFile,
                                      &BaseDllName,
                                      DllCharacteristics,
                                      &Section
                                     );

            RtlFreeHeap(RtlProcessHeap(), 0, NtFileName.Buffer);

            if (!NT_SUCCESS(st)) {
                RtlFreeUnicodeString(&FullDllName);
                RtlFreeUnicodeString(&BaseDllName);
                return st;
            }
#if DBG
            LdrpSectionCreates++;
#endif

        } else {

#ifdef WX86
Wx86MapDllNotFound:
#endif
            if ( StaticLink ) {
                PUNICODE_STRING ErrorStrings[2];
                UNICODE_STRING ErrorDllName, ErrorDllPath;
                ULONG ErrorResponse;

                ErrorStrings[0] = &ErrorDllName;
                ErrorStrings[1] = &ErrorDllPath;
                RtlInitUnicodeString(&ErrorDllName,DllName);
                RtlInitUnicodeString(&ErrorDllPath,
                        ARGUMENT_PRESENT(DllPath) ? DllPath : LdrpDefaultPath.Buffer);
                NtRaiseHardError(
                  (NTSTATUS)STATUS_DLL_NOT_FOUND,
                  2,
                  0x00000003,
                  (PULONG_PTR)ErrorStrings,
                  OptionOk,
                  &ErrorResponse
                  );
                if ( LdrpInLdrInit ) {
                    LdrpFatalHardErrorCount++;
                    }
                }

            return STATUS_DLL_NOT_FOUND;
        }
    } else {
        KnownDll = TRUE;
    }

    ViewBase = NULL;
    ViewSize = 0;


#if DBG
    LdrpSectionMaps++;
    if (LdrpDisplayLoadTime) {
        NtQueryPerformanceCounter(&MapBeginTime, NULL);
    }
#endif

    //
    // arrange for debugger to pick up the image name
    //

    ArbitraryUserPointer = Teb->NtTib.ArbitraryUserPointer;
    Teb->NtTib.ArbitraryUserPointer = (PVOID)FullDllName.Buffer;
    st = NtMapViewOfSection(
            Section,
            NtCurrentProcess(),
            (PVOID *)&ViewBase,
            0L,
            0L,
            NULL,
            &ViewSize,
            ViewShare,
            0L,
            PAGE_READWRITE
            );
    Teb->NtTib.ArbitraryUserPointer = ArbitraryUserPointer;

    if (!NT_SUCCESS(st)) {
        NtClose(Section);
        return st;
    }

    NtHeaders = RtlImageNtHeader(ViewBase);
    if ( !NtHeaders ) {
        NtUnmapViewOfSection(NtCurrentProcess(),ViewBase);
        NtClose(Section);
        return STATUS_INVALID_IMAGE_FORMAT;
        }

#if DBG
    if (LdrpDisplayLoadTime) {
        NtQueryPerformanceCounter(&MapEndTime, NULL);
        MapElapsedTime.QuadPart = MapEndTime.QuadPart - MapBeginTime.QuadPart;
        DbgPrint("Map View of Section Time %ld %ws\n",
            MapElapsedTime.LowPart,
            DllName
            );
    }
#endif

#if defined (BUILD_WOW6432)
    if (NtHeaders->OptionalHeader.SectionAlignment < NATIVE_PAGE_SIZE &&
        !NT_SUCCESS(LdrpWx86FormatVirtualImage((PIMAGE_NT_HEADERS32)NtHeaders, ViewBase)))
      {
        NtUnmapViewOfSection(NtCurrentProcess(),ViewBase);
        NtClose(Section);
        return st;
        }
#elif defined (_ALPHA_) && defined (WX86)
    if ((NtHeaders->OptionalHeader.SectionAlignment < PAGE_SIZE) &&
        !NT_SUCCESS(LdrpWx86FormatVirtualImage((PIMAGE_NT_HEADERS32)NtHeaders, ViewBase)))
      {
        NtUnmapViewOfSection(NtCurrentProcess(),ViewBase);
        NtClose(Section);
        return STATUS_INVALID_IMAGE_FORMAT;
        }

    if (st == STATUS_IMAGE_MACHINE_TYPE_MISMATCH && !StaticLink) {

        // Attempt to find a plugin provider dll that can thunk this interface
        // temporarily insert the image in the loaded-module-list

        Entry = LdrpAllocateDataTableEntry(ViewBase);
        if (!Entry) {
            return STATUS_NO_MEMORY;
        }

        Entry->Flags = 0;
        Entry->LoadCount = 0;
        Entry->FullDllName = FullDllName;
        Entry->BaseDllName = BaseDllName;
        Entry->EntryPoint = LdrpFetchAddressOfEntryPoint(Entry->DllBase);
        LdrpInsertMemoryTableEntry(Entry);

        // Determine if this dll can be thunked using a plugin

        st = Wx86IdentifyPlugin(ViewBase, &FullDllName );

         // remove the image from the loaded-module-list

        RemoveEntryList(&Entry->InLoadOrderLinks);
        RemoveEntryList(&Entry->InMemoryOrderLinks);
        RemoveEntryList(&Entry->HashLinks);
        RtlFreeHeap(RtlProcessHeap(), 0, Entry);
        Entry = NULL;

        if (st == STATUS_SUCCESS) {

            // Release the previous mapping because it's going to
            // be reloaded via Wx86.

            NtUnmapViewOfSection(NtCurrentProcess(),ViewBase);
            ViewBase = NULL;

            // The plugin providers must be statically bound to Wx86
            // or dynamically load it in their DllInit.

            if (Wx86ProcessInit) {

                // Load the dll again using Wx86.

                st = LdrpWx86MapDll(NULL,
                                    DllCharacteristics,
                                    !Wx86KnownDll,          // swap sense so we don't get mismatch
                                    StaticLink,
                                    &FullDllName,
                                    &Entry,
                                    &ViewSize,
                                    &Section
                                    );

                // Load and thunk the plugin dll

                if (NT_SUCCESS(st)) {

                    ViewBase    = Entry->DllBase;
                    FullDllName = Entry->FullDllName;
                    BaseDllName = Entry->BaseDllName;
                    NtHeaders = RtlImageNtHeader(ViewBase);

                }
            }

            if (!Wx86ProcessInit || NT_ERROR(st)) {
                Wx86UnloadProviders(ViewBase);
                st = STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
            }
        }

        // Need to save the status here to put in loader ENTRY after it's allocated.

        Wx86Plugin = (st == STATUS_SUCCESS);

        if (ShowSnaps) {
            PCHAR Action;

            if (st == STATUS_SUCCESS) {
                Action = "Loaded";
            } else if (st == STATUS_IMAGE_MACHINE_TYPE_MISMATCH) {
                Action = "Unsupported";
            } else {
                Action = "Failed";
            }

            DbgPrint("LDRWx86: Plugin: %wZ %s.\n",
                &FullDllName, Action
                );
        }
    }

    if (!NT_SUCCESS(st) || (ViewBase == NULL)) {
        if (ViewBase) NtUnmapViewOfSection(NtCurrentProcess(),ViewBase);
        NtClose(Section);
        return st;
    } else if (Entry) {
        goto Wx86MapComplete;
    }
#endif

    //
    // Allocate a data table entry.
    //

    Entry = LdrpAllocateDataTableEntry(ViewBase);

    if (!Entry) {
        NtUnmapViewOfSection(NtCurrentProcess(),ViewBase);
        NtClose(Section);
        return STATUS_NO_MEMORY;
    }


    Entry->Flags = (USHORT)(StaticLink ? LDRP_STATIC_LINK : 0);
    Entry->LoadCount = 0;
    Entry->FullDllName = FullDllName;
    Entry->BaseDllName = BaseDllName;
    Entry->EntryPoint = LdrpFetchAddressOfEntryPoint(Entry->DllBase);

#ifdef WX86
Wx86MapComplete:
    if (Wx86Plugin) {
        Entry->Flags |= LDRP_WX86_PLUGIN;
    }
#endif


#if LDRDBG
    if (ShowSnaps) {
        DbgPrint("LDR: LdrpMapDll: Full Name %wZ, Base Name %wZ\n",
                &FullDllName,
                &BaseDllName
                );
    }
#endif

    LdrpInsertMemoryTableEntry(Entry);

    if ( st == STATUS_IMAGE_MACHINE_TYPE_MISMATCH ) {

        PIMAGE_NT_HEADERS ImageHeader = RtlImageNtHeader( NtCurrentPeb()->ImageBaseAddress );

        //
        // apps compiled for NT 3.x and below can load cross architecture
        // images
        //

        ErrorStatus = STATUS_SUCCESS;
        ErrorResponse = ResponseCancel;

        if ( ImageHeader->OptionalHeader.MajorSubsystemVersion <= 3 ) {

            Entry->EntryPoint = 0;

            //
            // Hard Error Time
            //

            //
            // Its error time...
            //

            ErrorParameters[0] = (ULONG_PTR)&FullDllName;

            ErrorStatus = NtRaiseHardError(
                            STATUS_IMAGE_MACHINE_TYPE_MISMATCH,
                            1,
                            1,
                            ErrorParameters,
                            OptionOkCancel,
                            &ErrorResponse
                            );
            }
        if ( NT_SUCCESS(ErrorStatus) && ErrorResponse == ResponseCancel ) {
            RemoveEntryList(&Entry->InLoadOrderLinks);
            RemoveEntryList(&Entry->InMemoryOrderLinks);
            RemoveEntryList(&Entry->HashLinks);
            RtlFreeHeap(RtlProcessHeap(), 0, Entry );
            NtUnmapViewOfSection(NtCurrentProcess(),ViewBase);
            NtClose(Section);

            if ( ImageHeader->OptionalHeader.MajorSubsystemVersion <= 3 ) {
                if ( LdrpInLdrInit ) {
                    LdrpFatalHardErrorCount++;
                    }
                }
            return STATUS_INVALID_IMAGE_FORMAT;
            }
        }
    else {
        if (NtHeaders->FileHeader.Characteristics & IMAGE_FILE_DLL) {
            Entry->Flags |= LDRP_IMAGE_DLL;
            }

        if (!(Entry->Flags & LDRP_IMAGE_DLL)) {
            Entry->EntryPoint = 0;
            }
        }
    *LdrDataTableEntry = Entry;

    if (st == STATUS_IMAGE_NOT_AT_BASE) {

        Entry->Flags |= LDRP_IMAGE_NOT_AT_BASE;

        //
        // now find the colliding dll. If we can not find a dll,
        // then the colliding dll must be dynamic memory
        //

        ImageBase = (PUCHAR)NtHeaders->OptionalHeader.ImageBase;
        ImageBounds = ImageBase + ViewSize;

        CollidingDllFound = FALSE;

        ScanHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
        ScanNext = ScanHead->Flink;

        while ( ScanNext != ScanHead ) {
            ScanEntry = CONTAINING_RECORD(ScanNext, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
            ScanNext = ScanNext->Flink;

            ScanBase = (PUCHAR)ScanEntry->DllBase;
            ScanTop = ScanBase + ScanEntry->SizeOfImage;

            //
            // when we unload, the memory order links flink field is nulled.
            // this is used to skip the entry pending list removal.
            //

            if ( !ScanEntry->InMemoryOrderLinks.Flink ) {
                continue;
                }

            //
            // See if the base address of the scan image is within the relocating dll
            // or if the top address of the scan image is within the relocating dll
            //

            if ( (ImageBase >= ScanBase && ImageBase <= ScanTop)

                 ||

                 (ImageBounds >= ScanBase && ImageBounds <= ScanTop)

                 ||

                 (ScanBase >= ImageBase && ScanBase <= ImageBounds)

                 ){

                CollidingDllFound = TRUE;
                CollidingDll = ScanEntry->FullDllName;
                break;
                }
            }

        if ( !CollidingDllFound ) {
            RtlInitUnicodeString(&CollidingDll,L"Dynamically Allocated Memory");
            }

#if DBG
        if ( BeginTime.LowPart || BeginTime.HighPart ) {
            DbgPrint("\nLDR: LdrpMapDll Relocateing Image Name %ws\n",
                    DllName
                    );
        }
        LdrpSectionRelocates++;
#endif
        if (Entry->Flags & LDRP_IMAGE_DLL) {

            BOOLEAN AllowRelocation;
            UNICODE_STRING SystemDll;

            if (!(NtHeaders->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)) {

                PVOID pBaseRelocs;
                ULONG BaseRelocCountBytes = 0;

                //
                // If the iamge doesn't have the reloc stripped bit set and there's no
                // relocs in the data directory, allow this through.  This is probably
                // a pure forwarder dll or data w/o relocs.
                //

                pBaseRelocs = RtlImageDirectoryEntryToData(
                        ViewBase, TRUE, IMAGE_DIRECTORY_ENTRY_BASERELOC, &BaseRelocCountBytes);

                if (!pBaseRelocs && !BaseRelocCountBytes) {
                    goto NoRelocNeeded;
                    }
                }

            //
            // decide whether or not to allow the relocation
            // certain system dll's like user32 and kernel32 are not relocatable
            // since addresses within these dll's are not always stored per process
            // do not allow these dll's to be relocated
            //

            AllowRelocation = TRUE;
            RtlInitUnicodeString(&SystemDll,L"user32.dll");
            if ( RtlEqualUnicodeString(&BaseDllName,&SystemDll,TRUE) ) {
                AllowRelocation = FALSE;
                }
            else {
                RtlInitUnicodeString(&SystemDll,L"kernel32.dll");
                if ( RtlEqualUnicodeString(&BaseDllName,&SystemDll,TRUE) ) {
                    AllowRelocation = FALSE;
                    }
                }
            if ( !AllowRelocation && KnownDll ) {

                //
                // totally disallow the relocation since this is a knowndll
                // that matches our system binaries and is being relocated
                //

                //
                // Hard Error Time
                //

                ErrorParameters[0] = (ULONG_PTR)&SystemDll;
                ErrorParameters[1] = (ULONG_PTR)&CollidingDll;

                NtRaiseHardError(
                  STATUS_ILLEGAL_DLL_RELOCATION,
                  2,
                  3,
                  ErrorParameters,
                  OptionOk,
                  &ErrorResponse
                  );

                if ( LdrpInLdrInit ) {
                    LdrpFatalHardErrorCount++;
                    }

                st = STATUS_CONFLICTING_ADDRESSES;
                goto skipreloc;
                }

            st = LdrpSetProtection(ViewBase, FALSE, StaticLink );
            if (NT_SUCCESS(st)) {
                st = (NTSTATUS)LdrRelocateImage(ViewBase,
                            "LDR",
                            (ULONG)STATUS_SUCCESS,
                            (ULONG)STATUS_CONFLICTING_ADDRESSES,
                            (ULONG)STATUS_INVALID_IMAGE_FORMAT
                            );
                if (NT_SUCCESS(st)) {

                    //
                    // If we did relocations, then map the section again.
                    // this will force the debug event
                    //

                    //
                    // arrange for debugger to pick up the image name
                    //

                    ArbitraryUserPointer = Teb->NtTib.ArbitraryUserPointer;
                    Teb->NtTib.ArbitraryUserPointer = (PVOID)FullDllName.Buffer;
                    NtMapViewOfSection(
                        Section,
                        NtCurrentProcess(),
                        (PVOID *)&ViewBase,
                        0L,
                        0L,
                        NULL,
                        &ViewSize,
                        ViewShare,
                        0L,
                        PAGE_READWRITE
                        );
                    Teb->NtTib.ArbitraryUserPointer = ArbitraryUserPointer;

                    st = LdrpSetProtection(ViewBase, TRUE, StaticLink);

                }
            }
skipreloc:
            //
            // if the set protection failed, or if the relocation failed, then
            // remove the partially loaded dll from the lists and clear entry
            // that it has been freed.
            //

            if ( !NT_SUCCESS(st) ) {
                RemoveEntryList(&Entry->InLoadOrderLinks);
                RemoveEntryList(&Entry->InMemoryOrderLinks);
                RemoveEntryList(&Entry->HashLinks);
                NtUnmapViewOfSection(NtCurrentProcess(),ViewBase);
                Entry = NULL;
                }

            if (ShowSnaps) {
                DbgPrint("LDR: Fixups %successfully re-applied @ %lx\n",
                       NT_SUCCESS(st) ? "s" : "uns", ViewBase);
            }
        } else {
NoRelocNeeded:
                 st = STATUS_SUCCESS;

                 //
                 // arrange for debugger to pick up the image name
                 //

                 ArbitraryUserPointer = Teb->NtTib.ArbitraryUserPointer;
                 Teb->NtTib.ArbitraryUserPointer = (PVOID)FullDllName.Buffer;
                 NtMapViewOfSection(
                     Section,
                     NtCurrentProcess(),
                     (PVOID *)&ViewBase,
                     0L,
                     0L,
                     NULL,
                     &ViewSize,
                     ViewShare,
                     0L,
                     PAGE_READWRITE
                     );
                 Teb->NtTib.ArbitraryUserPointer = ArbitraryUserPointer;

                 if (ShowSnaps) {
                     DbgPrint("LDR: Fixups won't be re-applied to non-Dll @ %lx\n",
                              ViewBase);
                 }
               }
    }

#if defined (_ALPHA_)

    //
    // Find and apply Alpha architecture fixups for this dll
    //
    if (Entry->Flags & LDRP_IMAGE_DLL)
        AlphaFindArchitectureFixups(NtHeaders, ViewBase, StaticLink);
#endif

#if defined(_X86_)
    if ( LdrpNumberOfProcessors > 1 && Entry && (Entry->Flags & LDRP_IMAGE_DLL) ) {
        LdrpValidateImageForMp(Entry);
        }
#endif
    NtClose(Section);
    return st;
}

#if defined (_ALPHA_)

#define asm __asm
#pragma intrinsic (__asm)

VOID
AlphaApplyArchitectureFixups(
    IN PIMAGE_ARCHITECTURE_HEADER ArchHeader,
    IN ULONG ArchHeaderByteSize,
    IN ULONG_PTR BaseAddr
    )

/*++

Routine Description:

    This applies the Alpha architecture enhancement fixups. If an image contains
    a .arch section, then the image/dll, if running on a newer chip, may
    be eligible to take advantage of new instructions. Check the replacement
    constraints vs. chip capabilities and apply fixup if warranted.


Arguments:

    ArchHeader - Supplies a pointer to the first architecture specific
        fixup header record.

    ArchHeaderByteSize - Supplies the size (in bytes) of the
        Architecture headers to be processed.

    BaseAddr - Base address to be added to RVA's in architecture section

Return Value:

    None.

--*/

{
    ULONG Count, Entry;
    ULONG SystemAmask;
    ULONG ReplacementInstruction;
    PIMAGE_ARCHITECTURE_HEADER ArchHeaderEnd;
    PIMAGE_ARCHITECTURE_ENTRY ArchEntry;

    // Figure out the end value for ArchHeader

    ArchHeaderEnd = (PIMAGE_ARCHITECTURE_HEADER)((PCHAR)ArchHeader + ArchHeaderByteSize);
    //
    // Get the architecture mask and implementation version
    // of the running system
    //

    while (*(PULONGLONG)ArchHeader == 0L)
        ArchHeader++; // skip over quadwords of zero

    SystemAmask = ~asm("amask $16, $0", -1);

    while ((*(PULONGLONG)ArchHeader != 0xffffffffL) && (ArchHeader < ArchHeaderEnd)) {
        if (*(PULONGLONG)ArchHeader != 0L) { // maybe zero's intermixed
#if DBG && ARCH_DBG
            DbgPrint("ARCH: Amask=0x%x, Current Header=0x%08x, Shift=%d, Value=%d\n",
                    SystemAmask, *ArchHeader,
                    ArchHeader->AmaskShift, ArchHeader->AmaskValue);
#endif
            if (((SystemAmask >> ArchHeader->AmaskShift) & 1) != ArchHeader->AmaskValue) {

                //
                // fixup needed
                //

                PIMAGE_ARCHITECTURE_ENTRY ArchEntry;

                ArchEntry = (PIMAGE_ARCHITECTURE_ENTRY)(ArchHeader->FirstEntryRVA + BaseAddr);

                while (*(PULONGLONG)ArchEntry != 0xffffffffL) {

                    if (*(PULONGLONG)ArchEntry != 0L) { // maybe zero's intermixed
#if DBG && ARCH_DBG
                        DbgPrint("ARCH: 0x%08x -> 0x%08x\n",
                                 *(PALPHA_INSTRUCTION)(ArchEntry->FixupInstRVA + BaseAddr),
                                 ArchEntry->NewInst
                                );
#endif
                        *(ULONG *)(ArchEntry->FixupInstRVA + BaseAddr) = ArchEntry->NewInst;
                    }
                    ArchEntry++;
                }
            } // if this header describes fixups for this machine
        }
        ArchHeader++;
    }
}

VOID
AlphaFindArchitectureFixups(
    PIMAGE_NT_HEADERS NtHeaders,
    PVOID ViewBase,
    BOOLEAN StaticLink
    )

/*++

Routine Description:

    This routine runs through a mapped image/dll looking for an architecture
    enhancement section (.arch). If found, it applies the architecture
    fixups.


Arguments:

    NtHeaders   - the NT header for this image or dll.
    ViewBase    - the Mapped view base used for page protection munging.
    StaticLink  - TRUE if this DLL has a static link to it

Return Value:

    None.

--*/

{
    ULONG Size;
    PIMAGE_ARCHITECTURE_HEADER ImageArchData;

    if (NtHeaders->FileHeader.Machine != IMAGE_FILE_MACHINE_ALPHA) {
        //
        // If the image isnt Alpha, don't try to find/apply Alpha fixups.
        // The image may be an x86 image linked with the x86 Watcom
        // linker which stuffed its version number in the .arch directory
        // entry in the image, fooling the Alpha .arch checker.
        //
        return;
    }

    ImageArchData =
        (PIMAGE_ARCHITECTURE_HEADER)RtlImageDirectoryEntryToData(
                                ViewBase,
                                TRUE,
                                IMAGE_DIRECTORY_ENTRY_ARCHITECTURE,
                                &Size
                                );
    if (ImageArchData) {
        NTSTATUS st;

        st = LdrpSetProtection(ViewBase, FALSE, StaticLink); // access to r/w

        if (NT_SUCCESS(st)) {
            AlphaApplyArchitectureFixups(ImageArchData, Size, (ULONG_PTR)ViewBase);
            LdrpSetProtection(ViewBase, TRUE, StaticLink); // back to r/o
        }
    }
}
#endif

NTSTATUS
LdrpCreateDllSection(
    IN PUNICODE_STRING NtFullDllName,
    IN HANDLE DllFile,
    IN PUNICODE_STRING BaseName,
    IN PULONG DllCharacteristics OPTIONAL,
    OUT PHANDLE SectionHandle
    )
{
    HANDLE File;
    HANDLE Section;
    NTSTATUS st;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;

    if ( !DllFile ) {
        //
        // Since ntsdk does not search paths well, we can't use
        // relative object names
        //

        InitializeObjectAttributes(
            &ObjectAttributes,
            NtFullDllName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        st = NtOpenFile(
                &File,
                SYNCHRONIZE | FILE_EXECUTE,
                &ObjectAttributes,
                &IoStatus,
                FILE_SHARE_READ | FILE_SHARE_DELETE,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                );
        if (!NT_SUCCESS(st)) {

#if DBG
            DbgPrint("LDR: LdrCreateDllSection - NtOpenFile( %wZ ) failed. Status == %X\n",
                NtFullDllName,
                st
                );
#endif
            *SectionHandle = NULL;
            return st;
        }


    } else {
             File = DllFile;
           }


    st = NtCreateSection(
            SectionHandle,
            SECTION_MAP_READ | SECTION_MAP_EXECUTE | SECTION_MAP_WRITE | SECTION_QUERY,
            NULL,
            NULL,
            PAGE_EXECUTE,
            SEC_IMAGE,
            File
            );
    NtClose( File );

    if (!NT_SUCCESS(st)) {

        //
        // hard error time
        //

        ULONG_PTR ErrorParameters[1];
        ULONG ErrorResponse;

        *SectionHandle = NULL;
        ErrorParameters[0] = (ULONG_PTR)NtFullDllName;

        NtRaiseHardError(
          STATUS_INVALID_IMAGE_FORMAT,
          1,
          1,
          ErrorParameters,
          OptionOk,
          &ErrorResponse
          );

        if ( LdrpInLdrInit ) {
            LdrpFatalHardErrorCount++;
            }


#if DBG
        if (st != STATUS_INVALID_IMAGE_NE_FORMAT &&
            st != STATUS_INVALID_IMAGE_LE_FORMAT &&
            st != STATUS_INVALID_IMAGE_WIN_16
           ) {
            DbgPrint("LDR: LdrCreateDllSection - NtCreateSection %wZ failed. Status == %X\n",
                     NtFullDllName,
                     st
                    );
        }
#endif
    }

    return st;
}

NTSTATUS
LdrpSnapIAT (
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry_Export,
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry_Import,
    IN PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor,
    IN BOOLEAN SnapForwardersOnly
    )

/*++

Routine Description:

    This function snaps the Import Address Table for this
    Import Descriptor.

Arguments:

    LdrDataTableEntry_Export - Information about the image to import from.

    LdrDataTableEntry_Import - Information about the image to import to.

    ImportDescriptor - Contains a pointer to the IAT to snap.

    SnapForwardersOnly - TRUE if just snapping forwarders only.

Return Value:

    Status value

--*/

{
    PPEB Peb;
    NTSTATUS st;
    ULONG ExportSize;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    PIMAGE_THUNK_DATA Thunk, OriginalThunk;
    PSZ ImportName;
    ULONG ForwarderChain;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER NtSection;
    ULONG i, Rva;
    PVOID IATBase;
    SIZE_T IATSize;
    ULONG LittleIATSize;
    ULONG OldProtect;

    Peb = NtCurrentPeb();

    ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(
                       LdrDataTableEntry_Export->DllBase,
                       TRUE,
                       IMAGE_DIRECTORY_ENTRY_EXPORT,
                       &ExportSize
                       );

    if (!ExportDirectory) {
        KdPrint(("LDR: %wZ doesn't contain an EXPORT table\n", &LdrDataTableEntry_Export->BaseDllName));
        return STATUS_INVALID_IMAGE_FORMAT;
        }

    //
    // Determine the location and size of the IAT.  If the linker did
    // not tell use explicitly, then use the location and size of the
    // image section that contains the import table.
    //

    IATBase = RtlImageDirectoryEntryToData( LdrDataTableEntry_Import->DllBase,
                                            TRUE,
                                            IMAGE_DIRECTORY_ENTRY_IAT,
                                            &LittleIATSize
                                          );
    IATSize = LittleIATSize;
    if (IATBase == NULL) {
        NtHeaders = RtlImageNtHeader( LdrDataTableEntry_Import->DllBase );
        NtSection = IMAGE_FIRST_SECTION( NtHeaders );
        Rva = NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress;
        if (Rva != 0) {
            for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++) {
                if (Rva >= NtSection->VirtualAddress &&
                    Rva < (NtSection->VirtualAddress + NtSection->SizeOfRawData)
                   ) {
                    IATBase = (PVOID)
                        ((ULONG_PTR)(LdrDataTableEntry_Import->DllBase) + NtSection->VirtualAddress);

                    LittleIATSize = NtSection->Misc.VirtualSize;
                    if (LittleIATSize == 0) {
                        LittleIATSize = NtSection->SizeOfRawData;
                        }
                    break;
                    }

                ++NtSection;
                }
            }

        if (IATBase == NULL) {
            KdPrint(( "LDR: Unable to unprotect IAT for %wZ (Image Base %x)\n",
                      &LdrDataTableEntry_Import->BaseDllName,
                      LdrDataTableEntry_Import->DllBase
                   ));
            return STATUS_INVALID_IMAGE_FORMAT;
            }
        IATSize = LittleIATSize;
        }

    st = NtProtectVirtualMemory( NtCurrentProcess(),
                                 &IATBase,
                                 &IATSize,
                                 PAGE_READWRITE,
                                 &OldProtect
                               );
    if (!NT_SUCCESS(st)) {
        KdPrint(( "LDR: Unable to unprotect IAT for %wZ (Status %x)\n",
                  &LdrDataTableEntry_Import->BaseDllName,
                  st
               ));
        return st;
        }

    //
    // If just snapping forwarded entries, walk that list
    //
    if (SnapForwardersOnly) {
        ImportName = (PSZ)((ULONG_PTR)LdrDataTableEntry_Import->DllBase + ImportDescriptor->Name);
        ForwarderChain = ImportDescriptor->ForwarderChain;
        while (ForwarderChain != -1) {
            OriginalThunk = (PIMAGE_THUNK_DATA)((ULONG_PTR)LdrDataTableEntry_Import->DllBase +
                            ImportDescriptor->OriginalFirstThunk +
                            (ForwarderChain * sizeof(IMAGE_THUNK_DATA)));
            Thunk = (PIMAGE_THUNK_DATA)((ULONG_PTR)LdrDataTableEntry_Import->DllBase +
                            ImportDescriptor->FirstThunk +
                            (ForwarderChain * sizeof(IMAGE_THUNK_DATA)));
            ForwarderChain = (ULONG) Thunk->u1.Ordinal;
            try {
                st = LdrpSnapThunk(LdrDataTableEntry_Export->DllBase,
                        LdrDataTableEntry_Import->DllBase,
                        OriginalThunk,
                        Thunk,
                        ExportDirectory,
                        ExportSize,
                        TRUE,
                        ImportName
                        );
                Thunk++;
                }
            except (EXCEPTION_EXECUTE_HANDLER) {
                st = GetExceptionCode();
                }
            if (!NT_SUCCESS(st) ) {
                break;
                }
            }
        }
    else

    //
    // Otherwise, walk through the IAT and snap all the thunks.
    //

    if ( ImportDescriptor->FirstThunk ) {
        Thunk = (PIMAGE_THUNK_DATA)((ULONG_PTR)LdrDataTableEntry_Import->DllBase + ImportDescriptor->FirstThunk);

        NtHeaders = RtlImageNtHeader( LdrDataTableEntry_Import->DllBase );
        //
        // If the OriginalFirstThunk field does not point inside the image, then ignore
        // it.  This is will detect bogus Borland Linker 2.25 images that did not fill
        // this field in.
        //

        if (ImportDescriptor->Characteristics < NtHeaders->OptionalHeader.SizeOfHeaders ||
            ImportDescriptor->Characteristics >= NtHeaders->OptionalHeader.SizeOfImage
           ) {
            OriginalThunk = Thunk;
        } else {
            OriginalThunk = (PIMAGE_THUNK_DATA)((ULONG_PTR)LdrDataTableEntry_Import->DllBase +
                            ImportDescriptor->OriginalFirstThunk);
        }
        ImportName = (PSZ)((ULONG_PTR)LdrDataTableEntry_Import->DllBase + ImportDescriptor->Name);
        while (OriginalThunk->u1.AddressOfData) {
            try {
                st = LdrpSnapThunk(LdrDataTableEntry_Export->DllBase,
                        LdrDataTableEntry_Import->DllBase,
                        OriginalThunk,
                        Thunk,
                        ExportDirectory,
                        ExportSize,
                        TRUE,
                        ImportName
                        );
                OriginalThunk++;
                Thunk++;
                }
            except (EXCEPTION_EXECUTE_HANDLER) {
                st = GetExceptionCode();
                }

            if (!NT_SUCCESS(st) ) {
                break;
                }
            }
        }

    //
    // Restore protection for IAT and flush instruction cache.
    //

    NtProtectVirtualMemory( NtCurrentProcess(),
                            &IATBase,
                            &IATSize,
                            OldProtect,
                            &OldProtect
                          );
    NtFlushInstructionCache( NtCurrentProcess(), IATBase, LittleIATSize );

    return st;
}

NTSTATUS
LdrpSnapThunk (
    IN PVOID DllBase,
    IN PVOID ImageBase,
    IN PIMAGE_THUNK_DATA OriginalThunk,
    IN OUT PIMAGE_THUNK_DATA Thunk,
    IN PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    IN ULONG ExportSize,
    IN BOOLEAN StaticSnap,
    IN PSZ DllName
    )

/*++

Routine Description:

    This function snaps a thunk using the specified Export Section data.
    If the section data does not support the thunk, then the thunk is
    partially snapped (Dll field is still non-null, but snap address is
    set).

Arguments:

    DllBase - Base of Dll.

    ImageBase - Base of image that contains the thunks to snap.

    Thunk - On input, supplies the thunk to snap.  When successfully
        snapped, the function field is set to point to the address in
        the DLL, and the DLL field is set to NULL.

    ExportDirectory - Supplies the Export Section data from a DLL.

    StaticSnap - If TRUE, then loader is attempting a static snap,
                 and any ordinal/name lookup failure will be reported.

Return Value:

    STATUS_SUCCESS or STATUS_PROCEDURE_NOT_FOUND

--*/

{
    BOOLEAN Ordinal;
    USHORT OrdinalNumber;
    ULONG OriginalOrdinalNumber;
    PIMAGE_IMPORT_BY_NAME AddressOfData;
    PULONG NameTableBase;
    PUSHORT NameOrdinalTableBase;
    PULONG Addr;
    USHORT HintIndex;
    NTSTATUS st;
    PSZ ImportString;

    //
    // Determine if snap is by name, or by ordinal
    //

    Ordinal = (BOOLEAN)IMAGE_SNAP_BY_ORDINAL(OriginalThunk->u1.Ordinal);

    if (Ordinal) {
        OriginalOrdinalNumber = (ULONG)IMAGE_ORDINAL(OriginalThunk->u1.Ordinal);
        OrdinalNumber = (USHORT)(OriginalOrdinalNumber - ExportDirectory->Base);
    } else {
             //
             // Change AddressOfData from an RVA to a VA.
             //

             AddressOfData = (PIMAGE_IMPORT_BY_NAME)((ULONG_PTR)ImageBase + ((ULONG_PTR)OriginalThunk->u1.AddressOfData & 0xffffffff));
             ImportString = (PSZ)AddressOfData->Name;

             //
             // Lookup Name in NameTable
             //

             NameTableBase = (PULONG)((ULONG_PTR)DllBase + (ULONG)ExportDirectory->AddressOfNames);
             NameOrdinalTableBase = (PUSHORT)((ULONG_PTR)DllBase + (ULONG)ExportDirectory->AddressOfNameOrdinals);

             //
             // Before dropping into binary search, see if
             // the hint index results in a successful
             // match. If the hint index is zero, then
             // drop into binary search.
             //

             HintIndex = AddressOfData->Hint;
             if ((ULONG)HintIndex < ExportDirectory->NumberOfNames &&
                 !strcmp(ImportString, (PSZ)((ULONG_PTR)DllBase + NameTableBase[HintIndex]))) {
                 OrdinalNumber = NameOrdinalTableBase[HintIndex];
#if LDRDBG
                 if (ShowSnaps) {
                     DbgPrint("LDR: Snapping %s\n", ImportString);
                 }
#endif
             } else {
#if LDRDBG
                      if (HintIndex) {
                          DbgPrint("LDR: Warning HintIndex Failure. Name %s (%lx) Hint 0x%lx\n",
                              ImportString,
                              (ULONG)ImportString,
                              (ULONG)HintIndex
                              );
                      }
#endif
                      OrdinalNumber = LdrpNameToOrdinal(
                                        ImportString,
                                        ExportDirectory->NumberOfNames,
                                        DllBase,
                                        NameTableBase,
                                        NameOrdinalTableBase
                                        );
                    }
           }

    //
    // If OrdinalNumber is not within the Export Address Table,
    // then DLL does not implement function. Snap to LDRP_BAD_DLL.
    //

    if ((ULONG)OrdinalNumber >= ExportDirectory->NumberOfFunctions) {
baddllref:
#if DBG
        if (StaticSnap) {
            if (Ordinal) {
                DbgPrint("LDR: Can't locate ordinal 0x%lx\n", OriginalOrdinalNumber);
                }
            else {
                DbgPrint("LDR: Can't locate %s\n", ImportString);
                }
        }
#endif
        if ( StaticSnap ) {
            //
            // Hard Error Time
            //

            ULONG_PTR ErrorParameters[3];
            UNICODE_STRING ErrorDllName, ErrorEntryPointName;
            ANSI_STRING AnsiScratch;
            ULONG ParameterStringMask;
            ULONG ErrorResponse;

            RtlInitAnsiString(&AnsiScratch,DllName ? DllName : "Unknown");
            RtlAnsiStringToUnicodeString(&ErrorDllName,&AnsiScratch,TRUE);
            ErrorParameters[1] = (ULONG_PTR)&ErrorDllName;
            ParameterStringMask = 2;

            if ( Ordinal ) {
                ErrorParameters[0] = OriginalOrdinalNumber;
                }
            else {
                RtlInitAnsiString(&AnsiScratch,ImportString);
                RtlAnsiStringToUnicodeString(&ErrorEntryPointName,&AnsiScratch,TRUE);
                ErrorParameters[0] = (ULONG_PTR)&ErrorEntryPointName;
                ParameterStringMask = 3;
                }


            NtRaiseHardError(
              Ordinal ? STATUS_ORDINAL_NOT_FOUND : STATUS_ENTRYPOINT_NOT_FOUND,
              2,
              ParameterStringMask,
              ErrorParameters,
              OptionOk,
              &ErrorResponse
              );

            if ( LdrpInLdrInit ) {
                LdrpFatalHardErrorCount++;
                }
            RtlFreeUnicodeString(&ErrorDllName);
            if ( !Ordinal ) {
                RtlFreeUnicodeString(&ErrorEntryPointName);
                RtlRaiseStatus(STATUS_ENTRYPOINT_NOT_FOUND);
                }
            RtlRaiseStatus(STATUS_ORDINAL_NOT_FOUND);
            }
        Thunk->u1.Function = (ULONG_PTR)LDRP_BAD_DLL;
        st = Ordinal ? STATUS_ORDINAL_NOT_FOUND : STATUS_ENTRYPOINT_NOT_FOUND;
    } else {
             Addr = (PULONG)((ULONG_PTR)DllBase + (ULONG)ExportDirectory->AddressOfFunctions);
             Thunk->u1.Function = ((ULONG_PTR)DllBase + Addr[OrdinalNumber]);
             if (Thunk->u1.Function > (ULONG_PTR)ExportDirectory &&
                 Thunk->u1.Function < ((ULONG_PTR)ExportDirectory + ExportSize)
                ) {
                UNICODE_STRING UnicodeString;
                ANSI_STRING ForwardDllName;
                PVOID ForwardDllHandle;
                PUNICODE_STRING ForwardProcName;
                ULONG ForwardProcOrdinal;

                ImportString = (PSZ)Thunk->u1.Function;
                ForwardDllName.Buffer = ImportString,
                ForwardDllName.Length = (USHORT)(strchr(ImportString, '.') - ImportString);
                ForwardDllName.MaximumLength = ForwardDllName.Length;
                st = RtlAnsiStringToUnicodeString(&UnicodeString, &ForwardDllName, TRUE);

                if (NT_SUCCESS(st)) {
#if defined (WX86)
                    if (Wx86ProcessInit) {
                        NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = RtlImageNtHeader(DllBase)->FileHeader.Machine
                                                   == IMAGE_FILE_MACHINE_I386;

                        }
#endif


                        st = LdrpLoadDll(NULL, NULL, &UnicodeString, &ForwardDllHandle,FALSE);
                    RtlFreeUnicodeString(&UnicodeString);
                    }

                if (!NT_SUCCESS(st)) {
                    goto baddllref;
                    }

                RtlInitAnsiString( &ForwardDllName,
                                   ImportString + ForwardDllName.Length + 1
                                 );
                if (ForwardDllName.Length > 1 &&
                    *ForwardDllName.Buffer == '#'
                   ) {
                    ForwardProcName = NULL;
                    st = RtlCharToInteger( ForwardDllName.Buffer+1,
                                           0,
                                           &ForwardProcOrdinal
                                         );
                    if (!NT_SUCCESS(st)) {
                        goto baddllref;
                        }
                    }
                else {
                    ForwardProcName = (PUNICODE_STRING)&ForwardDllName;

                    //
                    // Following line is not needed since this is a by name lookup
                    //
                    //
                    //ForwardProcOrdinal = (ULONG)&ForwardDllName;
                    //
                    }

                st = LdrpGetProcedureAddress( ForwardDllHandle,
                                              (PANSI_STRING )ForwardProcName,
                                              ForwardProcOrdinal,
                                              &(PVOID)Thunk->u1.Function,
                                              FALSE
                                            );
                if (!NT_SUCCESS(st)) {
                    goto baddllref;
                    }
                }
             else {
                if ( !Addr[OrdinalNumber] ) {
                    goto baddllref;
                    }
#if defined (_ALPHA_) && defined (WX86)
                else {
                   PIMAGE_NT_HEADERS ExportNtHeaders = RtlImageNtHeader(DllBase);

                   if ((ExportNtHeaders->OptionalHeader.SectionAlignment < PAGE_SIZE) &&
                       (ExportNtHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)) {
                       Thunk->u1.Function += LdrpWx86RelocatedFixupDiff(DllBase, Addr[OrdinalNumber]);
                    }
                }
#endif //  defined (_ALPHA_) && defined (WX86)
             }
             st = STATUS_SUCCESS;
           }
    return st;
}

USHORT
LdrpNameToOrdinal (
    IN PSZ Name,
    IN ULONG NumberOfNames,
    IN PVOID DllBase,
    IN PULONG NameTableBase,
    IN PUSHORT NameOrdinalTableBase
    )
{
    LONG High;
    LONG Low;
    LONG Middle;
    LONG Result;

    //
    // Lookup the import name in the name table using a binary search.
    //

    Low = 0;
    High = NumberOfNames - 1;
    while (High >= Low) {

        //
        // Compute the next probe index and compare the import name
        // with the export name entry.
        //

        Middle = (Low + High) >> 1;
        Result = strcmp(Name, (PCHAR)((ULONG_PTR)DllBase + NameTableBase[Middle]));

        if (Result < 0) {
            High = Middle - 1;

        } else if (Result > 0) {
            Low = Middle + 1;

        } else {
            break;
        }
    }

    //
    // If the high index is less than the low index, then a matching
    // table entry was not found. Otherwise, get the ordinal number
    // from the ordinal table.
    //

    if (High < Low) {
        return (USHORT)-1;
    } else {
        return NameOrdinalTableBase[Middle];
    }

}

VOID
LdrpUpdateLoadCount (
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry,
    IN BOOLEAN IncrementCount
    )

/*++

Routine Description:

    This function dereferences a loaded DLL adjusting its reference
    count.  It then dereferences each dll referenced by this dll.

Arguments:

    LdrDataTableEntry - Supplies the address of the DLL to dereference

    IncrementCount - TRUE if adding one to LoadCount, O.W. subtracting one

Return Value:

    None.

--*/

{
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
    PIMAGE_BOUND_IMPORT_DESCRIPTOR NewImportDescriptor;
    PIMAGE_BOUND_FORWARDER_REF NewImportForwarder;
    PSZ ImportName, NewImportStringBase;
    ULONG i, ImportSize, NewImportSize;
    ANSI_STRING AnsiString;
    PUNICODE_STRING ImportDescriptorName_U;
    PLDR_DATA_TABLE_ENTRY Entry;
    NTSTATUS st;
    BOOLEAN Wx86KnownDll = FALSE;
    PIMAGE_THUNK_DATA FirstThunk;

    if (IncrementCount)
        if (LdrDataTableEntry->Flags & LDRP_LOAD_IN_PROGRESS) {
            return;
        } else {
            LdrDataTableEntry->Flags |= LDRP_LOAD_IN_PROGRESS;
        }
    else
        if (LdrDataTableEntry->Flags & LDRP_UNLOAD_IN_PROGRESS) {
            return;
        } else {
            LdrDataTableEntry->Flags |= LDRP_UNLOAD_IN_PROGRESS;
        }

    //
    // For each DLL used by this DLL, reference or dereference the DLL.
    //

    ImportDescriptorName_U = &NtCurrentTeb()->StaticUnicodeString;

#if defined (WX86)
    if (Wx86ProcessInit) {
        Wx86KnownDll = RtlImageNtHeader(LdrDataTableEntry->DllBase)->FileHeader.Machine
                           == IMAGE_FILE_MACHINE_I386;
        }
#endif




    //
    // See if there is a bound import table.  If so, walk that to
    // determine DLL names to reference or dereference.  Avoids touching
    // the .idata section
    //
    NewImportDescriptor = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(
                           LdrDataTableEntry->DllBase,
                           TRUE,
                           IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT,
                           &NewImportSize
                           );
    if (NewImportDescriptor) {
        if (IncrementCount) {
            LdrDataTableEntry->Flags |= LDRP_LOAD_IN_PROGRESS;
        } else {
            LdrDataTableEntry->Flags |= LDRP_UNLOAD_IN_PROGRESS;
        }

        NewImportStringBase = (LPSTR)NewImportDescriptor;
        while (NewImportDescriptor->OffsetModuleName) {
            ImportName = NewImportStringBase +
                         NewImportDescriptor->OffsetModuleName;
            RtlInitAnsiString(&AnsiString, ImportName);
            st = RtlAnsiStringToUnicodeString(ImportDescriptorName_U, &AnsiString, FALSE);
            if ( NT_SUCCESS(st) ) {
                if (LdrpCheckForLoadedDll( NULL,
                                           ImportDescriptorName_U,
                                           TRUE,
                                           Wx86KnownDll,
                                           &Entry
                                         )
                   ) {
                    if ( Entry->LoadCount != 0xffff ) {
                        if (IncrementCount) {
                            Entry->LoadCount++;

                            if (ShowSnaps) {
                                DbgPrint("LDR: Refcount   %wZ (%lx)\n",
                                        ImportDescriptorName_U,
                                        (ULONG)Entry->LoadCount
                                        );
                            }
                        } else {
                            Entry->LoadCount--;

                            if (ShowSnaps) {
                                DbgPrint("LDR: Derefcount   %wZ (%lx)\n",
                                        ImportDescriptorName_U,
                                        (ULONG)Entry->LoadCount
                                        );
                            }
                        }
                    }
                    LdrpUpdateLoadCount(Entry, IncrementCount);
                }
            }

            NewImportForwarder = (PIMAGE_BOUND_FORWARDER_REF)(NewImportDescriptor+1);
            for (i=0; i<NewImportDescriptor->NumberOfModuleForwarderRefs; i++) {
                ImportName = NewImportStringBase +
                             NewImportForwarder->OffsetModuleName;

                RtlInitAnsiString(&AnsiString, ImportName);
                st = RtlAnsiStringToUnicodeString(ImportDescriptorName_U, &AnsiString, FALSE);
                if ( NT_SUCCESS(st) ) {
                    if (LdrpCheckForLoadedDll( NULL,
                                               ImportDescriptorName_U,
                                               TRUE,
                                               Wx86KnownDll,
                                               &Entry
                                             )
                       ) {
                        if ( Entry->LoadCount != 0xffff ) {
                            if (IncrementCount) {
                                Entry->LoadCount++;

                                if (ShowSnaps) {
                                    DbgPrint("LDR: Refcount   %wZ (%lx)\n",
                                            ImportDescriptorName_U,
                                            (ULONG)Entry->LoadCount
                                            );
                                }
                            } else {
                                Entry->LoadCount--;

                                if (ShowSnaps) {
                                    DbgPrint("LDR: Derefcount   %wZ (%lx)\n",
                                            ImportDescriptorName_U,
                                            (ULONG)Entry->LoadCount
                                            );
                                }
                            }
                        }
                        LdrpUpdateLoadCount(Entry, IncrementCount);
                    }
                }

                NewImportForwarder += 1;
            }

            NewImportDescriptor = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)NewImportForwarder;
        }

        return;
    }

    ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(
                        LdrDataTableEntry->DllBase,
                        TRUE,
                        IMAGE_DIRECTORY_ENTRY_IMPORT,
                        &ImportSize
                        );
    if (ImportDescriptor) {

        while (ImportDescriptor->Name && ImportDescriptor->FirstThunk) {

            //
            // Match code in walk that skips references like this. IE3 had
            // some dll's with these bogus links to url.dll. On load, the url.dll
            // ref was skipped. On unload, it was not skipped because
            // this code was missing.
            //
            // Since the skip logic is only in the old style import
            // descriptor path, it is only duplicated here.
            //
            // check for import that has no references
            //
            FirstThunk = (PIMAGE_THUNK_DATA)((ULONG_PTR)LdrDataTableEntry->DllBase + ImportDescriptor->FirstThunk);
            if ( !FirstThunk->u1.Function ) {
                goto skipskippedimport;
                }

            ImportName = (PSZ)((ULONG_PTR)LdrDataTableEntry->DllBase + ImportDescriptor->Name);

            RtlInitAnsiString(&AnsiString, ImportName);
            st = RtlAnsiStringToUnicodeString(ImportDescriptorName_U, &AnsiString, FALSE);
            if ( NT_SUCCESS(st) ) {
                if (LdrpCheckForLoadedDll( NULL,
                                           ImportDescriptorName_U,
                                           TRUE,
                                           Wx86KnownDll,
                                           &Entry
                                         )
                   ) {
                    if ( Entry->LoadCount != 0xffff ) {
                        if (IncrementCount) {
                            Entry->LoadCount++;

                            if (ShowSnaps) {
                                DbgPrint("LDR: Refcount   %wZ (%lx)\n",
                                        ImportDescriptorName_U,
                                        (ULONG)Entry->LoadCount
                                        );
                            }
                        } else {
                            Entry->LoadCount--;

                            if (ShowSnaps) {
                                DbgPrint("LDR: Derefcount   %wZ (%lx)\n",
                                        ImportDescriptorName_U,
                                        (ULONG)Entry->LoadCount
                                        );
                            }
                        }
                    }
                    LdrpUpdateLoadCount(Entry, IncrementCount);
                }
            }
skipskippedimport:
            ++ImportDescriptor;
        }
    }
}

PLDR_DATA_TABLE_ENTRY
LdrpAllocateDataTableEntry (
    IN PVOID DllBase
    )

/*++

Routine Description:

    This function allocates an entry in the loader data table. If the
    table is going to overflow, then a new table is allocated.

Arguments:

    DllBase - Supplies the address of the base of the DLL Image.
        be added to the loader data table.

Return Value:

    Returns the address of the allocated loader data table entry

--*/

{
    PLDR_DATA_TABLE_ENTRY Entry;
    PIMAGE_NT_HEADERS NtHeaders;

    NtHeaders = RtlImageNtHeader(DllBase);

    Entry = NULL;
    if ( NtHeaders ) {
        Entry = RtlAllocateHeap(RtlProcessHeap(),MAKE_TAG( LDR_TAG ) | HEAP_ZERO_MEMORY,sizeof( *Entry ));
        if ( Entry ) {
            Entry->DllBase = DllBase;
            Entry->SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
            Entry->TimeDateStamp = NtHeaders->FileHeader.TimeDateStamp;
            }
        }
    return Entry;
}

VOID
LdrpInsertMemoryTableEntry (
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    )

/*++

Routine Description:

    This function inserts a loader data table entry into the
    list of loaded modules for this process. The insertion is
    done in "image memory base order".

Arguments:

    LdrDataTableEntry - Supplies the address of the loader data table
        entry to insert in the list of loaded modules for this process.

Return Value:

    None.

--*/

{
    PPEB_LDR_DATA Ldr;
    ULONG i;

    Ldr = NtCurrentPeb()->Ldr;

    i = LDRP_COMPUTE_HASH_INDEX(LdrDataTableEntry->BaseDllName.Buffer[0]);
    InsertTailList(&LdrpHashTable[i],&LdrDataTableEntry->HashLinks);
    InsertTailList(&Ldr->InLoadOrderModuleList, &LdrDataTableEntry->InLoadOrderLinks);
    InsertTailList(&Ldr->InMemoryOrderModuleList, &LdrDataTableEntry->InMemoryOrderLinks);
}

BOOLEAN
LdrpResolveDllName (
    IN PWSTR DllPath OPTIONAL,
    IN PWSTR DllName,
    OUT PUNICODE_STRING FullDllName,
    OUT PUNICODE_STRING BaseDllName,
    OUT PHANDLE DllFile
    )

/*++

Routine Description:

    This function computes the DLL pathname and base dll name (the
    unqualified, extensionless portion of the file name) for the specified
    DLL.

Arguments:

    DllPath - Supplies the DLL search path.

    DllName - Supplies the name of the DLL.

    FullDllName - Returns the fully qualified pathname of the
        DLL. The Buffer field of this string is dynamically
        allocated from the processes heap.

    BaseDLLName - Returns the base dll name of the dll.  The base name
        is the file name portion of the dll path without the trailing
        extension. The Buffer field of this string is dynamically
        allocated from the processes heap.

    DllFile - Returns an open handle to the DLL file. This parameter may
        still be NULL even upon success.

Return Value:

    TRUE - The operation was successful. A DLL file was found, and the
        FullDllName->Buffer & BaseDllName->Buffer field points to the
        base of process heap allocated memory.

    FALSE - The DLL could not be found.

--*/

{
    ULONG Length;
    PWCH p, pp;
    PWCH FullBuffer;

    *DllFile = NULL;
    FullDllName->Buffer = RtlAllocateHeap(RtlProcessHeap(),MAKE_TAG( TEMP_TAG ),530+sizeof(UNICODE_NULL));
    if (FullDllName->Buffer == NULL) {
        return FALSE;
        }

    Length = RtlDosSearchPath_U(
                ARGUMENT_PRESENT(DllPath) ? DllPath : LdrpDefaultPath.Buffer,
                DllName,
                NULL,
                530,
                FullDllName->Buffer,
                &BaseDllName->Buffer
                );

    if ( !Length || Length > 530 ) {

        if (ShowSnaps) {
            DbgPrint("LDR: LdrResolveDllName - Unable To Locate ");
            DbgPrint("%ws from %ws\n",
                DllName,
                ARGUMENT_PRESENT(DllPath) ? DllPath : LdrpDefaultPath.Buffer
                );
        }

        RtlFreeUnicodeString(FullDllName);
        return FALSE;
    }

    FullDllName->Length = (USHORT)Length;
    FullDllName->MaximumLength = FullDllName->Length + (USHORT)sizeof(UNICODE_NULL);
    FullBuffer = RtlAllocateHeap(RtlProcessHeap(),MAKE_TAG( LDR_TAG ),FullDllName->MaximumLength);
    if ( FullBuffer ) {
        RtlCopyMemory(FullBuffer,FullDllName->Buffer,FullDllName->MaximumLength);
        RtlFreeHeap(RtlProcessHeap(), 0, FullDllName->Buffer);
        FullDllName->Buffer = FullBuffer;
        }
    else {
        return FALSE;
        }
    //
    // Compute Length of base dll name
    //

    pp = UNICODE_NULL;
    p = FullDllName->Buffer;
    while (*p) {
        if (*p++ == (WCHAR)'\\') {
            pp = p;
        }
    }

    p = pp ? pp : DllName;
    pp = p;

    while (*p) {
        ++p;
    }

    BaseDllName->Length = (USHORT)((ULONG_PTR)p - (ULONG_PTR)pp);
    BaseDllName->MaximumLength = BaseDllName->Length + (USHORT)sizeof(UNICODE_NULL);
    BaseDllName->Buffer = RtlAllocateHeap(RtlProcessHeap(),MAKE_TAG( LDR_TAG ), BaseDllName->MaximumLength);
    if ( BaseDllName->Buffer ) {
        RtlMoveMemory(BaseDllName->Buffer,
                       pp,
                       BaseDllName->Length
                     );

        BaseDllName->Buffer[BaseDllName->Length >> 1] = UNICODE_NULL;
        }
    else {
        RtlFreeHeap(RtlProcessHeap(), 0, FullBuffer);
        return FALSE;
        }

    return TRUE;
}


PVOID
LdrpFetchAddressOfEntryPoint (
    IN PVOID Base
    )

/*++

Routine Description:

    This function returns the address of the initialization routine.

Arguments:

    Base - Base of image.

Return Value:

    Status value

--*/

{
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG_PTR ep;

    NtHeaders = RtlImageNtHeader(Base);
    ep = NtHeaders->OptionalHeader.AddressOfEntryPoint;
    if (ep) {
        ep += (ULONG_PTR)Base;
    }

    return (PVOID)ep;
}

HANDLE
LdrpCheckForKnownDll (
    IN PWSTR DllName,
    OUT PUNICODE_STRING FullDllName,
    OUT PUNICODE_STRING BaseDllName
    )

/*++

Routine Description:

    This function checks to see if the specified DLL is a known DLL.
    It assumes it is only called for static DLL's, and when
    the know DLL directory structure has been set up.

Arguments:

    DllName - Supplies the name of the DLL.

    FullDllName - Returns the fully qualified pathname of the
        DLL. The Buffer field of this string is dynamically
        allocated from the processes heap.

    BaseDLLName - Returns the base dll name of the dll.  The base name
        is the file name portion of the dll path without the trailing
        extension. The Buffer field of this string is dynamically
        allocated from the processes heap.

Return Value:

    NON-NULL - Returns an open handle to the section associated with
        the DLL.

    NULL - The DLL is not known.

--*/

{

    UNICODE_STRING Unicode;
    HANDLE Section;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    PSZ p;
    PWSTR pw;

    Section = NULL;

    //
    // calculate base name
    //

    RtlInitUnicodeString(&Unicode,DllName);


    BaseDllName->Length = Unicode.Length;
    BaseDllName->MaximumLength = Unicode.MaximumLength;
    BaseDllName->Buffer = RtlAllocateHeap(
                            RtlProcessHeap(),MAKE_TAG( LDR_TAG ),
                            Unicode.MaximumLength
                            );
    if ( !BaseDllName->Buffer ) {
        return NULL;
        }

    RtlMoveMemory(BaseDllName->Buffer,Unicode.Buffer,Unicode.MaximumLength);

    //
    // now compute the full name for the dll
    //

    FullDllName->Length = (USHORT)(LdrpKnownDllPath.Length +  // path prefix
                                   (USHORT)sizeof(WCHAR)   +  // seperator
                                   BaseDllName->Length        // base
                                  );

    FullDllName->MaximumLength = FullDllName->Length + (USHORT)sizeof(UNICODE_NULL);
    FullDllName->Buffer = RtlAllocateHeap(
                            RtlProcessHeap(),MAKE_TAG( LDR_TAG ),
                            FullDllName->MaximumLength
                            );
    if ( !FullDllName->Buffer ) {
        RtlFreeHeap(RtlProcessHeap(),0,BaseDllName->Buffer);
        return NULL;
        }

    p = (PSZ)FullDllName->Buffer;
    RtlMoveMemory(p,LdrpKnownDllPath.Buffer,LdrpKnownDllPath.Length);
    p += LdrpKnownDllPath.Length;
    pw = (PWSTR)p;
    *pw++ = (WCHAR)'\\';
    p = (PSZ)pw;

    //
    // This is the relative name of the section
    //

    Unicode.Buffer = (PWSTR)p;
    Unicode.Length = BaseDllName->Length;       // base
    Unicode.MaximumLength = Unicode.Length + (USHORT)sizeof(UNICODE_NULL);

    RtlMoveMemory(p,BaseDllName->Buffer,BaseDllName->MaximumLength);

    //
    // open the section object
    //

    InitializeObjectAttributes(
        &Obja,
        &Unicode,
        OBJ_CASE_INSENSITIVE,
        LdrpKnownDllObjectDirectory,
        NULL
        );

    Status = NtOpenSection(
            &Section,
            SECTION_MAP_READ | SECTION_MAP_EXECUTE | SECTION_MAP_WRITE,
            &Obja
            );

    if ( !NT_SUCCESS(Status) ) {
        Section = NULL;
        RtlFreeHeap(RtlProcessHeap(),0,BaseDllName->Buffer);
        RtlFreeHeap(RtlProcessHeap(),0,FullDllName->Buffer);
        }
#if DBG
    else {
        LdrpSectionOpens++;
        }
#endif // DBG
    return Section;
}

NTSTATUS
LdrpSetProtection (
    IN PVOID Base,
    IN BOOLEAN Reset,
    IN BOOLEAN StaticLink
    )

/*++

Routine Description:

    This function loops thru the images sections/objects, setting
    all sections/objects marked r/o to r/w. It also resets the
    original section/object protections.

Arguments:

    Base - Base of image.

    Reset - If TRUE, reset section/object protection to original
            protection described by the section/object headers.
            If FALSE, then set all sections/objects to r/w.

    StaticLink - TRUE if this is a static link.

Return Value:

    SUCCESS or reason NtProtectVirtualMemory failed.

--*/

{
    HANDLE CurrentProcessHandle;
    SIZE_T RegionSize;
    ULONG NewProtect, OldProtect;
    PVOID VirtualAddress;
    ULONG i;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER SectionHeader;
    NTSTATUS st;

    CurrentProcessHandle = NtCurrentProcess();

    NtHeaders = RtlImageNtHeader(Base);

#if defined (_ALPHA_)
    if (NtHeaders->OptionalHeader.SectionAlignment < PAGE_SIZE) {
        //
        // if SectionAlignment < PAGE_SIZE the entire image is
        // exec-copy on write, so we have nothing to do.
        //
        return STATUS_SUCCESS;
        }
#endif

    SectionHeader = (PIMAGE_SECTION_HEADER)((ULONG_PTR)NtHeaders + sizeof(ULONG) +
                        sizeof(IMAGE_FILE_HEADER) +
                        NtHeaders->FileHeader.SizeOfOptionalHeader
                        );

    for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++) {
        if (!(SectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE) &&
             (SectionHeader->SizeOfRawData))
         {
            //
            // Object isn't writeable and has a non-zero on disk size, change it.
            //
            if (Reset) {
                if (SectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) {
                    NewProtect = PAGE_EXECUTE;
                } else {
                         NewProtect = PAGE_READONLY;
                }
                NewProtect |= (SectionHeader->Characteristics & IMAGE_SCN_MEM_NOT_CACHED) ? PAGE_NOCACHE : 0;
            } else {
                NewProtect = PAGE_READWRITE;
            }
            VirtualAddress = (PVOID)((ULONG_PTR)Base + SectionHeader->VirtualAddress);
            RegionSize = SectionHeader->SizeOfRawData;

            if (RegionSize != 0) {
                st = NtProtectVirtualMemory(CurrentProcessHandle, &VirtualAddress,
                              &RegionSize, NewProtect, &OldProtect);

                if (!NT_SUCCESS(st)) {
                    return st;
                }
            }

        }
        ++SectionHeader;
    }

    if (Reset) {
        NtFlushInstructionCache(NtCurrentProcess(), NULL, 0);
    }
    return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////////////
////////////////////////////////////////// Page heap target dll logic
/////////////////////////////////////////////////////////////////////

BOOLEAN LdrpDphKernel32Snapped;
BOOLEAN LdrpDphMsvcrtSnapped;

#define SNAP_ROUTINE_GLOBALALLOC     0
#define SNAP_ROUTINE_GLOBALREALLOC   1
#define SNAP_ROUTINE_GLOBALFREE      2
#define SNAP_ROUTINE_LOCALALLOC      3
#define SNAP_ROUTINE_LOCALREALLOC    4
#define SNAP_ROUTINE_LOCALFREE       5
#define SNAP_ROUTINE_HEAPALLOC       6
#define SNAP_ROUTINE_HEAPREALLOC     7
#define SNAP_ROUTINE_HEAPFREE        8
#define SNAP_ROUTINE_HEAPCREATE      9
#define SNAP_ROUTINE_MALLOC          10
#define SNAP_ROUTINE_CALLOC          11
#define SNAP_ROUTINE_REALLOC         12
#define SNAP_ROUTINE_FREE            13
#define SNAP_ROUTINE_NEW             14
#define SNAP_ROUTINE_DELETE          15
#define SNAP_ROUTINE_NEW_ARRAY       16
#define SNAP_ROUTINE_DELETE_ARRAY    17
#define SNAP_ROUTINE_MAX_INDEX       18

PVOID LdrpDphSnapRoutines [SNAP_ROUTINE_MAX_INDEX];

typedef struct _DPH_SNAP_NAME {

    PSTR Name;
    ULONG Index;

} DPH_SNAP_NAME, * PDPH_SNAP_NAME;

DPH_SNAP_NAME 
LdrpDphSnapNamesForKernel32 [] = {

    { "GlobalAlloc",   0 },
    { "GlobalReAlloc", 1 },
    { "GlobalFree",    2 },
    { "LocalAlloc",    3 },
    { "LocalReAlloc",  4 },
    { "LocalFree",     5 },
    { "HeapAlloc",     6 },
    { "HeapReAlloc",   7 },
    { "HeapFree",      8 },
    { "HeapCreate",    9 }, 
    { NULL, 0 }
};

DPH_SNAP_NAME 
LdrpDphSnapNamesForMsvcrt [] = {

    { "malloc",        10},
    { "calloc",        11},
    { "realloc",       12},
    { "free",          13},
    { "??2@YAPAXI@Z",  14}, // operator new
    { "??3@YAXPAX@Z",  15}, // operator delete
    { "??_U@YAPAXI@Z", 16}, // operator new[]
    { "??_V@YAXPAX@Z", 17}, // operator delete[]
    { NULL, 0 }
};

//
// Declarations for replacement functions
//

PVOID
RtlpDphDllHeapAlloc (
    IN PVOID  HeapHandle,
    IN ULONG  Flags,
    IN SIZE_T Size
    );

PVOID
RtlpDphDllHeapReAlloc (
    IN PVOID  HeapHandle,
    IN ULONG  Flags,
    IN PVOID Address,
    IN SIZE_T Size
    );

BOOLEAN
RtlpDphDllHeapFree(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address
    );

PVOID
RtlpDphDllLocalAlloc (
    IN ULONG  Flags,
    IN SIZE_T Size
    );

PVOID
RtlpDphDllLocalReAlloc (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG  Flags
    );

PVOID
RtlpDphDllLocalFree(
    IN PVOID Address
    );

PVOID
RtlpDphDllGlobalAlloc (
    IN ULONG  Flags,
    IN SIZE_T Size
    );

PVOID
RtlpDphDllGlobalReAlloc (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG  Flags
    );

PVOID
RtlpDphDllGlobalFree(
    IN PVOID Address
    );

PVOID __cdecl
RtlpDphDllmalloc (
    IN SIZE_T Size
    );

PVOID __cdecl
RtlpDphDllcalloc (
    IN SIZE_T Number,
    IN SIZE_T Size
    );

PVOID __cdecl
RtlpDphDllrealloc (
    IN PVOID Address,
    IN SIZE_T Size
    );

VOID __cdecl
RtlpDphDllfree (
    IN PVOID Address
    );

PVOID __cdecl
RtlpDphDllNew (
    IN SIZE_T Size
    );

VOID __cdecl
RtlpDphDllDelete (
    IN PVOID Address
    );

PVOID __cdecl
RtlpDphDllNewArray (
    IN SIZE_T Size
    );

VOID __cdecl
RtlpDphDllDeleteArray (
    IN PVOID Address
    );

//
// Replacement function for msvcrt HeapCreate used to intercept
// the CRT heap creation.
//

PVOID
RtlpDphDllHeapCreate (
    ULONG Options,
    SIZE_T InitialSize,
    SIZE_T MaximumSize
    );

//
// Address of heap created by msvcrt. This is needed
// by the replacements of malloc/free etc.
//

PVOID RtlpDphMsvcrtHeap;

//
// Snap implementation
//

BOOLEAN
LdrpDphDetectSnapRoutines (
    PWSTR DllString,
    PDPH_SNAP_NAME SnapNames
    )
{
    PLDR_DATA_TABLE_ENTRY DllData;
    PIMAGE_EXPORT_DIRECTORY Directory;
    ULONG Size;
    PCHAR NameAddress;
    PCHAR FunctionAddress;
    PCHAR Base;
    PCHAR IndexAddress;
    ULONG Index;
    ULONG RealIndex;
    BOOLEAN Result;
    UNICODE_STRING DllName;
    PDPH_SNAP_NAME CurrentSnapName;

    RtlInitUnicodeString (
        &DllName, 
        DllString);

    Result = LdrpCheckForLoadedDll (
        NULL,
        &DllName,
        TRUE,
        FALSE,
        &DllData);

    if (Result == FALSE) {
        return FALSE;
    }

    Base = DllData->DllBase;

    Directory = RtlImageDirectoryEntryToData (
        DllData->DllBase,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_EXPORT,    
        &Size
        );

    if (Directory == NULL) {
        return FALSE;
    }

    for (CurrentSnapName = SnapNames; CurrentSnapName->Name; CurrentSnapName += 1) {

        for (Index = 0; Index < Directory->NumberOfFunctions; Index += 1) {

            NameAddress = Base + Directory->AddressOfNames;
            NameAddress = Base + ((ULONG *)NameAddress)[Index];

            IndexAddress = Base + Directory->AddressOfNameOrdinals;
            RealIndex = (ULONG)(((USHORT *)IndexAddress)[Index]);

            if (_stricmp (NameAddress, CurrentSnapName->Name) == 0) {

                FunctionAddress = Base + Directory->AddressOfFunctions;
                FunctionAddress = Base + ((ULONG *)FunctionAddress)[RealIndex];

                LdrpDphSnapRoutines[CurrentSnapName->Index] = FunctionAddress;
                DbgPrint ("Page heap: found %s @ address %p \n", NameAddress, FunctionAddress);
            }
        }
    }

    return TRUE;
}

BOOLEAN
LdrpDphSnapImports (
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry,
    BOOLEAN CallToDetectCrtHeap
    )
{
    PVOID IATBase;
    SIZE_T BigIATSize;
    ULONG  LittleIATSize;
    PVOID *ProcAddresses;
    ULONG NumberOfProcAddresses;
    ULONG OldProtect;
    USHORT TagIndex;
    NTSTATUS st;

    //
    // Determine the location and size of the IAT.  If found, scan the
    // IAT address to see if any are pointing to alloc/free functions
    // and replace those thunks.
    //

    IATBase = RtlImageDirectoryEntryToData(
        LdrDataTableEntry->DllBase,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_IAT,
        &LittleIATSize);

    BigIATSize = LittleIATSize;

    if (IATBase != NULL) {
        st = NtProtectVirtualMemory(
            NtCurrentProcess(),
            &IATBase,
            &BigIATSize,
            PAGE_READWRITE,
            &OldProtect);

        if (!NT_SUCCESS(st)) {
            DbgPrint( "LDR: Unable to unprotect IAT to enable per DLL page heap.\n" );
            return FALSE;
        }
        else {
            ProcAddresses = (PVOID *)IATBase;
            NumberOfProcAddresses = (ULONG)(BigIATSize / sizeof(PVOID));
            while (NumberOfProcAddresses--) {

                if (CallToDetectCrtHeap) {
                    if (*ProcAddresses == LdrpDphSnapRoutines[SNAP_ROUTINE_HEAPCREATE]) {
                        *ProcAddresses = RtlpDphDllHeapCreate;
                        DbgPrint ("Snapped (%ws) HeapCreate ... \n",
                            LdrDataTableEntry->BaseDllName.Buffer);
                    }
                }
                else {

                    //
                    // ntdll imports
                    //

                    if (*ProcAddresses == RtlAllocateHeap) {
                        *ProcAddresses = RtlpDphDllHeapAlloc;
                        DbgPrint ("Snapped (%ws) RtlAllocateHeap ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }
                    else if (*ProcAddresses == RtlReAllocateHeap) {
                        *ProcAddresses = RtlpDphDllHeapReAlloc;
                        DbgPrint ("Snapped (%ws) RtlReAllocateHeap ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }
                    else if (*ProcAddresses == RtlFreeHeap) {
                        *ProcAddresses = RtlpDphDllHeapFree;
                        DbgPrint ("Snapped (%ws) RtlFreeHeap ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }

                    //
                    // kernel32 imports
                    //

                    else if (*ProcAddresses == LdrpDphSnapRoutines[SNAP_ROUTINE_HEAPALLOC]) {
                        *ProcAddresses = RtlpDphDllHeapAlloc;
                        DbgPrint ("Snapped (%ws) HeapAlloc ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }
                    else if (*ProcAddresses == LdrpDphSnapRoutines[SNAP_ROUTINE_HEAPREALLOC]) {
                        *ProcAddresses = RtlpDphDllHeapReAlloc;
                        DbgPrint ("Snapped (%ws) HeapReAlloc ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }
                    else if (*ProcAddresses == LdrpDphSnapRoutines[SNAP_ROUTINE_HEAPFREE]) {
                        *ProcAddresses = RtlpDphDllHeapFree;
                        DbgPrint ("Snapped (%ws) HeapFree ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }

                    else if (*ProcAddresses == LdrpDphSnapRoutines[SNAP_ROUTINE_LOCALALLOC]) {
                        *ProcAddresses = RtlpDphDllLocalAlloc;
                        DbgPrint ("Snapped (%ws) LocalAlloc ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }
                    else if (*ProcAddresses == LdrpDphSnapRoutines[SNAP_ROUTINE_LOCALREALLOC]) {
                        *ProcAddresses = RtlpDphDllLocalReAlloc;
                        DbgPrint ("Snapped (%ws) LocalReAlloc ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }
                    else if (*ProcAddresses == LdrpDphSnapRoutines[SNAP_ROUTINE_LOCALFREE]) {
                        *ProcAddresses = RtlpDphDllLocalFree;
                        DbgPrint ("Snapped (%ws) LocalFree ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }

                    else if (*ProcAddresses == LdrpDphSnapRoutines[SNAP_ROUTINE_GLOBALALLOC]) {
                        *ProcAddresses = RtlpDphDllGlobalAlloc;
                        DbgPrint ("Snapped (%ws) GlobalAlloc ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }
                    else if (*ProcAddresses == LdrpDphSnapRoutines[SNAP_ROUTINE_GLOBALREALLOC]) {
                        *ProcAddresses = RtlpDphDllGlobalReAlloc;
                        DbgPrint ("Snapped (%ws) GlobalReAlloc ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }
                    else if (*ProcAddresses == LdrpDphSnapRoutines[SNAP_ROUTINE_GLOBALFREE]) {
                        *ProcAddresses = RtlpDphDllGlobalFree;
                        DbgPrint ("Snapped (%ws) GlobalFree ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }

                    //
                    // msvcrt imports
                    //

                    else if (*ProcAddresses == LdrpDphSnapRoutines[SNAP_ROUTINE_MALLOC]) {
                        *ProcAddresses = RtlpDphDllmalloc;
                        DbgPrint ("Snapped (%ws) malloc ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }
                    else if (*ProcAddresses == LdrpDphSnapRoutines[SNAP_ROUTINE_REALLOC]) {
                        *ProcAddresses = RtlpDphDllrealloc;
                        DbgPrint ("Snapped (%ws) realloc ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }
                    else if (*ProcAddresses == LdrpDphSnapRoutines[SNAP_ROUTINE_CALLOC]) {
                        *ProcAddresses = RtlpDphDllcalloc;
                        DbgPrint ("Snapped (%ws) calloc ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }
                    else if (*ProcAddresses == LdrpDphSnapRoutines[SNAP_ROUTINE_FREE]) {
                        *ProcAddresses = RtlpDphDllfree;
                        DbgPrint ("Snapped (%ws) free ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }

                    else if (*ProcAddresses == LdrpDphSnapRoutines[SNAP_ROUTINE_NEW]) {
                        *ProcAddresses = RtlpDphDllNew;
                        DbgPrint ("Snapped (%ws) operator new ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }
                    else if (*ProcAddresses == LdrpDphSnapRoutines[SNAP_ROUTINE_DELETE]) {
                        *ProcAddresses = RtlpDphDllDelete;
                        DbgPrint ("Snapped (%ws) operator delete ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }
                    else if (*ProcAddresses == LdrpDphSnapRoutines[SNAP_ROUTINE_NEW_ARRAY]) {
                        *ProcAddresses = RtlpDphDllNewArray;
                        DbgPrint ("Snapped (%ws) operator new[] ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }
                    else if (*ProcAddresses == LdrpDphSnapRoutines[SNAP_ROUTINE_DELETE_ARRAY]) {
                        *ProcAddresses = RtlpDphDllDeleteArray;
                        DbgPrint ("Snapped (%ws) operator delete[] ... \n",
                                  LdrDataTableEntry->BaseDllName.Buffer);
                    }
                }
                
                ProcAddresses += 1;
            }

            NtProtectVirtualMemory(
                NtCurrentProcess(),
                &IATBase,
                &BigIATSize,
                OldProtect,
                &OldProtect);
        }
    }

    return TRUE;
}

VOID
LdrpDphInitializeTargetDll (
    PLDR_DATA_TABLE_ENTRY LoadedDllData
    )
{
    BOOLEAN Kernel32JustSnapped = FALSE;
    BOOLEAN MsvcrtJustSnapped = FALSE;

    //
    // If we do not have per dll page heap feature enabled
    // we return immediately.
    //

    if (! (RtlpDphGlobalFlags & PAGE_HEAP_USE_DLL_NAMES)) {
        return;
    }

    if (! LdrpDphKernel32Snapped) {

        Kernel32JustSnapped = LdrpDphDetectSnapRoutines (
            L"kernel32.dll",
            LdrpDphSnapNamesForKernel32);

        LdrpDphKernel32Snapped = Kernel32JustSnapped;
    }

    if (! LdrpDphMsvcrtSnapped) {

        MsvcrtJustSnapped = LdrpDphDetectSnapRoutines (
            L"msvcrt.dll",
            LdrpDphSnapNamesForMsvcrt);

        LdrpDphMsvcrtSnapped = MsvcrtJustSnapped;
    }

    //
    // Snap everything already loaded if we just managed
    // to detect snap routines.
    //
    
    if (Kernel32JustSnapped || MsvcrtJustSnapped) {
        
        PWSTR Current;
        PWSTR End;
        WCHAR SavedChar;
        PLDR_DATA_TABLE_ENTRY DllData;
        BOOLEAN Result;
        UNICODE_STRING DllName;

        Current = RtlpDphTargetDlls;
        
        while (*Current) {

            while (*Current == L' ') {
                Current += 1;
            }

            End = Current;

            while (*End && *End != L' ') {
                End += 1;
            }

            if (*Current == L'\0') {
                break;
            }

            SavedChar = *End;
            *End = L'\0';

            RtlInitUnicodeString (
                &DllName, 
                Current);

            Result = LdrpCheckForLoadedDll (
                NULL,
                &DllName,
                TRUE,
                FALSE,
                &DllData);

            if (Result) {

                if (DllData->DllBase == LoadedDllData->DllBase) {
#if DBG
                    DbgPrint ("Page heap: oversnapping %ws \n", DllData->BaseDllName);
#endif
                }

                LdrpDphSnapImports (DllData, FALSE);
            }

            *End = SavedChar;
            Current = End;
        }
    }

    //
    // If we just loaded msvcrt.dll we need to redirect HeapCreate call
    // in order to detect when the CRT heap gets created.
    //

    if (_wcsicmp (LoadedDllData->BaseDllName.Buffer, L"msvcrt.dll") == 0) {

        LdrpDphSnapImports (LoadedDllData, TRUE);
    }

    //
    // Call back into page heap manager to figure out if the
    // currently loaded dll is a target for page heap.
    //
    
    if (RtlpDphIsDllTargeted (LoadedDllData->BaseDllName.Buffer)) {
        
        LdrpDphSnapImports (LoadedDllData, FALSE);
    }
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// Snap routines
/////////////////////////////////////////////////////////////////////

//
// A biased heap pointer signals to the page heap manager that
// this allocation needs to get into page heap (not normal heap).
// This needs to happen only for allocation function (not free, delete).
//

#define BIAS_POINTER(p) ((PVOID)((ULONG_PTR)(p) | 0x01))

PVOID
RtlpDphDllHeapAlloc (
    IN PVOID  HeapHandle,
    IN ULONG  Flags,
    IN SIZE_T Size
    )
{
    return RtlpDebugPageHeapAllocate (
        BIAS_POINTER(HeapHandle),
        Flags,
        Size);
}

PVOID
RtlpDphDllHeapReAlloc (
    IN PVOID  HeapHandle,
    IN ULONG  Flags,
    IN PVOID Address,
    IN SIZE_T Size
    )
{
    return RtlpDebugPageHeapReAllocate (
        BIAS_POINTER(HeapHandle),
        Flags,
        Address,
        Size);
}

BOOLEAN
RtlpDphDllHeapFree(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address
    )
{
    return RtlpDebugPageHeapFree (
        HeapHandle,
        Flags,
        Address);
}

//
// LocalAlloc, LocalReAlloc, LocalFree
// GlobalAlloc, GlobalReAlloc, GlobalFree
//
// The following macros are copied from sdk\inc\winbase.h
// There is very low probability that anybody will ever change
// these values for application compatibility reasons.
// 

#define LMEM_MOVEABLE       0x0002
#define LMEM_ZEROINIT       0x0040

#if defined(_IA64_) || defined(_AXP64_)
#define BASE_HANDLE_MARK_BIT 0x08
#else
#define BASE_HANDLE_MARK_BIT 0x04
#endif

typedef PVOID
(* FUN_LOCAL_ALLOC) (
    IN ULONG  Flags,
    IN SIZE_T Size
    );

typedef PVOID
(* FUN_LOCAL_REALLOC) (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG  Flags
    );

typedef PVOID
(* FUN_LOCAL_FREE)(
    IN PVOID Address
    );

typedef PVOID
(* FUN_GLOBAL_ALLOC) (
    IN ULONG  Flags,
    IN SIZE_T Size
    );

typedef PVOID
(* FUN_GLOBAL_REALLOC) (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG  Flags
    );

typedef PVOID
(* FUN_GLOBAL_FREE)(
    IN PVOID Address
    );

PVOID
RtlpDphDllLocalAlloc (
    IN ULONG  Flags,
    IN SIZE_T Size
    )
{
    PVOID Block;
    FUN_LOCAL_ALLOC Original;

    if (!(Flags & LMEM_MOVEABLE)) {
        
        Block = RtlpDebugPageHeapAllocate (
            BIAS_POINTER(RtlProcessHeap()),
            0,
            Size);

        if ((Flags & LMEM_ZEROINIT)) {
            RtlZeroMemory (Block, Size);
        }
        
        return Block;
    }
    else {

        Original = (FUN_LOCAL_ALLOC)(LdrpDphSnapRoutines[SNAP_ROUTINE_LOCALALLOC]);
        return (* Original) (Flags, Size);
    }
}

PVOID
RtlpDphDllLocalReAlloc (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG  Flags
    )
{
    PVOID Block;
    FUN_LOCAL_REALLOC Original;

    if (!(Flags & LMEM_MOVEABLE)) {
        
        Block = RtlpDebugPageHeapReAllocate (
            BIAS_POINTER(RtlProcessHeap()),
            0,
            Address,
            Size);

        return Block;
    }
    else {

        Original = (FUN_LOCAL_REALLOC)(LdrpDphSnapRoutines[SNAP_ROUTINE_LOCALREALLOC]);
        return (* Original) (Address, Size, Flags);
    }
}

PVOID
RtlpDphDllLocalFree(
    IN PVOID Address
    )
{
    BOOLEAN Result;
    FUN_LOCAL_FREE Original;

    if ((ULONG_PTR)Address & BASE_HANDLE_MARK_BIT) {
        
        Original = (FUN_LOCAL_FREE)(LdrpDphSnapRoutines[SNAP_ROUTINE_LOCALFREE]);
        return (* Original) (Address);
    }
    else {

        Result = RtlpDebugPageHeapFree (
            RtlProcessHeap(),
            0,
            Address);

        if (Result) {
            return NULL;
        }
        else {
            return Address;
        }
    }
}

PVOID
RtlpDphDllGlobalAlloc (
    IN ULONG  Flags,
    IN SIZE_T Size
    )
{
    PVOID Block;
    FUN_GLOBAL_ALLOC Original;

    if (!(Flags & LMEM_MOVEABLE)) {
        
        Block = RtlpDebugPageHeapAllocate (
            BIAS_POINTER(RtlProcessHeap()),
            0,
            Size);

        if ((Flags & LMEM_ZEROINIT)) {
            RtlZeroMemory (Block, Size);
        }
        
        return Block;
    }
    else {

        Original = (FUN_GLOBAL_ALLOC)(LdrpDphSnapRoutines[SNAP_ROUTINE_GLOBALALLOC]);
        return (* Original) (Flags, Size);
    }
}

PVOID
RtlpDphDllGlobalReAlloc (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG  Flags
    )
{
    PVOID Block;
    FUN_GLOBAL_REALLOC Original;

    if (!(Flags & LMEM_MOVEABLE)) {
        
        Block = RtlpDebugPageHeapReAllocate (
            BIAS_POINTER(RtlProcessHeap()),
            0,
            Address,
            Size);

        return Block;
    }
    else {

        Original = (FUN_GLOBAL_REALLOC)(LdrpDphSnapRoutines[SNAP_ROUTINE_GLOBALREALLOC]);
        return (* Original) (Address, Size, Flags);
    }
}

PVOID
RtlpDphDllGlobalFree(
    IN PVOID Address
    )
{
    BOOLEAN Result;
    FUN_GLOBAL_FREE Original;

    if ((ULONG_PTR)Address & BASE_HANDLE_MARK_BIT) {
        
        Original = (FUN_GLOBAL_FREE)(LdrpDphSnapRoutines[SNAP_ROUTINE_GLOBALFREE]);
        return (* Original) (Address);
    }
    else {

        Result = RtlpDebugPageHeapFree (
            RtlProcessHeap(),
            0,
            Address);

        if (Result) {
            return NULL;
        }
        else {
            return Address;
        }
    }
}

//
// malloc, calloc, realloc, free
//

PVOID __cdecl
RtlpDphDllmalloc (
    IN SIZE_T Size
    )
{
    PVOID Block;

    ASSERT(RtlpDphMsvcrtHeap != NULL);

    Block = RtlpDebugPageHeapAllocate (
        BIAS_POINTER(RtlpDphMsvcrtHeap),
        0,
        Size);

    return Block;
}

PVOID __cdecl
RtlpDphDllcalloc (
    IN SIZE_T Number,
    IN SIZE_T Size
    )
{                    
    PVOID Block;

    ASSERT(RtlpDphMsvcrtHeap != NULL);

    Block =  RtlpDebugPageHeapAllocate (
        BIAS_POINTER(RtlpDphMsvcrtHeap),
        0,
        Size * Number);

    RtlZeroMemory (Block, Size * Number);
    return Block;
}

PVOID __cdecl
RtlpDphDllrealloc (
    IN PVOID Address,
    IN SIZE_T Size
    )
{
    PVOID Block;

    ASSERT(RtlpDphMsvcrtHeap != NULL);

    if (Address == NULL) {
        
        Block = RtlpDebugPageHeapAllocate (
            BIAS_POINTER(RtlpDphMsvcrtHeap),
            0,
            Size);
    }
    else {

        Block = RtlpDebugPageHeapReAllocate (
            BIAS_POINTER(RtlpDphMsvcrtHeap),
            0,
            Address,
            Size);
    }

    return Block;
}

VOID __cdecl
RtlpDphDllfree (
    IN PVOID Address
    )
{                                    
    ASSERT(RtlpDphMsvcrtHeap != NULL);

    RtlpDebugPageHeapFree (
        RtlpDphMsvcrtHeap,
        0,
        Address);
}

//
// operator new, delete
// operator new[], delete[]
//

PVOID __cdecl
RtlpDphDllNew (
    IN SIZE_T Size
    )
{                              
    PVOID Block;

    ASSERT(RtlpDphMsvcrtHeap != NULL);

    Block = RtlpDebugPageHeapAllocate (
        BIAS_POINTER(RtlpDphMsvcrtHeap),
        0,
        Size);

    return Block;
}

VOID __cdecl
RtlpDphDllDelete (
    IN PVOID Address
    )
{                                    
    ASSERT(RtlpDphMsvcrtHeap != NULL);

    RtlpDebugPageHeapFree (
        RtlpDphMsvcrtHeap,
        0,
        Address);
}

PVOID __cdecl
RtlpDphDllNewArray (
    IN SIZE_T Size
    )
{                                    
    ASSERT(RtlpDphMsvcrtHeap != NULL);

    return RtlpDebugPageHeapAllocate (
        BIAS_POINTER(RtlpDphMsvcrtHeap),
        0,
        Size);
}

VOID __cdecl
RtlpDphDllDeleteArray (
    IN PVOID Address
    )
{                                    
    ASSERT(RtlpDphMsvcrtHeap != NULL);

    RtlpDebugPageHeapFree (
        RtlpDphMsvcrtHeap,
        0,
        Address);
}

//
// HeapCreate 
//

typedef PVOID
(* FUN_HEAP_CREATE) (
    ULONG Options,
    SIZE_T InitialSize,
    SIZE_T MaximumSize
    );

PVOID
RtlpDphDllHeapCreate (
    ULONG Options,
    SIZE_T InitialSize,
    SIZE_T MaximumSize
    )
{
    PVOID Heap;
    FUN_HEAP_CREATE Original;

    Original = (FUN_HEAP_CREATE)(LdrpDphSnapRoutines[SNAP_ROUTINE_HEAPCREATE]);
    Heap = (* Original) (Options, InitialSize, MaximumSize);

    RtlpDphMsvcrtHeap = Heap;
    DbgPrint ("Page heap: detected CRT heap @ %p \n", RtlpDphMsvcrtHeap);

    return Heap;
}


