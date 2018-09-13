/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ldrinit.c

Abstract:

    This module implements loader initialization.

Author:

    Mike O'Leary (mikeol) 26-Mar-1990

Revision History:

--*/

#include <ntos.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <heap.h>
#include <apcompat.h>
#include "ldrp.h"
#include <ctype.h>


BOOLEAN LdrpShutdownInProgress = FALSE;
BOOLEAN LdrpImageHasTls = FALSE;
BOOLEAN LdrpVerifyDlls = FALSE;
BOOLEAN LdrpLdrDatabaseIsSetup = FALSE;
BOOLEAN LdrpInLdrInit = FALSE;

#if defined(_WIN64)
PVOID Wow64Handle;
ULONG UseWOW64;
typedef VOID (*tWOW64LdrpInitialize)(IN PCONTEXT Context);
tWOW64LdrpInitialize Wow64LdrpInitialize;
PVOID Wow64PrepareForException;
#endif

PVOID NtDllBase;

extern ULONG RtlpDisableHeapLookaside;  // defined in rtl\heap.c

#if defined(_ALPHA_)
ULONG_PTR LdrpGpValue;
#endif // ALPHA

#if defined (_X86_)
void
LdrpValidateImageForMp(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    );
#endif

#if defined (_ALPHA_)
VOID
AlphaFindArchitectureFixups(
    PIMAGE_NT_HEADERS NtHeaders,
    PVOID ViewBase,
    BOOLEAN StaticLink
    );
#endif

VOID
LdrpRelocateStartContext (
    IN PCONTEXT Context,
    IN LONG_PTR Diff
    );

NTSTATUS
LdrpForkProcess( VOID );

VOID
LdrpInitializeThread(
    IN PCONTEXT Context
    );

BOOLEAN
NtdllOkayToLockRoutine(
    IN PVOID Lock
    );

VOID
RtlpInitDeferedCriticalSection( VOID );

VOID
LdrQueryApplicationCompatibilityGoo(
    IN PUNICODE_STRING UnicodeImageName
    );

NTSTATUS
LdrFindAppCompatVariableInfo(
    IN  ULONG dwTypeSeeking,
    OUT PAPP_VARIABLE_INFO *AppVariableInfo
    );

NTSTATUS
LdrpSearchResourceSection_U(
    IN PVOID DllHandle,
    IN PULONG_PTR ResourceIdPath,
    IN ULONG ResourceIdPathLength,
    IN BOOLEAN FindDirectoryEntry,
    IN BOOLEAN ExactLangMatchOnly,
    OUT PVOID *ResourceDirectoryOrData
    );

NTSTATUS
LdrpAccessResourceData(
    IN PVOID DllHandle,
    IN PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
    OUT PVOID *Address OPTIONAL,
    OUT PULONG Size OPTIONAL
    );

PVOID
NtdllpAllocateStringRoutine(
    SIZE_T NumberOfBytes
    )
{
    return RtlAllocateHeap(RtlProcessHeap(), 0, NumberOfBytes);
}

VOID
NtdllpFreeStringRoutine(
    PVOID Buffer
    )
{
    RtlFreeHeap(RtlProcessHeap(), 0, Buffer);
}

PRTL_ALLOCATE_STRING_ROUTINE RtlAllocateStringRoutine;
PRTL_FREE_STRING_ROUTINE RtlFreeStringRoutine;
RTL_BITMAP TlsBitMap;
RTL_BITMAP TlsExpansionBitMap;

RTL_CRITICAL_SECTION_DEBUG LoaderLockDebug;
RTL_CRITICAL_SECTION LoaderLock = {
    &LoaderLockDebug,
    -1
    };
BOOLEAN LoaderLockInitialized;


#if defined(_ALPHA_)
VOID
LdrpSetGp(
    IN ULONG_PTR GpValue
    );
#endif // ALPHA

VOID
LdrpInitializationFailure(
    IN NTSTATUS FailureCode
    )
{

    NTSTATUS ErrorStatus;
    ULONG_PTR ErrorParameter;
    ULONG ErrorResponse;

    if ( LdrpFatalHardErrorCount ) {
        return;
        }

    //
    // Its error time...
    //
    ErrorParameter = (ULONG_PTR)FailureCode;
    ErrorStatus = NtRaiseHardError(
                    STATUS_APP_INIT_FAILURE,
                    1,
                    0,
                    &ErrorParameter,
                    OptionOk,
                    &ErrorResponse
                    );
}


