/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

   sysload.c

Abstract:

    This module contains the code to load DLLs into the system portion of
    the address space and calls the DLL at its initialization entry point.

Author:

    Lou Perazzoli 21-May-1991
    Landy Wang 02-June-1997

Revision History:

--*/

#include "mi.h"

//
// The LDR_DATA_TABLE_ENTRY->LoadedImports is used as a list of imported DLLs.
//
// This field is zero if the module was loaded at boot time and the
// import information was never filled in.
//
// This field is -1 if no imports are defined by the module.
//
// This field contains a valid paged pool PLDR_DATA_TABLE_ENTRY pointer
// with a low-order (bit 0) tag of 1 if there is only 1 usable import needed
// by this driver.
//
// This field will contain a valid paged pool PLOAD_IMPORTS pointer in all
// other cases (ie: where at least 2 imports exist).
//

typedef struct _LOAD_IMPORTS {
    SIZE_T                  Count;
    PLDR_DATA_TABLE_ENTRY   Entry[1];
} LOAD_IMPORTS, *PLOAD_IMPORTS;

#define LOADED_AT_BOOT  ((PLOAD_IMPORTS)0)
#define NO_IMPORTS_USED ((PLOAD_IMPORTS)-2)

#define SINGLE_ENTRY(ImportVoid)    ((ULONG)((ULONG_PTR)(ImportVoid) & 0x1))

#define SINGLE_ENTRY_TO_POINTER(ImportVoid)    ((PLDR_DATA_TABLE_ENTRY)((ULONG_PTR)(ImportVoid) & ~0x1))

#define POINTER_TO_SINGLE_ENTRY(Pointer)    ((PLDR_DATA_TABLE_ENTRY)((ULONG_PTR)(Pointer) | 0x1))

KMUTANT MmSystemLoadLock;

ULONG MmTotalSystemDriverPages;

ULONG MmDriverCommit;

BOOLEAN MiFirstDriverLoadEver = TRUE;

//
// This key is set to TRUE to make more memory below 16mb available for drivers.
// It can be cleared via the registry.
//

LOGICAL MmMakeLowMemory = TRUE;

//
// Enabled via the registry to identify drivers which unload without releasing
// resources or still have active timers, etc.
//

PUNLOADED_DRIVERS MiUnloadedDrivers;

ULONG MiLastUnloadedDriver;
ULONG MiTotalUnloads;
ULONG MiUnloadsSkipped;

//
// This can be set by the registry.
//

ULONG MmEnforceWriteProtection = 1;

//
// Referenced by ke\bugcheck.c.
//

PVOID ExPoolCodeStart;
PVOID ExPoolCodeEnd;
PVOID MmPoolCodeStart;
PVOID MmPoolCodeEnd;
PVOID MmPteCodeStart;
PVOID MmPteCodeEnd;

//
// ****** temporary ******
//
// Define reference to external spin lock.
//
// ****** temporary ******
//

extern KSPIN_LOCK PsLoadedModuleSpinLock;

ULONG
LdrDoubleRelocateImage (
    IN PVOID NewBase,
    IN PVOID CurrentBase,
    IN PUCHAR LoaderName,
    IN ULONG Success,
    IN ULONG Conflict,
    IN ULONG Invalid
    );

#if DBG
PFN_NUMBER MiPagesConsumed;
#endif

ULONG
CacheImageSymbols(
    IN PVOID ImageBase
    );

NTSTATUS
MiResolveImageReferences(
    PVOID ImageBase,
    IN PUNICODE_STRING ImageFileDirectory,
    IN PUNICODE_STRING NamePrefix OPTIONAL,
    IN BOOLEAN LoadInSessionSpace,
    OUT PCHAR *MissingProcedureName,
    OUT PWSTR *MissingDriverName,
    OUT PLOAD_IMPORTS *LoadedImports
    );

NTSTATUS
MiSnapThunk(
    IN PVOID DllBase,
    IN PVOID ImageBase,
    IN PIMAGE_THUNK_DATA NameThunk,
    OUT PIMAGE_THUNK_DATA AddrThunk,
    IN PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    IN ULONG ExportSize,
    IN BOOLEAN SnapForwarder,
    OUT PCHAR *MissingProcedureName
    );

NTSTATUS
MiLoadImageSection (
    IN PSECTION SectionPointer,
    OUT PVOID *ImageBase,
    IN PUNICODE_STRING ImageFileName,
    IN BOOLEAN LoadInSessionSpace
    );

VOID
MiEnablePagingOfDriver (
    IN PVOID ImageHandle
    );

VOID
MiSetPagingOfDriver (
    IN PMMPTE PointerPte,
    IN PMMPTE LastPte,
    IN BOOLEAN SessionSpace
    );

PVOID
MiLookupImageSectionByName (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN PCHAR SectionName,
    OUT PULONG SectionSize
    );

VOID
MiClearImports(
    IN PLDR_DATA_TABLE_ENTRY DataTableEntry
    );

NTSTATUS
MiBuildImportsForBootDrivers(
    VOID
    );

NTSTATUS
MmCheckSystemImage(
    IN HANDLE ImageFileHandle
    );

LONG
MiMapCacheExceptionFilter (
    OUT PNTSTATUS Status,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );

ULONG
MiSetProtectionOnTransitionPte (
    IN PMMPTE PointerPte,
    IN ULONG ProtectionMask
    );

NTSTATUS
MiDereferenceImports (
    IN PLOAD_IMPORTS ImportList
    );

LOGICAL
MiCallDllUnloadAndUnloadDll(
    IN PLDR_DATA_TABLE_ENTRY DataTableEntry
    );

PVOID
MiLocateExportName (
    IN PVOID DllBase,
    IN PCHAR FunctionName
    );

NTSTATUS
MiLoadSystemImage (
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING NamePrefix OPTIONAL,
    IN PUNICODE_STRING LoadedBaseName OPTIONAL,
    IN BOOLEAN LoadInSessionSpace,
    OUT PVOID *ImageHandle,
    OUT PVOID *ImageBaseAddress,
    IN BOOLEAN LockDownPages
    );

VOID
MiRememberUnloadedDriver (
    IN PUNICODE_STRING DriverName,
    IN PVOID Address,
    IN ULONG Length
    );

VOID
MiWriteProtectSystemImage (
    IN PVOID DllBase
    );

VOID
MiLocateKernelSections (
    IN PLDR_DATA_TABLE_ENTRY DataTableEntry
    );

VOID
MiUpdateThunks (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PVOID OldAddress,
    IN PVOID NewAddress,
    IN ULONG NumberOfBytes
    );

NTSTATUS
MiCheckPageFilePath (
    PFILE_OBJECT FileObject
    );

PVOID
MiFindExportedRoutineByName(
    IN PLDR_DATA_TABLE_ENTRY DataTableEntry,
    IN PANSI_STRING AnsiImageRoutineName
    );

extern LOGICAL MiNoLowMemory;

VOID
MiRemoveLowPages (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MmCheckSystemImage)
#pragma alloc_text(PAGE,MmLoadSystemImage)
#pragma alloc_text(PAGE,MmLoadAndLockSystemImage)
#pragma alloc_text(PAGE,MiLoadSystemImage)
#pragma alloc_text(PAGE,MiResolveImageReferences)
#pragma alloc_text(PAGE,MiSnapThunk)
#pragma alloc_text(PAGE,MiEnablePagingOfDriver)
#pragma alloc_text(PAGE,MmPageEntireDriver)
#pragma alloc_text(PAGE,MiSetImageProtect)
#pragma alloc_text(PAGE,MiDereferenceImports)
#pragma alloc_text(PAGE,MiCallDllUnloadAndUnloadDll)
#pragma alloc_text(PAGE,MiLocateExportName)
#pragma alloc_text(PAGE,MiClearImports)
#pragma alloc_text(PAGE,MiWriteProtectSystemImage)
#pragma alloc_text(PAGE,MmGetSystemRoutineAddress)
#pragma alloc_text(PAGE,MiFindExportedRoutineByName)
#pragma alloc_text(INIT,MiBuildImportsForBootDrivers)
#pragma alloc_text(INIT,MiReloadBootLoadedDrivers)
#pragma alloc_text(INIT,MiUpdateThunks)
#pragma alloc_text(INIT,MiInitializeLoadedModuleList)
#pragma alloc_text(INIT,MiLocateKernelSections)
#pragma alloc_text(INIT,MmCallDllInitialize)

#if !defined(NT_UP)
#pragma alloc_text(PAGE,MmVerifyImageIsOkForMpUse)
#endif // NT_UP

#pragma alloc_text(PAGELK,MiLoadImageSection)
#pragma alloc_text(PAGELK,MmFreeDriverInitialization)
#pragma alloc_text(PAGELK,MmUnloadSystemImage)
#pragma alloc_text(PAGELK,MiRememberUnloadedDriver)
#pragma alloc_text(PAGELK,MiSetPagingOfDriver)
#pragma alloc_text(PAGELK,MmResetDriverPaging)
#endif

CHAR MiPteStr[] = "\0";


NTSTATUS
MmLoadSystemImage (
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING NamePrefix OPTIONAL,
    IN PUNICODE_STRING LoadedBaseName OPTIONAL,
    IN BOOLEAN LoadInSessionSpace,
    OUT PVOID *ImageHandle,
    OUT PVOID *ImageBaseAddress
    )

/*++

Routine Description:

    This routine reads the image pages from the specified section into
    the system and returns the address of the DLL's header.

    At successful completion, the Section is referenced so it remains
    until the system image is unloaded.

Arguments:

    ImageName - Supplies the Unicode name of the image to load.

    ImageFileName - Supplies the full path name (including the image name)
                    of the image to load.

    NamePrefix - Supplies the prefix to use with the image name on load
                 operations.  This is used to load the same image multiple
                 times, by using different prefixes

    LoadedBaseName - If present, supplies the base name to use on the
                     loaded image instead of the base name found on the
                     image name.

    LoadInSessionSpace - Supplies whether to load this image in session space -
                     ie: each session will have a different copy of this driver
                     with pages shared as much as possible via copy on write.

    ImageHandle - Returns an opaque pointer to the referenced section object
                  of the image that was loaded.

    ImageBaseAddress - Returns the image base within the system.

Return Value:

    Status of the load operation.

--*/

{
    PAGED_CODE();

    return MiLoadSystemImage (
        ImageFileName,
        NamePrefix,
        LoadedBaseName,
        LoadInSessionSpace,
        ImageHandle,
        ImageBaseAddress,
        FALSE
        );
}

NTSTATUS
MmLoadAndLockSystemImage (
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING NamePrefix OPTIONAL,
    IN PUNICODE_STRING LoadedBaseName OPTIONAL,
    OUT PVOID *ImageHandle,
    OUT PVOID *ImageBaseAddress
    )

/*++

Routine Description:

    This routine reads the image pages from the specified section into
    the system and returns the address of the DLL's header.  Very similar
    to MmLoadSystemImage, except that it also locks down the driver pages.
    This is needed for things like the dump driver because we cannot page it
    back in after the system has crashed (when we want to write the system
    dump to the pagefile).

    At successful completion, the Section is referenced so it remains
    until the system image is unloaded.

Arguments:

    ImageName - Supplies the Unicode name of the image to load.

    ImageFileName - Supplies the full path name (including the image name)
                    of the image to load.

    NamePrefix - Supplies the prefix to use with the image name on load
                 operations.  This is used to load the same image multiple
                 times, by using different prefixes

    LoadedBaseName - If present, supplies the base name to use on the
                     loaded image instead of the base name found on the
                     image name.

    ImageHandle - Returns an opaque pointer to the referenced section object
                  of the image that was loaded.

    ImageBaseAddress - Returns the image base within the system.

Return Value:

    Status of the load operation.

--*/

{
    PAGED_CODE();

    return MiLoadSystemImage (
        ImageFileName,
        NamePrefix,
        LoadedBaseName,
        FALSE,
        ImageHandle,
        ImageBaseAddress,
        TRUE
        );
}


NTSTATUS
MiLoadSystemImage (
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING NamePrefix OPTIONAL,
    IN PUNICODE_STRING LoadedBaseName OPTIONAL,
    IN BOOLEAN LoadInSessionSpace,
    OUT PVOID *ImageHandle,
    OUT PVOID *ImageBaseAddress,
    IN BOOLEAN LockDownPages
    )

/*++

Routine Description:

    This routine reads the image pages from the specified section into
    the system and returns the address of the DLL's header.

    At successful completion, the Section is referenced so it remains
    until the system image is unloaded.

Arguments:

    ImageFileName - Supplies the full path name (including the image name)
                    of the image to load.

    NamePrefix - If present, supplies the prefix to use with the image name on
                 load operations.  This is used to load the same image multiple
                 times, by using different prefixes.

    LoadedBaseName - If present, supplies the base name to use on the
                     loaded image instead of the base name found on the
                     image name.

    LoadInSessionSpace - Supplies whether to load this image in session space.
                         Each session gets a different copy of this driver with
                         pages shared as much as possible via copy on write.

    ImageHandle - Returns an opaque pointer to the referenced section object
                  of the image that was loaded.

    ImageBaseAddress - Returns the image base within the system.

    LockDownPages - Supplies TRUE if the image pages should be made nonpagable.

Return Value:

    Status of the load operation.

--*/

{
    KIRQL OldIrql;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    LDR_DATA_TABLE_ENTRY TempDataTableEntry;
    NTSTATUS Status;
    PSECTION SectionPointer;
    PIMAGE_NT_HEADERS NtHeaders;
    UNICODE_STRING PrefixedImageName;
    UNICODE_STRING BaseName;
    UNICODE_STRING BaseDirectory;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE FileHandle;
    HANDLE SectionHandle;
    IO_STATUS_BLOCK IoStatus;
    CHAR NameBuffer[ MAXIMUM_FILENAME_LENGTH ];
    PLIST_ENTRY NextEntry;
    ULONG NumberOfPtes;
    ULONG_PTR ViewSize;
    PCHAR MissingProcedureName;
    PWSTR MissingDriverName;
    PLOAD_IMPORTS LoadedImports;
    PMMSESSION Session;
    BOOLEAN AlreadyOpen;
    BOOLEAN IssueUnloadOnFailure;
    ULONG SectionAccess;
    volatile PMMPTE PointerPpe;
    MMPTE PpeContents;

    PAGED_CODE();

    LoadedImports = (PLOAD_IMPORTS)NO_IMPORTS_USED;
    SectionPointer = (PVOID)-1;
    FileHandle = (HANDLE)0;
    MissingProcedureName = NULL;
    MissingDriverName = NULL;
    IssueUnloadOnFailure = FALSE;

    if (LoadInSessionSpace == TRUE) {

        ASSERT (NamePrefix == NULL);
        ASSERT (LoadedBaseName == NULL);

        if (MiHydra == TRUE) {

            if (PsGetCurrentProcess()->Vm.u.Flags.ProcessInSession == 0) {
#if DBG
                DbgPrint ("MiLoadSystemImage: no session space!\n");
#endif
                return STATUS_NO_MEMORY;
            }

            ASSERT (MmIsAddressValid (MmSessionSpace) == TRUE);

            Session = &MmSessionSpace->Session;
        }
        else {
            Session = &MmSession;
            LoadInSessionSpace = FALSE;
        }
    }
    else {
        Session = &MmSession;
    }

    //
    // Get name roots
    //

    if (ImageFileName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR) {
        PWCHAR p;
        ULONG l;

        p = &ImageFileName->Buffer[ImageFileName->Length>>1];
        while (*(p-1) != OBJ_NAME_PATH_SEPARATOR) {
            p--;
        }
        l = (ULONG)(&ImageFileName->Buffer[ImageFileName->Length>>1] - p);
        l *= sizeof(WCHAR);
        BaseName.Length = (USHORT)l;
        BaseName.Buffer = p;
    } else {
        BaseName.Length = ImageFileName->Length;
        BaseName.Buffer = ImageFileName->Buffer;
    }

    BaseName.MaximumLength = BaseName.Length;
    BaseDirectory = *ImageFileName;
    BaseDirectory.Length -= BaseName.Length;
    BaseDirectory.MaximumLength = BaseDirectory.Length;
    PrefixedImageName = *ImageFileName;

    //
    // If there's a name prefix, add it to the PrefixedImageName.
    //

    if (NamePrefix) {
        PrefixedImageName.MaximumLength =
                BaseDirectory.Length + NamePrefix->Length + BaseName.Length;

        PrefixedImageName.Buffer = ExAllocatePoolWithTag (
                                    NonPagedPool,
                                    PrefixedImageName.MaximumLength,
                                    'dLmM'
                                    );

        if (!PrefixedImageName.Buffer) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        PrefixedImageName.Length = 0;
        RtlAppendUnicodeStringToString(&PrefixedImageName, &BaseDirectory);
        RtlAppendUnicodeStringToString(&PrefixedImageName, NamePrefix);
        RtlAppendUnicodeStringToString(&PrefixedImageName, &BaseName);

        //
        // Alter the basename to match
        //

        BaseName.Buffer = PrefixedImageName.Buffer + BaseDirectory.Length / sizeof(WCHAR);
        BaseName.Length += NamePrefix->Length;
        BaseName.MaximumLength += NamePrefix->Length;
    }

    //
    // If there's a loaded base name, use it instead of the base name.
    //

    if (LoadedBaseName) {
        BaseName = *LoadedBaseName;
    }

#if DBG
    if ( NtGlobalFlag & FLG_SHOW_LDR_SNAPS ) {
        DbgPrint( "MM:SYSLDR Loading %wZ (%wZ) %s\n",
            &PrefixedImageName,
            &BaseName,
            LoadInSessionSpace ? "in session space" : " ");
    }
#endif

    AlreadyOpen = FALSE;

    //
    // Arbitrary process context so prevent suspend APCs now.
    //

    KeEnterCriticalRegion();

    KeWaitForSingleObject (&MmSystemLoadLock,
                           WrVirtualMemory,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)NULL);

    //
    // Check to see if this name already exists in the loader database.
    //

    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList) {

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        if (RtlEqualUnicodeString (&PrefixedImageName,
                                   &DataTableEntry->FullDllName,
                                   TRUE)) {

            if (LoadInSessionSpace == TRUE) {

                if (MI_IS_SESSION_ADDRESS (DataTableEntry->DllBase) == FALSE) {

                    //
                    // The caller is trying to load a driver in session space
                    // that has already been loaded in system space.  This is
                    // not allowed.
                    //

                    Status = STATUS_CONFLICTING_ADDRESSES;
                    goto return2;
                }

                AlreadyOpen = TRUE;

                //
                // The LoadCount should generally not be 0 here, but it is
                // possible in the case where an attempt has been made to
                // unload a DLL on last dereference, but the DLL refused to
                // unload.
                //

                DataTableEntry->LoadCount += 1;
                SectionPointer = DataTableEntry->SectionPointer;
                break;
            }
            else {
                if (MI_IS_SESSION_ADDRESS (DataTableEntry->DllBase) == TRUE) {

                    //
                    // The caller is trying to load a driver in systemwide space
                    // that has already been loaded in session space.  This is
                    // not allowed.
                    //

                    Status = STATUS_CONFLICTING_ADDRESSES;
                    goto return2;
                }
            }

            *ImageHandle = DataTableEntry;
            *ImageBaseAddress = DataTableEntry->DllBase;
            Status = STATUS_IMAGE_ALREADY_LOADED;
            goto return2;
        }

        NextEntry = NextEntry->Flink;
    }

    ASSERT (AlreadyOpen == TRUE || NextEntry == &PsLoadedModuleList);

    if (AlreadyOpen == FALSE) {

        DataTableEntry = NULL;

        //
        // Attempt to open the driver image itself.  If this fails, then the
        // driver image cannot be located, so nothing else matters.
        //

        InitializeObjectAttributes (&ObjectAttributes,
                                    ImageFileName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL );

        Status = ZwOpenFile (&FileHandle,
                             FILE_EXECUTE,
                             &ObjectAttributes,
                             &IoStatus,
                             FILE_SHARE_READ | FILE_SHARE_DELETE,
                             0 );

        if (!NT_SUCCESS(Status)) {

#if DBG
            if (NtGlobalFlag & FLG_SHOW_LDR_SNAPS) {
                DbgPrint ("MiLoadImageSection: cannot open %wZ\n",
                    ImageFileName);
            }
#endif
            //
            // Don't raise hard error status for file not found.
            //

            goto return2;
        }

        Status = MmCheckSystemImage(FileHandle);
        if ((Status == STATUS_IMAGE_CHECKSUM_MISMATCH) ||
            (Status == STATUS_IMAGE_MP_UP_MISMATCH) ||
            (Status == STATUS_INVALID_IMAGE_PROTECT)) {
            goto return1;
        }

        //
        // Now attempt to create an image section for the file.  If this fails,
        // then the driver file is not an image.  Session space drivers are
        // shared text with copy on write data, so don't allow writes here.
        //

        if (LoadInSessionSpace == TRUE) {
            SectionAccess = SECTION_MAP_READ | SECTION_MAP_EXECUTE;
        }
        else {
            SectionAccess = SECTION_ALL_ACCESS;
        }

        Status = ZwCreateSection (&SectionHandle,
                                  SectionAccess,
                                  (POBJECT_ATTRIBUTES) NULL,
                                  (PLARGE_INTEGER) NULL,
                                  PAGE_EXECUTE,
                                  SEC_IMAGE,
                                  FileHandle );

        if (!NT_SUCCESS(Status)) {
            goto return1;
        }

        //
        // Now reference the section handle.
        //

        Status = ObReferenceObjectByHandle (SectionHandle,
                                        SECTION_MAP_EXECUTE,
                                        MmSectionObjectType,
                                        KernelMode,
                                        (PVOID *) &SectionPointer,
                                        (POBJECT_HANDLE_INFORMATION) NULL );

        ZwClose (SectionHandle);
        if (!NT_SUCCESS (Status)) {
            goto return1;
        }

        if (SectionPointer->Segment->ControlArea->NumberOfSubsections == 1) {
            if ((LoadInSessionSpace == FALSE) &&
                (SectionPointer->Segment->BasedAddress != (PVOID)Session->SystemSpaceViewStart)) {

                PSECTION SectionPointer2;

                //
                // The driver was linked with subsection alignment such that
                // it is mapped with one subsection.  Since the CreateSection
                // above guarantees that the driver image is indeed a
                // satisfactory executable, map it directly now to reuse the
                // cache from the MmCheckSystemImage call above.
                //

                Status = ZwCreateSection (&SectionHandle,
                                          SectionAccess,
                                          (POBJECT_ATTRIBUTES) NULL,
                                          (PLARGE_INTEGER) NULL,
                                          PAGE_EXECUTE,
                                          SEC_COMMIT,
                                          FileHandle );

                if (NT_SUCCESS(Status)) {

                    Status = ObReferenceObjectByHandle (
                                            SectionHandle,
                                            SECTION_MAP_EXECUTE,
                                            MmSectionObjectType,
                                            KernelMode,
                                            (PVOID *) &SectionPointer2,
                                            (POBJECT_HANDLE_INFORMATION) NULL );

                    ZwClose (SectionHandle);

                    if (NT_SUCCESS (Status)) {

                        //
                        // The number of PTEs won't match if the image is
                        // stripped and the debug directory crosses the last
                        // sector boundary of the file.  We could still use the
                        // new section, but these cases are under 2% of all the
                        // drivers loaded so don't bother.
                        //

                        if (SectionPointer->Segment->TotalNumberOfPtes == SectionPointer2->Segment->TotalNumberOfPtes) {
                            ObDereferenceObject (SectionPointer);
                            SectionPointer = SectionPointer2;
                        }
                        else {
                            ObDereferenceObject (SectionPointer2);
                        }
                    }
                }
            }
        }

    }

    if ((LoadInSessionSpace == FALSE) &&
        (SectionPointer->Segment->BasedAddress == (PVOID)Session->SystemSpaceViewStart) &&
        (SectionPointer->Segment->ControlArea->NumberOfMappedViews == 0)) {
        NumberOfPtes = 0;
        ViewSize = 0;

        Status = MmMapViewInSystemSpace (SectionPointer,
                                         ImageBaseAddress,
                                         &ViewSize);
        if ((NT_SUCCESS( Status ) &&
            (*ImageBaseAddress == SectionPointer->Segment->BasedAddress))) {

            SectionPointer->Segment->SystemImageBase = *ImageBaseAddress;
            SectionPointer->Segment->ControlArea->u.Flags.ImageMappedInSystemSpace = 1;
            NumberOfPtes = (ULONG)((ViewSize + 1) >> PAGE_SHIFT);
            MiSetImageProtect(SectionPointer->Segment, MM_EXECUTE_READWRITE);

#if defined (_AXP64_)

            //
            // This is needed because win32k calls are made from the system
            // process on non-Hydra systems.  On Hydra systems, these calls
            // are always made from the relevant session's csrss process.
            // If these were the same, this PPE duplication could be removed.
            //

            PointerPpe = MiGetPpeAddress (*ImageBaseAddress);
            ASSERT (PointerPpe->u.Long != 0);
            PpeContents = *PointerPpe;
            KeAttachProcess (&PsInitialSystemProcess->Pcb);
            ASSERT (PointerPpe->u.Long == 0);
            *PointerPpe = PpeContents;
            KeDetachProcess();
#endif
            goto BindImage;
        }
    }

    MmLockPagableSectionByHandle (ExPageLockHandle);

    //
    // Load the driver from the filesystem and pick a virtual address for it.
    // For Hydra, this means also allocating session virtual space, and
    // after mapping a view of the image, either copying or sharing the
    // driver's code and data in the session virtual space.
    //
    // If it is a share map because the image was loaded at its based address,
    // the disk image will remain busy.
    //

    Status = MiLoadImageSection (SectionPointer,
                                 ImageBaseAddress,
                                 ImageFileName,
                                 LoadInSessionSpace);

    MmUnlockPagableImageSection (ExPageLockHandle);

    NumberOfPtes = SectionPointer->Segment->TotalNumberOfPtes;

    if (Status == STATUS_ALREADY_COMMITTED) {

        //
        // This is a driver that was relocated that is being loaded into
        // the same session space twice.  Don't increment the overall load
        // count - just the image count in the session which has already been
        // done.
        //

        ASSERT (MiHydra == TRUE);
        ASSERT (AlreadyOpen == TRUE);
        ASSERT (LoadInSessionSpace == TRUE);
        ASSERT (DataTableEntry != NULL);
        ASSERT (DataTableEntry->LoadCount > 1);

        *ImageHandle = DataTableEntry;
        *ImageBaseAddress = DataTableEntry->DllBase;

        DataTableEntry->LoadCount -= 1;
        Status = STATUS_SUCCESS;
        goto return1;
    }

    if (MiFirstDriverLoadEver == TRUE) {

        NTSTATUS PagingPathStatus;

        //
        // Check with all of the drivers along the path to win32k.sys to
        // ensure that they are willing to follow the rules required
        // of them and to give them a chance to lock down code and data
        // that needs to be locked.  If any of the drivers along the path
        // refuses to participate, fail the win32k.sys load.
        //

        //
        // It is assumed that all drivers live on the same physical drive, so
        // when the very first driver is loaded, this check can be made.
        // This eliminates the need to check for things like relocated win32ks,
        // Terminal Server systems, etc.
        //

        PagingPathStatus = MiCheckPageFilePath (SectionPointer->Segment->ControlArea->FilePointer);

        if (!NT_SUCCESS(PagingPathStatus)) {

            KdPrint(( "MiCheckPageFilePath FAILED for win32k.sys: %x\n",
                PagingPathStatus ));

            //
            // BUGBUG: Failing the insertion of win32k.sys' device in the
            // pagefile path is commented out until the storage drivers have
            // been modified to correctly handle this request.  Add code
            // here to release relevant resources for the error path.
            //
        }

        MiFirstDriverLoadEver = FALSE;
    }

    //
    // Normal drivers are dereferenced here and their images can then be
    // overwritten.  This is ok because we've already read the whole thing
    // into memory and from here until reboot (or unload), we back them
    // with the pagefile.
    //
    // win32k.sys and session space drivers are the exception - these images
    // are inpaged from the filesystem and we need to keep our reference to
    // the file so that it doesn't get overwritten.
    //

    if (LoadInSessionSpace == FALSE) {
        ObDereferenceObject (SectionPointer);
        SectionPointer = (PVOID)-1;
    }

    //
    // The module LoadCount will be 1 here if the module was just loaded.
    // The LoadCount will be >1 if it was attached to by a session (as opposed
    // to just loaded).
    //

    if (!NT_SUCCESS(Status)) {
        if (AlreadyOpen == TRUE) {

            //
            // We're failing and we were just attaching to an already loaded
            // driver.  We don't want to go through the forced unload path
            // because we've already deleted the address space.  Simply
            // decrement our reference and null the DataTableEntry
            // so we don't go through the forced unload path.
            //

            ASSERT (DataTableEntry != NULL);
            DataTableEntry->LoadCount -= 1;
            DataTableEntry = NULL;
        }
        goto return1;
    }

    //
    // Error recovery from this point out for sessions works as follows:
    //
    // For sessions, we may or may not have a DataTableEntry at this point.
    // If we do, it's because we're attaching to a driver that has already
    // been loaded - and the DataTableEntry->LoadCount has been bumped - so
    // the error recovery from here on out is to just call
    // MmUnloadSystemImage with the DataTableEntry.
    //
    // If this is the first load of a given driver into a session space, we
    // have no DataTableEntry at this point.  The view has already been mapped
    // and committed and the group/session addresses reserved for this DLL.
    // The error recovery path handles all this because
    // MmUnloadSystemImage will zero the relevant fields in the
    // LDR_DATA_TABLE_ENTRY so that MmUnloadSystemImage will work properly.
    //

    IssueUnloadOnFailure = TRUE;

    if (LoadInSessionSpace == FALSE || *ImageBaseAddress != SectionPointer->Segment->BasedAddress) {

#if DBG

        //
        // Warn users about session images that cannot be shared
        // because they were linked at a bad address.
        //

        if (LoadInSessionSpace == TRUE && MmSessionSpace->SessionId) {
            DbgPrint ("MM: Session %d image %wZ is linked at a nonsharable address (%p)\n",
                    MmSessionSpace->SessionId,
                    ImageFileName,
                    SectionPointer->Segment->BasedAddress);
            DbgPrint ("MM: Image %wZ has been moved to address (%p) by the system so it can run,\n",
                    ImageFileName,
                    *ImageBaseAddress);
            DbgPrint (" but this needs to be fixed in the image for sharing to occur.\n");
        }
#endif

        //
        // Apply the fixups to the section and resolve its image references.
        //

        try {
            Status = (NTSTATUS)LdrRelocateImage(*ImageBaseAddress,
                                                "SYSLDR",
                                                (ULONG)STATUS_SUCCESS,
                                                (ULONG)STATUS_CONFLICTING_ADDRESSES,
                                                (ULONG)STATUS_INVALID_IMAGE_FORMAT
                                                );
        } except (EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
            KdPrint(("MM:sysload - LdrRelocateImage failed status %lx\n",
                      Status));
        }

        if (!NT_SUCCESS(Status)) {

            //
            // Unload the system image and dereference the section.
            //

            goto return1;
        }
    }

