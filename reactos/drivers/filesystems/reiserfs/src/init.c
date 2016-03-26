/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             init.c
 * PURPOSE:          
 * PROGRAMMER:       Mark Piper, Matt Wu, Bo Brantén.
 * HOMEPAGE:         
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "rfsd.h"

/* GLOBALS ***************************************************************/

PRFSD_GLOBAL    RfsdGlobal = NULL;

/* DEFINITIONS ***********************************************************/

NTSTATUS NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath   );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, RfsdQueryParameters)
#pragma alloc_text(INIT, DriverEntry)
#if RFSD_UNLOAD
#pragma alloc_text(PAGE, DriverUnload)
#endif
#endif

/* FUNCTIONS ***************************************************************/

#if RFSD_UNLOAD

/*
 * FUNCTION: Called by the system to unload the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 * RETURNS:  None
 */

VOID NTAPI
DriverUnload (IN PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING  DosDeviceName;

    PAGED_CODE();

    RfsdPrint((DBG_FUNC, "Rfsd: Unloading routine.\n"));

    RtlInitUnicodeString(&DosDeviceName, DOS_DEVICE_NAME);
    IoDeleteSymbolicLink(&DosDeviceName);

    ExDeleteResourceLite(&RfsdGlobal->LAResource);
    ExDeleteResourceLite(&RfsdGlobal->CountResource);
    ExDeleteResourceLite(&RfsdGlobal->Resource);

    ExDeletePagedLookasideList(&(RfsdGlobal->RfsdMcbLookasideList));
    ExDeleteNPagedLookasideList(&(RfsdGlobal->RfsdCcbLookasideList));
    ExDeleteNPagedLookasideList(&(RfsdGlobal->RfsdFcbLookasideList));
    ExDeleteNPagedLookasideList(&(RfsdGlobal->RfsdIrpContextLookasideList));

    RfsdUnloadAllNls();

    IoDeleteDevice(RfsdGlobal->DeviceObject);
}

#endif