VOID
LdrpInitialize (
    IN PCONTEXT Context,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This function is called as a User-Mode APC routine as the first
    user-mode code executed by a new thread. It's function is to initialize
    loader context, perform module initialization callouts...

Arguments:

    Context - Supplies an optional context buffer that will be restore
              after all DLL initialization has been completed.  If this
              parameter is NULL then this is a dynamic snap of this module.
              Otherwise this is a static snap prior to the user process
              gaining control.

    SystemArgument1 - Supplies the base address of the System Dll.

    SystemArgument2 - not used.

Return Value:

    None.

--*/

{
    NTSTATUS st, InitStatus;
    PPEB Peb;
    PTEB Teb;
    UNICODE_STRING UnicodeImageName;
    MEMORY_BASIC_INFORMATION MemInfo;
    BOOLEAN AlreadyFailed;
    LARGE_INTEGER DelayValue;
#if defined(_WIN64)
    PIMAGE_NT_HEADERS NtHeader;
#endif

    SystemArgument2;

    AlreadyFailed = FALSE;
    Peb = NtCurrentPeb();
    Teb = NtCurrentTeb();

    if (!Peb->Ldr) {
#if defined(_ALPHA_)
        ULONG temp;

        //
        // Set GP register
        //

        LdrpGpValue = (ULONG_PTR)RtlImageDirectoryEntryToData(
                Peb->ImageBaseAddress,
                TRUE,
                IMAGE_DIRECTORY_ENTRY_GLOBALPTR,
                &temp
                );
        if (Context != NULL) {
            LdrpSetGp( LdrpGpValue );
            Context->IntGp = LdrpGpValue;
        }
#endif // ALPHA

//#if DBG
        if (TRUE)
//#else
//        if (Peb->BeingDebugged || Peb->ReadImageFileExecOptions)
//#endif
        {
            PWSTR pw;

            pw = (PWSTR)Peb->ProcessParameters->ImagePathName.Buffer;
            if (!(Peb->ProcessParameters->Flags & RTL_USER_PROC_PARAMS_NORMALIZED)) {
                pw = (PWSTR)((PCHAR)pw + (ULONG_PTR)(Peb->ProcessParameters));
                }
            UnicodeImageName.Buffer = pw;
            UnicodeImageName.Length = Peb->ProcessParameters->ImagePathName.Length;
            UnicodeImageName.MaximumLength = UnicodeImageName.Length;

            //
            //  Hack for NT4 SP4.  So we don't overload another GlobalFlag
            //  bit that we have to be "compatible" with for NT5, look for
            //  another value named "DisableHeapLookaside".
            //

            LdrQueryImageFileExecutionOptions( &UnicodeImageName,
                                               L"DisableHeapLookaside",
                                               REG_DWORD,
                                               &RtlpDisableHeapLookaside,
                                               sizeof( RtlpDisableHeapLookaside ),
                                               NULL
                                             );

            st = LdrQueryImageFileExecutionOptions( &UnicodeImageName,
                                                    L"GlobalFlag",
                                                    REG_DWORD,
                                                    &Peb->NtGlobalFlag,
                                                    sizeof( Peb->NtGlobalFlag ),
                                                    NULL
                                                  );
            if (!NT_SUCCESS( st )) {

                if (Peb->BeingDebugged) {
                    Peb->NtGlobalFlag |= FLG_HEAP_ENABLE_FREE_CHECK |
                                         FLG_HEAP_ENABLE_TAIL_CHECK |
                                         FLG_HEAP_VALIDATE_PARAMETERS;
                    }
                }

#if defined(_WIN64)
            NtHeader = RtlImageNtHeader(Peb->ImageBaseAddress);
            if (NtHeader && NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
                UseWOW64 = TRUE;
            }
#endif
        }

        if ( Peb->NtGlobalFlag & FLG_HEAP_PAGE_ALLOCS ) {

            //
            // We will enable page heap (RtlpDebugPageHeap) only after
            // all other initializations for page heap are finished.
            //

            //
            // If page heap is enabled we need to disable any flag that
            // might force creation of debug heaps for normal NT heaps.
            // This is due to a dependency between page heap and NT heap
            // where the page heap within PageHeapCreate tries to create
            // a normal NT heap to accomodate some of the allocations.
            // If we do not disable these flags we will get an infinite
            // recursion between RtlpDebugPageHeapCreate and RtlCreateHeap.
            //

            Peb->NtGlobalFlag &= ~( FLG_HEAP_ENABLE_TAGGING      |
                FLG_HEAP_ENABLE_TAG_BY_DLL   |
                FLG_HEAP_ENABLE_TAIL_CHECK   |
                FLG_HEAP_ENABLE_FREE_CHECK   |
                FLG_HEAP_VALIDATE_PARAMETERS |
                FLG_HEAP_VALIDATE_ALL        |
                FLG_USER_STACK_TRACE_DB      );

            //
            // Read page heap per process global flags. If we fail
            // to read a value, the default ones are kept.
            //

            LdrQueryImageFileExecutionOptions(
                &UnicodeImageName,
                L"PageHeapFlags",
                REG_DWORD,
                &RtlpDphGlobalFlags,
                sizeof(RtlpDphGlobalFlags),
                NULL
                );

            //
            // Read several page heap parameters.
            //

            LdrQueryImageFileExecutionOptions(
                &UnicodeImageName,
                L"PageHeapSizeRangeStart",
                REG_DWORD,
                &RtlpDphSizeRangeStart,
                sizeof(RtlpDphSizeRangeStart),
                NULL
                );

            LdrQueryImageFileExecutionOptions(
                &UnicodeImageName,
                L"PageHeapSizeRangeEnd",
                REG_DWORD,
                &RtlpDphSizeRangeEnd,
                sizeof(RtlpDphSizeRangeEnd),
                NULL
                );

            LdrQueryImageFileExecutionOptions(
                &UnicodeImageName,
                L"PageHeapRandomProbability",
                REG_DWORD,
                &RtlpDphRandomProbability,
                sizeof(RtlpDphRandomProbability),
                NULL
                );

            //
            // The two values below should be read as PVOIDs so that
            // this works on 64-bit architetures. However since this
            // feature relies on good stack traces and since we can get
            // reliable stack traces only on X86 architectures we will
            // leave it as it is.
            //

            LdrQueryImageFileExecutionOptions(
                &UnicodeImageName,
                L"PageHeapDllRangeStart",
                REG_DWORD,       
                &RtlpDphDllRangeStart,
                sizeof(RtlpDphDllRangeStart),
                NULL
                );

            LdrQueryImageFileExecutionOptions(
                &UnicodeImageName,
                L"PageHeapDllRangeEnd",
                REG_DWORD,       
                &RtlpDphDllRangeEnd,
                sizeof(RtlpDphDllRangeEnd),
                NULL
                );

            LdrQueryImageFileExecutionOptions(
                &UnicodeImageName,
                L"PageHeapTargetDlls",
                REG_SZ,
                &RtlpDphTargetDlls,
                512,
                NULL
                );

            //
            //  Turn on BOOLEAN RtlpDebugPageHeap to indicate that
            //  new heaps should be created with debug page heap manager
            //  when possible.
            //

            RtlpDebugPageHeap = TRUE;
        }
    }
#if defined(_ALPHA_)
    else
    if (Context != NULL) {
        LdrpSetGp( LdrpGpValue );
        Context->IntGp = LdrpGpValue;
    }
#endif // ALPHA

    //
    // Serialize for here on out
    //

    Peb->LoaderLock = (PVOID)&LoaderLock;

    if ( !RtlTryEnterCriticalSection(&LoaderLock) ) {
        if ( LoaderLockInitialized ) {
            RtlEnterCriticalSection(&LoaderLock);
            }
        else {

            //
            // drop into a 30ms delay loop
            //

            DelayValue.QuadPart = Int32x32To64( 30, -10000 );
            while ( !LoaderLockInitialized ) {
                NtDelayExecution(FALSE,&DelayValue);
                }
            RtlEnterCriticalSection(&LoaderLock);
            }
        }

    if (Teb->DeallocationStack == NULL) {
        st = NtQueryVirtualMemory(
                NtCurrentProcess(),
                Teb->NtTib.StackLimit,
                MemoryBasicInformation,
                (PVOID)&MemInfo,
                sizeof(MemInfo),
                NULL
                );
        if ( !NT_SUCCESS(st) ) {
            LdrpInitializationFailure(st);
            RtlRaiseStatus(st);
            return;
            }
        else {
            Teb->DeallocationStack = MemInfo.AllocationBase;
#if defined(_IA64_)
            Teb->DeallocationBStore = (PVOID)((ULONG_PTR)MemInfo.AllocationBase + MemInfo.RegionSize);
#endif // defined(_IA64_)
        }
    }

    InitStatus = STATUS_SUCCESS;
    try {
        if (!Peb->Ldr) {
            LdrpInLdrInit = TRUE;
#if DBG
            //
            // Time the load.
            //

            if (LdrpDisplayLoadTime) {
                NtQueryPerformanceCounter(&BeginTime, NULL);
            }
#endif // DBG

            try {
                InitStatus = LdrpInitializeProcess( Context,
                                                    SystemArgument1,
                                                    &UnicodeImageName
                                                  );
                }
            except ( EXCEPTION_EXECUTE_HANDLER ) {
                InitStatus = GetExceptionCode();
                AlreadyFailed = TRUE;
                LdrpInitializationFailure(GetExceptionCode());
                }

#if DBG
            if (LdrpDisplayLoadTime) {
                NtQueryPerformanceCounter(&EndTime, NULL);
                NtQueryPerformanceCounter(&ElapsedTime, &Interval);
                ElapsedTime.QuadPart = EndTime.QuadPart - BeginTime.QuadPart;
                DbgPrint("\nLoadTime %ld In units of %ld cycles/second \n",
                    ElapsedTime.LowPart,
                    Interval.LowPart
                    );

                ElapsedTime.QuadPart = EndTime.QuadPart - InitbTime.QuadPart;
                DbgPrint("InitTime %ld\n",
                    ElapsedTime.LowPart
                    );
                DbgPrint("Compares %d Bypasses %d Normal Snaps %d\nSecOpens %d SecCreates %d Maps %d Relocates %d\n",
                    LdrpCompareCount,
                    LdrpSnapBypass,
                    LdrpNormalSnap,
                    LdrpSectionOpens,
                    LdrpSectionCreates,
                    LdrpSectionMaps,
                    LdrpSectionRelocates
                    );
            }
#endif // DBG


        } else {
            if ( Peb->InheritedAddressSpace ) {
                InitStatus = LdrpForkProcess();
                }
            else {

#if defined (WX86)
                if (Teb->Vdm) {
                    InitStatus = LdrpInitWx86(Teb->Vdm, Context, TRUE);
                    }
#endif

#if defined(_WIN64)
                //
                // Load in WOW64 if the image is supposed to run simulated
                //
                if (UseWOW64) {
                    RtlLeaveCriticalSection(&LoaderLock);
                    (*Wow64LdrpInitialize)(Context);
                    // This never returns.  It will destroy the process.
                }
#endif
                LdrpInitializeThread(Context);
                }
        }
    } finally {
        LdrpInLdrInit = FALSE;
        RtlLeaveCriticalSection(&LoaderLock);
        }

    NtTestAlert();

    if (!NT_SUCCESS(InitStatus)) {

        if ( AlreadyFailed == FALSE ) {
            LdrpInitializationFailure(InitStatus);
            }
        RtlRaiseStatus(InitStatus);
    }


}

NTSTATUS
LdrpForkProcess( VOID )
{
    NTSTATUS st;
    PPEB Peb;

    Peb = NtCurrentPeb();

    //
    // Initialize the critical section package.
    //

    RtlpInitDeferedCriticalSection();

    InsertTailList(&RtlCriticalSectionList, &LoaderLock.DebugInfo->ProcessLocksList);
    LoaderLock.DebugInfo->CriticalSection = &LoaderLock;
    LoaderLockInitialized = TRUE;

    st = RtlInitializeCriticalSection(&FastPebLock);
    if ( !NT_SUCCESS(st) ) {
        RtlRaiseStatus(st);
        }
    Peb->FastPebLock = &FastPebLock;
    Peb->FastPebLockRoutine = (PVOID)&RtlEnterCriticalSection;
    Peb->FastPebUnlockRoutine = (PVOID)&RtlLeaveCriticalSection;
    Peb->InheritedAddressSpace = FALSE;
    RtlInitializeHeapManager();
    Peb->ProcessHeap = RtlCreateHeap( HEAP_GROWABLE,    // Flags
                                      NULL,             // HeapBase
                                      64 * 1024,        // ReserveSize
                                      4096,             // CommitSize
                                      NULL,             // Lock to use for serialization
                                      NULL              // GrowthThreshold
                                    );
    if (Peb->ProcessHeap == NULL) {
        return STATUS_NO_MEMORY;
    }

    return st;
}

NTSTATUS
LdrpInitializeProcess (
    IN PCONTEXT Context OPTIONAL,
    IN PVOID SystemDllBase,
    IN PUNICODE_STRING UnicodeImageName
    )

/*++

Routine Description:

    This function initializes the loader for the process.
    This includes:

        - Initializing the loader data table

        - Connecting to the loader subsystem

        - Initializing all staticly linked DLLs

Arguments:

    Context - Supplies an optional context buffer that will be restore
              after all DLL initialization has been completed.  If this
              parameter is NULL then this is a dynamic snap of this module.
              Otherwise this is a static snap prior to the user process
              gaining control.

    SystemDllBase - Supplies the base address of the system dll.

Return Value:

    Status value

--*/

{
    PPEB Peb;
    NTSTATUS st;
    PWCH p, pp;
    UNICODE_STRING CurDir;
    UNICODE_STRING FullImageName;
    UNICODE_STRING CommandLine;
    ULONG DebugProcessHeapOnly = 0 ;
    HANDLE LinkHandle;
    WCHAR SystemDllPathBuffer[DOS_MAX_PATH_LENGTH];
    UNICODE_STRING SystemDllPath;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    UNICODE_STRING Unicode;
    OBJECT_ATTRIBUTES Obja;
    BOOLEAN StaticCurDir = FALSE;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeader = RtlImageNtHeader( NtCurrentPeb()->ImageBaseAddress );
    PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigData;
    ULONG ProcessHeapFlags;
    RTL_HEAP_PARAMETERS HeapParameters;
    NLSTABLEINFO InitTableInfo;
    LARGE_INTEGER LongTimeout;
    UNICODE_STRING NtSystemRoot;
    LONG_PTR Diff;
    ULONG_PTR OldBase;

    NtDllBase = SystemDllBase;

    if (
#if defined(_WIN64)
        NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC &&
#endif
        NtHeader->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_NATIVE ) {

        //
        // Native subsystems load slower, but validate their DLLs
        // This is to help CSR detect bad images faster
        //

        LdrpVerifyDlls = TRUE;

        }


    Peb = NtCurrentPeb();

#if defined(BUILD_WOW6432)
    {
        //
        // The process is running in WOW64.  Sort out the optional header
        // format and reformat the image if its page size is smaller than
        // the native page size.
        //
        PIMAGE_NT_HEADERS32 NtHeader32 = (PIMAGE_NT_HEADERS32)NtHeader;

        if (NtHeader32->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 &&
            NtHeader32->OptionalHeader.SectionAlignment < NATIVE_PAGE_SIZE &&
            !NT_SUCCESS(st = LdrpWx86FormatVirtualImage(NtHeader32,
                                 NtCurrentPeb()->ImageBaseAddress
                                 //bugbug: should be:  (PVOID)NtHeader32->OptionalHeader.ImageBase
                                 ))) {
            return st;
        }
    }
#elif defined (_ALPHA_) && defined (WX86)
     //
     // Deal with the native page size which is larger than x86
     // This needs to be done before any code reads beyond the file headers.
     //
    if (NtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 &&
        NtHeader->OptionalHeader.SectionAlignment < PAGE_SIZE &&
        !NT_SUCCESS(st = LdrpWx86FormatVirtualImage((PIMAGE_NT_HEADERS32)NtHeader,
                             (PVOID)NtHeader->OptionalHeader.ImageBase
                             )))
      {
        return st;
        }
#endif


    LdrpNumberOfProcessors = Peb->NumberOfProcessors;
    RtlpTimeout = Peb->CriticalSectionTimeout;
    LongTimeout.QuadPart = Int32x32To64( 3600, -10000000 );

    if (ProcessParameters = RtlNormalizeProcessParams(Peb->ProcessParameters)) {
        FullImageName = *(PUNICODE_STRING)&ProcessParameters->ImagePathName;
        CommandLine = *(PUNICODE_STRING)&ProcessParameters->CommandLine;
        }
    else {
        RtlInitUnicodeString( &FullImageName, NULL );
        RtlInitUnicodeString( &CommandLine, NULL );
        }


    RtlInitNlsTables(
        Peb->AnsiCodePageData,
        Peb->OemCodePageData,
        Peb->UnicodeCaseTableData,
        &InitTableInfo
        );

    RtlResetRtlTranslations(&InitTableInfo);

#if defined(_WIN64)
    if (UseWOW64) {
        //
        // Ignore image config data when initializing the 64-bit loader.
        // The 32-bit loader in ntdll32 will look at the config data
        // and do the right thing.
        //
        ImageConfigData = NULL;
    } else
#endif
    {

        ImageConfigData = RtlImageDirectoryEntryToData( Peb->ImageBaseAddress,
                                                        TRUE,
                                                        IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                                        &i
                                                      );
    }

    RtlZeroMemory( &HeapParameters, sizeof( HeapParameters ) );
    ProcessHeapFlags = HEAP_GROWABLE | HEAP_CLASS_0;
    HeapParameters.Length = sizeof( HeapParameters );
    if (ImageConfigData != NULL && i == sizeof( *ImageConfigData )) {
        Peb->NtGlobalFlag &= ~ImageConfigData->GlobalFlagsClear;
        Peb->NtGlobalFlag |= ImageConfigData->GlobalFlagsSet;

        if (ImageConfigData->CriticalSectionDefaultTimeout != 0) {
            //
            // Convert from milliseconds to NT time scale (100ns)
            //
            RtlpTimeout.QuadPart = Int32x32To64( (LONG)ImageConfigData->CriticalSectionDefaultTimeout,
                                                 -10000
                                               );

            }

        if (ImageConfigData->ProcessHeapFlags != 0) {
            ProcessHeapFlags = ImageConfigData->ProcessHeapFlags;
            }

        if (ImageConfigData->DeCommitFreeBlockThreshold != 0) {
            HeapParameters.DeCommitFreeBlockThreshold = ImageConfigData->DeCommitFreeBlockThreshold;
            }

        if (ImageConfigData->DeCommitTotalFreeThreshold != 0) {
            HeapParameters.DeCommitTotalFreeThreshold = ImageConfigData->DeCommitTotalFreeThreshold;
            }

        if (ImageConfigData->MaximumAllocationSize != 0) {
            HeapParameters.MaximumAllocationSize = ImageConfigData->MaximumAllocationSize;
            }

        if (ImageConfigData->VirtualMemoryThreshold != 0) {
            HeapParameters.VirtualMemoryThreshold = ImageConfigData->VirtualMemoryThreshold;
            }
        }

//    //
//    // Check if the image has the fast heap flag set
//    //
//
//    if (NtHeader->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_FAST_HEAP) {
//        RtlpDisableHeapLookaside = 0;
//    } else {
//        RtlpDisableHeapLookaside = 1;
//    }

    ShowSnaps = (BOOLEAN)(FLG_SHOW_LDR_SNAPS & Peb->NtGlobalFlag);



    //
    // This field is non-zero if the image file that was used to create this
    // process contained a non-zero value in its image header.  If so, then
    // set the affinity mask for the process using this value.  It could also
    // be non-zero if the parent process created us suspended and poked our
    // PEB with a non-zero value before resuming.
    //
    if (Peb->ImageProcessAffinityMask) {
        st = NtSetInformationProcess( NtCurrentProcess(),
                                      ProcessAffinityMask,
                                      &Peb->ImageProcessAffinityMask,
                                      sizeof( Peb->ImageProcessAffinityMask )
                                    );
        if (NT_SUCCESS( st )) {
            KdPrint(( "LDR: Using ProcessAffinityMask of 0x%x from image.\n",
                      Peb->ImageProcessAffinityMask
                   ));
            }
        else {
            KdPrint(( "LDR: Failed to set ProcessAffinityMask of 0x%x from image (Status == %08x).\n",
                      Peb->ImageProcessAffinityMask, st
                   ));
            }
        }

    if (RtlpTimeout.QuadPart < LongTimeout.QuadPart) {
        RtlpTimoutDisable = TRUE;
        }

    if (ShowSnaps) {
        DbgPrint( "LDR: PID: 0x%x started - '%wZ'\n",
                  NtCurrentTeb()->ClientId.UniqueProcess,
                  &CommandLine
                );
    }

    for(i=0;i<LDRP_HASH_TABLE_SIZE;i++) {
        InitializeListHead(&LdrpHashTable[i]);
    }

    //
    // Initialize the critical section package.
    //

    RtlpInitDeferedCriticalSection();

    Peb->TlsBitmap = (PVOID)&TlsBitMap;
    Peb->TlsExpansionBitmap = (PVOID)&TlsExpansionBitMap;

    RtlInitializeBitMap (
        &TlsBitMap,
        &Peb->TlsBitmapBits[0],
        TLS_MINIMUM_AVAILABLE
        );

    RtlInitializeBitMap (
        &TlsExpansionBitMap,
        &Peb->TlsExpansionBitmapBits[0],
        TLS_EXPANSION_SLOTS
        );

    InsertTailList(&RtlCriticalSectionList, &LoaderLock.DebugInfo->ProcessLocksList);
    LoaderLock.DebugInfo->CriticalSection = &LoaderLock;
    LoaderLockInitialized = TRUE;

    //
    // Initialize the stack trace data base if requested
    //

#if i386
    if (Peb->NtGlobalFlag & FLG_USER_STACK_TRACE_DB) {
        PVOID BaseAddress = NULL;
        ULONG ReserveSize = 2 * 1024 * 1024;

        st = NtAllocateVirtualMemory( NtCurrentProcess(),
                                      (PVOID *)&BaseAddress,
                                      0,
                                      &ReserveSize,
                                      MEM_RESERVE,
                                      PAGE_READWRITE
                                    );
        if ( NT_SUCCESS( st ) ) {
            st = RtlInitializeStackTraceDataBase( BaseAddress,
                                                  0,
                                                  ReserveSize
                                                );
            if ( !NT_SUCCESS( st ) ) {
                NtFreeVirtualMemory( NtCurrentProcess(),
                                     (PVOID *)&BaseAddress,
                                     &ReserveSize,
                                     MEM_RELEASE
                                   );
                }
            else {
                Peb->NtGlobalFlag |= FLG_HEAP_VALIDATE_PARAMETERS;
                }
            }
        }
#endif // i386

    //
    // Initialize the loader data based in the PEB.
    //

    st = RtlInitializeCriticalSection(&FastPebLock);
    if ( !NT_SUCCESS(st) ) {
        return st;
        }
    Peb->FastPebLock = &FastPebLock;
    Peb->FastPebLockRoutine = (PVOID)&RtlEnterCriticalSection;
    Peb->FastPebUnlockRoutine = (PVOID)&RtlLeaveCriticalSection;

    RtlInitializeHeapManager();
#if defined(_WIN64)
    if (UseWOW64) {
        //
        // Create a heap using all defaults.  The 32-bit process heap
        // will be created later by ntdll32 using the parameters from the exe.
        //
        Peb->ProcessHeap = RtlCreateHeap( ProcessHeapFlags,
                                          NULL,
                                          0,
                                          0,
                                          NULL,
                                          &HeapParameters
                                        );
    } else
#endif
    {

        if (NtHeader->OptionalHeader.MajorSubsystemVersion <= 3 &&
            NtHeader->OptionalHeader.MinorSubsystemVersion < 51
           ) {
            ProcessHeapFlags |= HEAP_CREATE_ALIGN_16;
            }

        Peb->ProcessHeap = RtlCreateHeap( ProcessHeapFlags,
                                          NULL,
                                          NtHeader->OptionalHeader.SizeOfHeapReserve,
                                          NtHeader->OptionalHeader.SizeOfHeapCommit,
                                          NULL,             // Lock to use for serialization
                                          &HeapParameters
                                        );
    }
    if (Peb->ProcessHeap == NULL) {
        return STATUS_NO_MEMORY;
    }

    NtdllBaseTag = RtlCreateTagHeap( Peb->ProcessHeap,
                                     0,
                                     L"NTDLL!",
                                     L"!Process\0"                  // Heap Name
                                     L"CSRSS Client\0"
                                     L"LDR Database\0"
                                     L"Current Directory\0"
                                     L"TLS Storage\0"
                                     L"DBGSS Client\0"
                                     L"SE Temporary\0"
                                     L"Temporary\0"
                                     L"LocalAtom\0"
                                   );

    RtlAllocateStringRoutine = NtdllpAllocateStringRoutine;
    RtlFreeStringRoutine = NtdllpFreeStringRoutine;

    RtlInitializeAtomPackage( MAKE_TAG( ATOM_TAG ) );

    //
    // Allow only the process heap to have page allocations turned on
    //

    st = LdrQueryImageFileExecutionOptions( UnicodeImageName,
                                            L"DebugProcessHeapOnly",
                                            REG_DWORD,
                                            &DebugProcessHeapOnly,
                                            sizeof( DebugProcessHeapOnly ),
                                            NULL
                                          );
    if (NT_SUCCESS( st )) {

        if ( RtlpDebugPageHeap &&
             ( DebugProcessHeapOnly != 0 ) ) {

            RtlpDebugPageHeap = FALSE ;

            }

        }

    SystemDllPath.Buffer = SystemDllPathBuffer;
    SystemDllPath.Length = 0;
    SystemDllPath.MaximumLength = sizeof( SystemDllPathBuffer );
    RtlInitUnicodeString( &NtSystemRoot, USER_SHARED_DATA->NtSystemRoot );
    RtlAppendUnicodeStringToString( &SystemDllPath, &NtSystemRoot );
    RtlAppendUnicodeToString( &SystemDllPath, L"\\System32\\" );

    RtlInitUnicodeString(&Unicode,L"\\KnownDlls");
    InitializeObjectAttributes( &Obja,
                                  &Unicode,
                                  OBJ_CASE_INSENSITIVE,
                                  NULL,
                                  NULL
                                );
    st = NtOpenDirectoryObject(
            &LdrpKnownDllObjectDirectory,
            DIRECTORY_QUERY | DIRECTORY_TRAVERSE,
            &Obja
            );
    if ( !NT_SUCCESS(st) ) {
        LdrpKnownDllObjectDirectory = NULL;
        // KnownDlls directory doesn't exist - assume it's system32.
        RtlInitUnicodeString(&LdrpKnownDllPath, SystemDllPath.Buffer);
        LdrpKnownDllPath.Length -= sizeof(WCHAR);    // remove trailing '\'
        }
    else {

        //
        // Open up the known dll pathname link
        // and query its value
        //

        RtlInitUnicodeString(&Unicode,L"KnownDllPath");
        InitializeObjectAttributes( &Obja,
                                      &Unicode,
                                      OBJ_CASE_INSENSITIVE,
                                      LdrpKnownDllObjectDirectory,
                                      NULL
                                    );
        st = NtOpenSymbolicLinkObject( &LinkHandle,
                                       SYMBOLIC_LINK_QUERY,
                                       &Obja
                                     );
        if (NT_SUCCESS( st )) {
            LdrpKnownDllPath.Length = 0;
            LdrpKnownDllPath.MaximumLength = sizeof(LdrpKnownDllPathBuffer);
            LdrpKnownDllPath.Buffer = LdrpKnownDllPathBuffer;
            st = NtQuerySymbolicLinkObject( LinkHandle,
                                            &LdrpKnownDllPath,
                                            NULL
                                          );
            NtClose(LinkHandle);
            if ( !NT_SUCCESS(st) ) {
                return st;
                }
            }
        else {
            return st;
            }
        }

    if (ProcessParameters) {

        //
        // If the process was created with process parameters,
        // than extract:
        //
        //      - Library Search Path
        //
        //      - Starting Current Directory
        //

        if (ProcessParameters->DllPath.Length) {
            LdrpDefaultPath = *(PUNICODE_STRING)&ProcessParameters->DllPath;
            }
        else {
            LdrpInitializationFailure( STATUS_INVALID_PARAMETER );
            }

        StaticCurDir = TRUE;
        CurDir = ProcessParameters->CurrentDirectory.DosPath;
        if (CurDir.Buffer == NULL || CurDir.Buffer[ 0 ] == UNICODE_NULL || CurDir.Length == 0) {
            CurDir.Buffer = (RtlAllocateStringRoutine)( (3+1) * sizeof( WCHAR ) );
            ASSERT(CurDir.Buffer != NULL);
            RtlMoveMemory( CurDir.Buffer,
                           USER_SHARED_DATA->NtSystemRoot,
                           3 * sizeof( WCHAR )
                         );
            CurDir.Buffer[ 3 ] = UNICODE_NULL;
            }
        }

    //
    // Make sure the module data base is initialized before we take any
    // exceptions.
    //

    Peb->Ldr = RtlAllocateHeap(Peb->ProcessHeap, MAKE_TAG( LDR_TAG ), sizeof(PEB_LDR_DATA));
    if ( !Peb->Ldr ) {
        RtlRaiseStatus(STATUS_NO_MEMORY);
        }

    Peb->Ldr->Length = sizeof(PEB_LDR_DATA);
    Peb->Ldr->Initialized = TRUE;
    Peb->Ldr->SsHandle = NULL;
    InitializeListHead(&Peb->Ldr->InLoadOrderModuleList);
    InitializeListHead(&Peb->Ldr->InMemoryOrderModuleList);
    InitializeListHead(&Peb->Ldr->InInitializationOrderModuleList);

    //
    // Allocate the first data table entry for the image. Since we
    // have already mapped this one, we need to do the allocation by hand.
    // Its characteristics identify it as not a Dll, but it is linked
    // into the table so that pc correlation searching doesn't have to
    // be special cased.
    //

    LdrDataTableEntry = LdrpImageEntry = LdrpAllocateDataTableEntry(Peb->ImageBaseAddress);
    LdrDataTableEntry->LoadCount = (USHORT)0xffff;
    LdrDataTableEntry->EntryPoint = LdrpFetchAddressOfEntryPoint(LdrDataTableEntry->DllBase);
    LdrDataTableEntry->FullDllName = FullImageName;
    LdrDataTableEntry->Flags = 0;

    // p = strrchr(FullImageName, '\\');
    pp = UNICODE_NULL;
    p = FullImageName.Buffer;
    while (*p) {
        if (*p++ == (WCHAR)'\\') {
            pp = p;
        }
    }

    LdrDataTableEntry->FullDllName.Length = (USHORT)((ULONG_PTR)p - (ULONG_PTR)FullImageName.Buffer);
    LdrDataTableEntry->FullDllName.MaximumLength = LdrDataTableEntry->FullDllName.Length + (USHORT)sizeof(UNICODE_NULL);

    if (pp) {
       LdrDataTableEntry->BaseDllName.Length = (USHORT)((ULONG_PTR)p - (ULONG_PTR)pp);
       LdrDataTableEntry->BaseDllName.MaximumLength = LdrDataTableEntry->BaseDllName.Length + (USHORT)sizeof(UNICODE_NULL);
       LdrDataTableEntry->BaseDllName.Buffer = RtlAllocateHeap(Peb->ProcessHeap, MAKE_TAG( LDR_TAG ),
                                                               LdrDataTableEntry->BaseDllName.MaximumLength
                                                              );
       RtlMoveMemory(LdrDataTableEntry->BaseDllName.Buffer,
                     pp,
                     LdrDataTableEntry->BaseDllName.MaximumLength
                    );
    }  else {
              LdrDataTableEntry->BaseDllName = LdrDataTableEntry->FullDllName;
            }
    LdrpInsertMemoryTableEntry(LdrDataTableEntry);
    LdrDataTableEntry->Flags |= LDRP_ENTRY_PROCESSED;

    if (ShowSnaps) {
        DbgPrint( "LDR: NEW PROCESS\n" );
        DbgPrint( "     Image Path: %wZ (%wZ)\n",
                  &LdrDataTableEntry->FullDllName,
                  &LdrDataTableEntry->BaseDllName
                );
        DbgPrint( "     Current Directory: %wZ\n", &CurDir );
        DbgPrint( "     Search Path: %wZ\n", &LdrpDefaultPath );
    }

    //
    // The process references the system DLL, so map this one next. Since
    // we have already mapped this one, we need to do the allocation by
    // hand. Since every application will be statically linked to the
    // system Dll, we'll keep the LoadCount initialized to 0.
    //

    LdrDataTableEntry = LdrpAllocateDataTableEntry(SystemDllBase);
    LdrDataTableEntry->Flags = (USHORT)LDRP_IMAGE_DLL;
    LdrDataTableEntry->EntryPoint = LdrpFetchAddressOfEntryPoint(LdrDataTableEntry->DllBase);
    LdrDataTableEntry->LoadCount = (USHORT)0xffff;

    LdrDataTableEntry->BaseDllName.Length = SystemDllPath.Length;
    RtlAppendUnicodeToString( &SystemDllPath, L"ntdll.dll" );
    LdrDataTableEntry->BaseDllName.Length = SystemDllPath.Length - LdrDataTableEntry->BaseDllName.Length;
    LdrDataTableEntry->BaseDllName.MaximumLength = LdrDataTableEntry->BaseDllName.Length + sizeof( UNICODE_NULL );

    LdrDataTableEntry->FullDllName.Buffer =
        (RtlAllocateStringRoutine)( SystemDllPath.Length + sizeof( UNICODE_NULL ) );
    ASSERT(LdrDataTableEntry->FullDllName.Buffer != NULL);
    RtlMoveMemory( LdrDataTableEntry->FullDllName.Buffer,
                   SystemDllPath.Buffer,
                   SystemDllPath.Length
                 );
    LdrDataTableEntry->FullDllName.Buffer[ SystemDllPath.Length / sizeof( WCHAR ) ] = UNICODE_NULL;
    LdrDataTableEntry->FullDllName.Length = SystemDllPath.Length;
    LdrDataTableEntry->FullDllName.MaximumLength = SystemDllPath.Length + sizeof( UNICODE_NULL );
    LdrDataTableEntry->BaseDllName.Buffer = (PWSTR)
        ((PCHAR)(LdrDataTableEntry->FullDllName.Buffer) +
         LdrDataTableEntry->FullDllName.Length -
         LdrDataTableEntry->BaseDllName.Length
        );
    LdrpInsertMemoryTableEntry(LdrDataTableEntry);

    //
    // Add init routine to list
    //

    InsertHeadList(&Peb->Ldr->InInitializationOrderModuleList,
                   &LdrDataTableEntry->InInitializationOrderLinks);

    //
    // Inherit the current directory
    //

    st = RtlSetCurrentDirectory_U(&CurDir);
    if (!NT_SUCCESS(st)) {
        if ( !StaticCurDir ) {
            RtlFreeUnicodeString(&CurDir);
            }
        CurDir = NtSystemRoot;
        st = RtlSetCurrentDirectory_U(&CurDir);
        }
    else {
        if ( !StaticCurDir ) {
            RtlFreeUnicodeString(&CurDir);
            }
        }

#if defined(WX86)

    //
    // Load in x86 emulator for risc (Wx86.dll)
    //

    if (NtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386) {
        st = LdrpLoadWx86Dll(Context);
        if (!NT_SUCCESS(st)) {
            return st;
            }
        }

#endif

#if defined(_WIN64)
    //
    // Load in WOW64 if the image is supposed to run simulated
    //
    if (UseWOW64) {
        UNICODE_STRING Wow64Name;
        ANSI_STRING ProcName;
        RtlInitUnicodeString(&Wow64Name, L"wow64.dll");
        st = LdrLoadDll(NULL, NULL, &Wow64Name, &Wow64Handle);
        if (!NT_SUCCESS(st)) {
            if (ShowSnaps) {
                DbgPrint("wow64.dll not found.  Status=%x\n", st);
            }
            return st;
        }

        //
        // Get the entrypoints.  They are roughly cloned from ntos\ps\psinit.c
        // PspInitSystemDll().
        //
        RtlInitAnsiString(&ProcName, "Wow64LdrpInitialize");
        st = LdrGetProcedureAddress(Wow64Handle,
                                    &ProcName,
                                    0,
                                    (PVOID *)&Wow64LdrpInitialize);
        if (!NT_SUCCESS(st)) {
            if (ShowSnaps) {
                DbgPrint("Wow64LdrpInitialize not found.  Status=%x\n", st);
            }
            return st;
        }

        RtlInitAnsiString(&ProcName, "Wow64PrepareForException");
        st = LdrGetProcedureAddress(Wow64Handle,
                                    &ProcName,
                                    0,
                                    (PVOID *)&Wow64PrepareForException);
        if (!NT_SUCCESS(st)) {
            if (ShowSnaps) {
                DbgPrint("Wow64PrepareForException not found.  Status=%x\n", st);
            }
            return st;
        }

        DbgPrint("WARNING:  PROCESS HAS BEEN CONVERTED TO WOW64!!!\n");
        if (Peb->ProcessParameters) {
           DbgPrint("CommandLine: %wZ\n", &Peb->ProcessParameters->CommandLine);
           DbgPrint("ImagePathName: %wZ\n", &Peb->ProcessParameters->ImagePathName);
        }

        //
        // Now that all DLLs are loaded, if the process is being debugged,
        // signal the debugger with an exception
        //

        if ( Peb->BeingDebugged ) {
             DbgBreakPoint();
        }

        //
        // Release the loaderlock now - this thread doesn't need it any more.
        //
        RtlLeaveCriticalSection(&LoaderLock);

        //
        // Call wow64 to load and run 32-bit ntdll.dll.
        //
        (*Wow64LdrpInitialize)(Context);
        // This never returns.  It will destroy the process.
    }
#endif


    st = LdrpWalkImportDescriptor(
            LdrpDefaultPath.Buffer,
            LdrpImageEntry
            );

    if ((PVOID)NtHeader->OptionalHeader.ImageBase != NtCurrentPeb()->ImageBaseAddress ) {

        //
        // The executable is not at its original address.  It must be
        // relocated now.
        //

        PVOID ViewBase;
        NTSTATUS status;

        ViewBase = NtCurrentPeb()->ImageBaseAddress;

        status = LdrpSetProtection(ViewBase, FALSE, TRUE);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        status = (NTSTATUS)LdrRelocateImage(ViewBase,
                    "LDR",
                    (ULONG)STATUS_SUCCESS,
                    (ULONG)STATUS_CONFLICTING_ADDRESSES,
                    (ULONG)STATUS_INVALID_IMAGE_FORMAT
                    );

        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // Update the initial thread context record as per the relocation.
        //

        if (Context) {

            OldBase = NtHeader->OptionalHeader.ImageBase;
            Diff = (PCHAR)ViewBase - (PCHAR)OldBase;

            LdrpRelocateStartContext(Context, Diff);
        }

        status = LdrpSetProtection(ViewBase, TRUE, TRUE);

        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

#if defined (_ALPHA_)

    //
    // Find and apply Alpha architecture fixups for this image
    //

    AlphaFindArchitectureFixups(NtHeader, NtCurrentPeb()->ImageBaseAddress, TRUE);
#endif

    LdrpReferenceLoadedDll(LdrpImageEntry);

    //
    // Lock the loaded DLLs to prevent dlls that back link to the exe to
    // cause problems when they are unloaded.
    //

    {
        PLDR_DATA_TABLE_ENTRY Entry;
        PLIST_ENTRY Head,Next;

        Head = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
        Next = Head->Flink;

        while ( Next != Head ) {
            Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
            Entry->LoadCount = 0xffff;
            Next = Next->Flink;
        }
    }

    //
    // All static DLLs are now pinned in place. No init routines have been run yet
    //

    LdrpLdrDatabaseIsSetup = TRUE;


    if (!NT_SUCCESS(st)) {
#if DBG
        DbgPrint("LDR: Initialize of image failed. Returning Error Status\n");
#endif
        return st;
    }

    if ( !NT_SUCCESS(LdrpInitializeTls()) ) {
        return st;
        }

    //
    // Now that all DLLs are loaded, if the process is being debugged,
    // signal the debugger with an exception
    //

    if ( Peb->BeingDebugged ) {
         DbgBreakPoint();
         ShowSnaps = (BOOLEAN)(FLG_SHOW_LDR_SNAPS & Peb->NtGlobalFlag);
    }

#if defined (_X86_)
    if ( LdrpNumberOfProcessors > 1 ) {
        LdrpValidateImageForMp(LdrDataTableEntry);
        }
#endif

#if DBG
    if (LdrpDisplayLoadTime) {
        NtQueryPerformanceCounter(&InitbTime, NULL);
    }
#endif // DBG

    //
    // Get all application goo here (hacks, flags, etc.)
    //
    LdrQueryApplicationCompatibilityGoo(UnicodeImageName);

    st = LdrpRunInitializeRoutines(Context);

    if ( NT_SUCCESS(st) && Peb->PostProcessInitRoutine ) {
        (Peb->PostProcessInitRoutine)();
        }

    return st;
}


VOID
LdrShutdownProcess (
    VOID
    )

/*++

Routine Description:

    This function is called by a process that is terminating cleanly.
    It's purpose is to call all of the processes DLLs to notify them
    that the process is detaching.

Arguments:

    None

Return Value:

    None.

--*/

{
    PPEB Peb;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PDLL_INIT_ROUTINE InitRoutine;
    PLIST_ENTRY Next;

    //
    // only unload once ! DllTerm routines might call exit process in fatal situations
    //

    if ( LdrpShutdownInProgress ) {
        return;
        }

    Peb = NtCurrentPeb();

    if (ShowSnaps) {
        UNICODE_STRING CommandLine;

        CommandLine = Peb->ProcessParameters->CommandLine;
        if (!(Peb->ProcessParameters->Flags & RTL_USER_PROC_PARAMS_NORMALIZED)) {
            CommandLine.Buffer = (PWSTR)((PCHAR)CommandLine.Buffer + (ULONG_PTR)(Peb->ProcessParameters));
        }

        DbgPrint( "LDR: PID: 0x%x finished - '%wZ'\n",
                  NtCurrentTeb()->ClientId.UniqueProcess,
                  &CommandLine
                );
    }

    LdrpShutdownThreadId = NtCurrentTeb()->ClientId.UniqueThread;
    LdrpShutdownInProgress = TRUE;
    RtlEnterCriticalSection(&LoaderLock);

    try {

        //
        // check to see if the heap is locked. If so, do not do ANY
        // dll processing since it is very likely that a dll will need
        // to do heap operations, but that the heap is not in good shape.
        // ExitProcess called in a very active app can leave threads terminated
        // in the middle of the heap code or in other very bad places. Checking the
        // heap lock is a good indication that the process was very active when it
        // called ExitProcess
        //

        if ( RtlpHeapIsLocked( RtlProcessHeap() )) {
            ;
            }
        else {
            if ( Peb->BeingDebugged ) {
                RtlValidateProcessHeaps();
                }

            //
            // Go in reverse order initialization order and build
            // the unload list
            //

            Next = Peb->Ldr->InInitializationOrderModuleList.Blink;
            while ( Next != &Peb->Ldr->InInitializationOrderModuleList) {
                LdrDataTableEntry
                    = (PLDR_DATA_TABLE_ENTRY)
                      (CONTAINING_RECORD(Next,LDR_DATA_TABLE_ENTRY,InInitializationOrderLinks));

                Next = Next->Blink;

                //
                // Walk through the entire list looking for
                // entries. For each entry, that has an init
                // routine, call it.
                //

                if (Peb->ImageBaseAddress != LdrDataTableEntry->DllBase) {
                    InitRoutine = (PDLL_INIT_ROUTINE)LdrDataTableEntry->EntryPoint;
                    if (InitRoutine && (LdrDataTableEntry->Flags & LDRP_PROCESS_ATTACH_CALLED) ) {
                        if (LdrDataTableEntry->Flags) {
                            if ( LdrDataTableEntry->TlsIndex ) {
                                LdrpCallTlsInitializers(LdrDataTableEntry->DllBase,DLL_PROCESS_DETACH);
                                }

#if defined (WX86)
                            if (!Wx86ProcessInit ||
                                LdrpRunWx86DllEntryPoint(InitRoutine,
                                                        NULL,
                                                        LdrDataTableEntry->DllBase,
                                                        DLL_PROCESS_DETACH,
                                                        (PVOID)1
                                                        ) ==  STATUS_IMAGE_MACHINE_TYPE_MISMATCH)
#endif
                               {
                                LdrpCallInitRoutine(InitRoutine,
                                                    LdrDataTableEntry->DllBase,
                                                    DLL_PROCESS_DETACH,
                                                    (PVOID)1);
                                }

                            }
                        }
                    }
                }

            //
            // If the image has tls than call its initializers
            //

            if ( LdrpImageHasTls ) {
                LdrpCallTlsInitializers(NtCurrentPeb()->ImageBaseAddress,DLL_PROCESS_DETACH);
                }
            }

    } finally {
        RtlLeaveCriticalSection(&LoaderLock);
    }

}

VOID
LdrShutdownThread (
    VOID
    )

/*++

Routine Description:

    This function is called by a thread that is terminating cleanly.
    It's purpose is to call all of the processes DLLs to notify them
    that the thread is detaching.

Arguments:

    None

Return Value:

    None.

--*/

{
    PPEB Peb;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PDLL_INIT_ROUTINE InitRoutine;
    PLIST_ENTRY Next;

    Peb = NtCurrentPeb();

    RtlEnterCriticalSection(&LoaderLock);

    try {


        //
        // Go in reverse order initialization order and build
        // the unload list
        //

        Next = Peb->Ldr->InInitializationOrderModuleList.Blink;
        while ( Next != &Peb->Ldr->InInitializationOrderModuleList) {
            LdrDataTableEntry
                = (PLDR_DATA_TABLE_ENTRY)
                  (CONTAINING_RECORD(Next,LDR_DATA_TABLE_ENTRY,InInitializationOrderLinks));

            Next = Next->Blink;

            //
            // Walk through the entire list looking for
            // entries. For each entry, that has an init
            // routine, call it.
            //

            if (Peb->ImageBaseAddress != LdrDataTableEntry->DllBase) {
                if ( !(LdrDataTableEntry->Flags & LDRP_DONT_CALL_FOR_THREADS)) {
                    InitRoutine = (PDLL_INIT_ROUTINE)LdrDataTableEntry->EntryPoint;
                    if (InitRoutine && (LdrDataTableEntry->Flags & LDRP_PROCESS_ATTACH_CALLED) ) {
                        if (LdrDataTableEntry->Flags & LDRP_IMAGE_DLL) {
                            if ( LdrDataTableEntry->TlsIndex ) {
                                LdrpCallTlsInitializers(LdrDataTableEntry->DllBase,DLL_THREAD_DETACH);
                                }

#if defined (WX86)
                            if (!Wx86ProcessInit ||
                                LdrpRunWx86DllEntryPoint(InitRoutine,
                                                        NULL,
                                                        LdrDataTableEntry->DllBase,
                                                        DLL_THREAD_DETACH,
                                                        NULL
                                                        ) ==  STATUS_IMAGE_MACHINE_TYPE_MISMATCH)
#endif
                               {
                                LdrpCallInitRoutine(InitRoutine,
                                                    LdrDataTableEntry->DllBase,
                                                    DLL_THREAD_DETACH,
                                                    NULL);
                                }
                            }
                        }
                    }
                }
            }

        //
        // If the image has tls than call its initializers
        //

        if ( LdrpImageHasTls ) {
            LdrpCallTlsInitializers(NtCurrentPeb()->ImageBaseAddress,DLL_THREAD_DETACH);
            }
        LdrpFreeTls();

    } finally {

        RtlLeaveCriticalSection(&LoaderLock);
    }
}

VOID
LdrpInitializeThread(
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called by a thread that is terminating cleanly.
    It's purpose is to call all of the processes DLLs to notify them
    that the thread is detaching.

Arguments:

    Context - Context that will be restored after loader initializes.

Return Value:

    None.

--*/

{
    PPEB Peb;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PDLL_INIT_ROUTINE InitRoutine;
    PLIST_ENTRY Next;

    Peb = NtCurrentPeb();

    if ( LdrpShutdownInProgress ) {
        return;
        }

    LdrpAllocateTls();

    Next = Peb->Ldr->InMemoryOrderModuleList.Flink;
    while (Next != &Peb->Ldr->InMemoryOrderModuleList) {
        LdrDataTableEntry
            = (PLDR_DATA_TABLE_ENTRY)
              (CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks));

        //
        // Walk through the entire list looking for
        // entries. For each entry, that has an init
        // routine, call it.
        //
        if (Peb->ImageBaseAddress != LdrDataTableEntry->DllBase) {
            if ( !(LdrDataTableEntry->Flags & LDRP_DONT_CALL_FOR_THREADS)) {
                InitRoutine = (PDLL_INIT_ROUTINE)LdrDataTableEntry->EntryPoint;
                if (InitRoutine && (LdrDataTableEntry->Flags & LDRP_PROCESS_ATTACH_CALLED) ) {
                    if (LdrDataTableEntry->Flags & LDRP_IMAGE_DLL) {
                        if ( LdrDataTableEntry->TlsIndex ) {
                            if ( !LdrpShutdownInProgress ) {
                                LdrpCallTlsInitializers(LdrDataTableEntry->DllBase,DLL_THREAD_ATTACH);
                                }
                            }

#if defined (WX86)
                        if (!Wx86ProcessInit ||
                            LdrpRunWx86DllEntryPoint(InitRoutine,
                                                    NULL,
                                                    LdrDataTableEntry->DllBase,
                                                    DLL_THREAD_ATTACH,
                                                    NULL
                                                    ) ==  STATUS_IMAGE_MACHINE_TYPE_MISMATCH)
#endif
                           {
                            if ( !LdrpShutdownInProgress ) {
                                LdrpCallInitRoutine(InitRoutine,
                                                    LdrDataTableEntry->DllBase,
                                                    DLL_THREAD_ATTACH,
                                                    NULL);
                                }
                            }
                        }
                    }
                }
            }
        Next = Next->Flink;
        }

    //
    // If the image has tls than call its initializers
    //

    if ( LdrpImageHasTls && !LdrpShutdownInProgress ) {
        LdrpCallTlsInitializers(NtCurrentPeb()->ImageBaseAddress,DLL_THREAD_ATTACH);
        }

}


NTSTATUS
LdrQueryImageFileExecutionOptions(
    IN PUNICODE_STRING ImagePathName,
    IN PWSTR OptionName,
    IN ULONG Type,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG ResultSize OPTIONAL
    )
{
    BOOLEAN bNeedToFree=FALSE;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    PWSTR pw;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    UNICODE_STRING KeyPath;
    WCHAR KeyPathBuffer[ DOS_MAX_COMPONENT_LENGTH + 100 ];
    ULONG KeyValueBuffer[ 256 ];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    ULONG AllocLength;
    ULONG ResultLength;

    KeyPath.Buffer = KeyPathBuffer;
    KeyPath.Length = 0;
    KeyPath.MaximumLength = sizeof( KeyPathBuffer );

    RtlAppendUnicodeToString( &KeyPath,
        L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\"
        );

    UnicodeString = *ImagePathName;
    pw = (PWSTR)((PCHAR)UnicodeString.Buffer + UnicodeString.Length);
    UnicodeString.MaximumLength = UnicodeString.Length;
    while (UnicodeString.Length != 0) {
        if (pw[ -1 ] == OBJ_NAME_PATH_SEPARATOR) {
            break;
        }
        pw--;
        UnicodeString.Length -= sizeof( *pw );
    }
    UnicodeString.Buffer = pw;
    UnicodeString.Length = UnicodeString.MaximumLength - UnicodeString.Length;

    RtlAppendUnicodeStringToString( &KeyPath, &UnicodeString );

    InitializeObjectAttributes( &ObjectAttributes,
        &KeyPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenKey( &KeyHandle,
        GENERIC_READ,
        &ObjectAttributes
        );

    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    RtlInitUnicodeString( &UnicodeString, OptionName );
    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)&KeyValueBuffer;
    Status = NtQueryValueKey( KeyHandle,
        &UnicodeString,
        KeyValuePartialInformation,
        KeyValueInformation,
        sizeof( KeyValueBuffer ),
        &ResultLength
        );
    if (Status == STATUS_BUFFER_OVERFLOW) {
        //
        // This function can be called before the process heap gets created 
        // therefore we need to protect against this case. The majority of the 
        // code will not hit this code path because they read just strings
        // containing hex numbers and for this the size of KeyValueBuffer is 
        // more than sufficient.
        //

        if (RtlProcessHeap()) {

            AllocLength = sizeof( *KeyValueInformation ) +
                KeyValueInformation->DataLength;
            KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)
            RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TEMP_TAG ), AllocLength);

            if (KeyValueInformation == NULL) {
                Status = STATUS_NO_MEMORY;
            }
            else {
                bNeedToFree = TRUE;
                Status = NtQueryValueKey( KeyHandle,
                    &UnicodeString,
                    KeyValuePartialInformation,
                    KeyValueInformation,
                    AllocLength,
                    &ResultLength
                    );
            }
        }
        else {

            Status = STATUS_NO_MEMORY;
        }
    }

    if (NT_SUCCESS( Status )) {
        if (KeyValueInformation->Type == REG_BINARY) {
            if ((Buffer) &&
                (KeyValueInformation->DataLength <= BufferSize)) {
                RtlMoveMemory( Buffer, &KeyValueInformation->Data, KeyValueInformation->DataLength);
            }
            else {
                Status = STATUS_BUFFER_OVERFLOW;
            }
            if (ARGUMENT_PRESENT( ResultSize )) {
                *ResultSize = KeyValueInformation->DataLength;
            }
        }
        else if (KeyValueInformation->Type == REG_DWORD) {

            if (Type != REG_DWORD) {
                Status = STATUS_OBJECT_TYPE_MISMATCH;
            }
            else {
                if ((Buffer)
                    && (BufferSize == sizeof(ULONG))
                    && (KeyValueInformation->DataLength == BufferSize)) {

                    RtlMoveMemory( Buffer, &KeyValueInformation->Data, KeyValueInformation->DataLength);
                }
                else {
                    Status = STATUS_BUFFER_OVERFLOW;
                }

                if (ARGUMENT_PRESENT( ResultSize )) {
                    *ResultSize = KeyValueInformation->DataLength;
                }
            }
        }
        else if (KeyValueInformation->Type != REG_SZ) {
            Status = STATUS_OBJECT_TYPE_MISMATCH;
        }
        else {
            if (Type == REG_DWORD) {
                if (BufferSize != sizeof( ULONG )) {
                    BufferSize = 0;
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else {
                    UnicodeString.Buffer = (PWSTR)&KeyValueInformation->Data;
                    UnicodeString.Length = (USHORT)
                    (KeyValueInformation->DataLength - sizeof( UNICODE_NULL ));
                    UnicodeString.MaximumLength = (USHORT)KeyValueInformation->DataLength;
                    Status = RtlUnicodeStringToInteger( &UnicodeString, 0, (PULONG)Buffer );
                }
            }
            else {
                if (KeyValueInformation->DataLength > BufferSize) {
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                else {
                    BufferSize = KeyValueInformation->DataLength;
                }

                RtlMoveMemory( Buffer, &KeyValueInformation->Data, BufferSize );
            }

            if (ARGUMENT_PRESENT( ResultSize )) {
                *ResultSize = BufferSize;
            }
        }
    }

    if (bNeedToFree)
        RtlFreeHeap(RtlProcessHeap(), 0, KeyValueInformation);
    NtClose( KeyHandle );
    return Status;
}


NTSTATUS
LdrpInitializeTls(
        VOID
        )
{
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY Head,Next;
    PIMAGE_TLS_DIRECTORY TlsImage;
    PLDRP_TLS_ENTRY TlsEntry;
    ULONG TlsSize;
    BOOLEAN FirstTimeThru = TRUE;

    InitializeListHead(&LdrpTlsList);

    //
    // Walk through the loaded modules an look for TLS. If we find TLS,
    // lock in the module and add to the TLS chain.
    //

    Head = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    Next = Head->Flink;

    while ( Next != Head ) {
        Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        Next = Next->Flink;

        TlsImage = (PIMAGE_TLS_DIRECTORY)RtlImageDirectoryEntryToData(
                           Entry->DllBase,
                           TRUE,
                           IMAGE_DIRECTORY_ENTRY_TLS,
                           &TlsSize
                           );

        //
        // mark whether or not the image file has TLS
        //

        if ( FirstTimeThru ) {
            FirstTimeThru = FALSE;
            if ( TlsImage && !LdrpImageHasTls) {
                RtlpSerializeHeap( RtlProcessHeap() );
                LdrpImageHasTls = TRUE;
                }
            }

        if ( TlsImage ) {
            if (ShowSnaps) {
                DbgPrint( "LDR: Tls Found in %wZ at %lx\n",
                            &Entry->BaseDllName,
                            TlsImage
                        );
                }

            TlsEntry = RtlAllocateHeap(RtlProcessHeap(),MAKE_TAG( TLS_TAG ),sizeof(*TlsEntry));
            if ( !TlsEntry ) {
                return STATUS_NO_MEMORY;
                }

            //
            // Since this DLL has TLS, lock it in
            //

            Entry->LoadCount = (USHORT)0xffff;

            //
            // Mark this as having thread local storage
            //

            Entry->TlsIndex = (USHORT)0xffff;

            TlsEntry->Tls = *TlsImage;
            InsertTailList(&LdrpTlsList,&TlsEntry->Links);

            //
            // Update the index for this dll's thread local storage
            //


            *(PLONG)TlsEntry->Tls.AddressOfIndex = LdrpNumberOfTlsEntries;
            TlsEntry->Tls.Characteristics = LdrpNumberOfTlsEntries++;
            }
        }

    //
    // We now have walked through all static DLLs and know
    // all DLLs that reference thread local storage. Now we
    // just have to allocate the thread local storage for the current
    // thread and for all subsequent threads
    //

    return LdrpAllocateTls();
}

NTSTATUS
LdrpAllocateTls(
    VOID
    )
{
    PTEB Teb;
    PLIST_ENTRY Head, Next;
    PLDRP_TLS_ENTRY TlsEntry;
    PVOID *TlsVector;

    Teb = NtCurrentTeb();

    //
    // Allocate the array of thread local storage pointers
    //

    if ( LdrpNumberOfTlsEntries ) {
        TlsVector = RtlAllocateHeap(RtlProcessHeap(),MAKE_TAG( TLS_TAG ),sizeof(PVOID)*LdrpNumberOfTlsEntries);
        if ( !TlsVector ) {
            return STATUS_NO_MEMORY;
            }

        Teb->ThreadLocalStoragePointer = TlsVector;
        Head = &LdrpTlsList;
        Next = Head->Flink;

        while ( Next != Head ) {
            TlsEntry = CONTAINING_RECORD(Next, LDRP_TLS_ENTRY, Links);
            Next = Next->Flink;
            TlsVector[TlsEntry->Tls.Characteristics] = RtlAllocateHeap(
                                                        RtlProcessHeap(),
                                                        MAKE_TAG( TLS_TAG ),
                                                        TlsEntry->Tls.EndAddressOfRawData - TlsEntry->Tls.StartAddressOfRawData
                                                        );
            if (!TlsVector[TlsEntry->Tls.Characteristics] ) {
                return STATUS_NO_MEMORY;
                }

            if (ShowSnaps) {
                DbgPrint("LDR: TlsVector %x Index %d = %x copied from %x to %x\n",
                    TlsVector,
                    TlsEntry->Tls.Characteristics,
                    &TlsVector[TlsEntry->Tls.Characteristics],
                    TlsEntry->Tls.StartAddressOfRawData,
                    TlsVector[TlsEntry->Tls.Characteristics]
                    );
                }

            RtlCopyMemory(
                TlsVector[TlsEntry->Tls.Characteristics],
                (PVOID)TlsEntry->Tls.StartAddressOfRawData,
                TlsEntry->Tls.EndAddressOfRawData - TlsEntry->Tls.StartAddressOfRawData
                );

            //
            // Do the TLS Callouts
            //

            }
        }
    return STATUS_SUCCESS;
}

VOID
LdrpFreeTls(
    VOID
    )
{
    PTEB Teb;
    PLIST_ENTRY Head, Next;
    PLDRP_TLS_ENTRY TlsEntry;
    PVOID *TlsVector;

    Teb = NtCurrentTeb();

    TlsVector = Teb->ThreadLocalStoragePointer;

    if ( TlsVector ) {
        Head = &LdrpTlsList;
        Next = Head->Flink;

        while ( Next != Head ) {
            TlsEntry = CONTAINING_RECORD(Next, LDRP_TLS_ENTRY, Links);
            Next = Next->Flink;

            //
            // Do the TLS callouts
            //

            if ( TlsVector[TlsEntry->Tls.Characteristics] ) {
                RtlFreeHeap(
                    RtlProcessHeap(),
                    0,
                    TlsVector[TlsEntry->Tls.Characteristics]
                    );

                }
            }

        RtlFreeHeap(
            RtlProcessHeap(),
            0,
            TlsVector
            );
        }
}

VOID
LdrpCallTlsInitializers(
    PVOID DllBase,
    ULONG Reason
    )
{
    PIMAGE_TLS_DIRECTORY TlsImage;
    ULONG TlsSize;
    PIMAGE_TLS_CALLBACK *CallBackArray;
    PIMAGE_TLS_CALLBACK InitRoutine;

    TlsImage = (PIMAGE_TLS_DIRECTORY)RtlImageDirectoryEntryToData(
                       DllBase,
                       TRUE,
                       IMAGE_DIRECTORY_ENTRY_TLS,
                       &TlsSize
                       );


    try {
        if ( TlsImage ) {
            CallBackArray = (PIMAGE_TLS_CALLBACK *)TlsImage->AddressOfCallBacks;
            if ( CallBackArray ) {
                if (ShowSnaps) {
                    DbgPrint( "LDR: Tls Callbacks Found. Imagebase %lx Tls %lx CallBacks %lx\n",
                                DllBase,
                                TlsImage,
                                CallBackArray
                            );
                    }

                while(*CallBackArray){
                    InitRoutine = *CallBackArray++;

                    if (ShowSnaps) {
                        DbgPrint( "LDR: Calling Tls Callback Imagebase %lx Function %lx\n",
                                    DllBase,
                                    InitRoutine
                                );
                        }

#if defined (WX86)
                    if (!Wx86ProcessInit ||
                        LdrpRunWx86DllEntryPoint(
                             (PDLL_INIT_ROUTINE)InitRoutine,
                             NULL,
                             DllBase,
                             Reason,
                             NULL
                             ) ==  STATUS_IMAGE_MACHINE_TYPE_MISMATCH)
#endif
                       {
                        LdrpCallInitRoutine((PDLL_INIT_ROUTINE)InitRoutine,
                                            DllBase,
                                            Reason,
                                            0);
                        }

                    }
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        ;
        }
}



ULONG GetNextCommaValue( IN OUT WCHAR **p, IN OUT ULONG *len )
{
    ULONG Number = 0;

    while (*len && (UNICODE_NULL != **p) && **p != L',')
    {
        // Let's ignore spaces
        if ( L' ' != **p )
        {
            Number = (Number * 10) + ( (ULONG)**p - L'0' );
        }

        (*p)++;
        (*len)--;
    }

    //
    // If we're at a comma, get past it for the next call
    //
    if ( L',' == **p )
    {
        (*p)++;
        (*len)--;
    }

    return Number;
}



VOID
LdrQueryApplicationCompatibilityGoo(
    IN PUNICODE_STRING UnicodeImageName
    )

/*++

Routine Description:

    This function is called by LdrpInitialize after its initialized the
    process.  It's purpose is to query any application specific flags,
    hacks, etc.  If any app specific information is found, its hung off
    the PEB for other components to test against.

    Besides setting hanging the AppCompatInfo struct off the PEB, the
    only other action that will occur in here is setting OS version
    numbers in the PEB if the appropriate Version lie app flag is set.

Arguments:

    UnicodeImageName - Actual image name (including path)

Return Value:

    None.

--*/

{

    PPEB Peb;
    PVOID ResourceInfo;
    ULONG TotalGooLength;
    ULONG AppCompatLength;
    ULONG ResultSize;
    ULONG ResourceSize;
    ULONG InputCompareLength;
    ULONG OutputCompareLength;
    LANGID LangId;
    NTSTATUS st;
    BOOLEAN bImageContainsVersionResourceInfo;
    ULONG_PTR IdPath[3];
    APP_COMPAT_GOO LocalAppCompatGoo;
    PAPP_COMPAT_GOO AppCompatGoo;
    PAPP_COMPAT_INFO AppCompatInfo;
    PAPP_VARIABLE_INFO AppVariableInfo;
    PPRE_APP_COMPAT_INFO AppCompatEntry;
    PIMAGE_RESOURCE_DATA_ENTRY DataEntry;
    PEFFICIENTOSVERSIONINFOEXW OSVerInfo;
    UNICODE_STRING EnvName;
    UNICODE_STRING EnvValue;
    WCHAR *NewCSDString;
    WCHAR TempString[ 128 ];   // is the size of szCSDVersion in OSVERSIONINFOW
    BOOLEAN fNewCSDVersionBuffer = FALSE;

    struct {
        USHORT TotalSize;
        USHORT DataSize;
        USHORT Type;
        WCHAR Name[16];              // L"VS_VERSION_INFO" + unicode nul
    } *Resource;


    //
    // Check execution options to see if there's any Goo for this app.
    // We purposely feed a small struct to LdrQueryImageFileExecOptions,
    // so that it can come back with success/failure, and if success we see
    // how much we need to alloc.  As the results coming back will be of
    // variable length.
    //
    Peb = NtCurrentPeb();
    Peb->AppCompatInfo = NULL;
    RtlInitUnicodeString(&Peb->CSDVersion, NULL);

    st = LdrQueryImageFileExecutionOptions( UnicodeImageName,
                                            L"ApplicationGoo",
                                            REG_BINARY,
                                            &LocalAppCompatGoo,
                                            sizeof(APP_COMPAT_GOO),
                                            &ResultSize
                                          );

    //
    // If there's an entry there, we're guaranteed to get overflow error.
    //
    if (st == STATUS_BUFFER_OVERFLOW) {

        //
        // Something is there, alloc memory for the "Pre" Goo struct right now
        //
        AppCompatGoo = \
            RtlAllocateHeap(Peb->ProcessHeap, HEAP_ZERO_MEMORY, ResultSize);

        if (!AppCompatGoo) {
            return;
        }

        //
        // Now that we've got the memory, hit it again
        //
        st = LdrQueryImageFileExecutionOptions( UnicodeImageName,
                                                L"ApplicationGoo",
                                                REG_BINARY,
                                                AppCompatGoo,
                                                ResultSize,
                                                &ResultSize
                                              );

        if (!NT_SUCCESS( st )) {
            RtlFreeHeap(Peb->ProcessHeap, 0, AppCompatGoo);
            return;
        }

        //
        // Got a hit on this key, however we don't know fer sure that its
        // an exact match.  There could be multiple App Compat entries
        // within this Goo.  So we get the version resource information out
        // of the Image hdr (if avail) and later we compare it against all of
        // the entries found within the Goo hoping for a match.
        //
        // Need Language Id in order to query the resource info
        //
        bImageContainsVersionResourceInfo = FALSE;
//        NtQueryDefaultUILanguage(&LangId);
        IdPath[0] = 16;                             // RT_VERSION
        IdPath[1] = 1;                              // VS_VERSION_INFO
        IdPath[2] = 0; // LangId;

        //
        // Search for version resource information
        //
        try {
            st = LdrpSearchResourceSection_U(
                    Peb->ImageBaseAddress,
                    IdPath,
                    3,
                    FALSE,
                    FALSE,   // TRUE,
                    &DataEntry
                    );

        } except(EXCEPTION_EXECUTE_HANDLER) {
            st = STATUS_UNSUCCESSFUL;
        }

        if (NT_SUCCESS( st )) {

            //
            // Give us a pointer to the resource information
            //
            try {
                st = LdrpAccessResourceData(
                        Peb->ImageBaseAddress,
                        DataEntry,
                        &Resource,
                        &ResourceSize
                        );

            } except(EXCEPTION_EXECUTE_HANDLER) {
                st = STATUS_UNSUCCESSFUL;
            }

            if (NT_SUCCESS( st )) {
                bImageContainsVersionResourceInfo = TRUE;
            }

        }

        //
        // Now that we either have (or have not) the version resource info,
        // bounce down each app compat entry looking for a match.  If there
        // wasn't any version resource info in the image hdr, its going to be
        // an automatic match to an entry that also doesn't have anything for
        // its version resource info.  Obviously there can be only one of these
        // "empty" entries within the Goo (as the first one will always be
        // matched first.
        //
        st = STATUS_SUCCESS;
        AppCompatEntry = AppCompatGoo->AppCompatEntry;
        TotalGooLength = \
            AppCompatGoo->dwTotalGooSize - sizeof(AppCompatGoo->dwTotalGooSize);
        while (TotalGooLength) {

            try {

                //
                // Compare what we're told to by the resource info size.  The
                // ResourceInfo (if avail) is directly behind the AppCompatEntry
                //
                InputCompareLength = AppCompatEntry->dwResourceInfoSize;
                ResourceInfo = AppCompatEntry + 1;
                if (bImageContainsVersionResourceInfo) {

                    if (InputCompareLength > Resource->TotalSize) {
                        InputCompareLength = Resource->TotalSize;
                    }

                    OutputCompareLength = \
                        (ULONG)RtlCompareMemory(
                            ResourceInfo,
                            Resource,
                            InputCompareLength
                            );

                }

                //
                // In this case, we don't have any version resource info in
                // the image header, so set OutputCompareLength to zero.
                // If InputCompareLength was set to zero (above) due to the
                // AppCompatEntry also having no version resource info, then
                // the test will succeed (below) and we've found our match.
                // Otherwise, this is not the same app and it won't be a match.
                //
                else {
                    OutputCompareLength = 0;
                }

            } except(EXCEPTION_EXECUTE_HANDLER) {
                st = STATUS_UNSUCCESSFUL;
            }

            if ((!NT_SUCCESS( st )) ||
                (InputCompareLength != OutputCompareLength)) {

                //
                // Wasn't a match for some reason or another, goto next entry
                //
                TotalGooLength -= AppCompatEntry->dwEntryTotalSize;
                (PUCHAR) AppCompatEntry += AppCompatEntry->dwEntryTotalSize;
                continue;
            }

            //
            // We're a match!!!  Now we have to create the final "Post"
            // app compat structure that will be used by everyone to follow.
            // This guy hangs off the Peb and it doesn't have the resource
            // info still lying around in there.
            //
            AppCompatLength = AppCompatEntry->dwEntryTotalSize;
            AppCompatLength -= AppCompatEntry->dwResourceInfoSize;
            Peb->AppCompatInfo = \
                RtlAllocateHeap(Peb->ProcessHeap, HEAP_ZERO_MEMORY, AppCompatLength);

            if (!Peb->AppCompatInfo) {
                break;
            }

            AppCompatInfo = Peb->AppCompatInfo;
            AppCompatInfo->dwTotalSize = AppCompatLength;

            //
            // Copy what was beyond the resource info to near the top starting at
            // the Application compat flags.
            //
            RtlMoveMemory(
                &AppCompatInfo->CompatibilityFlags,
                (PUCHAR) ResourceInfo + AppCompatEntry->dwResourceInfoSize,
                AppCompatInfo->dwTotalSize - FIELD_OFFSET(APP_COMPAT_INFO, CompatibilityFlags)
                );

            //
            // Now that we've created the "Post" app compat info struct to be
            // used by everyone, we need to check if version lying for this
            // app is requested.  If so, we need to stuff the Peb right now.
            //
            if (AppCompatInfo->CompatibilityFlags.QuadPart & KACF_VERSIONLIE) {

                //
                // Find the variable version lie struct somwhere within
                //
                if( STATUS_SUCCESS != LdrFindAppCompatVariableInfo(AVT_OSVERSIONINFO, &AppVariableInfo)) {
                    break;
                }

                //
                // The variable length information itself comes at the end
                // of the normal struct and could be of any aribitrary length
                //
                AppVariableInfo++;
                OSVerInfo = (PEFFICIENTOSVERSIONINFOEXW) AppVariableInfo;
                Peb->OSMajorVersion = OSVerInfo->dwMajorVersion;
                Peb->OSMinorVersion = OSVerInfo->dwMinorVersion;
                Peb->OSBuildNumber = (USHORT) OSVerInfo->dwBuildNumber;
                Peb->OSCSDVersion = (OSVerInfo->wServicePackMajor << 8) & 0xFF00;
                Peb->OSCSDVersion |= OSVerInfo->wServicePackMinor;
                Peb->OSPlatformId = OSVerInfo->dwPlatformId;

                Peb->CSDVersion.Length = (USHORT)wcslen(&OSVerInfo->szCSDVersion[0])*sizeof(WCHAR);
                Peb->CSDVersion.MaximumLength = Peb->CSDVersion.Length + sizeof(WCHAR);
                Peb->CSDVersion.Buffer = \
                    RtlAllocateHeap(
                        Peb->ProcessHeap,
                        HEAP_ZERO_MEMORY,
                        Peb->CSDVersion.MaximumLength
                        );

                if (!Peb->CSDVersion.Buffer) {
                    break;
                }
                wcscpy(Peb->CSDVersion.Buffer, &OSVerInfo->szCSDVersion[0]);
                fNewCSDVersionBuffer = TRUE;

            }

            break;

        }

        RtlFreeHeap(Peb->ProcessHeap, 0, AppCompatGoo);
    }


    //
    // Only look at the ENV stuff if haven't already gotten new version info from the registry
    //
    if ( FALSE == fNewCSDVersionBuffer )
    {
        //
        // The format of this string is:
        // _COMPAT_VER_NNN = MajOSVer, MinOSVer, OSBldNum, MajCSD, MinCSD, PlatformID, CSDString
        //  eg:  _COMPAT_VER_NNN=4,0,1381,3,0,2,Service Pack 3
        //   (for NT 4 SP3)

        RtlInitUnicodeString(&EnvName, L"_COMPAT_VER_NNN");

        EnvValue.Buffer = TempString;
        EnvValue.Length = 0;
        EnvValue.MaximumLength = sizeof(TempString);


        st = RtlQueryEnvironmentVariable_U(
            NULL,
            &EnvName,
            &EnvValue
            );

        //
        // One of the possible error codes is BUFFER_TOO_SMALL - this indicates a
        // string that's wacko - they should not be larger than the size we define/expect
        // In this case, we'll ignore that string
        //
        if ( STATUS_SUCCESS == st )
        {
            WCHAR *p = EnvValue.Buffer;
            WCHAR *NewSPString;
            ULONG len = EnvValue.Length / sizeof(WCHAR);  // (Length is bytes, not chars)

            //
            // Ok, someone wants different version info.
            //
            Peb->OSMajorVersion = GetNextCommaValue( &p, &len );
            Peb->OSMinorVersion = GetNextCommaValue( &p, &len );
            Peb->OSBuildNumber = (USHORT)GetNextCommaValue( &p, &len );
            Peb->OSCSDVersion = (USHORT)(GetNextCommaValue( &p, &len )) << 8;
            Peb->OSCSDVersion |= (USHORT)GetNextCommaValue( &p, &len );
            Peb->OSPlatformId = GetNextCommaValue( &p, &len );


            //
            // Need to free the old buffer if there is one...
            //
            if ( fNewCSDVersionBuffer )
            {
                RtlFreeHeap( Peb->ProcessHeap, 0, Peb->CSDVersion.Buffer );
                Peb->CSDVersion.Buffer = NULL;
            }

            if ( len )
            {
                NewCSDString = \
                        RtlAllocateHeap(
                            Peb->ProcessHeap,
                            HEAP_ZERO_MEMORY,
                            ( len + 1 ) * sizeof(WCHAR)
                            );

                if ( NULL == NewCSDString )
                {
                    return;
                }

                //
                // Now copy the string to memory that we'll keep
                //
                // We do a movemem here rather than a string copy because current comments in
                // RtlQueryEnvironmentVariable() indicate that in an edge case, we might not
                // have a trailing NULL - berniem 7/7/99
                //
                RtlMoveMemory( NewCSDString, p, len * sizeof(WCHAR) );

            }
            else
            {
                NewCSDString = NULL;
            }

            RtlInitUnicodeString( &(Peb->CSDVersion), NewCSDString );

        }
    }

    return;
}


NTSTATUS
LdrFindAppCompatVariableInfo(
    IN  ULONG dwTypeSeeking,
    OUT PAPP_VARIABLE_INFO *AppVariableInfo
    )

/*++

Routine Description:

    This function is used to find a variable length struct by its type.
    The caller specifies what type its looking for and this function chews
    thru all the variable length structs to find it.  If it does it returns
    the pointer and TRUE, else FALSE.

Arguments:

    dwTypeSeeking - AVT that you are looking for

    AppVariableInfo - pointer to pointer of variable info to be returned

Return Value:

    TRUE or FALSE if entry is found

--*/

{
    PPEB Peb;
    ULONG TotalSize;
    ULONG CurOffset;
    PAPP_VARIABLE_INFO pCurrentEntry;

    Peb = NtCurrentPeb();
    if (Peb->AppCompatInfo) {

        //
        // Since we're not dealing with a fixed-size structure, TotalSize
        // will keep us from running off the end of the data list
        //
        TotalSize = ((PAPP_COMPAT_INFO) Peb->AppCompatInfo)->dwTotalSize;

        //
        // The first variable structure (if there is one) will start
        // immediately after the fixed stuff
        //
        CurOffset = sizeof(APP_COMPAT_INFO);

        while (CurOffset < TotalSize) {

            pCurrentEntry = (PAPP_VARIABLE_INFO) ((PUCHAR)(Peb->AppCompatInfo) + CurOffset);

            //
            // Have we found what we're looking for?
            //
            if (dwTypeSeeking == pCurrentEntry->dwVariableType) {
                *AppVariableInfo = pCurrentEntry;
                return (STATUS_SUCCESS);
            }

            //
            // Let's go look at the next blob
            //
            CurOffset += (ULONG)(pCurrentEntry->dwVariableInfoSize);
        }

    }

    return (STATUS_NOT_FOUND);
}