BindImage:

    try {
        MissingProcedureName = NameBuffer;

        //
        // Resolving the image references results in other DLLs being
        // loaded if they are referenced by the module that was just loaded.
        // An example is when an OEM printer or FAX driver links with
        // other general libraries.  This is not a problem for session space
        // because the general libraries do not have the global data issues
        // that win32k.sys and the video drivers do.  So we just call the
        // standard kernel reference resolver and any referenced libraries
        // get loaded into system global space.  Code in the routine
        // restricts which libraries can be referenced by a driver.
        //

        Status = MiResolveImageReferences(*ImageBaseAddress,
                                          &BaseDirectory,
                                          NamePrefix,
                                          FALSE,
                                          &MissingProcedureName,
                                          &MissingDriverName,
                                          &LoadedImports
                                         );
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        KdPrint(("MM:sysload - ResolveImageReferences failed status %x\n",
                    Status));
    }

    if (!NT_SUCCESS(Status)) {
#if DBG
        if (Status == STATUS_DRIVER_ORDINAL_NOT_FOUND ||
            Status == STATUS_DRIVER_ENTRYPOINT_NOT_FOUND) {

            if ((ULONG_PTR)MissingProcedureName & ~((ULONG_PTR) (X64K-1))) {
                //
                // If not an ordinal, print string
                //
                DbgPrint("MissingProcedureName %s\n", MissingProcedureName);
            }
            else {
                DbgPrint("MissingProcedureName 0x%p\n", MissingProcedureName);
            }

            if (MissingDriverName) {
                DbgPrint("MissingDriverName %ws\n", MissingDriverName);
            }
        }
#endif
        goto return1;
    }

#if DBG
    if (NtGlobalFlag & FLG_SHOW_LDR_SNAPS) {
        KdPrint (("MM:loaded driver - consumed %ld. pages\n",MiPagesConsumed));
    }
#endif

    if (AlreadyOpen == FALSE) {

        //
        // Since there was no loaded module list entry for this driver, create
        // one now.
        //

#if DBG
        NextEntry = PsLoadedModuleList.Flink;
        while (NextEntry != &PsLoadedModuleList) {
            DataTableEntry = CONTAINING_RECORD(NextEntry,
                                               LDR_DATA_TABLE_ENTRY,
                                               InLoadOrderLinks);

            if (RtlEqualUnicodeString (ImageFileName,
                                       &DataTableEntry->FullDllName,
                                       TRUE)) {

                DbgPrint("MM SYSLDR: Image already loaded! RefCount %x\n",
                    DataTableEntry->LoadCount);
                DbgBreakPoint();
            }

            NextEntry = NextEntry->Flink;
        }
#endif

        //
        // Allocate a data table entry for structured exception handling.
        //

        DataTableEntry = ExAllocatePoolWithTag (NonPagedPool,
                                                sizeof(LDR_DATA_TABLE_ENTRY),
                                                'dLmM');

        if (DataTableEntry == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto return1;
        }

        DataTableEntry->BaseDllName.Buffer = ExAllocatePoolWithTag (
                                        NonPagedPool,
                                        BaseName.Length + sizeof(UNICODE_NULL),
                                        'dLmM');

        if (DataTableEntry->BaseDllName.Buffer == NULL) {
            ExFreePool (DataTableEntry);
            DataTableEntry = NULL;
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto return1;
        }

        //
        // Initialize the address of the DLL image file header and the entry
        // point address.
        //

        NtHeaders = RtlImageNtHeader(*ImageBaseAddress);

        DataTableEntry->DllBase = *ImageBaseAddress;
        DataTableEntry->EntryPoint =
            ((PCHAR)*ImageBaseAddress + NtHeaders->OptionalHeader.AddressOfEntryPoint);
        DataTableEntry->SizeOfImage = NumberOfPtes << PAGE_SHIFT;
        DataTableEntry->CheckSum = NtHeaders->OptionalHeader.CheckSum;
        DataTableEntry->SectionPointer = (PVOID)SectionPointer;

        //
        // Store the DLL name.
        //

        DataTableEntry->BaseDllName.Length = BaseName.Length;
        DataTableEntry->BaseDllName.MaximumLength = BaseName.Length;
        RtlMoveMemory (DataTableEntry->BaseDllName.Buffer,
                       BaseName.Buffer,
                       BaseName.Length );
        DataTableEntry->BaseDllName.Buffer[BaseName.Length/sizeof(WCHAR)] = UNICODE_NULL;

        DataTableEntry->FullDllName.Buffer = ExAllocatePoolWithTag (PagedPool,
                                                         PrefixedImageName.Length + sizeof(UNICODE_NULL),
                                                         'TDmM');
        if (DataTableEntry->FullDllName.Buffer == NULL) {

            //
            // Pool could not be allocated, just set the length to 0.
            //

            DataTableEntry->FullDllName.Length = 0;
            DataTableEntry->FullDllName.MaximumLength = 0;
        } else {
            DataTableEntry->FullDllName.Length = PrefixedImageName.Length;
            DataTableEntry->FullDllName.MaximumLength = PrefixedImageName.Length;
            RtlMoveMemory (DataTableEntry->FullDllName.Buffer,
                           PrefixedImageName.Buffer,
                           PrefixedImageName.Length);
            DataTableEntry->FullDllName.Buffer[PrefixedImageName.Length/sizeof(WCHAR)] = UNICODE_NULL;
        }

        PERFINFO_IMAGE_LOAD(DataTableEntry);

        //
        // Initialize the flags, load count, and insert the data table entry
        // in the loaded module list.
        //

        DataTableEntry->Flags = LDRP_ENTRY_PROCESSED | LDRP_SYSTEM_MAPPED;
        DataTableEntry->LoadCount = 1;
        DataTableEntry->LoadedImports = (PVOID)LoadedImports;

        if ((NtHeaders->OptionalHeader.MajorOperatingSystemVersion >= 5) &&
            (NtHeaders->OptionalHeader.MajorImageVersion >= 5)) {
            DataTableEntry->Flags |= LDRP_ENTRY_NATIVE;
        }

        MiApplyDriverVerifier (DataTableEntry, NULL);

        MiWriteProtectSystemImage (DataTableEntry->DllBase);

        if (PsImageNotifyEnabled) {
            IMAGE_INFO ImageInfo;

            ImageInfo.Properties = 0;
            ImageInfo.ImageAddressingMode = IMAGE_ADDRESSING_MODE_32BIT;
            ImageInfo.SystemModeImage = TRUE;
            ImageInfo.ImageSize = DataTableEntry->SizeOfImage;
            ImageInfo.ImageBase = *ImageBaseAddress;
            ImageInfo.ImageSelector = 0;
            ImageInfo.ImageSectionNumber = 0;

            PsCallImageNotifyRoutines(ImageFileName, (HANDLE)NULL, &ImageInfo);
        }

        //
        // Acquire the loaded module list resource and insert this entry
        // into the list.
        //

        KeEnterCriticalRegion();
        ExAcquireResourceExclusive (&PsLoadedModuleResource, TRUE);

        ExAcquireSpinLock (&PsLoadedModuleSpinLock, &OldIrql);

        InsertTailList(&PsLoadedModuleList, &DataTableEntry->InLoadOrderLinks);

        ExReleaseSpinLock (&PsLoadedModuleSpinLock, OldIrql);

        ExReleaseResource (&PsLoadedModuleResource);
        KeLeaveCriticalRegion();

        if (CacheImageSymbols (*ImageBaseAddress)) {

            //
            //  TEMP TEMP TEMP rip out when debugger converted
            //

            ANSI_STRING AnsiName;
            UNICODE_STRING UnicodeName;

            //
            //  \SystemRoot is 11 characters in length
            //
            if (PrefixedImageName.Length > (11 * sizeof( WCHAR )) &&
                !_wcsnicmp( PrefixedImageName.Buffer, L"\\SystemRoot", 11 )
               ) {
                UnicodeName = PrefixedImageName;
                UnicodeName.Buffer += 11;
                UnicodeName.Length -= (11 * sizeof( WCHAR ));
                sprintf (NameBuffer, "%ws%wZ", &SharedUserData->NtSystemRoot[2], &UnicodeName);
            } else {
                sprintf (NameBuffer, "%wZ", &BaseName);
            }
            RtlInitString (&AnsiName, NameBuffer);
            DbgLoadImageSymbols (&AnsiName,
                                 *ImageBaseAddress,
                                 (ULONG_PTR) -1
                               );
            DataTableEntry->Flags |= LDRP_DEBUG_SYMBOLS_LOADED;
        }
    }

    //
    // Flush the instruction cache on all systems in the configuration.
    //

    KeSweepIcache (TRUE);
    *ImageHandle = DataTableEntry;
    Status = STATUS_SUCCESS;

    if (LoadInSessionSpace == TRUE) {
        MmPageEntireDriver (DataTableEntry->EntryPoint);
    }
    else if (SectionPointer == (PVOID)-1 && LockDownPages == FALSE) {
        MiEnablePagingOfDriver (DataTableEntry);
    }

return1:

    if (!NT_SUCCESS(Status)) {

#if DBG

        //
        // Only way we should fail after we get a valid DataTableEntry is if
        // we're Hydra loading a 2nd instance of a driver.
        //

        if (DataTableEntry) {
            ASSERT (MiHydra == TRUE && AlreadyOpen == TRUE && DataTableEntry->LoadCount > 1);
        }
#endif

        if (AlreadyOpen == FALSE && SectionPointer != (PVOID)-1) {

            //
            // This is needed for failed win32k.sys loads or any Hydra session's
            // load of the first instance of a driver.
            //

            ObDereferenceObject (SectionPointer);
        }

        if (IssueUnloadOnFailure == TRUE) {
            
            if (DataTableEntry == NULL) {
                RtlZeroMemory (&TempDataTableEntry, sizeof(LDR_DATA_TABLE_ENTRY));
            
                DataTableEntry = &TempDataTableEntry;
        
                DataTableEntry->DllBase = *ImageBaseAddress;
                DataTableEntry->SizeOfImage = NumberOfPtes << PAGE_SHIFT;
                DataTableEntry->LoadCount = 1;
                DataTableEntry->LoadedImports = LoadedImports;
            }
#if DBG
            else {
        
                //
                // If DataTableEntry is NULL, then we are unloading before one
                // got created.  Once a LDR_DATA_TABLE_ENTRY is created, the
                // load cannot fail, so if exists here, at least one other
                // session contains this image as well.
                //
        
                ASSERT (MiHydra == TRUE);
                ASSERT (DataTableEntry->LoadCount > 1);
            }
#endif
            
            MmUnloadSystemImage ((PVOID)DataTableEntry);
        }
    }

    if (FileHandle) {
        ZwClose (FileHandle);
    }
    if (!NT_SUCCESS(Status)) {

        ULONG_PTR ErrorParameters[ 3 ];
        ULONG NumberOfParameters;
        ULONG UnicodeStringParameterMask;
        ULONG ErrorResponse;
        ANSI_STRING AnsiString;
        UNICODE_STRING ProcedureName;
        UNICODE_STRING DriverName;

        //
        // Hard error time.  A driver could not be loaded.
        //

        KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
        KeLeaveCriticalRegion();
        ErrorParameters[ 0 ] = (ULONG_PTR)ImageFileName;
        NumberOfParameters = 1;
        UnicodeStringParameterMask = 1;

        RtlInitUnicodeString( &ProcedureName, NULL );
        if (Status == STATUS_DRIVER_ORDINAL_NOT_FOUND ||
            Status == STATUS_DRIVER_ENTRYPOINT_NOT_FOUND ||
            Status == STATUS_PROCEDURE_NOT_FOUND
           ) {
            NumberOfParameters = 3;
            UnicodeStringParameterMask = 0x5;
            RtlInitUnicodeString( &DriverName, MissingDriverName );
            ErrorParameters[ 2 ] = (ULONG_PTR)&DriverName;

            if ((ULONG_PTR)MissingProcedureName & ~((ULONG_PTR) (X64K-1))) {
                //
                // If not an ordinal, pass as a Unicode string
                //

                RtlInitAnsiString( &AnsiString, MissingProcedureName );
                RtlAnsiStringToUnicodeString( &ProcedureName, &AnsiString, TRUE );
                ErrorParameters[ 1 ] = (ULONG_PTR)&ProcedureName;
                UnicodeStringParameterMask |= 0x2;
            } else {
                //
                // Just pass ordinal values as is.
                //

                ErrorParameters[ 1 ] = (ULONG_PTR)MissingProcedureName;
            }
        } else {
            NumberOfParameters = 2;
            ErrorParameters[ 1 ] = (ULONG)Status;
            Status = STATUS_DRIVER_UNABLE_TO_LOAD;
            }

        ZwRaiseHardError (Status,
                          NumberOfParameters,
                          UnicodeStringParameterMask,
                          ErrorParameters,
                          OptionOkNoWait,
                          &ErrorResponse);

        if (ProcedureName.Buffer != NULL) {
            RtlFreeUnicodeString( &ProcedureName );
        }
        return Status;
    }

return2:
    if (NamePrefix) {
        ExFreePool (PrefixedImageName.Buffer);
    }

    KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
    KeLeaveCriticalRegion();
    return Status;
}


NTSTATUS
MiLoadImageSection (
    IN PSECTION SectionPointer,
    OUT PVOID *ImageBaseAddress,
    IN PUNICODE_STRING ImageFileName,
    IN BOOLEAN LoadInSessionSpace
    )

/*++

Routine Description:

    This routine loads the specified image into the kernel part of the
    address space.

Arguments:

    SectionPointer - Supplies the section object for the image.

    ImageBaseAddress - Returns the address that the image header is at.

    ImageFileName - Supplies the full path name (including the image name)
                    of the image to load.

    LoadInSessionSpace - Supplies whether to load this image in session space.
                         Each session gets a different copy of this driver with
                         pages shared as much as possible via copy on write.

Return Value:

    Status of the operation.

--*/