BOOLEAN
RfsdQueryParameters( IN PUNICODE_STRING  RegistryPath)
{
    NTSTATUS                    Status;
    UNICODE_STRING              ParameterPath;
    RTL_QUERY_REGISTRY_TABLE    QueryTable[2];

    ULONG                       WritingSupport;
    ULONG                       CheckingBitmap;
    ULONG                       Ext3ForceWriting;

    UNICODE_STRING              CodePage;
    ANSI_STRING                 AnsiName;

    ParameterPath.Length = 0;

    ParameterPath.MaximumLength =
        RegistryPath->Length + sizeof(PARAMETERS_KEY) + sizeof(WCHAR);

    ParameterPath.Buffer =
        (PWSTR) ExAllocatePoolWithTag(PagedPool, ParameterPath.MaximumLength, RFSD_POOL_TAG);

    if (!ParameterPath.Buffer) {
        return FALSE;
    }

    WritingSupport = 0;
    CheckingBitmap = 0;
    Ext3ForceWriting = 0;

    RtlCopyUnicodeString(&ParameterPath, RegistryPath);

    RtlAppendUnicodeToString(&ParameterPath, PARAMETERS_KEY);

    RtlZeroMemory(&QueryTable[0], sizeof(RTL_QUERY_REGISTRY_TABLE) * 2);

    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = WRITING_SUPPORT;
    QueryTable[0].EntryContext = &WritingSupport;

    Status = RtlQueryRegistryValues(
        RTL_REGISTRY_ABSOLUTE,
        ParameterPath.Buffer,
        &QueryTable[0],
        NULL,
        NULL        );

    RfsdPrint((DBG_USER, "RfsdQueryParameters: WritingSupport=%xh\n", WritingSupport));

    RtlZeroMemory(&QueryTable[0], sizeof(RTL_QUERY_REGISTRY_TABLE) * 2);

    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = CHECKING_BITMAP;
    QueryTable[0].EntryContext = &CheckingBitmap;

    Status = RtlQueryRegistryValues(
        RTL_REGISTRY_ABSOLUTE,
        ParameterPath.Buffer,
        &QueryTable[0],
        NULL,
        NULL        );

    RfsdPrint((DBG_USER, "RfsdQueryParameters: CheckingBitmap=%xh\n", CheckingBitmap));

    RtlZeroMemory(&QueryTable[0], sizeof(RTL_QUERY_REGISTRY_TABLE) * 2);

    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = EXT3_FORCEWRITING;
    QueryTable[0].EntryContext = &Ext3ForceWriting;

    Status = RtlQueryRegistryValues(
        RTL_REGISTRY_ABSOLUTE,
        ParameterPath.Buffer,
        &QueryTable[0],
        NULL,
        NULL        );

    RfsdPrint((DBG_USER, "RfsdQueryParameters: Ext3ForceWriting=%xh\n", Ext3ForceWriting));

    RtlZeroMemory(&QueryTable[0], sizeof(RTL_QUERY_REGISTRY_TABLE) * 2);

    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = EXT3_CODEPAGE;
    QueryTable[0].EntryContext = &(CodePage);
    CodePage.MaximumLength = CODEPAGE_MAXLEN * sizeof(WCHAR);
    CodePage.Length = 0;
    CodePage.Buffer = &(RfsdGlobal->CodePage.UniName[0]);

    Status = RtlQueryRegistryValues(
        RTL_REGISTRY_ABSOLUTE,
        ParameterPath.Buffer,
        &QueryTable[0],
        NULL,
        NULL        );

    if (NT_SUCCESS(Status)) {
        RfsdPrint((DBG_USER, "RfsdQueryParameters: RfsdCodePage=%S\n", CodePage.Buffer));
        AnsiName.MaximumLength = CODEPAGE_MAXLEN;
        AnsiName.Length = 0;
        AnsiName.Buffer = &(RfsdGlobal->CodePage.AnsiName[0]);

        Status = RtlUnicodeStringToAnsiString(
                    &AnsiName,
                    &CodePage,
                    FALSE);

        if (!NT_SUCCESS(Status)) {
            RfsdPrint((DBG_USER, "RfsdQueryParameters: Wrong CodePage ...\n"));
            RtlCopyMemory(&(RfsdGlobal->CodePage.AnsiName[0]),"default\0", 8);
        }

    } else {
        RfsdPrint((DBG_USER, "RfsdQueryParameters: CodePage not specified.\n"));
        RtlCopyMemory(&(RfsdGlobal->CodePage.AnsiName[0]),"default\0", 8);
    }

    {
        if (WritingSupport) {
            SetFlag(RfsdGlobal->Flags, RFSD_SUPPORT_WRITING);
        } else {
            ClearFlag(RfsdGlobal->Flags, RFSD_SUPPORT_WRITING);
        }

        if (CheckingBitmap) {
            SetFlag(RfsdGlobal->Flags, RFSD_CHECKING_BITMAP);
        } else {
            ClearFlag(RfsdGlobal->Flags, RFSD_CHECKING_BITMAP);
        }

        if (Ext3ForceWriting) {
            KdPrint(("Rfsd -- Warning: Ext3ForceWriring enabled !!!\n"));

            SetFlag(RfsdGlobal->Flags, EXT3_FORCE_WRITING);
            SetFlag(RfsdGlobal->Flags, RFSD_SUPPORT_WRITING);
        } else {
            ClearFlag(RfsdGlobal->Flags, EXT3_FORCE_WRITING);
        }
    }

    ExFreePool(ParameterPath.Buffer);

    return TRUE;
}

#define NLS_OEM_LEAD_BYTE_INFO            (*NlsOemLeadByteInfo)

#ifndef __REACTOS__
#define FsRtlIsLeadDbcsCharacter(DBCS_CHAR) (                      \
    (BOOLEAN)((UCHAR)(DBCS_CHAR) < 0x80 ? FALSE :                  \
              (NLS_MB_CODE_PAGE_TAG &&                             \
               (NLS_OEM_LEAD_BYTE_INFO[(UCHAR)(DBCS_CHAR)] != 0))) \
)
#endif