{
    PFN_NUMBER PagesRequired;
    PMMPTE ProtoPte;
    PMMPTE FirstPte;
    PMMPTE LastPte;
    PMMPTE PointerPte;
    PEPROCESS Process;
    PEPROCESS TargetProcess;
    ULONG NumberOfPtes;
    MMPTE PteContents;
    MMPTE TempPte;
    PMMPFN Pfn1;
    PFN_NUMBER PageFrameIndex;
    KIRQL OldIrql;
    PVOID UserVa;
    PVOID SystemVa;
    NTSTATUS Status;
    NTSTATUS ExceptionStatus;
    PVOID Base;
    ULONG_PTR ViewSize;
    LARGE_INTEGER SectionOffset;
    BOOLEAN LoadSymbols;
    PVOID BaseAddress;
    PFN_NUMBER CommittedPages;
    SIZE_T SectionSize;
    BOOLEAN AlreadyLoaded;
    PMM_SESSION_SPACE SessionGlobal;
    LOGICAL ProcessReferenced;
    PLIST_ENTRY NextProcessEntry;

    PAGED_CODE();

    PagesRequired = 0;

    //
    // Calculate the number of pages required to load this image.
    //

    ProtoPte = SectionPointer->Segment->PrototypePte;
    NumberOfPtes = SectionPointer->Segment->TotalNumberOfPtes;

    while (NumberOfPtes != 0) {
        PteContents = *ProtoPte;

        if ((PteContents.u.Hard.Valid == 1) ||
            (PteContents.u.Soft.Protection != MM_NOACCESS)) {
            PagesRequired += 1;
        }
        NumberOfPtes -= 1;
        ProtoPte += 1;
    }

    if (LoadInSessionSpace == TRUE) {

        ASSERT (MiHydra == TRUE);

        SectionSize = (ULONG_PTR)SectionPointer->Segment->TotalNumberOfPtes * PAGE_SIZE;

        //
        // Allocate a unique systemwide session space virtual address for
        // the driver.
        //

        Status = MiSessionWideReserveImageAddress (ImageFileName,
                                                   SectionPointer,
                                                   PAGE_SIZE,
                                                   &BaseAddress,
                                                   &AlreadyLoaded);

        if (!NT_SUCCESS(Status)) {
#if DBG
            if (NtGlobalFlag & FLG_SHOW_LDR_SNAPS) {
                DbgPrint ("MiLoadImageSection: Error 0x%x Allocating session space %p Bytes\n",Status,SectionSize);
            }
#endif
            return Status;
        }

        //
        // This is a request to load an existing driver.  This can
        // occur with printer drivers for example.
        //

        if (AlreadyLoaded == TRUE) {
            *ImageBaseAddress = BaseAddress;
            return STATUS_ALREADY_COMMITTED;
        }

#if DBG
        if (NtGlobalFlag & FLG_SHOW_LDR_SNAPS) {
            if (ImageFileName) {
                DbgPrint ("MM: MiLoadImageSection: Image %wZ, BasedAddress 0x%p, SegmentBaseAddress 0x%p, Allocated Session BaseAddress 0x%p\n",
                    ImageFileName,
                    SectionPointer->Segment->BasedAddress,
                    SectionPointer->Segment->SegmentBaseAddress,
                    BaseAddress);
            }
            else {
                DbgPrint ("MM: MiLoadImageSection: Image <NULL> BasedAddress 0x%p, SegmentBaseAddress 0x%p, Allocated Session BaseAddress 0x%p\n",
                    SectionPointer->Segment->BasedAddress,
                    SectionPointer->Segment->SegmentBaseAddress,
                    BaseAddress);
            }
        }
#endif

        if (BaseAddress == SectionPointer->Segment->BasedAddress) {

            //
            // We were able to load the image at its based address, so
            // map its image segments as backed directly by the file image.
            // All pristine pages of the image will be shared across all
            // sessions, with each page treated as copy-on-write on first write.
            //
            // NOTE: This makes the file image "busy", a different behavior
            // as normal kernel drivers are backed by the paging file only.
            //

#if DBG
            if (SectionPointer->Segment->SystemImageBase != 0) {
                ASSERT (BaseAddress == SectionPointer->Segment->SystemImageBase);
            }
#endif

            //
            // Map the image into session space.
            //

            Status = MiShareSessionImage (SectionPointer, &SectionSize);

            if (!NT_SUCCESS(Status)) {
#if DBG
                if (NtGlobalFlag & FLG_SHOW_LDR_SNAPS) {
                    DbgPrint ("MM: MiLoadImageSection: Error 0x%x Allocating session space %p Bytes\n", Status, SectionSize);
                }
#endif
                MiRemoveImageSessionWide (BaseAddress);
                return Status;
            }

            ASSERT (BaseAddress == SectionPointer->Segment->BasedAddress);

            *ImageBaseAddress = BaseAddress;

            //
            // Indicate that this section has been loaded into the system.
            //

            SectionPointer->Segment->SystemImageBase = BaseAddress;

#if DBG
            if (NtGlobalFlag & FLG_SHOW_LDR_SNAPS) {
                if (ImageFileName) {
                    DbgPrint ("MM: MiLoadImageSection: Mapped image %wZ at requested session address 0x%p\n",
                        ImageFileName,
                        BaseAddress);
                }
                else {
                    DbgPrint ("MM: MiLoadImageSection: Mapped a session image at requested address 0x%p\n",
                        BaseAddress);
                }
            }
#endif

            return Status;
        }

        //
        // The image could not be loaded at its based address.  It must be
        // copied to its new address using private page file backed pages.
        // Our caller will relocate the internal references and then bind the
        // image.  Allocate the pages and page tables for the image now.
        //

        Status = MiSessionCommitImagePages (BaseAddress, SectionSize);

        if (!NT_SUCCESS(Status)) {
#if DBG
            if (NtGlobalFlag & FLG_SHOW_LDR_SNAPS) {
                DbgPrint ("MM: MiLoadImageSection: Error 0x%x Allocating session space %p Bytes\n", Status, SectionSize);
            }
#endif
            MiRemoveImageSessionWide (BaseAddress);
            return Status;
        }
        SystemVa = BaseAddress;
    }
    else {

        //
        // See if ample pages exist to load this image.
        //

#if DBG
        MiPagesConsumed = PagesRequired;
#endif

        LOCK_PFN (OldIrql);

        if (MmResidentAvailablePages <= (SPFN_NUMBER)PagesRequired) {
            UNLOCK_PFN (OldIrql);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        MmResidentAvailablePages -= PagesRequired;
        MM_BUMP_COUNTER(14, PagesRequired);
        UNLOCK_PFN (OldIrql);

        //
        // Reserve the necessary system address space.
        //

        FirstPte = MiReserveSystemPtes (SectionPointer->Segment->TotalNumberOfPtes,
                                        SystemPteSpace,
                                        0,
                                        0,
                                        FALSE );

        if (FirstPte == NULL) {
            LOCK_PFN (OldIrql);
            MmResidentAvailablePages += PagesRequired;
            MM_BUMP_COUNTER(15, PagesRequired);
            UNLOCK_PFN (OldIrql);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        PointerPte = FirstPte;
        SystemVa = MiGetVirtualAddressMappedByPte (PointerPte);
    }

    //
    // Map a view into the user portion of the address space.
    //

    Process = PsGetCurrentProcess();

    //
    // Since callees are not always in the context of the system process,
    // attach here when necessary to guarantee the driver load occurs in a
    // known safe address space to prevent security holes.
    //

    ProcessReferenced = FALSE;
    TargetProcess = Process;
    if (Process->Peb && Process->Vm.u.Flags.SessionLeader == 0) {
        if (MiHydra == FALSE) {
            if (ExpDefaultErrorPortProcess && (Process != ExpDefaultErrorPortProcess)) {
                TargetProcess = ExpDefaultErrorPortProcess;
                KeAttachProcess (&TargetProcess->Pcb);
            }
        }
        else {

            SessionGlobal = SESSION_GLOBAL(MmSessionSpace);
            LOCK_EXPANSION (OldIrql);
            NextProcessEntry = SessionGlobal->ProcessList.Flink;

            if ((SessionGlobal->u.Flags.DeletePending == 0) &&
                (NextProcessEntry != &SessionGlobal->ProcessList)) {

                TargetProcess = CONTAINING_RECORD (NextProcessEntry,
                                                   EPROCESS,
                                                   SessionProcessLinks);

                if (Process != TargetProcess) {
                    ObReferenceObject (TargetProcess);
                    ProcessReferenced = TRUE;
                }
            }
            else {
                UNLOCK_EXPANSION (OldIrql);

                LOCK_PFN (OldIrql);
                MmResidentAvailablePages += PagesRequired;
                MM_BUMP_COUNTER(15, PagesRequired);
                UNLOCK_PFN (OldIrql);

                CommittedPages = MiDeleteSystemPagableVm (
                                          MiGetPteAddress (BaseAddress),
                                          BYTES_TO_PAGES (SectionSize),
                                          ZeroKernelPte,
                                          TRUE,
                                          NULL);
    
                LOCK_SESSION_SPACE_WS (OldIrql);
                MmSessionSpace->CommittedPages -= CommittedPages;
    
                MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_COMMIT_IMAGELOAD_FAILED1,
                    CommittedPages);
    
                UNLOCK_SESSION_SPACE_WS (OldIrql);
    
                //
                // Return the commitment we took out on the pagefile when
                // the page was allocated.  This is needed for collided images
                // since all the pages get committed regardless of writability.
                //
    
                MiRemoveImageSessionWide (BaseAddress);

                return STATUS_PROCESS_IS_TERMINATING;
            }

            UNLOCK_EXPANSION (OldIrql);

            if (Process != TargetProcess) {
                KeAttachProcess (&TargetProcess->Pcb);
            }
        }
    }

    ZERO_LARGE (SectionOffset);
    Base = NULL;
    ViewSize = 0;

    if (NtGlobalFlag & FLG_ENABLE_KDEBUG_SYMBOL_LOAD) {
        LoadSymbols = TRUE;
        NtGlobalFlag &= ~FLG_ENABLE_KDEBUG_SYMBOL_LOAD;
    }
    else {
        LoadSymbols = FALSE;
    }

    Status = MmMapViewOfSection ( SectionPointer,
                                  TargetProcess,
                                  &Base,
                                  0,
                                  0,
                                  &SectionOffset,
                                  &ViewSize,
                                  ViewUnmap,
                                  0,
                                  PAGE_EXECUTE);

    if (LoadSymbols) {
        NtGlobalFlag |= FLG_ENABLE_KDEBUG_SYMBOL_LOAD;
    }

    if (Status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH) {
        Status = STATUS_INVALID_IMAGE_FORMAT;
    }

    if (!NT_SUCCESS(Status)) {
        if (TargetProcess != Process) {
            KeDetachProcess();
            if (ProcessReferenced == TRUE) {
                ObDereferenceObject (TargetProcess);
            }
        }

        if (LoadInSessionSpace == TRUE) {

#if DBG
            if (NtGlobalFlag & FLG_SHOW_LDR_SNAPS) {
                DbgPrint ("MiLoadImageSection: Error 0x%x in session space mapping via MmMapViewOfSection\n", Status);
            }
#endif

            CommittedPages = MiDeleteSystemPagableVm (
                                      MiGetPteAddress (BaseAddress),
                                      BYTES_TO_PAGES (SectionSize),
                                      ZeroKernelPte,
                                      TRUE,
                                      NULL);

            LOCK_SESSION_SPACE_WS (OldIrql);
            MmSessionSpace->CommittedPages -= CommittedPages;

            MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_COMMIT_IMAGELOAD_FAILED1,
                CommittedPages);

            UNLOCK_SESSION_SPACE_WS (OldIrql);

            //
            // Return the commitment we took out on the pagefile when
            // the page was allocated.  This is needed for collided images
            // since all the pages get committed regardless of writability.
            //

            MiRemoveImageSessionWide (BaseAddress);
        }
        else {
            LOCK_PFN (OldIrql);
            MmResidentAvailablePages += PagesRequired;
            MM_BUMP_COUNTER(16, PagesRequired);
            UNLOCK_PFN (OldIrql);
            MiReleaseSystemPtes (FirstPte,
                                 SectionPointer->Segment->TotalNumberOfPtes,
                                 SystemPteSpace);
        }

        return Status;
    }

    //
    // Allocate a physical page(s) and copy the image data.
    // Note Hydra has already allocated the physical pages and just does
    // data copying here.
    //

    ProtoPte = SectionPointer->Segment->PrototypePte;
    NumberOfPtes = SectionPointer->Segment->TotalNumberOfPtes;

    *ImageBaseAddress = SystemVa;

    UserVa = Base;
    TempPte = ValidKernelPte;
#if defined(_IA64_)
    TempPte.u.Long |= MM_PTE_EXECUTE;
#endif

    while (NumberOfPtes != 0) {
        PteContents = *ProtoPte;
        if ((PteContents.u.Hard.Valid == 1) ||
            (PteContents.u.Soft.Protection != MM_NOACCESS)) {

            if (LoadInSessionSpace == FALSE) {
                LOCK_PFN (OldIrql);
                MiEnsureAvailablePageOrWait (NULL, NULL);
                PageFrameIndex = MiRemoveAnyPage(
                                    MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));
                PointerPte->u.Long = MM_KERNEL_DEMAND_ZERO_PTE;
#if defined(_IA64_)

                //
                // EM needs execute permission.
                //

                PointerPte->u.Soft.Protection |= MM_EXECUTE;

#endif
                MiInitializePfn (PageFrameIndex, PointerPte, 1);

                UNLOCK_PFN (OldIrql);
                TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
                MI_WRITE_VALID_PTE (PointerPte, TempPte);
                LastPte = PointerPte;

                ASSERT (MI_PFN_ELEMENT (PageFrameIndex)->u1.WsIndex == 0);
            }

            try {

                RtlMoveMemory (SystemVa, UserVa, PAGE_SIZE);

            } except (MiMapCacheExceptionFilter (&ExceptionStatus,
                                                 GetExceptionInformation())) {

                //
                // An exception occurred, unmap the view and
                // return the error to the caller.
                //

#if DBG
                DbgPrint("MiLoadImageSection: Exception 0x%x copying driver SystemVa 0x%p, UserVa 0x%p\n",ExceptionStatus,SystemVa,UserVa);
#endif

                if (LoadInSessionSpace == TRUE) {
                    ASSERT (MiHydra == TRUE);
                    CommittedPages = MiDeleteSystemPagableVm (
                                              MiGetPteAddress (BaseAddress),
                                              BYTES_TO_PAGES (SectionSize),
                                              ZeroKernelPte,
                                              TRUE,
                                              NULL);

                    LOCK_SESSION_SPACE_WS (OldIrql);
                    MmSessionSpace->CommittedPages -= CommittedPages;

                    MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_COMMIT_IMAGELOAD_FAILED2,
                        CommittedPages);

                    UNLOCK_SESSION_SPACE_WS (OldIrql);

                    //
                    // Return the commitment we took out on the pagefile when
                    // the page was allocated.  This is needed for collided
                    // images since all the pages get committed regardless
                    // of writability.
                    //

                    MiReturnCommitment (CommittedPages);
                    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_SESSION_DRIVER_LOAD_FAILURE1, CommittedPages);
                }
                else {
                    ProtoPte = FirstPte;
                    LOCK_PFN (OldIrql);
                    while (ProtoPte <= PointerPte) {
                        if (ProtoPte->u.Hard.Valid == 1) {

                            //
                            // Delete the page.
                            //

                            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (ProtoPte);

                            //
                            // Set the pointer to PTE as empty so the page
                            // is deleted when the reference count goes to zero.
                            //

                            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                            MiDecrementShareAndValidCount (Pfn1->PteFrame);
                            MI_SET_PFN_DELETED (Pfn1);
                            MiDecrementShareCountOnly (PageFrameIndex);

                            MI_WRITE_INVALID_PTE (ProtoPte, ZeroPte);
                        }
                        ProtoPte += 1;
                    }

                    MmResidentAvailablePages += PagesRequired;
                    MM_BUMP_COUNTER(17, PagesRequired);
                    UNLOCK_PFN (OldIrql);
                    MiReleaseSystemPtes (FirstPte,
                                         SectionPointer->Segment->TotalNumberOfPtes,
                                         SystemPteSpace);
                }

                Status = MmUnmapViewOfSection (TargetProcess, Base);

                ASSERT (NT_SUCCESS (Status));

                if (TargetProcess != Process) {
                    KeDetachProcess();
                    if (ProcessReferenced == TRUE) {
                        ObDereferenceObject (TargetProcess);
                    }
                }

                if (LoadInSessionSpace == TRUE) {
                    MiRemoveImageSessionWide (BaseAddress);
                }

                return ExceptionStatus;
            }

        }
        else {

            //
            // The PTE is no access - if this driver is being loaded in session
            // space we already preloaded the page so free it now.  The
            // commitment is returned when the whole image is unmapped.
            //

            if (LoadInSessionSpace == TRUE) {
                ASSERT (MiHydra == TRUE);
                CommittedPages = MiDeleteSystemPagableVm (
                                          MiGetPteAddress (SystemVa),
                                          1,
                                          ZeroKernelPte,
                                          TRUE,
                                          NULL);

                MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_COMMIT_IMAGELOAD_NOACCESS,
                    1);
            }
            else {
                MI_WRITE_INVALID_PTE (PointerPte, ZeroKernelPte);
            }
        }

        NumberOfPtes -= 1;
        ProtoPte += 1;
        PointerPte += 1;
        SystemVa = ((PCHAR)SystemVa + PAGE_SIZE);
        UserVa = ((PCHAR)UserVa + PAGE_SIZE);
    }

    Status = MmUnmapViewOfSection (TargetProcess, Base);
    ASSERT (NT_SUCCESS (Status));

    if (TargetProcess != Process) {
        KeDetachProcess();
        if (ProcessReferenced == TRUE) {
            ObDereferenceObject (TargetProcess);
        }
    }

    //
    // Indicate that this section has been loaded into the system.
    //

    SectionPointer->Segment->SystemImageBase = *ImageBaseAddress;

    //
    // Charge commitment for the number of pages that were used by
    // the driver.
    //

    if (LoadInSessionSpace == FALSE) {
        MiChargeCommitmentCantExpand (PagesRequired, TRUE);
        MM_TRACK_COMMIT (MM_DBG_COMMIT_DRIVER_PAGES, PagesRequired);
        MmDriverCommit += (ULONG)PagesRequired;
    }

    return Status;
}

VOID
MmFreeDriverInitialization (
    IN PVOID ImageHandle
    )

/*++

Routine Description:

    This routine removes the pages that relocate and debug information from
    the address space of the driver.

    NOTE:  This routine looks at the last sections defined in the image
           header and if that section is marked as DISCARDABLE in the
           characteristics, it is removed from the image.  This means
           that all discardable sections at the end of the driver are
           deleted.

Arguments:

    SectionObject - Supplies the section object for the image.

Return Value:

    None.

--*/

{
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PFN_NUMBER NumberOfPtes;
    PVOID Base;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER NtSection;
    PIMAGE_SECTION_HEADER FoundSection;
    PFN_NUMBER PagesDeleted;

    MmLockPagableSectionByHandle(ExPageLockHandle);
    DataTableEntry = (PLDR_DATA_TABLE_ENTRY)ImageHandle;
    Base = DataTableEntry->DllBase;

    ASSERT (MI_IS_SESSION_ADDRESS (Base) == FALSE);

    NumberOfPtes = DataTableEntry->SizeOfImage >> PAGE_SHIFT;
    LastPte = MiGetPteAddress (Base) + NumberOfPtes;

    NtHeaders = (PIMAGE_NT_HEADERS)RtlImageNtHeader(Base);

    NtSection = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeaders +
                        sizeof(ULONG) +
                        sizeof(IMAGE_FILE_HEADER) +
                        NtHeaders->FileHeader.SizeOfOptionalHeader
                        );

    NtSection += NtHeaders->FileHeader.NumberOfSections;

    FoundSection = NULL;
    for (i = 0; i < NtHeaders->FileHeader.NumberOfSections; i += 1) {
        NtSection -= 1;
        if ((NtSection->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) != 0) {
            FoundSection = NtSection;
        } else {

            //
            // There was a non discardable section between the this
            // section and the last non discardable section, don't
            // discard this section and don't look any more.
            //

            break;
        }
    }

    if (FoundSection != NULL) {

        PointerPte = MiGetPteAddress (ROUND_TO_PAGES (
                                    (PCHAR)Base + FoundSection->VirtualAddress));
        NumberOfPtes = (PFN_NUMBER)(LastPte - PointerPte);

        PagesDeleted = MiDeleteSystemPagableVm (PointerPte,
                                                NumberOfPtes,
                                                ZeroKernelPte,
                                                FALSE,
                                                NULL);

        MmResidentAvailablePages += PagesDeleted;
        MM_BUMP_COUNTER(18, PagesDeleted);
        MiReturnCommitment (PagesDeleted);
        MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_DRIVER_INIT_CODE, PagesDeleted);
        MmDriverCommit -= (ULONG)PagesDeleted;
#if DBG
        MiPagesConsumed -= PagesDeleted;
#endif
    }

    MmUnlockPagableImageSection(ExPageLockHandle);
    return;
}

VOID
MiEnablePagingOfDriver (
    IN PVOID ImageHandle
    )

{
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PMMPTE LastPte;
    PMMPTE PointerPte;
    PVOID Base;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER FoundSection;
    PIMAGE_OPTIONAL_HEADER OptionalHeader;

    //
    // Don't page kernel mode code if customer does not want it paged.
    //

    if (MmDisablePagingExecutive) {
        return;
    }

    //
    // If the driver has pagable code, make it paged.
    //

    DataTableEntry = (PLDR_DATA_TABLE_ENTRY)ImageHandle;
    Base = DataTableEntry->DllBase;

    NtHeaders = (PIMAGE_NT_HEADERS)RtlImageNtHeader(Base);

    OptionalHeader = (PIMAGE_OPTIONAL_HEADER)((PCHAR)NtHeaders +
#if defined (_WIN64)
                        FIELD_OFFSET (IMAGE_NT_HEADERS64, OptionalHeader));
#else
                        FIELD_OFFSET (IMAGE_NT_HEADERS32, OptionalHeader));
#endif

    FoundSection = IMAGE_FIRST_SECTION (NtHeaders);

    i = NtHeaders->FileHeader.NumberOfSections;

    PointerPte = NULL;

    while (i > 0) {
#if DBG
            if ((*(PULONG)FoundSection->Name == 'tini') ||
                (*(PULONG)FoundSection->Name == 'egap')) {
                DbgPrint("driver %wZ has lower case sections (init or pagexxx)\n",
                    &DataTableEntry->FullDllName);
            }
#endif //DBG

        //
        // Mark as pagable any section which starts with the
        // first 4 characters PAGE or .eda (for the .edata section).
        //

        if ((*(PULONG)FoundSection->Name == 'EGAP') ||
           (*(PULONG)FoundSection->Name == 'ade.')) {

            //
            // This section is pagable, save away the start and end.
            //

            if (PointerPte == NULL) {

                //
                // Previous section was NOT pagable, get the start address.
                //

                PointerPte = MiGetPteAddress (ROUND_TO_PAGES (
                                   (PCHAR)Base + FoundSection->VirtualAddress));
            }
            LastPte = MiGetPteAddress ((PCHAR)Base +
                                       FoundSection->VirtualAddress +
                                       (OptionalHeader->SectionAlignment - 1) +
                                       FoundSection->SizeOfRawData - PAGE_SIZE);

        } else {

            //
            // This section is not pagable, if the previous section was
            // pagable, enable it.
            //

            if (PointerPte != NULL) {
                MiSetPagingOfDriver (PointerPte, LastPte, FALSE);
                PointerPte = NULL;
            }
        }
        i -= 1;
        FoundSection += 1;
    }
    if (PointerPte != NULL) {
        MiSetPagingOfDriver (PointerPte, LastPte, FALSE);
    }
}


VOID
MiSetPagingOfDriver (
    IN PMMPTE PointerPte,
    IN PMMPTE LastPte,
    IN BOOLEAN SessionSpace
    )

/*++

Routine Description:

    This routine marks the specified range of PTEs as pagable.

Arguments:

    PointerPte - Supplies the starting PTE.

    LastPte - Supplies the ending PTE.

Return Value:

    None.

--*/

{
    PVOID Base;
    PFN_NUMBER PageCount;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn;
    MMPTE TempPte;
    MMPTE PreviousPte;
    KIRQL OldIrql1;
    KIRQL OldIrql;

    PAGED_CODE ();

    ASSERT ((SessionSpace == FALSE) ||
            (MmIsAddressValid(MmSessionSpace) == TRUE));

    if (MI_IS_PHYSICAL_ADDRESS(MiGetVirtualAddressMappedByPte(PointerPte))) {

        //
        // No need to lock physical addresses.
        //

        return;
    }

    PageCount = 0;

    //
    // Lock this routine into memory.
    //

    MmLockPagableSectionByHandle(ExPageLockHandle);

    if (SessionSpace == TRUE) {
        LOCK_SESSION_SPACE_WS (OldIrql1);
    }
    else {
        LOCK_SYSTEM_WS (OldIrql1);
    }

    LOCK_PFN (OldIrql);

    Base = MiGetVirtualAddressMappedByPte (PointerPte);

    while (PointerPte <= LastPte) {

        //
        // Check to make sure this PTE has not already been
        // made pagable (or deleted).  It is pagable if it
        // is not valid, or if the PFN database wsindex element
        // is non zero.
        //

        if (PointerPte->u.Hard.Valid == 1) {
            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
            Pfn = MI_PFN_ELEMENT (PageFrameIndex);
            ASSERT (Pfn->u2.ShareCount == 1);

            if (Pfn->u1.WsIndex == 0) {

                //
                // Original PTE may need to be set for drivers loaded
                // via ntldr.
                //
    
                if (Pfn->OriginalPte.u.Long == 0) {
                    Pfn->OriginalPte.u.Long = MM_KERNEL_DEMAND_ZERO_PTE;
#if defined(_IA64_)
                    Pfn->OriginalPte.u.Soft.Protection |= MM_EXECUTE;
#endif
                }
    
                TempPte = *PointerPte;

                MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                           Pfn->OriginalPte.u.Soft.Protection);

                PreviousPte.u.Flush = KeFlushSingleTb (Base,
                                                       TRUE,
                                                       TRUE,
                                                       (PHARDWARE_PTE)PointerPte,
                                                       TempPte.u.Flush);

                MI_CAPTURE_DIRTY_BIT_TO_PFN (&PreviousPte, Pfn);

                //
                // Flush the translation buffer and decrement the number of valid
                // PTEs within the containing page table page.  Note that for a
                // private page, the page table page is still needed because the
                // page is in transition.
                //

                MiDecrementShareCount (PageFrameIndex);
                MmResidentAvailablePages += 1;
                MM_BUMP_COUNTER(19, 1);
                MmTotalSystemDriverPages += 1;
                PageCount += 1;
            }
            else {
                //
                // This page is already pagable and has a WSLE entry.
                // Ignore it here and let the trimmer take it if memory
                // comes under pressure.
                //
            }
        }
        Base = (PVOID)((PCHAR)Base + PAGE_SIZE);
        PointerPte += 1;
    }

    if (SessionSpace == TRUE) {

        //
        // Session space has no ASN - flush the entire TB.
        //
    
        MI_FLUSH_ENTIRE_SESSION_TB (TRUE, TRUE);
    }

    UNLOCK_PFN (OldIrql);

    if (SessionSpace == TRUE) {

        //
        // These pages are no longer locked down.
        //

        MmSessionSpace->NonPagablePages -= PageCount;
        MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_DRIVER_PAGES_UNLOCKED, PageCount);

        UNLOCK_SESSION_SPACE_WS (OldIrql1);
    }
    else {
        UNLOCK_SYSTEM_WS (OldIrql1);
    }
    MmUnlockPagableImageSection(ExPageLockHandle);
}


PVOID
MmPageEntireDriver (
    IN PVOID AddressWithinSection
    )

/*++

Routine Description:

    This routine allows a driver to page out all of its code and
    data regardless of the attributes of the various image sections.

    Note, this routine can be called multiple times with no
    intervening calls to MmResetDriverPaging.

Arguments:

    AddressWithinSection - Supplies an address within the driver, e.g.
                           DriverEntry.

Return Value:

    Base address of driver.

Environment:

    Kernel mode, APC_LEVEL or below.

--*/

{
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PMMPTE FirstPte;
    PMMPTE LastPte;
    PVOID BaseAddress;
    PSECTION SectionPointer;
    BOOLEAN SessionSpace;

    PAGED_CODE();

    //
    // Don't page kernel mode code if disabled via registry.
    //

    DataTableEntry = MiLookupDataTableEntry (AddressWithinSection, FALSE);

    if (DataTableEntry == NULL) {
        return NULL;
    }

    SectionPointer = (PSECTION)DataTableEntry->SectionPointer;

    if (MmDisablePagingExecutive) {
        return DataTableEntry->DllBase;
    }

    SessionSpace = MI_IS_SESSION_IMAGE_ADDRESS (AddressWithinSection);

    if ((SectionPointer != NULL) && (SectionPointer != (PVOID)-1)) {

        //
        // Driver is mapped as an image (ie: win32k), this is always pagable.
        // For session space, an image that has been loaded at its desired
        // address is also always pagable.  If there was an address collision,
        // then we fall through because we have to explicitly page it.
        //

        if (SessionSpace == TRUE) {
            if (SectionPointer->Segment &&
                SectionPointer->Segment->BasedAddress == SectionPointer->Segment->SystemImageBase) {
                return DataTableEntry->DllBase;
            }
        }
        else {
            return DataTableEntry->DllBase;
        }
    }

    BaseAddress = DataTableEntry->DllBase;
    FirstPte = MiGetPteAddress (BaseAddress);
    LastPte = (FirstPte - 1) + (DataTableEntry->SizeOfImage >> PAGE_SHIFT);

    MiSetPagingOfDriver (FirstPte, LastPte, SessionSpace);

    return BaseAddress;
}


VOID
MmResetDriverPaging (
    IN PVOID AddressWithinSection
    )

/*++

Routine Description:

    This routines resets the driver paging to what the image specified.
    Hence image sections such as the IAT, .text, .data will be locked
    down in memory.

    Note, there is no requirement that MmPageEntireDriver was called.

Arguments:

     AddressWithinSection - Supplies an address within the driver, e.g.
                            DriverEntry.

Return Value:

    None.

Environment:

    Kernel mode, APC_LEVEL or below.

--*/

{
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PMMPTE LastPte;
    PMMPTE PointerPte;
    PVOID Base;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER FoundSection;
    KIRQL OldIrql;
    KIRQL OldIrqlWs;

    PAGED_CODE();

    //
    // Don't page kernel mode code if disabled via registry.
    //

    if (MmDisablePagingExecutive) {
        return;
    }

    if (MI_IS_PHYSICAL_ADDRESS(AddressWithinSection)) {
        return;
    }

    //
    // If the driver has pagable code, make it paged.
    //

    DataTableEntry = MiLookupDataTableEntry (AddressWithinSection, FALSE);

    if ((DataTableEntry->SectionPointer != NULL) &&
        (DataTableEntry->SectionPointer != (PVOID)-1)) {

        //
        // Driver is mapped by image hence already paged.
        //

        return;
    }

    Base = DataTableEntry->DllBase;

    NtHeaders = (PIMAGE_NT_HEADERS)RtlImageNtHeader(Base);

    FoundSection = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeaders +
                        sizeof(ULONG) +
                        sizeof(IMAGE_FILE_HEADER) +
                        NtHeaders->FileHeader.SizeOfOptionalHeader
                        );

    i = NtHeaders->FileHeader.NumberOfSections;
    PointerPte = NULL;

    while (i > 0) {
#if DBG
            if ((*(PULONG)FoundSection->Name == 'tini') ||
                (*(PULONG)FoundSection->Name == 'egap')) {
                DbgPrint("driver %wZ has lower case sections (init or pagexxx)\n",
                    &DataTableEntry->FullDllName);
            }
#endif

        //
        // Don't lock down code for sections marked as discardable or
        // sections marked with the first 4 characters PAGE or .eda
        // (for the .edata section) or INIT.
        //

        if (((FoundSection->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) != 0) ||
           (*(PULONG)FoundSection->Name == 'EGAP') ||
           (*(PULONG)FoundSection->Name == 'ade.') ||
           (*(PULONG)FoundSection->Name == 'TINI')) {

            NOTHING;

        } else {

            //
            // This section is nonpagable.
            //

            PointerPte = MiGetPteAddress (
                                   (PCHAR)Base + FoundSection->VirtualAddress);
            LastPte = MiGetPteAddress ((PCHAR)Base +
                                       FoundSection->VirtualAddress +
                                      (FoundSection->SizeOfRawData - 1));
            ASSERT (PointerPte <= LastPte);
            MmLockPagableSectionByHandle(ExPageLockHandle);
            LOCK_SYSTEM_WS (OldIrqlWs);
            LOCK_PFN (OldIrql);
            MiLockCode (PointerPte, LastPte, MM_LOCK_BY_NONPAGE);
            UNLOCK_PFN (OldIrql);
            UNLOCK_SYSTEM_WS (OldIrqlWs);
            MmUnlockPagableImageSection(ExPageLockHandle);
        }
        i -= 1;
        FoundSection += 1;
    }
    return;
}


VOID
MiClearImports(
    IN PLDR_DATA_TABLE_ENTRY DataTableEntry
    )
/*++

Routine Description:

    Free up the import list and clear the pointer.  This stops the
    recursion performed in MiDereferenceImports().

Arguments:

    DataTableEntry - provided for the driver.

Return Value:

    Status of the import list construction operation.

--*/

{
    PAGED_CODE();

    if (DataTableEntry->LoadedImports == (PVOID)LOADED_AT_BOOT) {
        return;
    }

    if (DataTableEntry->LoadedImports == (PVOID)NO_IMPORTS_USED) {
        NOTHING;
    }
    else if (SINGLE_ENTRY(DataTableEntry->LoadedImports)) {
        NOTHING;
    }
    else {
        //
        // free the memory
        //
        ExFreePool ((PVOID)DataTableEntry->LoadedImports);
    }

    //
    // stop the recursion
    //
    DataTableEntry->LoadedImports = (PVOID)LOADED_AT_BOOT;
}

VOID
MiRememberUnloadedDriver (
    IN PUNICODE_STRING DriverName,
    IN PVOID Address,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine saves information about unloaded drivers so that ones that
    forget to delete lookaside lists or queues can be caught.

Arguments:

    DriverName - Supplies a Unicode string containing the driver's name.

    Address - Supplies the address the driver was loaded at.

    Length - Supplies the number of bytes the driver load spanned.

Return Value:

    None.

--*/

{
    PUNLOADED_DRIVERS Entry;
    ULONG NumberOfBytes;

    if (DriverName->Length == 0) {

        //
        // This is an aborted load and the driver name hasn't been filled
        // in yet.  No need to save it.
        //

        return;
    }

    //
    // Serialization is provided by the caller, so just update the list now.
    // Note the allocations are nonpaged so they can be searched at bugcheck
    // time.
    //

    if (MiUnloadedDrivers == NULL) {
        NumberOfBytes = MI_UNLOADED_DRIVERS * sizeof (UNLOADED_DRIVERS);

        MiUnloadedDrivers = (PUNLOADED_DRIVERS)ExAllocatePoolWithTag (NonPagedPool,
                                                                      NumberOfBytes,
                                                                      'TDmM');
        if (MiUnloadedDrivers == NULL) {
            return;
        }
        RtlZeroMemory (MiUnloadedDrivers, NumberOfBytes);
        MiLastUnloadedDriver = 0;
    }
    else if (MiLastUnloadedDriver >= MI_UNLOADED_DRIVERS) {
        MiLastUnloadedDriver = 0;
    }

    Entry = &MiUnloadedDrivers[MiLastUnloadedDriver];

    //
    // Free the old entry as we recycle into the new.
    //

    RtlFreeUnicodeString (&Entry->Name);

    Entry->Name.Buffer = ExAllocatePoolWithTag (NonPagedPool,
                                                DriverName->Length,
                                                'TDmM');

    if (Entry->Name.Buffer == NULL) {
        Entry->Name.MaximumLength = 0;
        Entry->Name.Length = 0;
        MiUnloadsSkipped += 1;
        return;
    }

    RtlMoveMemory(Entry->Name.Buffer, DriverName->Buffer, DriverName->Length);
    Entry->Name.Length = DriverName->Length;
    Entry->Name.MaximumLength = DriverName->MaximumLength;

    Entry->StartAddress = Address;
    Entry->EndAddress = (PVOID)((PCHAR)Address + Length);

    KeQuerySystemTime (&Entry->CurrentTime);

    MiTotalUnloads += 1;
    MiLastUnloadedDriver += 1;
}

PUNICODE_STRING
MmLocateUnloadedDriver (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This routine attempts to find the specified virtual address in the
    unloaded driver list.

Arguments:

    VirtualAddress - Supplies a virtual address that might be within a driver
                     that has already unloaded.

Return Value:

    A pointer to a Unicode string containing the unloaded driver's name.

Environment:

    Kernel mode, bugcheck time.

--*/

{
    PUNLOADED_DRIVERS Entry;
    ULONG i;
    ULONG Index;

    //
    // No serialization is needed because we've crashed.
    //

    if (MiUnloadedDrivers == NULL) {
        return NULL;
    }

    Index = MiLastUnloadedDriver - 1;

    for (i = 0; i < MI_UNLOADED_DRIVERS; i += 1) {
        if (Index >= MI_UNLOADED_DRIVERS) {
            Index = MI_UNLOADED_DRIVERS - 1;
        }
        Entry = &MiUnloadedDrivers[Index];
        if (Entry->Name.Buffer != NULL) {
            if ((VirtualAddress >= Entry->StartAddress) &&
                (VirtualAddress < Entry->EndAddress)) {
                    return &Entry->Name;
            }
        }
        Index -= 1;
    }

    return NULL;
}


NTSTATUS
MmUnloadSystemImage (
    IN PVOID ImageHandle
    )

/*++

Routine Description:

    This routine unloads a previously loaded system image and returns
    the allocated resources.

Arguments:

    ImageHandle - Supplies a pointer to the section object of the
                  image to unload.

Return Value:

    Various NTSTATUS codes.

--*/

{
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PMMPTE LastPte;
    PFN_NUMBER PagesRequired;
    PFN_NUMBER ResidentPages;
    PMMPTE PointerPte;
    PFN_NUMBER NumberOfPtes;
    KIRQL OldIrql;
    PVOID BasedAddress;
    SIZE_T NumberOfBytes;
    BOOLEAN MustFree;
    SIZE_T CommittedPages;
    BOOLEAN ViewDeleted;
    PIMAGE_ENTRY_IN_SESSION DriverImage;
    NTSTATUS Status;
    PVOID StillQueued;
    PSECTION SectionPointer;

    //
    // Arbitrary process context so prevent suspend APCs now.
    //

    KeEnterCriticalRegion();

    KeWaitForSingleObject (&MmSystemLoadLock,
                           WrVirtualMemory,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)NULL);

    MmLockPagableSectionByHandle(ExPageLockHandle);

    ViewDeleted = FALSE;
    DataTableEntry = (PLDR_DATA_TABLE_ENTRY)ImageHandle;
    BasedAddress = DataTableEntry->DllBase;

#if DBGXX
    //
    // MiUnloadSystemImageByForce violates this check so remove it for now.
    //

    if (PsLoadedModuleList.Flink) {
        LOGICAL Found;
        PLIST_ENTRY NextEntry;
        PLDR_DATA_TABLE_ENTRY DataTableEntry2;

        Found = FALSE;
        NextEntry = PsLoadedModuleList.Flink;
        while (NextEntry != &PsLoadedModuleList) {
    
            DataTableEntry2 = CONTAINING_RECORD(NextEntry,
                                                LDR_DATA_TABLE_ENTRY,
                                                InLoadOrderLinks);
            if (DataTableEntry == DataTableEntry2) {
                Found = TRUE;
                break;
            }
            NextEntry = NextEntry->Flink;
        }
        ASSERT (Found == TRUE);
    }
#endif

#if DBG_SYSLOAD
    if (DataTableEntry->SectionPointer == NULL) {
        DbgPrint ("MM: Called to unload boot driver %wZ\n",
            &DataTableEntry->FullDllName);
    }
    else {
        DbgPrint ("MM: Called to unload non-boot driver %wZ\n",
            &DataTableEntry->FullDllName);
    }
#endif

    //
    // Any driver loaded at boot that did not have its import list
    // and LoadCount reconstructed cannot be unloaded because we don't
    // know how many other drivers may be linked to it.
    //

    if (DataTableEntry->LoadedImports == (PVOID)LOADED_AT_BOOT) {
        MmUnlockPagableImageSection(ExPageLockHandle);
        KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
        KeLeaveCriticalRegion();
        return STATUS_SUCCESS;
    }

    ASSERT (DataTableEntry->LoadCount != 0);

    if (MI_IS_SESSION_IMAGE_ADDRESS (BasedAddress)) {

        //
        // A printer driver may be referenced multiple times for the
        // same session space.  Only unload the last reference.
        //

        DriverImage = MiSessionLookupImage (BasedAddress);

        ASSERT (DriverImage);

        ASSERT (DriverImage->ImageCountInThisSession);

        if (DriverImage->ImageCountInThisSession > 1) {

            DriverImage->ImageCountInThisSession -= 1;
            MmUnlockPagableImageSection(ExPageLockHandle);
            KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
            KeLeaveCriticalRegion();

            return STATUS_SUCCESS;
        }

        //
        // The reference count for this image has dropped to zero in this
        // session, so we can delete this session's view of the image.
        //

        Status = MiSessionWideGetImageSize (BasedAddress,
                                            &NumberOfBytes,
                                            &CommittedPages);

        if (!NT_SUCCESS(Status)) {

            KeBugCheckEx (MEMORY_MANAGEMENT,
                          0x41286,
                          (ULONG_PTR)MmSessionSpace->SessionId,
                          (ULONG_PTR)BasedAddress,
                          0);
        }

        //
        // Free the session space taken up by the image, unmapping it from
        // the current VA space - note this does not remove page table pages
        // from the session PageTables[].  Each data page is only freed
        // if there are no other references to it (ie: from any other
        // sessions).
        //

        PointerPte = MiGetPteAddress (BasedAddress);
        LastPte = MiGetPteAddress ((ULONG_PTR)BasedAddress + NumberOfBytes);

        PagesRequired = MiDeleteSystemPagableVm (PointerPte,
                                                 (PFN_NUMBER)(LastPte - PointerPte),
                                                 ZeroKernelPte,
                                                 TRUE,
                                                 &ResidentPages);

        if (MmDisablePagingExecutive == 0) {

            SectionPointer = (PSECTION)DataTableEntry->SectionPointer;
        
            if ((SectionPointer == NULL) ||
                (SectionPointer == (PVOID)-1) ||
                (SectionPointer->Segment == NULL) ||
                (SectionPointer->Segment->BasedAddress != SectionPointer->Segment->SystemImageBase)) {

                MmTotalSystemDriverPages -= (ULONG)(PagesRequired - ResidentPages);
            }
        }

        LOCK_SESSION_SPACE_WS (OldIrql);
        MmSessionSpace->CommittedPages -= CommittedPages;

        MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_COMMIT_IMAGE_UNLOAD,
            CommittedPages);

        UNLOCK_SESSION_SPACE_WS (OldIrql);

        ViewDeleted = TRUE;

        //
        // Return the commitment we took out on the pagefile when the
        // image was allocated.
        //

        MiReturnCommitment (CommittedPages);
        MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_DRIVER_UNLOAD, CommittedPages);

        //
        // Tell the session space image handler that we are releasing
        // our claim to the image.
        //

        Status = MiRemoveImageSessionWide (BasedAddress);

        ASSERT (NT_SUCCESS (Status));
    }

    ASSERT (DataTableEntry->LoadCount != 0);

    DataTableEntry->LoadCount -= 1;

    if (DataTableEntry->LoadCount != 0) {
        MmUnlockPagableImageSection(ExPageLockHandle);
        KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
        KeLeaveCriticalRegion();
        return STATUS_SUCCESS;
    }

#if DBG
    if (MI_IS_SESSION_IMAGE_ADDRESS (BasedAddress)) {
        ASSERT (MiSessionLookupImage (BasedAddress) == NULL);
    }
#endif

    if (MmSnapUnloads) {
#if 0
        StillQueued = KeCheckForTimer (DataTableEntry->DllBase,
                                       DataTableEntry->SizeOfImage);

        if (StillQueued != NULL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x18,
                          (ULONG_PTR)StillQueued,
                          (ULONG_PTR)-1,
                          (ULONG_PTR)DataTableEntry->DllBase);
        }

        StillQueued = ExpCheckForResource (DataTableEntry->DllBase,
                                           DataTableEntry->SizeOfImage);

        if (StillQueued != NULL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x19,
                          (ULONG_PTR)StillQueued,
                          (ULONG_PTR)-1,
                          (ULONG_PTR)DataTableEntry->DllBase);
        }
#endif
    }

    if (DataTableEntry->Flags & LDRP_IMAGE_VERIFYING) {
        MiVerifyingDriverUnloading (DataTableEntry);
    }

    if (MiActiveVerifierThunks != 0) {
        MiVerifierCheckThunks (DataTableEntry);
    }

    //
    // Unload symbols from debugger.
    //

    if (DataTableEntry->Flags & LDRP_DEBUG_SYMBOLS_LOADED) {

        //
        //  TEMP TEMP TEMP rip out when debugger converted
        //

        ANSI_STRING AnsiName;
        NTSTATUS Status;

        Status = RtlUnicodeStringToAnsiString( &AnsiName,
                                               &DataTableEntry->BaseDllName,
                                               TRUE );

        if (NT_SUCCESS( Status)) {
            DbgUnLoadImageSymbols( &AnsiName,
                                   BasedAddress,
                                   (ULONG)-1);
            RtlFreeAnsiString( &AnsiName );
        }
    }

    //
    // No unload can happen till after Mm has finished Phase 1 initialization.
    // Therefore, large pages are already in effect (if this platform supports
    // it).
    //

    if (ViewDeleted == FALSE) {

        NumberOfPtes = DataTableEntry->SizeOfImage >> PAGE_SHIFT;

        if (MmSnapUnloads) {
            MiRememberUnloadedDriver (&DataTableEntry->BaseDllName,
                                      BasedAddress,
                                      (ULONG)(NumberOfPtes << PAGE_SHIFT));
        }

        if (DataTableEntry->Flags & LDRP_SYSTEM_MAPPED) {

            PointerPte = MiGetPteAddress (BasedAddress);

            PagesRequired = MiDeleteSystemPagableVm (PointerPte,
                                                     NumberOfPtes,
                                                     ZeroKernelPte,
                                                     FALSE,
                                                     &ResidentPages);
    
            MmTotalSystemDriverPages -= (ULONG)(PagesRequired - ResidentPages);

            //
            // Note that drivers loaded at boot that have not been relocated
            // have no system PTEs or commit charged.
            //
    
            MiReleaseSystemPtes (PointerPte,
                                 (ULONG)NumberOfPtes,
                                 SystemPteSpace);

            LOCK_PFN (OldIrql);
            MmResidentAvailablePages += ResidentPages;
            MM_BUMP_COUNTER(21, ResidentPages);
            UNLOCK_PFN (OldIrql);

            //
            // Only return commitment for drivers that weren't loaded by the
            // boot loader.
            //
    
            if (DataTableEntry->SectionPointer != NULL) {
                MiReturnCommitment (PagesRequired);
                MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_DRIVER_UNLOAD1, PagesRequired);
                MmDriverCommit -= (ULONG)PagesRequired;
            }
        }
        else {

            //
            // This must be a boot driver that was not relocated into
            // system PTEs.  If large or super pages are enabled, the
            // image pages must be freed without referencing the
            // non-existent page table pages.  If large/super pages are
            // not enabled, note that system PTEs were not used to map the
            // image and thus, cannot be freed.

            //
            // This is further complicated by the fact that the INIT and/or
            // discardable portions of these images may have already been freed.
            //
        }
    }

    //
    // Search the loaded module list for the data table entry that describes
    // the DLL that was just unloaded. It is possible an entry is not in the
    // list if a failure occurred at a point in loading the DLL just before
    // the data table entry was generated.
    //

    if (DataTableEntry->InLoadOrderLinks.Flink != NULL) {
        KeEnterCriticalRegion();
        ExAcquireResourceExclusive (&PsLoadedModuleResource, TRUE);

        ExAcquireSpinLock (&PsLoadedModuleSpinLock, &OldIrql);

        RemoveEntryList(&DataTableEntry->InLoadOrderLinks);
        ExReleaseSpinLock (&PsLoadedModuleSpinLock, OldIrql);

        ExReleaseResource (&PsLoadedModuleResource);
        KeLeaveCriticalRegion();

        MustFree = TRUE;
    }
    else {
        MustFree = FALSE;
    }

    //
    // Handle unloading of any dependent DLLs that we loaded automatically
    // for this image.
    //

    MiDereferenceImports ((PLOAD_IMPORTS)DataTableEntry->LoadedImports);

    MiClearImports (DataTableEntry);

    //
    // Free this loader entry.
    //

    if (MustFree == TRUE) {

        if (DataTableEntry->FullDllName.Buffer != NULL) {
            ExFreePool (DataTableEntry->FullDllName.Buffer);
        }

        if (DataTableEntry->BaseDllName.Buffer != NULL) {
            ExFreePool (DataTableEntry->BaseDllName.Buffer);
        }

        //
        // Dereference the section object if there is one.
        // There should only be one for win32k.sys and Hydra session images.
        //

        if ((DataTableEntry->SectionPointer != NULL) &&
            (DataTableEntry->SectionPointer != (PVOID)-1)) {

            ObDereferenceObject (DataTableEntry->SectionPointer);
        }

        ExFreePool((PVOID)DataTableEntry);
    }

    MmUnlockPagableImageSection(ExPageLockHandle);

    KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
    KeLeaveCriticalRegion();

    PERFINFO_IMAGE_UNLOAD(BasedAddress);

    return STATUS_SUCCESS;
}


NTSTATUS
MiBuildImportsForBootDrivers(
    VOID
    )

/*++

Routine Description:

    Construct an import list chain for boot-loaded drivers.
    If this cannot be done for an entry, its chain is set to LOADED_AT_BOOT.

    If a chain can be successfully built, then this driver's DLLs
    will be automatically unloaded if this driver goes away (provided
    no other driver is also using them).  Otherwise, on driver unload,
    its dependent DLLs would have to be explicitly unloaded.

    Note that the incoming LoadCount values are not correct and thus, they
    are reinitialized here.

Arguments:

    None.

Return Value:

    Various NTSTATUS codes.

--*/