/*
 * NAME: DriverEntry
 * FUNCTION: Called by the system to initalize the driver
 *
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
NTSTATUS NTAPI
DriverEntry (
         IN PDRIVER_OBJECT   DriverObject,
         IN PUNICODE_STRING  RegistryPath
         )
{
    PDEVICE_OBJECT              DeviceObject;
    PFAST_IO_DISPATCH           FastIoDispatch;
    PCACHE_MANAGER_CALLBACKS    CacheManagerCallbacks;
    PRFSDFS_EXT                 DeviceExt;

    UNICODE_STRING              DeviceName;
    NTSTATUS                    Status;

#if RFSD_UNLOAD
    UNICODE_STRING              DosDeviceName;
#endif

    DbgPrint(
        "Rfsd "
        RFSD_VERSION
#if RFSD_READ_ONLY
        " read-only"
#endif
#if DBG
        " checked"
#endif
        " " __DATE__ " " __TIME__
        "\nCopyright (C) 1999-2015 Mark Piper, Matt Wu, Bo Branten.\n");

    RfsdPrint((DBG_FUNC, "Rfsd DriverEntry ...\n"));	

    RtlInitUnicodeString(&DeviceName, DEVICE_NAME);
    
    Status = IoCreateDevice(
        DriverObject,
        sizeof(RFSDFS_EXT),
        &DeviceName,
        FILE_DEVICE_DISK_FILE_SYSTEM,
        0,
        FALSE,
        &DeviceObject );
    
    if (!NT_SUCCESS(Status)) {
        RfsdPrint((DBG_ERROR, "IoCreateDevice fs object error.\n"));
        return Status;
    }

    DeviceExt = (PRFSDFS_EXT) DeviceObject->DeviceExtension;
    RtlZeroMemory(DeviceExt, sizeof(RFSDFS_EXT));
    
    RfsdGlobal = &(DeviceExt->RfsdGlobal);

    RfsdGlobal->Identifier.Type = RFSDFGD;
    RfsdGlobal->Identifier.Size = sizeof(RFSD_GLOBAL);
    RfsdGlobal->DeviceObject = DeviceObject;
    RfsdGlobal->DriverObject = DriverObject;

    RfsdQueryParameters(RegistryPath);

    DriverObject->MajorFunction[IRP_MJ_CREATE]              = RfsdBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]               = RfsdBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_READ]                = RfsdBuildRequest;
#if !RFSD_READ_ONLY
    DriverObject->MajorFunction[IRP_MJ_WRITE]               = RfsdBuildRequest;
#endif // !RFSD_READ_ONLY

    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]       = RfsdBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]            = RfsdBuildRequest;

    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]   = RfsdBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]     = RfsdBuildRequest;

    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION]    = RfsdBuildRequest;
#if !RFSD_READ_ONLY
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION]      = RfsdBuildRequest;
#endif // !RFSD_READ_ONLY

    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]   = RfsdBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = RfsdBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]      = RfsdBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL]        = RfsdBuildRequest;

    DriverObject->MajorFunction[IRP_MJ_CLEANUP]             = RfsdBuildRequest;

#if (_WIN32_WINNT >= 0x0500)
    DriverObject->MajorFunction[IRP_MJ_PNP]                 = RfsdBuildRequest;
#endif //(_WIN32_WINNT >= 0x0500)

#if RFSD_UNLOAD
    DriverObject->DriverUnload                              = DriverUnload;
#else
    DriverObject->DriverUnload                              = NULL;
#endif

    //
    // Initialize the fast I/O entry points
    //
    FastIoDispatch = &(RfsdGlobal->FastIoDispatch);
    
    FastIoDispatch->SizeOfFastIoDispatch        = sizeof(FAST_IO_DISPATCH);
    FastIoDispatch->FastIoCheckIfPossible       = RfsdFastIoCheckIfPossible;
#if DBG
    FastIoDispatch->FastIoRead                  = RfsdFastIoRead;
#if !RFSD_READ_ONLY
    FastIoDispatch->FastIoWrite                 = RfsdFastIoWrite;
#endif // !RFSD_READ_ONLY
#else
#pragma prefast( suppress: 28155, "allowed in file system drivers" )
    FastIoDispatch->FastIoRead                  = FsRtlCopyRead;
#if !RFSD_READ_ONLY
#pragma prefast( suppress: 28155, "allowed in file system drivers" )
    FastIoDispatch->FastIoWrite                 = FsRtlCopyWrite;
#endif // !RFSD_READ_ONLY
#endif
    FastIoDispatch->FastIoQueryBasicInfo        = RfsdFastIoQueryBasicInfo;
    FastIoDispatch->FastIoQueryStandardInfo     = RfsdFastIoQueryStandardInfo;
    FastIoDispatch->FastIoLock                  = RfsdFastIoLock;
    FastIoDispatch->FastIoUnlockSingle          = RfsdFastIoUnlockSingle;
    FastIoDispatch->FastIoUnlockAll             = RfsdFastIoUnlockAll;
    FastIoDispatch->FastIoUnlockAllByKey        = RfsdFastIoUnlockAllByKey;
    FastIoDispatch->FastIoQueryNetworkOpenInfo  = RfsdFastIoQueryNetworkOpenInfo;

#ifdef _MSC_VER
#pragma prefast( suppress: 28175, "allowed in file system drivers" )
#endif
    DriverObject->FastIoDispatch = FastIoDispatch;

    switch ( MmQuerySystemSize() ) {

    case MmSmallSystem:

        RfsdGlobal->MaxDepth = 16;
        break;

    case MmMediumSystem:

        RfsdGlobal->MaxDepth = 64;
        break;

    case MmLargeSystem:

        RfsdGlobal->MaxDepth = 256;
        break;
    }

    //
    // Initialize the Cache Manager callbacks
    //
    CacheManagerCallbacks = &(RfsdGlobal->CacheManagerCallbacks);
    CacheManagerCallbacks->AcquireForLazyWrite  = RfsdAcquireForLazyWrite;
    CacheManagerCallbacks->ReleaseFromLazyWrite = RfsdReleaseFromLazyWrite;
    CacheManagerCallbacks->AcquireForReadAhead  = RfsdAcquireForReadAhead;
    CacheManagerCallbacks->ReleaseFromReadAhead = RfsdReleaseFromReadAhead;

    RfsdGlobal->CacheManagerNoOpCallbacks.AcquireForLazyWrite  = RfsdNoOpAcquire;
    RfsdGlobal->CacheManagerNoOpCallbacks.ReleaseFromLazyWrite = RfsdNoOpRelease;
    RfsdGlobal->CacheManagerNoOpCallbacks.AcquireForReadAhead  = RfsdNoOpAcquire;
    RfsdGlobal->CacheManagerNoOpCallbacks.ReleaseFromReadAhead = RfsdNoOpRelease;

    //
    // Initialize the global data
    //

    InitializeListHead(&(RfsdGlobal->VcbList));
    ExInitializeResourceLite(&(RfsdGlobal->Resource));
    ExInitializeResourceLite(&(RfsdGlobal->CountResource));
    ExInitializeResourceLite(&(RfsdGlobal->LAResource));

    ExInitializeNPagedLookasideList( &(RfsdGlobal->RfsdIrpContextLookasideList),
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(RFSD_IRP_CONTEXT),
                                     '2TXE',
                                     0 );

    ExInitializeNPagedLookasideList( &(RfsdGlobal->RfsdFcbLookasideList),
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(RFSD_FCB),
                                     '2TXE',
                                     0 );

    ExInitializeNPagedLookasideList( &(RfsdGlobal->RfsdCcbLookasideList),
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(RFSD_CCB),
                                     '2TXE',
                                     0 );

    ExInitializePagedLookasideList( &(RfsdGlobal->RfsdMcbLookasideList),
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(RFSD_MCB),
                                     '2TXE',
                                     0 );

#if RFSD_UNLOAD
    RtlInitUnicodeString(&DosDeviceName, DOS_DEVICE_NAME);
    IoCreateSymbolicLink(&DosDeviceName, &DeviceName);
#endif

#if DBG
    ProcessNameOffset = RfsdGetProcessNameOffset();
#endif

    RfsdLoadAllNls();

    PAGE_TABLE = load_nls(RfsdGlobal->CodePage.AnsiName);

    if (!PAGE_TABLE) {
        KdPrint(( "Rfsd: User specified codepage (%s) was not found.\n"
                  "         Defulat system OEM codepage will be adopted.\n",
                  RfsdGlobal->CodePage.AnsiName ));
        //DbgBreak();
    }

    IoRegisterFileSystem(DeviceObject);

    return Status;
}