{
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PLIST_ENTRY NextEntry;
    PLDR_DATA_TABLE_ENTRY DataTableEntry2;
    PLIST_ENTRY NextEntry2;
    ULONG i;
    ULONG j;
    ULONG ImageCount;
    PVOID *ImageReferences;
    PVOID LastImageReference;
    PULONG_PTR ImportThunk;
    ULONG_PTR BaseAddress;
    ULONG_PTR LastAddress;
    ULONG ImportSize;
    ULONG ImportListSize;
    PLOAD_IMPORTS ImportList;
    LOGICAL UndoEverything;
    PLDR_DATA_TABLE_ENTRY KernelDataTableEntry;
    PLDR_DATA_TABLE_ENTRY HalDataTableEntry;
    UNICODE_STRING KernelString;
    UNICODE_STRING HalString;

    PAGED_CODE();

    ImageCount = 0;

    KernelDataTableEntry = NULL;
    HalDataTableEntry = NULL;

    RtlInitUnicodeString (&KernelString, L"ntoskrnl.exe");
    RtlInitUnicodeString (&HalString, L"hal.dll");

    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList) {

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        if (RtlEqualUnicodeString (&KernelString,
                                   &DataTableEntry->BaseDllName,
                                   TRUE)) {

            KernelDataTableEntry = CONTAINING_RECORD(NextEntry,
                                                     LDR_DATA_TABLE_ENTRY,
                                                     InLoadOrderLinks);
        }
        else if (RtlEqualUnicodeString (&HalString,
                                        &DataTableEntry->BaseDllName,
                                        TRUE)) {

            HalDataTableEntry = CONTAINING_RECORD(NextEntry,
                                                  LDR_DATA_TABLE_ENTRY,
                                                  InLoadOrderLinks);
        }

        //
        // Initialize these properly so error recovery is simplified.
        //

        DataTableEntry->LoadCount = 1;
        DataTableEntry->LoadedImports = (PVOID)LOADED_AT_BOOT;

        ImageCount += 1;
        NextEntry = NextEntry->Flink;
    }

    if (KernelDataTableEntry == NULL || HalDataTableEntry == NULL) {
        return STATUS_NOT_FOUND;
    }

    ImageReferences = (PVOID *) ExAllocatePoolWithTag (PagedPool,
                                                       ImageCount * sizeof (PVOID),
                                                       'TDmM');

    if (ImageReferences == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UndoEverything = FALSE;

    NextEntry = PsLoadedModuleList.Flink;

    for ( ; NextEntry != &PsLoadedModuleList; NextEntry = NextEntry->Flink) {

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        ImportThunk = (PULONG_PTR)RtlImageDirectoryEntryToData(
                                           DataTableEntry->DllBase,
                                           TRUE,
                                           IMAGE_DIRECTORY_ENTRY_IAT,
                                           &ImportSize);
    
        if (ImportThunk == NULL) {
            DataTableEntry->LoadedImports = NO_IMPORTS_USED;
            continue;
        }
    
        RtlZeroMemory (ImageReferences, ImageCount * sizeof (PVOID));

        ImportSize /= sizeof(PULONG_PTR);

        BaseAddress = 0;
        for (i = 0; i < ImportSize; i += 1, ImportThunk += 1) {

            //
            // Check the hint first.
            //

            if (BaseAddress) {
                if (*ImportThunk >= BaseAddress && *ImportThunk < LastAddress) {
                    ASSERT (ImageReferences[j]);
                    continue;
                }
            }

            j = 0;
            NextEntry2 = PsLoadedModuleList.Flink;

            while (NextEntry2 != &PsLoadedModuleList) {

                DataTableEntry2 = CONTAINING_RECORD(NextEntry2,
                                                    LDR_DATA_TABLE_ENTRY,
                                                    InLoadOrderLinks);
    
                BaseAddress = (ULONG_PTR) DataTableEntry2->DllBase;
                LastAddress = BaseAddress + DataTableEntry2->SizeOfImage;

                if (*ImportThunk >= BaseAddress && *ImportThunk < LastAddress) {
                    ImageReferences[j] = DataTableEntry2;
                    break;
                }

                NextEntry2 = NextEntry2->Flink;
                j += 1;
            }

            if (*ImportThunk < BaseAddress || *ImportThunk >= LastAddress) {
                if (*ImportThunk) {
#if DBG
                    DbgPrint ("MM: broken import linkage %p %p %p\n",
                        DataTableEntry,
                        ImportThunk,
                        *ImportThunk);
                    DbgBreakPoint ();
#endif
                    UndoEverything = TRUE;
                    goto finished;
                }

                BaseAddress = 0;
            }
        }

        ImportSize = 0;

        for (i = 0; i < ImageCount; i += 1) {

            if ((ImageReferences[i] != NULL) &&
                (ImageReferences[i] != KernelDataTableEntry) &&
                (ImageReferences[i] != HalDataTableEntry)) {

                    LastImageReference = ImageReferences[i];
                    ImportSize += 1;
            }
        }

        if (ImportSize == 0) {
            DataTableEntry->LoadedImports = NO_IMPORTS_USED;
        }
        else if (ImportSize == 1) {
#if DBG_SYSLOAD
            DbgPrint("driver %wZ imports %wZ\n",
                &DataTableEntry->FullDllName,
                &((PLDR_DATA_TABLE_ENTRY)LastImageReference)->FullDllName);
#endif

            DataTableEntry->LoadedImports = POINTER_TO_SINGLE_ENTRY (LastImageReference);
            ((PLDR_DATA_TABLE_ENTRY)LastImageReference)->LoadCount += 1;
        }
        else {
#if DBG_SYSLOAD
            DbgPrint("driver %wZ imports many\n", &DataTableEntry->FullDllName);
#endif

            ImportListSize = ImportSize * sizeof(PVOID) + sizeof(SIZE_T);

            ImportList = (PLOAD_IMPORTS) ExAllocatePoolWithTag (PagedPool,
                                                                ImportListSize,
                                                                'TDmM');

            if (ImportList == NULL) {
                UndoEverything = TRUE;
                break;
            }

            ImportList->Count = ImportSize;

            j = 0;
            for (i = 0; i < ImageCount; i += 1) {
    
                if ((ImageReferences[i] != NULL) &&
                    (ImageReferences[i] != KernelDataTableEntry) &&
                    (ImageReferences[i] != HalDataTableEntry)) {
    
#if DBG_SYSLOAD
                        DbgPrint("driver %wZ imports %wZ\n",
                            &DataTableEntry->FullDllName,
                            &((PLDR_DATA_TABLE_ENTRY)ImageReferences[i])->FullDllName);
#endif

                        ImportList->Entry[j] = ImageReferences[i];
                        ((PLDR_DATA_TABLE_ENTRY)ImageReferences[i])->LoadCount += 1;
                        j += 1;
                }
            }

            ASSERT (j == ImportSize);

            DataTableEntry->LoadedImports = ImportList;
        }
#if DBG_SYSLOAD
        DbgPrint("\n");
#endif
    }

finished:

    ExFreePool ((PVOID)ImageReferences);

    //
    // The kernel and HAL are never unloaded.
    //

    if ((KernelDataTableEntry->LoadedImports != NO_IMPORTS_USED) &&
        (!POINTER_TO_SINGLE_ENTRY(KernelDataTableEntry->LoadedImports))) {
            ExFreePool ((PVOID)KernelDataTableEntry->LoadedImports);
    }

    if ((HalDataTableEntry->LoadedImports != NO_IMPORTS_USED) &&
        (!POINTER_TO_SINGLE_ENTRY(HalDataTableEntry->LoadedImports))) {
            ExFreePool ((PVOID)HalDataTableEntry->LoadedImports);
    }

    KernelDataTableEntry->LoadedImports = (PVOID)LOADED_AT_BOOT;
    HalDataTableEntry->LoadedImports = (PVOID)LOADED_AT_BOOT;

    if (UndoEverything == TRUE) {

#if DBG_SYSLOAD
        DbgPrint("driver %wZ import rebuild failed\n",
            &DataTableEntry->FullDllName);
        DbgBreakPoint();
#endif

        //
        // An error occurred and this is an all or nothing operation so
        // roll everything back.
        //
    
        NextEntry = PsLoadedModuleList.Flink;
        while (NextEntry != &PsLoadedModuleList) {
            DataTableEntry = CONTAINING_RECORD(NextEntry,
                                               LDR_DATA_TABLE_ENTRY,
                                               InLoadOrderLinks);

            ImportList = DataTableEntry->LoadedImports;
            if (ImportList == LOADED_AT_BOOT || ImportList == NO_IMPORTS_USED ||
                SINGLE_ENTRY(ImportList)) {
                    NOTHING;
            }
            else {
                ExFreePool (ImportList);
            }

            DataTableEntry->LoadedImports = (PVOID)LOADED_AT_BOOT;
            DataTableEntry->LoadCount = 1;
            NextEntry = NextEntry->Flink;
        }

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}


LOGICAL
MiCallDllUnloadAndUnloadDll(
    IN PLDR_DATA_TABLE_ENTRY DataTableEntry
    )

/*++

Routine Description:

    All the references from other drivers to this DLL have been cleared.
    The only remaining issue is that this DLL must support being unloaded.
    This means having no outstanding DPCs, allocated pool, etc.

    If the DLL has an unload routine that returns SUCCESS, then we clean
    it up and free up its memory now.

    Note this routine is NEVER called for drivers - only for DLLs that were
    loaded due to import references from various drivers.

Arguments:

    DataTableEntry - provided for the DLL.

Return Value:

    None.

--*/

{
    PMM_DLL_UNLOAD Func;
    NTSTATUS Status;
    LOGICAL Unloaded;

    PAGED_CODE();

    Unloaded = FALSE;

    Func = MiLocateExportName (DataTableEntry->DllBase, "DllUnload");

    if (Func) {

        //
        // The unload function was found in the DLL so unload it now.
        //

        Status = Func();

        if (NT_SUCCESS(Status)) {

            //
            // Set up the reference count so the import DLL looks like a regular
            // driver image is being unloaded.
            //

            ASSERT (DataTableEntry->LoadCount == 0);
            DataTableEntry->LoadCount = 1;

            MmUnloadSystemImage ((PVOID)DataTableEntry);
            Unloaded = TRUE;
        }
    }

    return Unloaded;
}


PVOID
MiLocateExportName (
    IN PVOID DllBase,
    IN PCHAR FunctionName
    )

/*++

Routine Description:

    This function is invoked to locate a function name in an export directory.

Arguments:

    DllBase - Supplies the image base.

    FunctionName - Supplies the the name to be located.

Return Value:

    The address of the located function or NULL.

--*/

{
    PVOID Func = NULL;
    PULONG NameTableBase;
    PUSHORT NameOrdinalTableBase;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    PULONG Addr;
    ULONG ExportSize;
    ULONG Low;
    ULONG Middle;
    ULONG High;
    LONG Result;
    USHORT OrdinalNumber;

    PAGED_CODE();

    //
    // Locate the DLL's export directory.
    //

    ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(
                                DllBase,
                                TRUE,
                                IMAGE_DIRECTORY_ENTRY_EXPORT,
                                &ExportSize
                                );
    if (ExportDirectory) {

        NameTableBase =  (PULONG)((PCHAR)DllBase + (ULONG)ExportDirectory->AddressOfNames);
        NameOrdinalTableBase = (PUSHORT)((PCHAR)DllBase + (ULONG)ExportDirectory->AddressOfNameOrdinals);

        //
        // Look in the export name table for the specified function name.
        //

        Low = 0;
        High = ExportDirectory->NumberOfNames - 1;

        while (High >= Low && (LONG)High >= 0) {

            //
            // Compute the next probe index and compare the export name entry
            // with the specified function name.
            //

            Middle = (Low + High) >> 1;
            Result = strcmp(FunctionName,
                            (PCHAR)((PCHAR)DllBase + NameTableBase[Middle]));

            if (Result < 0) {
                High = Middle - 1;
            } else if (Result > 0) {
                Low = Middle + 1;
            } else {
                break;
            }
        }

        //
        // If the high index is less than the low index, then a matching table
        // entry was not found.  Otherwise, get the ordinal number from the
        // ordinal table and location the function address.
        //

        if ((LONG)High >= (LONG)Low) {

            OrdinalNumber = NameOrdinalTableBase[Middle];
            Addr = (PULONG)((PCHAR)DllBase + (ULONG)ExportDirectory->AddressOfFunctions);
            Func = (PVOID)((PCHAR)DllBase + Addr[OrdinalNumber]);

            //
            // If the function address is w/in range of the export directory,
            // then the function is forwarded, which is not allowed, so ignore
            // it.
            //

            if ((ULONG_PTR)Func > (ULONG_PTR)ExportDirectory &&
                (ULONG_PTR)Func < ((ULONG_PTR)ExportDirectory + ExportSize)) {
                Func = NULL;
            }
        }
    }

    return Func;
}


NTSTATUS
MiDereferenceImports (
    IN PLOAD_IMPORTS ImportList
    )

/*++

Routine Description:

    Decrement the reference count on each DLL specified in the image import
    list.  If any DLL's reference count reaches zero, then free the DLL.

    No locks may be held on entry as MmUnloadSystemImage may be called.

    The parameter list is freed here as well.

Arguments:

    ImportList - Supplies the list of DLLs to dereference.

Return Value:

    Status of the dereference operation.

--*/

{
    ULONG i;
    LOGICAL Unloaded;
    PVOID SavedImports;
    LOAD_IMPORTS SingleTableEntry;
    PLDR_DATA_TABLE_ENTRY ImportTableEntry;

    PAGED_CODE();

    if (ImportList == LOADED_AT_BOOT || ImportList == NO_IMPORTS_USED) {
        return STATUS_SUCCESS;
    }

    if (SINGLE_ENTRY(ImportList)) {
        SingleTableEntry.Count = 1;
        SingleTableEntry.Entry[0] = SINGLE_ENTRY_TO_POINTER(ImportList);
        ImportList = &SingleTableEntry;
    }

    for (i = 0; i < ImportList->Count && ImportList->Entry[i]; i += 1) {
        ImportTableEntry = ImportList->Entry[i];

        if (ImportTableEntry->LoadedImports == (PVOID)LOADED_AT_BOOT) {

            //
            // Skip this one - it was loaded by ntldr.
            //

            continue;
        }

#if DBG
        {
            ULONG ImageCount;
            PLIST_ENTRY NextEntry;
            PLDR_DATA_TABLE_ENTRY DataTableEntry;

            //
            // Assert that the first 2 entries are never dereferenced as
            // unloading the kernel or HAL would be fatal.
            //

            NextEntry = PsLoadedModuleList.Flink;

            ImageCount = 0;
            while (NextEntry != &PsLoadedModuleList && ImageCount < 2) {
                DataTableEntry = CONTAINING_RECORD(NextEntry,
                                                   LDR_DATA_TABLE_ENTRY,
                                                   InLoadOrderLinks);
                ASSERT (ImportTableEntry != DataTableEntry);
                ASSERT (DataTableEntry->LoadCount == 1);
                NextEntry = NextEntry->Flink;
                ImageCount += 1;
            }
        }
#endif

        ASSERT (ImportTableEntry->LoadCount >= 1);

        ImportTableEntry->LoadCount -= 1;

        if (ImportTableEntry->LoadCount == 0) {

            //
            // Unload this dependent DLL - we only do this to non-referenced
            // non-boot-loaded drivers.  Stop the import list recursion prior
            // to unloading - we know we're done at this point.
            //
            // Note we can continue on afterwards without restarting
            // regardless of which locks get released and reacquired
            // because this chain is private.
            //

            SavedImports = ImportTableEntry->LoadedImports;

            ImportTableEntry->LoadedImports = (PVOID)NO_IMPORTS_USED;

            Unloaded = MiCallDllUnloadAndUnloadDll ((PVOID)ImportTableEntry);

            if (Unloaded == TRUE) {

                //
                // This DLL was unloaded so recurse through its imports and
                // attempt to unload all of those too.
                //

                MiDereferenceImports ((PLOAD_IMPORTS)SavedImports);

                if ((SavedImports != (PVOID)LOADED_AT_BOOT) &&
                    (SavedImports != (PVOID)NO_IMPORTS_USED) &&
                    (!SINGLE_ENTRY(SavedImports))) {

                        ExFreePool (SavedImports);
                }
            }
            else {
                ImportTableEntry->LoadedImports = SavedImports;
            }
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
MiResolveImageReferences (
    PVOID ImageBase,
    IN PUNICODE_STRING ImageFileDirectory,
    IN PUNICODE_STRING NamePrefix OPTIONAL,
    IN BOOLEAN LoadInSessionSpace,
    OUT PCHAR *MissingProcedureName,
    OUT PWSTR *MissingDriverName,
    OUT PLOAD_IMPORTS *LoadedImports
    )

/*++

Routine Description:

    This routine resolves the references from the newly loaded driver
    to the kernel, HAL and other drivers.

Arguments:

    ImageBase - Supplies the address of which the image header resides.

    ImageFileDirectory - Supplies the directory to load referenced DLLs.

Return Value:

    Status of the image reference resolution.

--*/

{
    PCHAR MissingProcedureStorageArea;
    PVOID ImportBase;
    ULONG ImportSize;
    ULONG ImportListSize;
    ULONG Count;
    ULONG i;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
    PIMAGE_IMPORT_DESCRIPTOR Imp;
    NTSTATUS st;
    ULONG ExportSize;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    PIMAGE_THUNK_DATA NameThunk;
    PIMAGE_THUNK_DATA AddrThunk;
    PSZ ImportName;
    PLIST_ENTRY NextEntry;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PLDR_DATA_TABLE_ENTRY SingleEntry;
    ANSI_STRING AnsiString;
    UNICODE_STRING ImportName_U;
    UNICODE_STRING ImportDescriptorName_U;
    UNICODE_STRING DllToLoad;
    PVOID Section;
    PVOID BaseAddress;
    BOOLEAN PrefixedNameAllocated;
    BOOLEAN ReferenceImport;
    ULONG LinkWin32k = 0;
    ULONG LinkNonWin32k = 0;
    PLOAD_IMPORTS ImportList;
    PLOAD_IMPORTS CompactedImportList;
    BOOLEAN Loaded;

    PAGED_CODE();

    *LoadedImports = NO_IMPORTS_USED;

    MissingProcedureStorageArea = *MissingProcedureName;

    ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(
                        ImageBase,
                        TRUE,
                        IMAGE_DIRECTORY_ENTRY_IMPORT,
                        &ImportSize);

    if (ImportDescriptor == NULL) {
        return STATUS_SUCCESS;
    }

    // Count the number of imports so we can allocate enough room to
    // store them all chained off this module's LDR_DATA_TABLE_ENTRY.
    //

    Count = 0;
    for (Imp = ImportDescriptor; Imp->Name && Imp->OriginalFirstThunk; Imp += 1) {
        Count += 1;
    }

    if (Count) {
        ImportListSize = Count * sizeof(PVOID) + sizeof(SIZE_T);

        ImportList = (PLOAD_IMPORTS) ExAllocatePoolWithTag (PagedPool,
                                             ImportListSize,
                                             'TDmM');

        //
        // Zero it so we can recover gracefully if we fail in the middle.
        // If the allocation failed, just don't build the import list.
        //
    
        if (ImportList) {
            RtlZeroMemory (ImportList, ImportListSize);
            ImportList->Count = Count;
        }
    }
    else {
        ImportList = (PLOAD_IMPORTS) 0;
    }

    Count = 0;
    while (ImportDescriptor->Name && ImportDescriptor->OriginalFirstThunk) {

        ImportName = (PSZ)((PCHAR)ImageBase + ImportDescriptor->Name);

        //
        // A driver can link with win32k.sys if and only if it is a GDI
        // driver.
        // Also display drivers can only link to win32k.sys (and lego ...).
        //
        // So if we get a driver that links to win32k.sys and has more
        // than one set of imports, we will fail to load it.
        //

        LinkWin32k = LinkWin32k |
             (!_strnicmp(ImportName, "win32k", sizeof("win32k") - 1));

        //
        // We don't want to count coverage, win32k and irt (lego) since
        // display drivers CAN link against these.
        //

        LinkNonWin32k = LinkNonWin32k |
            ((_strnicmp(ImportName, "win32k", sizeof("win32k") - 1)) &&
             (_strnicmp(ImportName, "dxapi", sizeof("dxapi") - 1)) &&
             (_strnicmp(ImportName, "coverage", sizeof("coverage") - 1)) &&
             (_strnicmp(ImportName, "irt", sizeof("irt") - 1)));


        if (LinkNonWin32k && LinkWin32k) {
            MiDereferenceImports (ImportList);
            if (ImportList) {
                ExFreePool (ImportList);
            }
            return (STATUS_PROCEDURE_NOT_FOUND);
        }

        if ((!_strnicmp(ImportName, "ntdll",    sizeof("ntdll") - 1))    ||
            (!_strnicmp(ImportName, "winsrv",   sizeof("winsrv") - 1))   ||
            (!_strnicmp(ImportName, "advapi32", sizeof("advapi32") - 1)) ||
            (!_strnicmp(ImportName, "kernel32", sizeof("kernel32") - 1)) ||
            (!_strnicmp(ImportName, "user32",   sizeof("user32") - 1))   ||
            (!_strnicmp(ImportName, "gdi32",    sizeof("gdi32") - 1)) ) {

            MiDereferenceImports (ImportList);

            if (ImportList) {
                ExFreePool (ImportList);
            }
            return (STATUS_PROCEDURE_NOT_FOUND);
        }

        if ((!_strnicmp(ImportName, "ntoskrnl", sizeof("ntoskrnl") - 1)) ||
            (!_strnicmp(ImportName, "win32k", sizeof("win32k") - 1))     ||
            (!_strnicmp(ImportName, "hal",   sizeof("hal") - 1))) {

                //
                // These imports don't get refcounted because we don't
                // ever want to unload them.
                //

                ReferenceImport = FALSE;
        }
        else {
                ReferenceImport = TRUE;
        }

        RtlInitAnsiString(&AnsiString, ImportName);
        st = RtlAnsiStringToUnicodeString(&ImportName_U, &AnsiString, TRUE);
        if (!NT_SUCCESS(st)) {
            MiDereferenceImports (ImportList);
            if (ImportList) {
                ExFreePool (ImportList);
            }
            return st;
        }

        if (NamePrefix  &&
            (_strnicmp(ImportName, "ntoskrnl", sizeof("ntoskrnl") - 1) &&
             _strnicmp(ImportName, "hal", sizeof("hal") - 1))) {

            ImportDescriptorName_U.MaximumLength = ImportName_U.Length + NamePrefix->Length;
            ImportDescriptorName_U.Buffer = ExAllocatePoolWithTag (NonPagedPool,
                                                ImportDescriptorName_U.MaximumLength,
                                                'TDmM');
            if (!ImportDescriptorName_U.Buffer) {
                RtlFreeUnicodeString(&ImportName_U);
                MiDereferenceImports (ImportList);
                if (ImportList) {
                    ExFreePool (ImportList);
                }
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            ImportDescriptorName_U.Length = 0;
            RtlAppendUnicodeStringToString(&ImportDescriptorName_U, NamePrefix);
            RtlAppendUnicodeStringToString(&ImportDescriptorName_U, &ImportName_U);
            PrefixedNameAllocated = TRUE;
        } else {
            ImportDescriptorName_U = ImportName_U;
            PrefixedNameAllocated = FALSE;
        }

        Loaded = FALSE;

ReCheck:
        NextEntry = PsLoadedModuleList.Flink;
        ImportBase = NULL;

        while (NextEntry != &PsLoadedModuleList) {

            DataTableEntry = CONTAINING_RECORD(NextEntry,
                                               LDR_DATA_TABLE_ENTRY,
                                               InLoadOrderLinks);

            if (RtlEqualUnicodeString (&ImportDescriptorName_U,
                                       &DataTableEntry->BaseDllName,
                                       TRUE
                                       )) {

                ImportBase = DataTableEntry->DllBase;

                //
                // Only bump the LoadCount if this thread did not initiate
                // the load below.  If this thread initiated the load, then
                // the LoadCount has already been bumped as part of the
                // load - we only want to increment it here if we are
                // "attaching" to a previously loaded DLL.
                //

                if (Loaded == FALSE && ReferenceImport == TRUE) {
                    DataTableEntry->LoadCount += 1;
                }

                break;
            }
            NextEntry = NextEntry->Flink;
        }

        if (!ImportBase) {

            //
            // The DLL name was not located, attempt to load this dll.
            //

            DllToLoad.MaximumLength = ImportName_U.Length +
                                        ImageFileDirectory->Length +
                                        (USHORT)sizeof(WCHAR);

            DllToLoad.Buffer = ExAllocatePoolWithTag (NonPagedPool,
                                               DllToLoad.MaximumLength,
                                               'TDmM');

            if (DllToLoad.Buffer) {
                DllToLoad.Length = ImageFileDirectory->Length;
                RtlMoveMemory (DllToLoad.Buffer,
                               ImageFileDirectory->Buffer,
                               ImageFileDirectory->Length);

                RtlAppendStringToString ((PSTRING)&DllToLoad,
                                         (PSTRING)&ImportName_U);

                st = MmLoadSystemImage (&DllToLoad,
                                        NamePrefix,
                                        NULL,
                                        LoadInSessionSpace,
                                        &Section,
                                        &BaseAddress);

                ExFreePool (DllToLoad.Buffer);
            } else {
                st = STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Call any needed DLL initialization now.
            //

            if (NT_SUCCESS(st)) {

                PMM_DLL_INITIALIZE Func;
                UNICODE_STRING RegistryPath;

                Loaded = TRUE;

                Func = MiLocateExportName (BaseAddress, "DllInitialize");
                if (Func) {

                    //
                    // This DLL has an initialization routine.  Manufacture
                    // the service name string and invoke the function with
                    // it.
                    //

                    RegistryPath.MaximumLength = CmRegistryMachineSystemCurrentControlSetServices.Length +
                                                    ImportName_U.Length +
                                                    (USHORT)(2*sizeof(WCHAR));
                    RegistryPath.Buffer = ExAllocatePoolWithTag (NonPagedPool,
                                                    RegistryPath.MaximumLength,
                                                    'TDmM');
                    if (RegistryPath.Buffer) {
                        PWCHAR Dot;

                        RegistryPath.Length = CmRegistryMachineSystemCurrentControlSetServices.Length;
                        RtlMoveMemory (RegistryPath.Buffer,
                                       CmRegistryMachineSystemCurrentControlSetServices.Buffer,
                                       CmRegistryMachineSystemCurrentControlSetServices.Length);

                        RtlAppendUnicodeToString (&RegistryPath, L"\\");
                        Dot = wcschr (ImportName_U.Buffer, L'.');
                        if (Dot) {
                            ImportName_U.Length = (USHORT)((Dot - ImportName_U.Buffer)*sizeof(WCHAR));
                        }
                        RtlAppendUnicodeStringToString (&RegistryPath,
                                                        &ImportName_U);

                        //
                        // Now invoke the DLL's initialization routine.
                        //

                        st = Func (&RegistryPath);
                        ExFreePool (RegistryPath.Buffer);

                        if (!NT_SUCCESS(st)) {

                            //
                            // Unload the DLL now as its initialization
                            // routine failed.
                            //

                            BOOLEAN Found;
                            PLIST_ENTRY Entry;
                            PLDR_DATA_TABLE_ENTRY DataTableEntry2;

                            Entry = PsLoadedModuleList.Flink;

                            Found = FALSE;
                            while (Entry != &PsLoadedModuleList) {

                                DataTableEntry2 = CONTAINING_RECORD(
                                                       Entry,
                                                       LDR_DATA_TABLE_ENTRY,
                                                       InLoadOrderLinks);

                                if (BaseAddress == DataTableEntry2->DllBase) {

                                    Found = TRUE;
                                    break;
                                }

                                Entry = Entry->Flink;
                            }

                            ASSERT (Found == TRUE);
                            MmUnloadSystemImage ((PVOID)DataTableEntry2);
                        }
                    }

                }
            }

            if (!NT_SUCCESS(st)) {

                RtlFreeUnicodeString( &ImportName_U );
                if (PrefixedNameAllocated) {
                    ExFreePool( ImportDescriptorName_U.Buffer );
                }
                MiDereferenceImports (ImportList);
                if (ImportList) {
                    ExFreePool (ImportList);
                }
                return st;
            }

            goto ReCheck;
        }

        if (ReferenceImport == TRUE && ImportList) {
            ImportList->Entry[Count] = DataTableEntry;
            Count += 1;
        }

        RtlFreeUnicodeString( &ImportName_U );
        if (PrefixedNameAllocated) {
            ExFreePool ( ImportDescriptorName_U.Buffer );
        }

        *MissingDriverName = DataTableEntry->BaseDllName.Buffer;

        ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(
                                    ImportBase,
                                    TRUE,
                                    IMAGE_DIRECTORY_ENTRY_EXPORT,
                                    &ExportSize
                                    );

        if (!ExportDirectory) {
            MiDereferenceImports (ImportList);
            if (ImportList) {
                ExFreePool (ImportList);
            }
            return STATUS_DRIVER_ENTRYPOINT_NOT_FOUND;
        }

        //
        // Walk through the IAT and snap all the thunks.
        //

        if (ImportDescriptor->OriginalFirstThunk) {

            NameThunk = (PIMAGE_THUNK_DATA)((PCHAR)ImageBase + (ULONG)ImportDescriptor->OriginalFirstThunk);
            AddrThunk = (PIMAGE_THUNK_DATA)((PCHAR)ImageBase + (ULONG)ImportDescriptor->FirstThunk);

            while (NameThunk->u1.AddressOfData) {
                st = MiSnapThunk(ImportBase,
                       ImageBase,
                       NameThunk++,
                       AddrThunk++,
                       ExportDirectory,
                       ExportSize,
                       FALSE,
                       MissingProcedureName
                       );
                if (!NT_SUCCESS(st) ) {
                    MiDereferenceImports (ImportList);
                    if (ImportList) {
                        ExFreePool (ImportList);
                    }
                    return st;
                }
                *MissingProcedureName = MissingProcedureStorageArea;
            }
        }

        ImportDescriptor += 1;
    }

    //
    // All the imports are successfully loaded so establish and compact
    // the import unload list.
    //

    if (ImportList) {

        //
        // Blank entries occur for things like the kernel, HAL & win32k.sys
        // that we never want to unload.  Especially for things like
        // win32k.sys where the reference count can really hit 0.
        //

        Count = 0;
        for (i = 0; i < ImportList->Count; i += 1) {
            if (ImportList->Entry[i]) {
                Count += 1;
            }
        }

        if (Count == 0) {

            ExFreePool(ImportList);
            ImportList = NO_IMPORTS_USED;
        }
        else if (Count == 1) {
            for (i = 0; i < ImportList->Count; i += 1) {
                if (ImportList->Entry[i]) {
                    SingleEntry = POINTER_TO_SINGLE_ENTRY(ImportList->Entry[i]);
                    break;
                }
            }

            ExFreePool(ImportList);
            ImportList = (PLOAD_IMPORTS)SingleEntry;
        }
        else if (Count != ImportList->Count) {

            ImportListSize = Count * sizeof(PVOID) + sizeof(SIZE_T);

            CompactedImportList = (PLOAD_IMPORTS)
                                        ExAllocatePoolWithTag (PagedPool,
                                        ImportListSize,
                                        'TDmM');
            if (CompactedImportList) {
                CompactedImportList->Count = Count;

                Count = 0;
                for (i = 0; i < ImportList->Count; i += 1) {
                    if (ImportList->Entry[i]) {
                        CompactedImportList->Entry[Count] = ImportList->Entry[i];
                        Count += 1;
                    }
                }

                ExFreePool(ImportList);
                ImportList = CompactedImportList;
            }
        }

        *LoadedImports = ImportList;
    }
    return STATUS_SUCCESS;
}


NTSTATUS
MiSnapThunk(
    IN PVOID DllBase,
    IN PVOID ImageBase,
    IN PIMAGE_THUNK_DATA NameThunk,
    OUT PIMAGE_THUNK_DATA AddrThunk,
    IN PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    IN ULONG ExportSize,
    IN BOOLEAN SnapForwarder,
    OUT PCHAR *MissingProcedureName
    )

/*++

Routine Description:

    This function snaps a thunk using the specified Export Section data.
    If the section data does not support the thunk, then the thunk is
    partially snapped (Dll field is still non-null, but snap address is
    set).

Arguments:

    DllBase - Base of DLL being snapped to.

    ImageBase - Base of image that contains the thunks to snap.

    Thunk - On input, supplies the thunk to snap.  When successfully
        snapped, the function field is set to point to the address in
        the DLL, and the DLL field is set to NULL.

    ExportDirectory - Supplies the Export Section data from a DLL.

    SnapForwarder - determine if the snap is for a forwarder, and therefore
       Address of Data is already setup.

Return Value:


    STATUS_SUCCESS or STATUS_DRIVER_ENTRYPOINT_NOT_FOUND or
        STATUS_DRIVER_ORDINAL_NOT_FOUND

--*/

{
    BOOLEAN Ordinal;
    USHORT OrdinalNumber;
    PULONG NameTableBase;
    PUSHORT NameOrdinalTableBase;
    PULONG Addr;
    USHORT HintIndex;
    ULONG High;
    ULONG Low;
    ULONG Middle;
    LONG Result;
    NTSTATUS Status;
    PCHAR MissingProcedureName2;
    CHAR NameBuffer[ MAXIMUM_FILENAME_LENGTH ];

    PAGED_CODE();

    //
    // Determine if snap is by name, or by ordinal
    //

    Ordinal = (BOOLEAN)IMAGE_SNAP_BY_ORDINAL(NameThunk->u1.Ordinal);

    if (Ordinal && !SnapForwarder) {

        OrdinalNumber = (USHORT)(IMAGE_ORDINAL(NameThunk->u1.Ordinal) -
                         ExportDirectory->Base);

        *MissingProcedureName = (PCHAR)(ULONG_PTR)OrdinalNumber;

    } else {

        //
        // Change AddressOfData from an RVA to a VA.
        //

        if (!SnapForwarder) {
            NameThunk->u1.AddressOfData = (ULONG_PTR)ImageBase + NameThunk->u1.AddressOfData;
        }

        strncpy( *MissingProcedureName,
                 &((PIMAGE_IMPORT_BY_NAME)NameThunk->u1.AddressOfData)->Name[0],
                 MAXIMUM_FILENAME_LENGTH - 1
               );

        //
        // Lookup Name in NameTable
        //

        NameTableBase = (PULONG)((PCHAR)DllBase + (ULONG)ExportDirectory->AddressOfNames);
        NameOrdinalTableBase = (PUSHORT)((PCHAR)DllBase + (ULONG)ExportDirectory->AddressOfNameOrdinals);

        //
        // Before dropping into binary search, see if
        // the hint index results in a successful
        // match. If the hint index is zero, then
        // drop into binary search.
        //

        HintIndex = ((PIMAGE_IMPORT_BY_NAME)NameThunk->u1.AddressOfData)->Hint;
        if ((ULONG)HintIndex < ExportDirectory->NumberOfNames &&
            !strcmp((PSZ)((PIMAGE_IMPORT_BY_NAME)NameThunk->u1.AddressOfData)->Name,
             (PSZ)((PCHAR)DllBase + NameTableBase[HintIndex]))) {
            OrdinalNumber = NameOrdinalTableBase[HintIndex];

        } else {

            //
            // Lookup the import name in the name table using a binary search.
            //

            Low = 0;
            High = ExportDirectory->NumberOfNames - 1;

            while (High >= Low) {

                //
                // Compute the next probe index and compare the import name
                // with the export name entry.
                //

                Middle = (Low + High) >> 1;
                Result = strcmp(&((PIMAGE_IMPORT_BY_NAME)NameThunk->u1.AddressOfData)->Name[0],
                                (PCHAR)((PCHAR)DllBase + NameTableBase[Middle]));

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
                return STATUS_DRIVER_ENTRYPOINT_NOT_FOUND;
            } else {
                OrdinalNumber = NameOrdinalTableBase[Middle];
            }
        }
    }

    //
    // If OrdinalNumber is not within the Export Address Table,
    // then DLL does not implement function. Snap to LDRP_BAD_DLL.
    //

    if ((ULONG)OrdinalNumber >= ExportDirectory->NumberOfFunctions) {
        Status = STATUS_DRIVER_ORDINAL_NOT_FOUND;

    } else {

        MissingProcedureName2 = NameBuffer;

        Addr = (PULONG)((PCHAR)DllBase + (ULONG)ExportDirectory->AddressOfFunctions);
        (PULONG)(AddrThunk->u1.Function) = (PULONG)((PCHAR)DllBase + Addr[OrdinalNumber]);

        // AddrThunk s/b used from here on.

        Status = STATUS_SUCCESS;

        if ( ((ULONG_PTR)AddrThunk->u1.Function > (ULONG_PTR)ExportDirectory) &&
             ((ULONG_PTR)AddrThunk->u1.Function < ((ULONG_PTR)ExportDirectory + ExportSize)) ) {

            UNICODE_STRING UnicodeString;
            ANSI_STRING ForwardDllName;

            PLIST_ENTRY NextEntry;
            PLDR_DATA_TABLE_ENTRY DataTableEntry;
            ULONG ExportSize;
            PIMAGE_EXPORT_DIRECTORY ExportDirectory;

            Status = STATUS_DRIVER_ENTRYPOINT_NOT_FOUND;

            //
            // Include the dot in the length so we can do prefix later on.
            //

            ForwardDllName.Buffer = (PCHAR)AddrThunk->u1.Function;
            ForwardDllName.Length = (USHORT)(strchr(ForwardDllName.Buffer, '.') -
                                           ForwardDllName.Buffer + 1);
            ForwardDllName.MaximumLength = ForwardDllName.Length;

            if (NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeString,
                                                        &ForwardDllName,
                                                        TRUE))) {

                NextEntry = PsLoadedModuleList.Flink;

                while (NextEntry != &PsLoadedModuleList) {

                    DataTableEntry = CONTAINING_RECORD(NextEntry,
                                                       LDR_DATA_TABLE_ENTRY,
                                                       InLoadOrderLinks);

                    //
                    // We have to do a case INSENSITIVE comparison for
                    // forwarder because the linker just took what is in the
                    // def file, as opposed to looking in the exporting
                    // image for the name.
                    // we also use the prefix function to ignore the .exe or
                    // .sys or .dll at the end.
                    //

                    if (RtlPrefixString((PSTRING)&UnicodeString,
                                        (PSTRING)&DataTableEntry->BaseDllName,
                                        TRUE)) {

                        ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)
                            RtlImageDirectoryEntryToData(DataTableEntry->DllBase,
                                                         TRUE,
                                                         IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                         &ExportSize);

                        if (ExportDirectory) {

                            IMAGE_THUNK_DATA thunkData;
                            PIMAGE_IMPORT_BY_NAME addressOfData;
                            ULONG length;

                            // one extra byte for NULL,

                            length = strlen(ForwardDllName.Buffer +
                                                ForwardDllName.Length) + 1;

                            addressOfData = (PIMAGE_IMPORT_BY_NAME)
                                ExAllocatePoolWithTag (PagedPool,
                                                      length +
                                                   sizeof(IMAGE_IMPORT_BY_NAME),
                                                   '  mM');

                            if (addressOfData) {

                                RtlCopyMemory(&(addressOfData->Name[0]),
                                              ForwardDllName.Buffer +
                                                  ForwardDllName.Length,
                                              length);

                                addressOfData->Hint = 0;

                                (PIMAGE_IMPORT_BY_NAME)(thunkData.u1.AddressOfData) = addressOfData;

                                Status = MiSnapThunk(DataTableEntry->DllBase,
                                                     ImageBase,
                                                     &thunkData,
                                                     &thunkData,
                                                     ExportDirectory,
                                                     ExportSize,
                                                     TRUE,
                                                     &MissingProcedureName2
                                                    );

                                ExFreePool(addressOfData);

                                AddrThunk->u1 = thunkData.u1;
                            }
                        }

                        break;
                    }

                    NextEntry = NextEntry->Flink;
                }

                RtlFreeUnicodeString(&UnicodeString);
            }

        }

    }
    return Status;
}
#if 0
PVOID
MiLookupImageSectionByName (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN PCHAR SectionName,
    OUT PULONG SectionSize
    )

/*++

Routine Description:

    This function locates a Directory Entry within the image header
    and returns either the virtual address or seek address of the
    data the Directory describes.

Arguments:

    Base - Supplies the base of the image or data file.

    MappedAsImage - FALSE if the file is mapped as a data file.
                  - TRUE if the file is mapped as an image.

    SectionName - Supplies the name of the section to lookup.

    SectionSize - Return the size of the section.

Return Value:

    NULL - The file does not contain data for the specified section.

    NON-NULL - Returns the address where the section is mapped in memory.

--*/

{
    ULONG i, j, Match;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER NtSection;

    NtHeaders = RtlImageNtHeader(Base);
    NtSection = IMAGE_FIRST_SECTION( NtHeaders );
    for (i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++) {
        Match = TRUE;
        for (j = 0; j < IMAGE_SIZEOF_SHORT_NAME; j++) {
            if (SectionName[j] != NtSection->Name[j]) {
                Match = FALSE;
                break;
            }
            if (SectionName[j] == '\0') {
                break;
            }
        }
        if (Match) {
            break;
        }
        NtSection += 1;
    }
    if (Match) {
        *SectionSize = NtSection->SizeOfRawData;
        if (MappedAsImage) {
            return( ((PCHAR)Base + NtSection->VirtualAddress));
        } else {
            return( ((PCHAR)Base + NtSection->PointerToRawData));
        }
    }
    return( NULL );
}
#endif //0


NTSTATUS
MmCheckSystemImage(
    IN HANDLE ImageFileHandle
    )

/*++

Routine Description:

    This function ensures the checksum for a system image is correct
    and matches the data in the image.

Arguments:

    ImageFileHandle - Supplies the file handle of the image.

Return Value:

    Status value.

--*/

{

    NTSTATUS Status;
    HANDLE Section;
    PVOID ViewBase;
    SIZE_T ViewSize;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION StandardInfo;

    PAGED_CODE();

    Status = ZwCreateSection(
                &Section,
                SECTION_MAP_EXECUTE,
                NULL,
                NULL,
                PAGE_EXECUTE,
                SEC_COMMIT,
                ImageFileHandle
                );

    if ( !NT_SUCCESS(Status) ) {
        return Status;
    }

    ViewBase = NULL;
    ViewSize = 0;

    Status = ZwMapViewOfSection(
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

    if ( !NT_SUCCESS(Status) ) {
        ZwClose(Section);
        return Status;
    }

    //
    // Now the image is mapped as a data file... Calculate its size and then
    // check its checksum.
    //

    Status = ZwQueryInformationFile(
                ImageFileHandle,
                &IoStatusBlock,
                &StandardInfo,
                sizeof(StandardInfo),
                FileStandardInformation
                );

    if ( NT_SUCCESS(Status) ) {

        try {
            if (!LdrVerifyMappedImageMatchesChecksum(ViewBase,StandardInfo.EndOfFile.LowPart)) {
                Status = STATUS_IMAGE_CHECKSUM_MISMATCH;
            }
#if !defined(NT_UP)
            if ( !MmVerifyImageIsOkForMpUse(ViewBase) ) {
                Status = STATUS_IMAGE_MP_UP_MISMATCH;
                }
#endif // NT_UP
        } except (EXCEPTION_EXECUTE_HANDLER) {
            Status = STATUS_IMAGE_CHECKSUM_MISMATCH;
        }
    }

    ZwUnmapViewOfSection(NtCurrentProcess(),ViewBase);
    ZwClose(Section);
    return Status;
}

#if !defined(NT_UP)
BOOLEAN
MmVerifyImageIsOkForMpUse(
    IN PVOID BaseAddress
    )
{
    PIMAGE_NT_HEADERS NtHeaders;

    PAGED_CODE();

    //
    // If the file is an image file, then subtract the two checksum words
    // in the optional header from the computed checksum before adding
    // the file length, and set the value of the header checksum.
    //

    NtHeaders = RtlImageNtHeader(BaseAddress);
    if (NtHeaders != NULL) {
        if ( KeNumberProcessors > 1 &&
             (NtHeaders->FileHeader.Characteristics & IMAGE_FILE_UP_SYSTEM_ONLY) ) {
            return FALSE;
        }
    }
    return TRUE;
}
#endif // NT_UP


PFN_NUMBER
MiDeleteSystemPagableVm (
    IN PMMPTE PointerPte,
    IN PFN_NUMBER NumberOfPtes,
    IN MMPTE NewPteValue,
    IN LOGICAL SessionAllocation,
    OUT PPFN_NUMBER ResidentPages
    )

/*++

Routine Description:

    This function deletes pagable system address space (paged pool
    or driver pagable sections).

Arguments:

    PointerPte - Supplies the start of the PTE range to delete.

    NumberOfPtes - Supplies the number of PTEs in the range.

    NewPteValue - Supplies the new value for the PTE.

    SessionAllocation - Supplies TRUE if this is a range in session space.  If
                        TRUE is specified, it is assumed that the caller has
                        already attached to the relevant session.

                        If FALSE is supplied, then it is assumed that the range
                        is in the systemwide global space instead.

    ResidentPages - If not NULL, the number of resident pages freed is
                    returned here.

Return Value:

    Returns the number of pages actually freed.

--*/

{
    PFN_NUMBER PageFrameIndex;
    MMPTE PteContents;
    PMMPFN Pfn1;
    PFN_NUMBER ValidPages;
    PFN_NUMBER PagesRequired;
    MMPTE NewContents;
    WSLE_NUMBER WsIndex;
    KIRQL OldIrql;
    MMPTE_FLUSH_LIST PteFlushList;
    MMPTE JunkPte;
    MMWSLENTRY Locked;

    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

    ValidPages = 0;
    PagesRequired = 0;
    PteFlushList.Count = 0;
    NewContents = NewPteValue;
    while (NumberOfPtes != 0) {
        PteContents = *PointerPte;

        if (PteContents.u.Long != ZeroKernelPte.u.Long) {

            if (PteContents.u.Hard.Valid == 1) {

                if (SessionAllocation == TRUE) {
                    LOCK_SESSION_SPACE_WS (OldIrql);
                }
                else {
                    LOCK_SYSTEM_WS (OldIrql);
                }

                PteContents = *(volatile MMPTE *)PointerPte;
                if (PteContents.u.Hard.Valid == 0) {
                    if (SessionAllocation == TRUE) {
                        UNLOCK_SESSION_SPACE_WS (OldIrql);
                    }
                    else {
                        UNLOCK_SYSTEM_WS (OldIrql);
                    }

                    continue;
                }

                //
                // Delete the page.
                //

                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&PteContents);

                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

                //
                // Check to see if this is a pagable page in which
                // case it needs to be removed from the working set list.
                //

                WsIndex = Pfn1->u1.WsIndex;
                if (WsIndex == 0) {
                    ValidPages += 1;
                } else {
                    if (SessionAllocation == FALSE) {
                        MiRemoveWsle (WsIndex,
                                      MmSystemCacheWorkingSetList );
                        MiReleaseWsle (WsIndex, &MmSystemCacheWs);
                    }
                    else {
                        WsIndex = MiLocateWsle(
                                              MiGetVirtualAddressMappedByPte(PointerPte),
                                              MmSessionSpace->Vm.VmWorkingSetList,
                                              WsIndex
                                              );

                        ASSERT (WsIndex != WSLE_NULL_INDEX);

                        //
                        // Check to see if this entry is locked in
                        // the working set or locked in memory.
                        //

                        Locked = MmSessionSpace->Wsle[WsIndex].u1.e1;

                        MiRemoveWsle (WsIndex, MmSessionSpace->Vm.VmWorkingSetList);

                        MiReleaseWsle (WsIndex, &MmSessionSpace->Vm);

                        if (Locked.LockedInWs == 1 || Locked.LockedInMemory == 1) {

                            //
                            // This entry is locked.
                            //
#if DBG
                            DbgPrint("MiDeleteSystemPagableVm: Session PointerPte 0x%p, Pfn 0x%p Locked in memory\n",
                                PointerPte,
                                Pfn1);

                            DbgBreakPoint();
#endif
                            ASSERT (WsIndex < MmSessionSpace->Vm.VmWorkingSetList->FirstDynamic);
                            MmSessionSpace->Vm.VmWorkingSetList->FirstDynamic -= 1;

                            if (WsIndex != MmSessionSpace->Vm.VmWorkingSetList->FirstDynamic) {
                                ULONG Entry;
                                PVOID SwapVa;

                                Entry = MmSessionSpace->Vm.VmWorkingSetList->FirstDynamic;
                                ASSERT (MmSessionSpace->Wsle[Entry].u1.e1.Valid);
                                SwapVa = MmSessionSpace->Wsle[Entry].u1.VirtualAddress;
                                SwapVa = PAGE_ALIGN (SwapVa);

                                MiSwapWslEntries (Entry,
                                                  WsIndex,
                                                  &MmSessionSpace->Vm);
                            }
                        }
                        else {
                            ASSERT (WsIndex >= MmSessionSpace->Vm.VmWorkingSetList->FirstDynamic);
                        }
                    }
                }

                if (SessionAllocation == TRUE) {
                    UNLOCK_SESSION_SPACE_WS (OldIrql);
                }
                else {
                    UNLOCK_SYSTEM_WS (OldIrql);
                }

                LOCK_PFN (OldIrql);
#if DBG
                if ((Pfn1->u3.e2.ReferenceCount > 1) &&
                    (Pfn1->u3.e1.WriteInProgress == 0)) {
                    DbgPrint ("MM:SYSLOAD - deleting pool locked for I/O pte %p, pfn %p, share=%x, refcount=%x, wsindex=%x\n",
                             PointerPte,
                             PageFrameIndex,
                             Pfn1->u2.ShareCount,
                             Pfn1->u3.e2.ReferenceCount,
                             Pfn1->u1.WsIndex);
                    //
                    // This case is valid only if the page being deleted
                    // contained a lookaside free list entry that wasn't mapped
                    // and multiple threads faulted on it and waited together.
                    // Some of the faulted threads are still on the ready
                    // list but haven't run yet, and so still have references
                    // to this page that they picked up during the fault.
                    // But this current thread has already allocated the
                    // lookaside entry and is now freeing the entire page.
                    //
                    // BUT - if it is NOT the above case, we really should
                    // trap here.  However, we don't have a good way to
                    // distinguish between the two cases.  Note
                    // that this complication was inserted when we went to
                    // cmpxchg8 because using locks would prevent anyone from
                    // accessing the lookaside freelist flinks like this.
                    //
                    // So, the ASSERT below comes out, but we leave the print
                    // above in (with more data added) for the case where it's
                    // not a lookaside contender with the reference count, but
                    // is instead a truly bad reference that needs to be
                    // debugged.  The system should crash shortly thereafter
                    // and we'll at least have the above print to help us out.
                    //
                    // ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
                }
#endif //DBG
                //
                // Check if this is a prototype PTE
                //
                if (Pfn1->u3.e1.PrototypePte == 1) {

                    PMMPTE PointerPde;

                    ASSERT (SessionAllocation == TRUE);

                    //
                    // Capture the state of the modified bit for this
                    // PTE.
                    //

                    MI_CAPTURE_DIRTY_BIT_TO_PFN (PointerPte, Pfn1);

                    //
                    // Decrement the share and valid counts of the page table
                    // page which maps this PTE.
                    //

                    PointerPde = MiGetPteAddress (PointerPte);
                    if (PointerPde->u.Hard.Valid == 0) {
#if !defined (_WIN64)
                        if (!NT_SUCCESS(MiCheckPdeForPagedPool (PointerPte))) {
#endif
                            KeBugCheckEx (MEMORY_MANAGEMENT,
                                          0x61940, 
                                          (ULONG_PTR)PointerPte,
                                          (ULONG_PTR)PointerPde->u.Long,
                                          (ULONG_PTR)MiGetVirtualAddressMappedByPte(PointerPte));
#if !defined (_WIN64)
                        }
#endif
                    }

                    MiDecrementShareAndValidCount (MI_GET_PAGE_FRAME_FROM_PTE (PointerPde));

                    //
                    // Decrement the share count for the physical page.
                    //

                    MiDecrementShareCount (PageFrameIndex);

                    //
                    // No need to worry about fork prototype PTEs
                    // for kernel addresses.
                    //

                    ASSERT (PointerPte > MiHighestUserPte);

                } else {
                    MiDecrementShareAndValidCount (Pfn1->PteFrame);
                    MI_SET_PFN_DELETED (Pfn1);
                    MiDecrementShareCountOnly (PageFrameIndex);
                }

                MI_WRITE_INVALID_PTE (PointerPte, NewContents);
                UNLOCK_PFN (OldIrql);

                //
                // Flush the TB for this page.
                //

                if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {

                    //
                    // We cannot rewrite the PTE without creating a race
                    // condition.  So don't let the TB flush do it anymore.
                    //
                    // PteFlushList.FlushPte[PteFlushList.Count] = PointerPte;
                    //

                    PteFlushList.FlushPte[PteFlushList.Count] =
                        (PMMPTE)&JunkPte;

                    PteFlushList.FlushVa[PteFlushList.Count] =
                                    MiGetVirtualAddressMappedByPte (PointerPte);
                    PteFlushList.Count += 1;
                }

            } else if (PteContents.u.Soft.Prototype) {

                ASSERT (SessionAllocation == TRUE);

                //
                // No need to worry about fork prototype PTEs
                // for kernel addresses.
                //

                ASSERT (PointerPte >= MiHighestUserPte);

                MI_WRITE_INVALID_PTE (PointerPte, NewContents);

                //
                // We currently commit for all prototype kernel mappings since
                // we could copy-on-write.
                //

            } else if (PteContents.u.Soft.Transition == 1) {

                LOCK_PFN (OldIrql);

                PteContents = *(volatile MMPTE *)PointerPte;

                if (PteContents.u.Soft.Transition == 0) {
                    UNLOCK_PFN (OldIrql);
                    continue;
                }

                //
                // Transition, release page.
                //

                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&PteContents);

                //
                // Set the pointer to PTE as empty so the page
                // is deleted when the reference count goes to zero.
                //

                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

                MI_SET_PFN_DELETED (Pfn1);

                MiDecrementShareCount (Pfn1->PteFrame);

                //
                // Check the reference count for the page, if the reference
                // count is zero, move the page to the free list, if the
                // reference count is not zero, ignore this page.  When the
                // reference count goes to zero, it will be placed on the
                // free list.
                //

                if (Pfn1->u3.e2.ReferenceCount == 0) {
                    MiUnlinkPageFromList (Pfn1);
                    MiReleasePageFileSpace (Pfn1->OriginalPte);
                    MiInsertPageInList (MmPageLocationList[FreePageList],
                                        PageFrameIndex);
                }
#if DBG
                if ((Pfn1->u3.e2.ReferenceCount > 1) &&
                    (Pfn1->u3.e1.WriteInProgress == 0)) {
                    DbgPrint ("MM:SYSLOAD - deleting pool locked for I/O %p\n",
                             PageFrameIndex);
                    DbgBreakPoint();
                }
#endif //DBG

                MI_WRITE_INVALID_PTE (PointerPte, NewContents);
                UNLOCK_PFN (OldIrql);
            } else {

                //
                // Demand zero, release page file space.
                //
                if (PteContents.u.Soft.PageFileHigh != 0) {
                    LOCK_PFN (OldIrql);
                    MiReleasePageFileSpace (PteContents);
                    UNLOCK_PFN (OldIrql);
                }

                MI_WRITE_INVALID_PTE (PointerPte, NewContents);
            }

            PagesRequired += 1;
        }

        NumberOfPtes -= 1;
        PointerPte += 1;
    }

    //
    // There is the thorny case where one of the pages we just deleted could
    // get faulted back in when we released the PFN lock above within the loop.
    // The only time when this can happen is if a thread faulted during an
    // interlocked pool allocation for an address that we've just deleted.
    //
    // If this thread sees the NewContents in the PTE, it will treat it as
    // demand zero, and incorrectly allocate a page, PTE and WSL.  It will
    // reference it once and realize it needs to reread the lookaside listhead
    // and restart the operation.  But this page would live on in paged pool
    // as modified (but unused) until the paged pool allocator chose to give
    // out its virtual address again.
    //
    // The code below rewrites the PTEs which is really bad if another thread
    // gets a zero page between our first setting of the PTEs above and our
    // second setting below - because we'll reset the PTE to demand zero, but
    // we'll still have a WSL entry that's valid, and we spiral from there.
    //
    // We really should remove the writing of the PTE below since we've already
    // done it above.  But for now, we're leaving it in - it's harmless because
    // we've chosen to fix this problem by checking for this case when we
    // materialize demand zero pages.  Note that we have to fix this problem
    // by checking in the demand zero path because a thread could be coming into
    // that path any time before or after we flush the PTE list and any fixes
    // here could only address the before case, not the after.
    //

    if (SessionAllocation == TRUE) {

        //
        // Session space has no ASN - flush the entire TB.
        //
    
        MI_FLUSH_ENTIRE_SESSION_TB (TRUE, TRUE);
    }

    LOCK_PFN (OldIrql);
    MiFlushPteList (&PteFlushList, TRUE, NewContents);
    UNLOCK_PFN (OldIrql);

    if (ARGUMENT_PRESENT(ResidentPages)) {
        *ResidentPages = ValidPages;
    }

    return PagesRequired;
}

VOID
MiSetImageProtect (
    IN PSEGMENT Segment,
    IN ULONG Protection
    )

/*++

Routine Description:

    This function sets the protection of all prototype PTEs to the specified
    protection.

Arguments:

     Segment - Supplies a pointer to the segment to protect.

     Protection - Supplies the protection value to set.

Return Value:

     None.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    MMPTE PteContents;
    PSUBSECTION SubSection;

    //
    // Set the subsection protections.
    //

    ASSERT (Segment->ControlArea->u.Flags.GlobalOnlyPerSession == 0);

    if ((Protection & MM_PROTECTION_WRITE_MASK) == 0) {
        SubSection = (PSUBSECTION)(Segment->ControlArea + 1);
        SubSection->u.SubsectionFlags.Protection = Protection;
        SubSection->u.SubsectionFlags.ReadOnly = 1;
    }

    PointerPte = Segment->PrototypePte;
    LastPte = PointerPte + Segment->NonExtendedPtes;

    MmLockPagedPool (PointerPte, (LastPte - PointerPte) * sizeof (MMPTE));

    do {
        PteContents = *PointerPte;
        ASSERT (PteContents.u.Hard.Valid == 0);
        if (PteContents.u.Long != ZeroPte.u.Long) {
            if ((PteContents.u.Soft.Prototype == 0) &&
                (PteContents.u.Soft.Transition == 1)) {
                if (MiSetProtectionOnTransitionPte (PointerPte, Protection)) {
                    continue;
                }
            }
            else {
                PointerPte->u.Soft.Protection = Protection;
            }
        }
        PointerPte += 1;
    } while (PointerPte < LastPte);

    PointerPte = Segment->PrototypePte;
    MmUnlockPagedPool (PointerPte, (LastPte - PointerPte) * sizeof (MMPTE));

    return;
}


VOID
MiSetSystemCodeProtection (
    IN PMMPTE FirstPte,
    IN PMMPTE LastPte
    )

/*++

Routine Description:

    This function sets the protection of system code to read only.
    Note this is different from protecting section-backed code like win32k.

Arguments:

    FirstPte - Supplies the starting PTE.

    LastPte - Supplies the ending PTE.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    MMPTE PteContents;
    MMPTE TempPte;
    MMPTE PreviousPte;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerProtoPte;
    ULONG ProtectionMask;
    PMMPFN Pfn1;
    PMMPFN ProtoPfn;
    LOGICAL SessionAddress;

#if defined(_X86_)
    ASSERT (MI_IS_PHYSICAL_ADDRESS(MiGetVirtualAddressMappedByPte(FirstPte)) == 0);
#endif

    SessionAddress = FALSE;

    if (MI_IS_SESSION_ADDRESS(MiGetVirtualAddressMappedByPte(FirstPte))) {
        SessionAddress = TRUE;
    }
    
    ProtectionMask = MM_EXECUTE_READ;

    //
    // Make these PTEs read only.
    //
    // Note that the write bit may already be off (in the valid PTE) if the
    // page has already been inpaged from the paging file and has not since
    // been dirtied.
    //

    PointerPte = FirstPte;

    LOCK_PFN (OldIrql);

    while (PointerPte <= LastPte) {

        PteContents = *PointerPte;

        if ((PteContents.u.Long == 0) || (!*MiPteStr)) {
            PointerPte += 1;
            continue;
        }

        if (PteContents.u.Hard.Valid == 1) {

            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
            Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;

            //
            // Note the dirty and write bits get turned off here.
            // Any existing pagefile addresses for clean pages are preserved.
            //

            if (MI_IS_PTE_DIRTY(PteContents)) {
                MI_CAPTURE_DIRTY_BIT_TO_PFN (&PteContents, Pfn1);
            }

            MI_MAKE_VALID_PTE (TempPte,
                               PteContents.u.Hard.PageFrameNumber,
                               Pfn1->OriginalPte.u.Soft.Protection,
                               PointerPte);

            if (SessionAddress == TRUE) {
        
                //
                // Session space has no ASN - flush the entire TB.
                //
            
                MI_FLUSH_SINGLE_SESSION_TB (MiGetVirtualAddressMappedByPte (PointerPte),
                             TRUE,
                             TRUE,
                             (PHARDWARE_PTE)PointerPte,
                             TempPte.u.Flush,
                             PreviousPte);
            }
            else {
                KeFlushSingleTb (MiGetVirtualAddressMappedByPte (PointerPte),
                                 TRUE,
                                 TRUE,
                                 (PHARDWARE_PTE)PointerPte,
                                 TempPte.u.Flush);
            }
        }
        else if (PteContents.u.Soft.Prototype == 1) {

            if (SessionAddress == TRUE) {
                PointerPte->u.Proto.ReadOnly = 1;
            }
            else {
                PointerProtoPte = MiPteToProto(PointerPte);
    
                if (!MI_IS_PHYSICAL_ADDRESS(PointerProtoPte)) {
                    PointerPde = MiGetPteAddress (PointerProtoPte);
                    if (PointerPde->u.Hard.Valid == 0) {
                        MiMakeSystemAddressValidPfn (PointerProtoPte);
    
                        //
                        // The world may change if we had to wait.
                        //
        
                        PteContents = *PointerPte;
                        if ((PteContents.u.Hard.Valid == 1) ||
                            (PteContents.u.Soft.Prototype == 0)) {
                                continue;
                        }
                    }
    
                    ProtoPfn = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);
                    MI_ADD_LOCKED_PAGE_CHARGE(ProtoPfn, 12);
                    ProtoPfn->u3.e2.ReferenceCount += 1;
                    ASSERT (ProtoPfn->u3.e2.ReferenceCount > 1);
                }
    
                PteContents = *PointerProtoPte;
    
                if (PteContents.u.Long != ZeroPte.u.Long) {
    
                    ASSERT (PteContents.u.Hard.Valid == 0);
    
                    PointerProtoPte->u.Soft.Protection = ProtectionMask;
    
                    if ((PteContents.u.Soft.Prototype == 0) &&
                        (PteContents.u.Soft.Transition == 1)) {
                        Pfn1 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);
                        Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;
                    }
                }

                if (!MI_IS_PHYSICAL_ADDRESS(PointerProtoPte)) {
                    ASSERT (ProtoPfn->u3.e2.ReferenceCount > 1);
                    MI_REMOVE_LOCKED_PAGE_CHARGE(ProtoPfn, 13);
                    ProtoPfn->u3.e2.ReferenceCount -= 1;
                }
            }
        }
        else if (PteContents.u.Soft.Transition == 1) {

            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);
            Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;
            PointerPte->u.Soft.Protection = ProtectionMask;

        }
        else {

            //
            // Must be page file space or demand zero.
            //

            PointerPte->u.Soft.Protection = ProtectionMask;
        }
        PointerPte += 1;
    }

    UNLOCK_PFN (OldIrql);
}


VOID
MiWriteProtectSystemImage (
    IN PVOID DllBase
    )

/*++

Routine Description:

    This function sets the protection of a system component to read only.

Arguments:

    DllBase - Supplies the base address of the system component.

Return Value:

    None.

--*/

{
    ULONG SectionProtection;
    ULONG NumberOfSubsections;
    ULONG SectionVirtualSize;
    ULONG ImageAlignment;
    ULONG OffsetToSectionTable;
    ULONG NumberOfPtes;
    ULONG_PTR VirtualAddress;
    ULONG_PTR LastVirtualAddress;
    PMMPTE PointerPte;
    PMMPTE FirstPte;
    PMMPTE LastPte;
    PMMPTE LastImagePte;
    PMMPTE WritablePte;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_SECTION_HEADER SectionTableEntry;

    PAGED_CODE();

    if (MI_IS_PHYSICAL_ADDRESS(DllBase)) {
        return;
    }

    NtHeader = RtlImageNtHeader (DllBase);

    ASSERT (NtHeader);

    ImageAlignment = NtHeader->OptionalHeader.SectionAlignment;

    //
    // All session drivers must be one way or the other - no mixing is allowed
    // within multiple copy-on-write drivers.
    //

    if (MI_IS_SESSION_ADDRESS(DllBase) == 0) {

        //
        // Images prior to NT5 were not protected from stepping all over
        // their (and others) code and readonly data.  Here we somewhat
        // preserve that behavior, but don't allow them to step on anyone else.
        //

        //
        // Note that drivers that are built for NT5 have presumably run on
        // NT5, and these get the benefits of full protection.
        //

        if ((NtHeader->OptionalHeader.MajorOperatingSystemVersion < 5) ||
            (NtHeader->OptionalHeader.MajorOperatingSystemVersion > 10)) {
            return;
        }
    
        if ((NtHeader->OptionalHeader.MajorImageVersion < 5) ||
            (NtHeader->OptionalHeader.MajorImageVersion > 10)) {
            return;
        }
    }

    NumberOfPtes = BYTES_TO_PAGES (NtHeader->OptionalHeader.SizeOfImage);

    FileHeader = &NtHeader->FileHeader;

    NumberOfSubsections = FileHeader->NumberOfSections;

    ASSERT (NumberOfSubsections != 0);

    OffsetToSectionTable = sizeof(ULONG) +
                              sizeof(IMAGE_FILE_HEADER) +
                              FileHeader->SizeOfOptionalHeader;

    SectionTableEntry = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeader +
                            OffsetToSectionTable);

    //
    // Verify the image contains subsections ordered by increasing virtual
    // address and that there are no overlaps.
    //

    FirstPte = NULL;
    LastVirtualAddress = (ULONG_PTR)DllBase;

    for ( ; NumberOfSubsections > 0; NumberOfSubsections -= 1, SectionTableEntry += 1) {

        if (SectionTableEntry->Misc.VirtualSize == 0) {
            SectionVirtualSize = SectionTableEntry->SizeOfRawData;
        }
        else {
            SectionVirtualSize = SectionTableEntry->Misc.VirtualSize;
        }

        VirtualAddress = (ULONG_PTR)DllBase + SectionTableEntry->VirtualAddress;
        if (VirtualAddress <= LastVirtualAddress) {

            //
            // Subsections are not in an increasing virtual address ordering.
            // No protection is provided for such a poorly linked image.
            //

            KdPrint (("MM:sysload - Image at %p is badly linked\n", DllBase));
            return;
        }
        LastVirtualAddress = VirtualAddress + SectionVirtualSize - 1;
    }

    NumberOfSubsections = FileHeader->NumberOfSections;
    ASSERT (NumberOfSubsections != 0);

    SectionTableEntry = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeader +
                            OffsetToSectionTable);

    LastVirtualAddress = 0;

    //
    // Set writable PTE here so the image headers are excluded.  This is
    // needed so that locking down of sections can continue to edit the
    // image headers for counts.
    //

    WritablePte = MiGetPteAddress ((ULONG_PTR)(SectionTableEntry + NumberOfSubsections) - 1);
    LastImagePte = MiGetPteAddress(DllBase) + NumberOfPtes;

    for ( ; NumberOfSubsections > 0; NumberOfSubsections -= 1, SectionTableEntry += 1) {

        if (SectionTableEntry->Misc.VirtualSize == 0) {
            SectionVirtualSize = SectionTableEntry->SizeOfRawData;
        }
        else {
            SectionVirtualSize = SectionTableEntry->Misc.VirtualSize;
        }

        VirtualAddress = (ULONG_PTR)DllBase + SectionTableEntry->VirtualAddress;

        PointerPte = MiGetPteAddress (VirtualAddress);

        if (PointerPte >= LastImagePte) {

            //
            // Skip relocation subsections (which aren't given VA space).
            //

            break;
        }

        SectionProtection = (SectionTableEntry->Characteristics & (IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE));

        if (SectionProtection & IMAGE_SCN_MEM_WRITE) {

            //
            // This is a writable subsection, skip it.  Make sure if it's
            // sharing a PTE (and update the linker so this doesn't happen
            // for the kernel at least) that the last PTE isn't made
            // read only.
            //

            WritablePte = MiGetPteAddress (VirtualAddress + SectionVirtualSize - 1);

            if (LastVirtualAddress) {
                LastPte = MiGetPteAddress (LastVirtualAddress);

                if (LastPte == PointerPte) {
                    LastPte -= 1;
                }

                if (FirstPte <= LastPte) {

                    ASSERT (PointerPte < LastImagePte);

                    if (LastPte >= LastImagePte) {
                        LastPte = LastImagePte - 1;
                    }

                    MiSetSystemCodeProtection (FirstPte, LastPte);
                }

                LastVirtualAddress = 0;
            }
            continue;
        }

        if (LastVirtualAddress == 0) {

            //
            // There is no previous subsection or the previous
            // subsection was writable.  Thus the current starting PTE
            // could be mapping both a readonly and a readwrite
            // subsection if the image alignment is less than PAGE_SIZE.
            // These cases (in either order) are handled here.
            //

            if (PointerPte == WritablePte) {
                LastPte = MiGetPteAddress (VirtualAddress + SectionVirtualSize - 1);
                if (PointerPte == LastPte) {

                    //
                    // Nothing can be protected in this subsection
                    // due to the image alignment specified for the executable.
                    //

                    continue;
                }
                PointerPte += 1;
            }
            FirstPte = PointerPte;
        }

        LastVirtualAddress = VirtualAddress + SectionVirtualSize - 1;
    }

    if (LastVirtualAddress) {
        LastPte = MiGetPteAddress (LastVirtualAddress);

        if ((FirstPte <= LastPte) && (FirstPte < LastImagePte)) {

            if (LastPte >= LastImagePte) {
                LastPte = LastImagePte - 1;
            }

            MiSetSystemCodeProtection (FirstPte, LastPte);
        }
    }
}


VOID
MiUpdateThunks (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PVOID OldAddress,
    IN PVOID NewAddress,
    IN ULONG NumberOfBytes
    )

/*++

Routine Description:

    This function updates the IATs of all the loaded modules in the system
    to handle a newly relocated image.

Arguments:

    LoaderBlock - Supplies a pointer to the system loader block.

    OldAddress - Supplies the old address of the DLL which was just relocated.

    NewAddress - Supplies the new address of the DLL which was just relocated.

    NumberOfBytes - Supplies the number of bytes spanned by the DLL.

Return Value:

    None.

--*/

{
    PULONG_PTR ImportThunk;
    ULONG_PTR OldAddressHigh;
    ULONG_PTR AddressDifference;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PLIST_ENTRY NextEntry;
    ULONG_PTR i;
    ULONG ImportSize;

    //
    // Note this routine must not call any modules outside the kernel.
    // This is because that module may itself be the one being relocated right
    // now.
    //

    OldAddressHigh = (ULONG_PTR)((PCHAR)OldAddress + NumberOfBytes - 1);
    AddressDifference = (ULONG_PTR)NewAddress - (ULONG_PTR)OldAddress;

    NextEntry = LoaderBlock->LoadOrderListHead.Flink;

    for ( ; NextEntry != &LoaderBlock->LoadOrderListHead; NextEntry = NextEntry->Flink) {

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        ImportThunk = (PULONG_PTR)RtlImageDirectoryEntryToData(
                                           DataTableEntry->DllBase,
                                           TRUE,
                                           IMAGE_DIRECTORY_ENTRY_IAT,
                                           &ImportSize);
    
        if (ImportThunk == NULL) {
            continue;
        }

        ImportSize /= sizeof(PULONG_PTR);

        for (i = 0; i < ImportSize; i += 1, ImportThunk += 1) {
            if (*ImportThunk >= (ULONG_PTR)OldAddress && *ImportThunk <= OldAddressHigh) {
                *ImportThunk += AddressDifference;
            }
        }
    }
}


VOID
MiReloadBootLoadedDrivers (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    The kernel, HAL and boot drivers are relocated by the loader.
    All the boot drivers are then relocated again here.

    This function relocates osloader-loaded images into system PTEs.  This
    gives these images the benefits that all other drivers already enjoy,
    including :

    1. Paging of the drivers (this is more than 500K today).
    2. Write-protection of their text sections.
    3. Automatic unload of drivers on last dereference.

    Note care must be taken when processing HIGHADJ relocations more than once.

Arguments:

    LoaderBlock - Supplies a pointer to the system loader block.

Return Value:

    None.

Environment:

    Kernel mode, Phase 0 Initialization.

--*/

{
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PLIST_ENTRY NextEntry;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_DATA_DIRECTORY DataDirectory;
    ULONG_PTR i;
    ULONG NumberOfPtes;
    ULONG NumberOfLoaderPtes;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE LoaderPte;
    MMPTE PteContents;
    MMPTE TempPte;
    PVOID LoaderImageAddress;
    PVOID NewImageAddress;
    NTSTATUS Status;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER PteFramePage;
    PMMPTE PteFramePointer;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    KIRQL OldIrql;
    PCHAR RelocatedVa;
    PCHAR NonRelocatedVa;
    LOGICAL StopMoving;

#if !defined (_X86_)

    //
    // Non-x86 platforms have map registers so no memory shuffling is needed.
    //

    MmMakeLowMemory = FALSE;
#endif
    StopMoving = FALSE;

    i = 0;
    NextEntry = LoaderBlock->LoadOrderListHead.Flink;

    for ( ; NextEntry != &LoaderBlock->LoadOrderListHead; NextEntry = NextEntry->Flink) {

        //
        // Skip the kernel and the HAL.  Note their relocation sections will
        // be automatically reclaimed.
        //

        i += 1;
        if (i <= 2) {
            continue;
        }

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        //
        // Ensure that the relocation section exists and that the loader
        // hasn't freed it already.
        //

        NtHeader = RtlImageNtHeader(DataTableEntry->DllBase);

        if (NtHeader == NULL) {
            continue;
        }

        if (IMAGE_DIRECTORY_ENTRY_BASERELOC >= NtHeader->OptionalHeader.NumberOfRvaAndSizes) {
            continue;
        }
    
        DataDirectory = &NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

        if (DataDirectory->VirtualAddress == 0) {
            continue;
        }

        if (DataDirectory->VirtualAddress + DataDirectory->Size > DataTableEntry->SizeOfImage) {

            //
            // The relocation section has already been freed, the user must
            // be using an old loader that didn't save the relocations.
            //

            continue;
        }

        LoaderImageAddress = DataTableEntry->DllBase;
        LoaderPte = MiGetPteAddress(DataTableEntry->DllBase);
        NumberOfLoaderPtes = (ULONG)((ROUND_TO_PAGES(DataTableEntry->SizeOfImage)) >> PAGE_SHIFT);

        LOCK_PFN (OldIrql);

        PointerPte = LoaderPte;
        LastPte = PointerPte + NumberOfLoaderPtes;

        while (PointerPte < LastPte) {
            ASSERT (PointerPte->u.Hard.Valid == 1);
            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            //
            // Mark the page as modified so boot drivers that call
            // MmPageEntireDriver don't lose their unmodified data !
            //
        
            Pfn1->u3.e1.Modified = 1;
            PointerPte += 1;
        }

        UNLOCK_PFN (OldIrql);

        //
        // Extra PTEs are allocated here to map the relocation section at the
        // new address so the image can be relocated.
        //

        NumberOfPtes = NumberOfLoaderPtes;

        PointerPte = MiReserveSystemPtes (NumberOfPtes,
                                          SystemPteSpace,
                                          0,
                                          0,
                                          FALSE);

        if (PointerPte == NULL) {
            continue;
        }

        LastPte = PointerPte + NumberOfPtes;

        NewImageAddress = MiGetVirtualAddressMappedByPte (PointerPte);

#if DBG_SYSLOAD
        DbgPrint ("Relocating %wZ from %p to %p, %x bytes\n",
                        &DataTableEntry->FullDllName,
                        DataTableEntry->DllBase,
                        NewImageAddress,
                        DataTableEntry->SizeOfImage
                        );
#endif

        //
        // This assert is important because the assumption is made that PTEs
        // (not superpages) are mapping these drivers.
        //

        ASSERT (InitializationPhase == 0);

        //
        // If the system is configured to make low memory available for ISA
        // type drivers, then copy the boot loaded drivers now.  Otherwise
        // only PTE adjustment is done.  Presumably some day when ISA goes
        // away this code can be removed.
        //

        RelocatedVa = NewImageAddress;
        NonRelocatedVa = (PCHAR) DataTableEntry->DllBase;

        while (PointerPte < LastPte) {

            PteContents = *LoaderPte;
            ASSERT (PteContents.u.Hard.Valid == 1);

            if (MmMakeLowMemory == TRUE) {
#if DBG
                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (LoaderPte);
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                ASSERT (Pfn1->u1.WsIndex == 0);
#endif
                LOCK_PFN (OldIrql);
                MiEnsureAvailablePageOrWait (NULL, NULL);
                PageFrameIndex = MiRemoveAnyPage(
                                    MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

                if (PageFrameIndex < (16*1024*1024)/PAGE_SIZE) {

                    //
                    // If the frames cannot be replaced with high pages
                    // then stop copying.
                    //

#if defined (_X86PAE_)
                  if (MiNoLowMemory == FALSE)
#endif
                    StopMoving = TRUE;
                }

                MI_MAKE_VALID_PTE (TempPte,
                                   PageFrameIndex,
                                   MM_EXECUTE_READWRITE,
                                   PointerPte);

                MI_SET_PTE_DIRTY (TempPte);
                MI_SET_ACCESSED_IN_PTE (&TempPte, 1);
                MI_WRITE_VALID_PTE (PointerPte, TempPte);

                MiInitializePfn (PageFrameIndex, PointerPte, 1);
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                Pfn1->u3.e1.Modified = 1;

                //
                // Initialize the WsIndex just like the original page had it.
                //

                Pfn1->u1.WsIndex = 0;

                UNLOCK_PFN (OldIrql);
                RtlMoveMemory (RelocatedVa, NonRelocatedVa, PAGE_SIZE);
                RelocatedVa += PAGE_SIZE;
                NonRelocatedVa += PAGE_SIZE;
            }
            else {
                MI_MAKE_VALID_PTE (TempPte,
                                   PteContents.u.Hard.PageFrameNumber,
                                   MM_EXECUTE_READWRITE,
                                   PointerPte);
    
                MI_WRITE_VALID_PTE (PointerPte, TempPte);
            }

            PointerPte += 1;
            LoaderPte += 1;
        }
        PointerPte -= NumberOfPtes;

        ASSERT (*(PULONG)NewImageAddress == *(PULONG)LoaderImageAddress);

        //
        // Image is now mapped at the new address.  Relocate it (again).
        //

#if defined(_ALPHA_)

        //
        // HIGHADJ relocations need special re-processing.  This needs to be
        // done before resetting ImageBase.
        //

        Status = (NTSTATUS)LdrDoubleRelocateImage(NewImageAddress,
                                            LoaderImageAddress,
                                            "SYSLDR",
                                            (ULONG)STATUS_SUCCESS,
                                            (ULONG)STATUS_CONFLICTING_ADDRESSES,
                                            (ULONG)STATUS_INVALID_IMAGE_FORMAT
                                            );
#endif

        NtHeader->OptionalHeader.ImageBase = (ULONG_PTR)LoaderImageAddress;
        if (MmMakeLowMemory == TRUE) {
            PIMAGE_NT_HEADERS NtHeader2;

            NtHeader2 = (PIMAGE_NT_HEADERS)((PCHAR)NtHeader + (RelocatedVa - NonRelocatedVa));
            NtHeader2->OptionalHeader.ImageBase = (ULONG_PTR)LoaderImageAddress;
        }

        Status = (NTSTATUS)LdrRelocateImage(NewImageAddress,
                                            "SYSLDR",
                                            (ULONG)STATUS_SUCCESS,
                                            (ULONG)STATUS_CONFLICTING_ADDRESSES,
                                            (ULONG)STATUS_INVALID_IMAGE_FORMAT
                                            );

        if (!NT_SUCCESS(Status)) {

            if (MmMakeLowMemory == TRUE) {
                while (PointerPte < LastPte) {
                    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
                    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                    MiDecrementShareAndValidCount (Pfn1->PteFrame);
                    MI_SET_PFN_DELETED (Pfn1);
                    MiDecrementShareCountOnly (PageFrameIndex);
                    PointerPte += 1;
                }
            }

            MiReleaseSystemPtes (PointerPte,
                                 NumberOfPtes,
                                 SystemPteSpace);

            if (StopMoving == TRUE) {
                MmMakeLowMemory = FALSE;
            }

            continue;
        }

        //
        // Update the IATs for all other loaded modules that reference this one.
        //

        NonRelocatedVa = (PCHAR) DataTableEntry->DllBase;
        DataTableEntry->DllBase = NewImageAddress;

        MiUpdateThunks (LoaderBlock,
                        LoaderImageAddress,
                        NewImageAddress,
                        DataTableEntry->SizeOfImage);


        //
        // Update the loaded module list entry.
        //

        DataTableEntry->Flags |= LDRP_SYSTEM_MAPPED;
        DataTableEntry->DllBase = NewImageAddress;
        DataTableEntry->EntryPoint =
            (PVOID)((PCHAR)NewImageAddress + NtHeader->OptionalHeader.AddressOfEntryPoint);
        DataTableEntry->SizeOfImage = NumberOfPtes << PAGE_SHIFT;

        //
        // Update the PFNs of the image to support trimming.
        // Note that the loader addresses are freed now so no references
        // to it are permitted after this point.
        //

        LoaderPte = MiGetPteAddress (NonRelocatedVa);

        LOCK_PFN (OldIrql);

        while (PointerPte < LastPte) {
            ASSERT (PointerPte->u.Hard.Valid == 1);

            if (MmMakeLowMemory == TRUE) {
                ASSERT (LoaderPte->u.Hard.Valid == 1);
                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (LoaderPte);
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

#if defined (_X86_) || defined (_IA64_)

                //
                // Decrement the share count on the original page table
                // page so it can be freed.
                //

                MiDecrementShareAndValidCount (Pfn1->PteFrame);
#endif

                MI_SET_PFN_DELETED (Pfn1);
                MiDecrementShareCountOnly (PageFrameIndex);
                LoaderPte += 1;
            }
            else {

                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

#if defined (_X86_) || defined (_IA64_)

                //
                // Decrement the share count on the original page table
                // page so it can be freed.
                //

                MiDecrementShareAndValidCount (Pfn1->PteFrame);
                *Pfn1->PteAddress = ZeroPte;
#endif

                //
                // Chain the PFN entry to its new page table.
                //
    
                PteFramePointer = MiGetPteAddress(PointerPte);
                PteFramePage = MI_GET_PAGE_FRAME_FROM_PTE (PteFramePointer);
    
                Pfn1->PteFrame = PteFramePage;
                Pfn1->PteAddress = PointerPte;
    
                //
                // Increment the share count for the page table page that now
                // contains the PTE that was copied.
                //
            
                Pfn2 = MI_PFN_ELEMENT (PteFramePage);
                Pfn2->u2.ShareCount += 1;
            }

            PointerPte += 1;
        }

        UNLOCK_PFN (OldIrql);

        //
        // The physical pages mapping the relocation section are freed
        // later with the rest of the initialization code spanned by the
        // DataTableEntry->SizeOfImage.
        //

        if (StopMoving == TRUE) {
            MmMakeLowMemory = FALSE;
        }
    }
#if defined (_X86PAE_)
    if (MiNoLowMemory == TRUE) {
        MiRemoveLowPages ();
    }
#endif
}

#if defined (_X86_)
PMMPTE MiKernelResourceStartPte;
PMMPTE MiKernelResourceEndPte;
#endif

VOID
MiLocateKernelSections (
    IN PLDR_DATA_TABLE_ENTRY DataTableEntry
    )

/*++

Routine Description:

    This function locates the resource section in the kernel so it can be
    made readwrite if we bugcheck later, as the bugcheck code will write
    into it.

Arguments:

    DataTableEntry - Supplies the kernel's data table entry.

Return Value:

    None.

Environment:

    Kernel mode, Phase 0 Initialization.

--*/

{
    PVOID CurrentBase;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER SectionTableEntry;
    LONG i;
    PMMPTE PointerPte;
    PVOID SectionBaseAddress;

    CurrentBase = (PVOID)DataTableEntry->DllBase;

    NtHeader = RtlImageNtHeader(CurrentBase);

    SectionTableEntry = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeader +
                            sizeof(ULONG) +
                            sizeof(IMAGE_FILE_HEADER) +
                            NtHeader->FileHeader.SizeOfOptionalHeader);

    //
    // From the image header, locate the section named '.rsrc'.
    //

    i = NtHeader->FileHeader.NumberOfSections;

    PointerPte = NULL;

    while (i > 0) {

        SectionBaseAddress = SECTION_BASE_ADDRESS(SectionTableEntry);

#if defined (_X86_)
        if (*(PULONG)SectionTableEntry->Name == 'rsr.') {

            MiKernelResourceStartPte = MiGetPteAddress ((ULONG_PTR)CurrentBase +
                                             SectionTableEntry->VirtualAddress);

            MiKernelResourceEndPte = MiGetPteAddress (ROUND_TO_PAGES((ULONG_PTR)CurrentBase +
                         SectionTableEntry->VirtualAddress +
                         (NtHeader->OptionalHeader.SectionAlignment - 1) +
                         SectionTableEntry->SizeOfRawData -
                         PAGE_SIZE));
            break;
        }
#endif
        if (*(PULONG)SectionTableEntry->Name == 'LOOP') {
            if (*(PULONG)&SectionTableEntry->Name[4] == 'EDOC') {
                ExPoolCodeStart = (PVOID)((ULONG_PTR)CurrentBase +
                                             SectionTableEntry->VirtualAddress);
                ExPoolCodeEnd = (PVOID)((ULONG_PTR)CurrentBase +
                                             SectionTableEntry->VirtualAddress +
                                             SectionTableEntry->SizeOfRawData);
            }
            else if (*(PUSHORT)&SectionTableEntry->Name[4] == 'IM') {
                MmPoolCodeStart = (PVOID)((ULONG_PTR)CurrentBase +
                                             SectionTableEntry->VirtualAddress);
                MmPoolCodeEnd = (PVOID)((ULONG_PTR)CurrentBase +
                                             SectionTableEntry->VirtualAddress +
                                             SectionTableEntry->SizeOfRawData);
            }
        }
        else if ((*(PULONG)SectionTableEntry->Name == 'YSIM') &&
                 (*(PULONG)&SectionTableEntry->Name[4] == 'ETPS')) {
                MmPteCodeStart = (PVOID)((ULONG_PTR)CurrentBase +
                                             SectionTableEntry->VirtualAddress);
                MmPteCodeEnd = (PVOID)((ULONG_PTR)CurrentBase +
                                             SectionTableEntry->VirtualAddress +
                                             SectionTableEntry->SizeOfRawData);
        }

        i -= 1;
        SectionTableEntry += 1;
    }
}

VOID
MmMakeKernelResourceSectionWritable (
    VOID
    )

/*++

Routine Description:

    This function makes the kernel's resource section readwrite so the bugcheck
    code can write into it.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.  Any IRQL.

--*/

{
#if defined (_X86_)
    MMPTE TempPte;
    MMPTE PteContents;
    PMMPTE PointerPte;

    if (MiKernelResourceStartPte == NULL) {
        return;
    }

    PointerPte = MiKernelResourceStartPte;

    if (MI_IS_PHYSICAL_ADDRESS (MiGetVirtualAddressMappedByPte (PointerPte))) {

        //
        // Mapped physically, doesn't need to be made readwrite.
        //

        return;
    }

    //
    // Since the entry state and IRQL are unknown, just go through the
    // PTEs without a lock and make them all readwrite.
    //

    do {
        PteContents = *PointerPte;
#if defined(NT_UP)
        if (PteContents.u.Hard.Write == 0)
#else
        if (PteContents.u.Hard.Writable == 0)
#endif
        {
            MI_MAKE_VALID_PTE (TempPte,
                               PteContents.u.Hard.PageFrameNumber,
                               MM_READWRITE,
                               PointerPte);
#if !defined(NT_UP)
            TempPte.u.Hard.Writable = 1;
#endif
            MI_WRITE_VALID_PTE_NEW_PROTECTION (PointerPte, TempPte);
        }
        PointerPte += 1;
    } while (PointerPte <= MiKernelResourceEndPte); 

    //
    // Don't do this more than once.
    //

    MiKernelResourceStartPte = NULL;

    //
    // Only flush this processor as the state of the others is unknown.
    //

    KeFlushCurrentTb ();
#endif
}

#ifdef i386
PVOID PsNtosImageBase = (PVOID)0x80100000;
#else
PVOID PsNtosImageBase;
#endif

LIST_ENTRY PsLoadedModuleList;
ERESOURCE PsLoadedModuleResource;
extern KSPIN_LOCK PsLoadedModuleSpinLock;

LOGICAL
MiInitializeLoadedModuleList (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function initializes the loaded module list based on the LoaderBlock.

Arguments:

    LoaderBlock - Supplies a pointer to the system loader block.

Return Value:

    None.

Environment:

    Kernel mode, Phase 0 Initialization.

--*/

{
    PLIST_ENTRY NextEntry;
    PLDR_DATA_TABLE_ENTRY DataTableEntry1;
    PLDR_DATA_TABLE_ENTRY DataTableEntry2;

    //
    // Initialize the loaded module list executive resource and spin lock.
    //

    ExInitializeResource (&PsLoadedModuleResource);
    KeInitializeSpinLock (&PsLoadedModuleSpinLock);

    InitializeListHead (&PsLoadedModuleList);

    //
    // Scan the loaded module list and allocate and initialize a data table
    // entry for each module. The data table entry is inserted in the loaded
    // module list and the initialization order list in the order specified
    // in the loader parameter block. The data table entry is inserted in the
    // memory order list in memory order.
    //

    NextEntry = LoaderBlock->LoadOrderListHead.Flink;
    DataTableEntry2 = CONTAINING_RECORD(NextEntry,
                                        LDR_DATA_TABLE_ENTRY,
                                        InLoadOrderLinks);
    PsNtosImageBase = DataTableEntry2->DllBase;

    MiLocateKernelSections (DataTableEntry2);

    while (NextEntry != &LoaderBlock->LoadOrderListHead) {

        DataTableEntry2 = CONTAINING_RECORD(NextEntry,
                                            LDR_DATA_TABLE_ENTRY,
                                            InLoadOrderLinks);

        //
        // Allocate a data table entry.
        //

        DataTableEntry1 = ExAllocatePoolWithTag (NonPagedPool,
                                                 sizeof(LDR_DATA_TABLE_ENTRY),
                                                 'dLmM');

        if (DataTableEntry1 == NULL) {
            return FALSE;
        }

        //
        // Copy the data table entry.
        //

        *DataTableEntry1 = *DataTableEntry2;

        DataTableEntry1->FullDllName.Buffer = ExAllocatePoolWithTag (PagedPool,
            DataTableEntry2->FullDllName.MaximumLength + sizeof(UNICODE_NULL),
            'TDmM');

        if (DataTableEntry1->FullDllName.Buffer == NULL) {
            ExFreePool (DataTableEntry1);
            return FALSE;
        }

        DataTableEntry1->BaseDllName.Buffer = ExAllocatePoolWithTag (NonPagedPool,
            DataTableEntry2->BaseDllName.MaximumLength + sizeof(UNICODE_NULL),
            'dLmM');

        if (DataTableEntry1->BaseDllName.Buffer == NULL) {
            ExFreePool (DataTableEntry1->FullDllName.Buffer);
            ExFreePool (DataTableEntry1);
            return FALSE;
        }

        //
        // Copy the strings.
        //

        RtlMoveMemory (DataTableEntry1->FullDllName.Buffer,
                       DataTableEntry2->FullDllName.Buffer,
                       DataTableEntry1->FullDllName.MaximumLength);

        RtlMoveMemory (DataTableEntry1->BaseDllName.Buffer,
                       DataTableEntry2->BaseDllName.Buffer,
                       DataTableEntry1->BaseDllName.MaximumLength);

        //
        // Insert the data table entry in the load order list in the order
        // they are specified.
        //

        InsertTailList(&PsLoadedModuleList,
                       &DataTableEntry1->InLoadOrderLinks);

        NextEntry = NextEntry->Flink;
    }

    MiBuildImportsForBootDrivers ();

    return TRUE;
}

NTSTATUS
MmCallDllInitialize(
    IN PLDR_DATA_TABLE_ENTRY DataTableEntry
)

/*++

Routine Description:

    This function calls the DLL's initialize routine.

Arguments:

    DataTableEntry - Supplies the kernel's data table entry.

Return Value:

    Various NTSTATUS error codes.

Environment:

    Kernel mode.

--*/

{
    NTSTATUS st;
    PWCHAR Dot;
    PMM_DLL_INITIALIZE Func;
    UNICODE_STRING RegistryPath;
    UNICODE_STRING ImportName;

    Func = MiLocateExportName (DataTableEntry->DllBase, "DllInitialize");

    if (!Func) {
        return STATUS_SUCCESS;
    }

    ImportName.MaximumLength = DataTableEntry->BaseDllName.Length;
    ImportName.Buffer = ExAllocatePoolWithTag (NonPagedPool,
                                               ImportName.MaximumLength,
                                               'TDmM');

    if (ImportName.Buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ImportName.Length = DataTableEntry->BaseDllName.Length;
    RtlMoveMemory (ImportName.Buffer,
                   DataTableEntry->BaseDllName.Buffer,
                   ImportName.Length);

    RegistryPath.MaximumLength = CmRegistryMachineSystemCurrentControlSetServices.Length +
                                    ImportName.Length +
                                    (USHORT)(2*sizeof(WCHAR));

    RegistryPath.Buffer = ExAllocatePoolWithTag (NonPagedPool,
                                                 RegistryPath.MaximumLength,
                                                 'TDmM');

    if (RegistryPath.Buffer == NULL) {
        ExFreePool (ImportName.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RegistryPath.Length = CmRegistryMachineSystemCurrentControlSetServices.Length;
    RtlMoveMemory (RegistryPath.Buffer,
                   CmRegistryMachineSystemCurrentControlSetServices.Buffer,
                   CmRegistryMachineSystemCurrentControlSetServices.Length);

    RtlAppendUnicodeToString (&RegistryPath, L"\\");
    Dot = wcschr (ImportName.Buffer, L'.');
    if (Dot) {
        ImportName.Length = (USHORT)((Dot - ImportName.Buffer) * sizeof(WCHAR));
    }

    RtlAppendUnicodeStringToString (&RegistryPath, &ImportName);
    ExFreePool (ImportName.Buffer);

    //
    // Invoke the DLL's initialization routine.
    //

    st = Func (&RegistryPath);

    ExFreePool (RegistryPath.Buffer);

    return st;
}

NTKERNELAPI
PVOID
MmGetSystemRoutineAddress (
    IN PUNICODE_STRING SystemRoutineName
)

/*++

Routine Description:

    This function returns the address of the argument function pointer if
    it is in the kernel or HAL, NULL if it is not.

Arguments:

    SystemRoutineName - Supplies the name of the desired routine.

Return Value:

    Non-NULL function pointer if successful.  NULL if not.

Environment:

    Kernel mode.

--*/

{
    ULONG AnsiLength;
    NTSTATUS Status;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    ANSI_STRING AnsiString;
    PLIST_ENTRY NextEntry;
    UNICODE_STRING KernelString;
    UNICODE_STRING HalString;
    PVOID FunctionAddress;
    LOGICAL Found;
    ULONG EntriesChecked;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    EntriesChecked = 0;
    FunctionAddress = NULL;

    RtlInitUnicodeString (&KernelString, L"ntoskrnl.exe");
    RtlInitUnicodeString (&HalString, L"hal.dll");

    do {
        Status = RtlUnicodeStringToAnsiString( &AnsiString,
                                               SystemRoutineName,
                                               TRUE );
    
        if (NT_SUCCESS( Status)) {
            break;
        }

        KeDelayExecutionThread (KernelMode, FALSE, &MmShortTime);

    } while (TRUE);
    
    //
    // Arbitrary process context so prevent suspend APCs now.
    //

    KeEnterCriticalRegion();
    ExAcquireResourceShared (&PsLoadedModuleResource, TRUE);

    //
    // Check only the kernel and the HAL for exports.
    //

    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList) {

        Found = FALSE;

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        if (RtlEqualUnicodeString (&KernelString,
                                   &DataTableEntry->BaseDllName,
                                   TRUE)) {

            Found = TRUE;
            EntriesChecked += 1;

        }
        else if (RtlEqualUnicodeString (&HalString,
                                        &DataTableEntry->BaseDllName,
                                        TRUE)) {

            Found = TRUE;
            EntriesChecked += 1;
        }

        if (Found == TRUE) {

            FunctionAddress = MiFindExportedRoutineByName (DataTableEntry,
                                                           &AnsiString);

            if (FunctionAddress != NULL) {
                break;
            }

            if (EntriesChecked == 2) {
                break;
            }
        }

        NextEntry = NextEntry->Flink;
    }

    ExReleaseResource (&PsLoadedModuleResource);
    KeLeaveCriticalRegion();

    RtlFreeAnsiString (&AnsiString);

    return FunctionAddress;
}

PVOID
MiFindExportedRoutineByName(
    IN PLDR_DATA_TABLE_ENTRY DataTableEntry,
    IN PANSI_STRING AnsiImageRoutineName
    )

/*++

Routine Description:

    This function snaps a thunk using the specified Export Section data.
    If the section data does not support the thunk, then the thunk is
    partially snapped (Dll field is still non-null, but snap address is
    set).

Arguments:

    DllBase - Base of DLL being snapped to.

    ImageBase - Base of image that contains the thunks to snap.

    Thunk - On input, supplies the thunk to snap.  When successfully
        snapped, the function field is set to point to the address in
        the DLL, and the DLL field is set to NULL.

    ExportDirectory - Supplies the Export Section data from a DLL.

    SnapForwarder - determine if the snap is for a forwarder, and therefore
       Address of Data is already setup.

Return Value:


    STATUS_SUCCESS or STATUS_DRIVER_ENTRYPOINT_NOT_FOUND or
        STATUS_DRIVER_ORDINAL_NOT_FOUND

--*/

{
    PCHAR DllBase;
    USHORT OrdinalNumber;
    PULONG NameTableBase;
    PUSHORT NameOrdinalTableBase;
    PULONG Addr;
    ULONG High;
    ULONG Low;
    ULONG Middle;
    LONG Result;
    ULONG ExportSize;
    PVOID FunctionAddress;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;

    PAGED_CODE();

    DllBase = DataTableEntry->DllBase;

    ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(
                                DllBase,
                                TRUE,
                                IMAGE_DIRECTORY_ENTRY_EXPORT,
                                &ExportSize
                                );

    ASSERT (ExportDirectory != NULL);

    //
    // Initialize the pointer to the array of RVA-based ansi export strings.
    //

    NameTableBase = (PULONG)(DllBase + (ULONG)ExportDirectory->AddressOfNames);

    //
    // Initialize the pointer to the array of USHORT ordinal numbers.
    //

    NameOrdinalTableBase = (PUSHORT)(DllBase + (ULONG)ExportDirectory->AddressOfNameOrdinals);

    //
    // Lookup the desired name in the name table using a binary search.
    //

    Low = 0;
    High = ExportDirectory->NumberOfNames - 1;

    while (High >= Low) {

        //
        // Compute the next probe index and compare the import name
        // with the export name entry.
        //

        Middle = (Low + High) >> 1;

        Result = strcmp(AnsiImageRoutineName->Buffer,
                        (PCHAR)(DllBase + NameTableBase[Middle]));

        if (Result < 0) {
            High = Middle - 1;
        }
        else if (Result > 0) {
            Low = Middle + 1;
        }
        else {
            break;
        }
    }

    //
    // If the high index is less than the low index, then a matching
    // table entry was not found. Otherwise, get the ordinal number
    // from the ordinal table.
    //

    if (High < Low) {
        return NULL;
    }

    OrdinalNumber = NameOrdinalTableBase[Middle];

    //
    // If the OrdinalNumber is not within the Export Address Table,
    // then this image does not implement the function.  Return not found.
    //

    if ((ULONG)OrdinalNumber >= ExportDirectory->NumberOfFunctions) {
        return NULL;
    }

    //
    // Index into the array of RVA export addresses by ordinal number.
    //

    Addr = (PULONG)(DllBase + (ULONG)ExportDirectory->AddressOfFunctions);

    FunctionAddress = (PVOID)(DllBase + Addr[OrdinalNumber]);

    //
    // Forwarders are not used by the kernel and HAL to each other.
    //

    ASSERT ((FunctionAddress <= (PVOID)ExportDirectory) ||
            (FunctionAddress >= (PVOID)((PCHAR)ExportDirectory + ExportSize)));

    return FunctionAddress;
}
