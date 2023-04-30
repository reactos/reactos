/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             init.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

PEXT2_GLOBAL    Ext2Global   = NULL;

/*
 *   Ext2Fsd version, building date/time
 */

CHAR            gVersion[]   = EXT2FSD_VERSION;
CHAR            gTime[] = __TIME__;
CHAR            gDate[] = __DATE__;


/* DEFINITIONS ***********************************************************/

#ifdef __REACTOS__
NTSTATUS NTAPI
#else
NTSTATUS
#endif
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath   );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, Ext2QueryGlobalParameters)
#pragma alloc_text(INIT, Ext2QueryRegistrySettings)
#pragma alloc_text(INIT, DriverEntry)
#if EXT2_UNLOAD
#pragma alloc_text(PAGE, DriverUnload)
#endif
#endif

/* FUNCTIONS ***************************************************************/

DECLARE_INIT(journal_init);
DECLARE_EXIT(journal_exit);

#if EXT2_UNLOAD

/*
 * FUNCTION: Called by the system to unload the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 * RETURNS:  None
 */
#ifdef __REACTOS__
VOID NTAPI
#else
VOID
#endif
DriverUnload (IN PDRIVER_OBJECT DriverObject)
{

    UNICODE_STRING              DosDeviceName;

    DEBUG(DL_FUN, ( "Ext2Fsd: Unloading routine.\n"));

    /*
     *  stop reaper thread ...
     */


    /*
     *  removing memory allocations and  objects
     */

    RtlInitUnicodeString(&DosDeviceName, DOS_DEVICE_NAME);
    IoDeleteSymbolicLink(&DosDeviceName);

    Ext2UnloadAllNls();

    ExDeleteResourceLite(&Ext2Global->Resource);

    ExDeleteNPagedLookasideList(&(Ext2Global->Ext2DentryLookasideList));
    ExDeleteNPagedLookasideList(&(Ext2Global->Ext2ExtLookasideList));
    ExDeleteNPagedLookasideList(&(Ext2Global->Ext2McbLookasideList));
    ExDeleteNPagedLookasideList(&(Ext2Global->Ext2CcbLookasideList));
    ExDeleteNPagedLookasideList(&(Ext2Global->Ext2FcbLookasideList));
    ExDeleteNPagedLookasideList(&(Ext2Global->Ext2IrpContextLookasideList));

    ObDereferenceObject(Ext2Global->DiskdevObject);
    ObDereferenceObject(Ext2Global->CdromdevObject);

    /* cleanup journal related caches */
    UNLOAD_MODULE(journal_exit);

    /* cleanup linux lib */
    ext2_destroy_linux();

    Ext2FreePool(Ext2Global, 'LG2E');
    Ext2Global = NULL;
}

#endif

#ifdef __REACTOS__
NTSTATUS NTAPI
#else
NTSTATUS
#endif
Ext2RegistryQueryCallback(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    ULONG  i = 0;
    BYTE   *s, *t;

    if (NULL == ValueName || NULL == ValueData)
        return STATUS_SUCCESS;

    if (ValueType == REG_DWORD && wcslen(ValueName) == wcslen(WRITING_SUPPORT) &&
        _wcsnicmp(ValueName, WRITING_SUPPORT, wcslen(WRITING_SUPPORT)) == 0) {

        if (ValueData && ValueLength == sizeof(DWORD)) {
            if (*((PULONG)ValueData)) {
                SetLongFlag(Ext2Global->Flags, EXT2_SUPPORT_WRITING);
            } else {
                ClearLongFlag(Ext2Global->Flags, EXT2_SUPPORT_WRITING);
            }
        }
    } else if (ValueType == REG_DWORD && wcslen(ValueName) == wcslen(CHECKING_BITMAP) &&
        _wcsnicmp(ValueName, CHECKING_BITMAP, wcslen(CHECKING_BITMAP)) == 0) {

        if (ValueData && ValueLength == sizeof(DWORD)) {
            if (*((PULONG)ValueData)) {
                SetLongFlag(Ext2Global->Flags, EXT2_CHECKING_BITMAP);
            } else {
                ClearLongFlag(Ext2Global->Flags, EXT2_CHECKING_BITMAP);
            }
        }
    } else if (ValueType == REG_DWORD && wcslen(ValueName) == wcslen(EXT3_FORCEWRITING) &&
        _wcsnicmp(ValueName, EXT3_FORCEWRITING, wcslen(EXT3_FORCEWRITING)) == 0) {

        if (ValueData && ValueLength == sizeof(DWORD)) {
            if (*((PULONG)ValueData)) {
                SetLongFlag(Ext2Global->Flags, EXT3_FORCE_WRITING);
                SetLongFlag(Ext2Global->Flags, EXT2_SUPPORT_WRITING);
            } else {
                ClearLongFlag(Ext2Global->Flags, EXT3_FORCE_WRITING);
            }
        }
    } else if (ValueType == REG_DWORD && wcslen(ValueName) == wcslen(AUTO_MOUNT) &&
        _wcsnicmp(ValueName, AUTO_MOUNT, wcslen(AUTO_MOUNT)) == 0) {

        if (ValueData && ValueLength == sizeof(DWORD)) {
            if (*((PULONG)ValueData)) {
                SetLongFlag(Ext2Global->Flags, EXT2_AUTO_MOUNT);
            } else {
                ClearLongFlag(Ext2Global->Flags, EXT2_AUTO_MOUNT);
            }
        }
    } else if (ValueType == REG_SZ && wcslen(ValueName) == wcslen(CODEPAGE_NAME) &&
        _wcsnicmp(ValueName, CODEPAGE_NAME, wcslen(CODEPAGE_NAME)) == 0) {

        if (ValueData && ValueLength <= sizeof(WCHAR) * CODEPAGE_MAXLEN) {
            RtlCopyMemory(&Ext2Global->Codepage.PageName[0],
                          ValueData, ValueLength);
        }
    } else if (ValueType == REG_SZ && wcslen(ValueName) == wcslen(HIDING_PREFIX) &&
        _wcsnicmp(ValueName, HIDING_PREFIX, wcslen(HIDING_PREFIX)) == 0) {

        if (ValueData && ValueLength <= sizeof(WCHAR) * HIDINGPAT_LEN) {
            RtlCopyMemory(&Ext2Global->wHidingPrefix[0],
                          ValueData, ValueLength);
        }
    } else  if (ValueType == REG_SZ && wcslen(ValueName) == wcslen(HIDING_SUFFIX) &&
        _wcsnicmp(ValueName, HIDING_SUFFIX, wcslen(HIDING_SUFFIX)) == 0) {

        if (ValueData && ValueLength <= sizeof(WCHAR) * HIDINGPAT_LEN) {
            RtlCopyMemory(&Ext2Global->wHidingSuffix[0],
                          ValueData, ValueLength);
        }
    }


    return STATUS_SUCCESS;
}

NTSTATUS
Ext2QueryGlobalParameters(IN PUNICODE_STRING RegistryPath)
{
    RTL_QUERY_REGISTRY_TABLE    QueryTable[8];
    int                         i = 0;
    NTSTATUS                    Status;

    RtlZeroMemory(&QueryTable[0], sizeof(RTL_QUERY_REGISTRY_TABLE) * 8);

    /*
     * 1 writing support
     */
    QueryTable[i].Flags = 0;
    QueryTable[i].Name = WRITING_SUPPORT;
    QueryTable[i].DefaultType = REG_NONE;
    QueryTable[i].DefaultLength = 0;
    QueryTable[i].DefaultData = NULL;
    QueryTable[i].EntryContext = NULL;
    QueryTable[i].QueryRoutine = Ext2RegistryQueryCallback;
    i++;

    /*
     * 2 checking bitmap
     */
    QueryTable[i].Flags = 0;
    QueryTable[i].Name = CHECKING_BITMAP;
    QueryTable[i].DefaultType = REG_NONE;
    QueryTable[i].DefaultLength = 0;
    QueryTable[i].DefaultData = NULL;
    QueryTable[i].EntryContext = NULL;
    QueryTable[i].QueryRoutine = Ext2RegistryQueryCallback;
    i++;

    /*
     * 3 force writing
     */
    QueryTable[i].Flags = 0;
    QueryTable[i].Name = EXT3_FORCEWRITING;
    QueryTable[i].DefaultType = REG_NONE;
    QueryTable[i].DefaultLength = 0;
    QueryTable[i].DefaultData = NULL;
    QueryTable[i].EntryContext = NULL;
    QueryTable[i].QueryRoutine = Ext2RegistryQueryCallback;
    i++;

    /*
     * 4 automount
     */
    QueryTable[i].Flags = 0;
    QueryTable[i].Name = AUTO_MOUNT;
    QueryTable[i].DefaultType = REG_NONE;
    QueryTable[i].DefaultLength = 0;
    QueryTable[i].DefaultData = NULL;
    QueryTable[i].EntryContext = NULL;
    QueryTable[i].QueryRoutine = Ext2RegistryQueryCallback;
    i++;

    /*
     * 5 codepage
     */
    QueryTable[i].Flags = 0;
    QueryTable[i].Name = CODEPAGE_NAME;
    QueryTable[i].DefaultType = REG_NONE;
    QueryTable[i].DefaultLength = 0;
    QueryTable[i].DefaultData = NULL;
    QueryTable[i].EntryContext = NULL;
    QueryTable[i].QueryRoutine = Ext2RegistryQueryCallback;
    i++;

    /*
     * 6 hidden prefix
     */
    QueryTable[i].Flags = 0;
    QueryTable[i].Name = HIDING_PREFIX;
    QueryTable[i].DefaultType = REG_NONE;
    QueryTable[i].DefaultLength = 0;
    QueryTable[i].DefaultData = NULL;
    QueryTable[i].EntryContext = NULL;
    QueryTable[i].QueryRoutine = Ext2RegistryQueryCallback;
    i++;


    /*
     * 7 hidden suffix
     */
    QueryTable[i].Flags = 0;
    QueryTable[i].Name = HIDING_SUFFIX;
    QueryTable[i].DefaultType = REG_NONE;
    QueryTable[i].DefaultLength = 0;
    QueryTable[i].DefaultData = NULL;
    QueryTable[i].EntryContext = NULL;
    QueryTable[i].QueryRoutine = Ext2RegistryQueryCallback;
    i++;

    Status = RtlQueryRegistryValues(
                 RTL_REGISTRY_ABSOLUTE,
                 RegistryPath->Buffer,
                 &QueryTable[0],
                 NULL,
                 NULL
            );

    return NT_SUCCESS(Status);
}


BOOLEAN
Ext2QueryRegistrySettings(IN PUNICODE_STRING  RegistryPath)
{
    UNICODE_STRING              ParameterPath;
    UNICODE_STRING              UniName;
    ANSI_STRING                 AnsiName;

    ULONG                       WritingSupport = 0;
    ULONG                       CheckingBitmap = 0;
    ULONG                       Ext3ForceWriting = 0;
    ULONG                       AutoMount = 0;

    WCHAR                       UniBuffer[CODEPAGE_MAXLEN];
    USHORT                      Buffer[HIDINGPAT_LEN];

    NTSTATUS                    Status;

    ParameterPath.Length = 0;
    ParameterPath.MaximumLength =
        RegistryPath->Length + sizeof(PARAMETERS_KEY) + sizeof(WCHAR);
    ParameterPath.Buffer =
        (PWSTR) Ext2AllocatePool(
            PagedPool,
            ParameterPath.MaximumLength,
            'LG2E'
        );
    if (!ParameterPath.Buffer) {
        DbgBreak();
        DEBUG(DL_ERR, ( "Ex2QueryParameters: failed to allocate Parameters...\n"));
        return FALSE;
    }

    RtlCopyUnicodeString(&ParameterPath, RegistryPath);
    RtlAppendUnicodeToString(&ParameterPath, PARAMETERS_KEY);

    /* enable automount of ext2/3/4 volumes */
    SetLongFlag(Ext2Global->Flags, EXT2_AUTO_MOUNT);

    /* query parameter settings from registry */
    Ext2QueryGlobalParameters(&ParameterPath);

    /* set global codepage settings */
    if (wcslen(&Ext2Global->Codepage.PageName[0])) {
        UniName.Length = sizeof(WCHAR) * wcslen(&Ext2Global->Codepage.PageName[0]);
        UniName.MaximumLength = CODEPAGE_MAXLEN * sizeof(WCHAR);
        UniName.Buffer = &Ext2Global->Codepage.PageName[0];
        AnsiName.MaximumLength = CODEPAGE_MAXLEN;
        AnsiName.Length = 0;
        AnsiName.Buffer = &Ext2Global->Codepage.AnsiName[0];
        Status = RtlUnicodeStringToAnsiString(
                     &AnsiName,
                     &UniName,
                     FALSE);
        if (!NT_SUCCESS(Status)) {
            DEBUG(DL_ERR, ( "Ext2QueryParameters: Wrong CodePage %wZ ...\n", &UniName));
            RtlCopyMemory(&(Ext2Global->Codepage.AnsiName[0]),"default\0", 8);
        }
    } else {
        DEBUG(DL_ERR, ( "Ext2QueryParameters: CodePage not specified.\n"));
        RtlCopyMemory(&(Ext2Global->Codepage.AnsiName[0]),"default\0", 8);
    }
    Ext2Global->Codepage.AnsiName[CODEPAGE_MAXLEN - 1] = 0;


    /* set global hidden prefix pattern */
    if (wcslen(&Ext2Global->wHidingPrefix[0])) {
        UniName.Length = sizeof(WCHAR) * wcslen(&Ext2Global->wHidingPrefix[0]);
        UniName.MaximumLength = HIDINGPAT_LEN * sizeof(WCHAR);
        UniName.Buffer = &Ext2Global->wHidingPrefix[0];
        AnsiName.MaximumLength = HIDINGPAT_LEN;
        AnsiName.Length = 0;
        AnsiName.Buffer = &(Ext2Global->sHidingPrefix[0]);

        Status = RtlUnicodeStringToAnsiString(
                     &AnsiName,
                     &UniName,
                     FALSE);
        if (NT_SUCCESS(Status)) {
            Ext2Global->bHidingPrefix = TRUE;
        } else {
            DEBUG(DL_ERR, ( "Ext2QueryParameters: Wrong HidingPrefix ...\n"));
        }
    } else {
        DEBUG(DL_ERR, ( "Ext2QueryParameters: HidingPrefix not specified.\n"));
    }
    Ext2Global->sHidingPrefix[HIDINGPAT_LEN - 1] = 0;


    /* set global hidden suffix pattern */
    if (wcslen(&Ext2Global->wHidingSuffix[0])) {
        UniName.Length = sizeof(WCHAR) * wcslen(&Ext2Global->wHidingSuffix[0]);
        UniName.MaximumLength = HIDINGPAT_LEN * sizeof(WCHAR);
        UniName.Buffer = &Ext2Global->wHidingSuffix[0];
        AnsiName.MaximumLength = HIDINGPAT_LEN;
        AnsiName.Length = 0;
        AnsiName.Buffer = &(Ext2Global->sHidingSuffix[0]);

        Status = RtlUnicodeStringToAnsiString(
                     &AnsiName,
                     &UniName,
                     FALSE);
        if (NT_SUCCESS(Status)) {
            Ext2Global->bHidingSuffix = TRUE;
        } else {
            DEBUG(DL_ERR, ( "Ext2QueryParameters: Wrong HidingSuffix ...\n"));
        }
    } else {
        DEBUG(DL_ERR, ( "Ext2QueryParameters: HidingSuffix not specified.\n"));
    }
    Ext2Global->sHidingPrefix[HIDINGPAT_LEN - 1] = 0;

    Ext2Global->RegistryPath.Buffer = ParameterPath.Buffer;
    Ext2Global->RegistryPath.Length = 0;
    Ext2Global->RegistryPath.MaximumLength = ParameterPath.MaximumLength;
    RtlCopyUnicodeString(&Ext2Global->RegistryPath, RegistryPath);
    RtlAppendUnicodeToString(&Ext2Global->RegistryPath, VOLUMES_KEY);

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

VOID
Ext2EresourceAlignmentChecking()
{
    /* Verify ERESOURCE alignment in structures */
    CL_ASSERT((FIELD_OFFSET(EXT2_GLOBAL, Resource) & 7) == 0);
    CL_ASSERT((FIELD_OFFSET(EXT2_VCB, MainResource) & 7) == 0);
    CL_ASSERT((FIELD_OFFSET(EXT2_VCB, PagingIoResource) & 7) == 0);
    CL_ASSERT((FIELD_OFFSET(EXT2_VCB, MetaInode) & 7) == 0);
    CL_ASSERT((FIELD_OFFSET(EXT2_VCB, MetaBlock) & 7) == 0);
    CL_ASSERT((FIELD_OFFSET(EXT2_VCB, McbLock) & 7) == 0);
    CL_ASSERT((FIELD_OFFSET(EXT2_VCB, FcbLock) & 7) == 0);
    CL_ASSERT((FIELD_OFFSET(EXT2_VCB, bd.bd_bh_lock) & 7) == 0);
    CL_ASSERT((FIELD_OFFSET(EXT2_VCB, sbi.s_gd_lock) & 7) == 0);
    CL_ASSERT((FIELD_OFFSET(EXT2_FCBVCB, MainResource) & 7) == 0);
    CL_ASSERT((FIELD_OFFSET(EXT2_FCBVCB, PagingIoResource) & 7) == 0);
    CL_ASSERT((FIELD_OFFSET(EXT2_FCB, MainResource) & 7) == 0);
    CL_ASSERT((FIELD_OFFSET(EXT2_FCB, PagingIoResource) & 7) == 0);
}

/*
 * NAME: DriverEntry
 * FUNCTION: Called by the system to initalize the driver
 *
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
#ifdef __REACTOS__
NTSTATUS NTAPI
#else
NTSTATUS
#endif
DriverEntry (
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
)
{
    PDEVICE_OBJECT              DiskdevObject = NULL;
    PDEVICE_OBJECT              CdromdevObject = NULL;
    UNICODE_STRING              DeviceName;
    UNICODE_STRING              DosDeviceName;

    PFAST_IO_DISPATCH           FastIoDispatch;
    PCACHE_MANAGER_CALLBACKS    CacheManagerCallbacks;

    NTSTATUS                    Status;

    int                         rc = 0;
    BOOLEAN                     linux_lib_inited = FALSE;
    BOOLEAN                     journal_module_inited = FALSE;

    /* Verity super block ... */
    ASSERT(sizeof(EXT2_SUPER_BLOCK) == 1024);
    ASSERT(FIELD_OFFSET(EXT2_SUPER_BLOCK, s_magic) == 56);

    DbgPrint(
        "Ext2Fsd --"
#ifdef _WIN2K_TARGET_
        " Win2k --"
#endif
        " Version "
        EXT2FSD_VERSION
#if EXT2_DEBUG
        " Checked"
#else
        " Free"
#endif
        " -- "
        __DATE__ " "
        __TIME__ ".\n");

    DEBUG(DL_FUN, ( "Ext2 DriverEntry ...\n"));

    /* initialize winlib structures */
    if (ext2_init_linux()) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto errorout;
    }
    linux_lib_inited = TRUE;

    /* initialize journal module structures */
    LOAD_MODULE(journal_init);
    if (rc != 0) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto errorout;
    }
    journal_module_inited = TRUE;

    /* allocate memory for Ext2Global */
    Ext2Global = Ext2AllocatePool(NonPagedPool, sizeof(EXT2_GLOBAL), 'LG2E');
    if (!Ext2Global) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto errorout;
    }

    /* initialize Ext2Global */
    RtlZeroMemory(Ext2Global, sizeof(EXT2_GLOBAL));
    Ext2Global->Identifier.Type = EXT2FGD;
    Ext2Global->Identifier.Size = sizeof(EXT2_GLOBAL);

    InitializeListHead(&(Ext2Global->VcbList));
    ExInitializeResourceLite(&(Ext2Global->Resource));

    /* query registry settings */
    Ext2QueryRegistrySettings(RegistryPath);

    /* create Ext2Fsd cdrom fs deivce */
    RtlInitUnicodeString(&DeviceName, CDROM_NAME);
    Status = IoCreateDevice(
                 DriverObject,
                 0,
                 &DeviceName,
                 FILE_DEVICE_CD_ROM_FILE_SYSTEM,
                 0,
                 FALSE,
                 &CdromdevObject );

    if (!NT_SUCCESS(Status)) {
        DEBUG(DL_ERR, ( "IoCreateDevice cdrom device object error.\n"));
        goto errorout;
    }

    /* create Ext2Fsd disk fs deivce */
    RtlInitUnicodeString(&DeviceName, DEVICE_NAME);
    Status = IoCreateDevice(
                 DriverObject,
                 0,
                 &DeviceName,
                 FILE_DEVICE_DISK_FILE_SYSTEM,
                 0,
                 FALSE,
                 &DiskdevObject );

    if (!NT_SUCCESS(Status)) {
        DEBUG(DL_ERR, ( "IoCreateDevice disk device object error.\n"));
        goto errorout;
    }

    Status= Ext2StartReaper(
                &Ext2Global->FcbReaper,
                Ext2FcbReaperThread);
    if (!NT_SUCCESS(Status)) {
        goto errorout;
    }

    /* start resource reaper thread */
    Status= Ext2StartReaper(
                &Ext2Global->McbReaper,
                Ext2McbReaperThread);
    if (!NT_SUCCESS(Status)) {
        Ext2StopReaper(&Ext2Global->FcbReaper);
        goto errorout;
    }

    Status= Ext2StartReaper(
                &Ext2Global->bhReaper,
                Ext2bhReaperThread);
    if (!NT_SUCCESS(Status)) {
        Ext2StopReaper(&Ext2Global->FcbReaper);
        Ext2StopReaper(&Ext2Global->McbReaper);
        goto errorout;
    }

#ifdef _PNP_POWER_
    DiskdevObject->DeviceObjectExtension->PowerControlNeeded = FALSE;
    CdromdevObject->DeviceObjectExtension->PowerControlNeeded = FALSE;
#endif

    /* initializing */
    Ext2Global->DiskdevObject  = DiskdevObject;
    Ext2Global->CdromdevObject = CdromdevObject;

    DriverObject->MajorFunction[IRP_MJ_CREATE]              = Ext2BuildRequest;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]               = Ext2BuildRequest;
    DriverObject->MajorFunction[IRP_MJ_READ]                = Ext2BuildRequest;
    DriverObject->MajorFunction[IRP_MJ_WRITE]               = Ext2BuildRequest;

    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]       = Ext2BuildRequest;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]            = Ext2BuildRequest;

    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]   = Ext2BuildRequest;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]     = Ext2BuildRequest;

    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION]    = Ext2BuildRequest;
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION]      = Ext2BuildRequest;

    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]   = Ext2BuildRequest;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = Ext2BuildRequest;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]      = Ext2BuildRequest;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL]        = Ext2BuildRequest;

    DriverObject->MajorFunction[IRP_MJ_QUERY_EA] = Ext2BuildRequest;
    DriverObject->MajorFunction[IRP_MJ_SET_EA] = Ext2BuildRequest;

    DriverObject->MajorFunction[IRP_MJ_CLEANUP]             = Ext2BuildRequest;

#if (_WIN32_WINNT >= 0x0500)
    DriverObject->MajorFunction[IRP_MJ_PNP]                 = Ext2BuildRequest;
#endif //(_WIN32_WINNT >= 0x0500)

#if EXT2_UNLOAD
    DriverObject->DriverUnload                              = DriverUnload;
#else
    DriverObject->DriverUnload                              = NULL;
#endif

    //
    // Initialize the fast I/O entry points
    //

    FastIoDispatch = &(Ext2Global->FastIoDispatch);

    FastIoDispatch->SizeOfFastIoDispatch          = sizeof(FAST_IO_DISPATCH);
    FastIoDispatch->FastIoCheckIfPossible         = Ext2FastIoCheckIfPossible;
    FastIoDispatch->FastIoRead                    = Ext2FastIoRead;
    FastIoDispatch->FastIoWrite                   = Ext2FastIoWrite;
    FastIoDispatch->FastIoQueryBasicInfo          = Ext2FastIoQueryBasicInfo;
    FastIoDispatch->FastIoQueryStandardInfo       = Ext2FastIoQueryStandardInfo;
    FastIoDispatch->FastIoLock                    = Ext2FastIoLock;
    FastIoDispatch->FastIoUnlockSingle            = Ext2FastIoUnlockSingle;
    FastIoDispatch->FastIoUnlockAll               = Ext2FastIoUnlockAll;
    FastIoDispatch->FastIoUnlockAllByKey          = Ext2FastIoUnlockAllByKey;
    FastIoDispatch->FastIoQueryNetworkOpenInfo    = Ext2FastIoQueryNetworkOpenInfo;

    FastIoDispatch->AcquireForModWrite            = Ext2AcquireFileForModWrite;
    FastIoDispatch->ReleaseForModWrite            = Ext2ReleaseFileForModWrite;
    FastIoDispatch->AcquireForModWrite            = Ext2AcquireFileForModWrite;
    FastIoDispatch->ReleaseForModWrite            = Ext2ReleaseFileForModWrite;
    FastIoDispatch->AcquireForCcFlush             = Ext2AcquireFileForCcFlush;
    FastIoDispatch->ReleaseForCcFlush             = Ext2ReleaseFileForCcFlush;
    FastIoDispatch->AcquireFileForNtCreateSection = Ext2AcquireForCreateSection;
    FastIoDispatch->ReleaseFileForNtCreateSection = Ext2ReleaseForCreateSection;

    DriverObject->FastIoDispatch = FastIoDispatch;

    //
    //  initializing structure sizes for statistics
    //  1 means flexible/not fixed for all allocations (for different volumes).
    //
    Ext2Global->PerfStat.Magic   = EXT2_PERF_STAT_MAGIC;
    Ext2Global->PerfStat.Version = EXT2_PERF_STAT_VER2;
    Ext2Global->PerfStat.Length  = sizeof(EXT2_PERF_STATISTICS_V2);

    Ext2Global->PerfStat.Unit.Slot[PS_IRP_CONTEXT] = sizeof(EXT2_IRP_CONTEXT);  /* 0 */
    Ext2Global->PerfStat.Unit.Slot[PS_VCB] = sizeof(EXT2_VCB);                  /* 1 */
    Ext2Global->PerfStat.Unit.Slot[PS_FCB] = sizeof(EXT2_FCB);                  /* 2 */
    Ext2Global->PerfStat.Unit.Slot[PS_CCB] = sizeof(EXT2_CCB);                  /* 3 */
    Ext2Global->PerfStat.Unit.Slot[PS_MCB] = sizeof(EXT2_MCB);                  /* 4 */
    Ext2Global->PerfStat.Unit.Slot[PS_EXTENT] = sizeof(EXT2_EXTENT);            /* 5 */
    Ext2Global->PerfStat.Unit.Slot[PS_RW_CONTEXT] = sizeof(EXT2_RW_CONTEXT);    /* 6 */
    Ext2Global->PerfStat.Unit.Slot[PS_VPB] = sizeof(VPB);                       /* 7 */
    Ext2Global->PerfStat.Unit.Slot[PS_FILE_NAME] = 1;                           /* 8 */
    Ext2Global->PerfStat.Unit.Slot[PS_MCB_NAME] = 1;                            /* 9 */
    Ext2Global->PerfStat.Unit.Slot[PS_INODE_NAME] = 1;                          /* a */
    Ext2Global->PerfStat.Unit.Slot[PS_DIR_ENTRY] = sizeof(EXT2_DIR_ENTRY2);     /* b */
    Ext2Global->PerfStat.Unit.Slot[PS_DIR_PATTERN] = 1;                         /* c */
    Ext2Global->PerfStat.Unit.Slot[PS_DISK_EVENT] = sizeof(KEVENT);             /* d */
    Ext2Global->PerfStat.Unit.Slot[PS_DISK_BUFFER] = 1;                         /* e */
    Ext2Global->PerfStat.Unit.Slot[PS_BLOCK_DATA] = 1;                          /* f */
    Ext2Global->PerfStat.Unit.Slot[PS_EXT2_INODE] = 1;                          /* 10 */
    Ext2Global->PerfStat.Unit.Slot[PS_DENTRY] = sizeof(struct dentry);          /* 11 */
    Ext2Global->PerfStat.Unit.Slot[PS_BUFF_HEAD] = sizeof(struct buffer_head);  /* 12 */

    switch ( MmQuerySystemSize() ) {

    case MmSmallSystem:

        Ext2Global->MaxDepth = 64;
        break;

    case MmMediumSystem:

        Ext2Global->MaxDepth = 128;
        break;

    case MmLargeSystem:

        Ext2Global->MaxDepth = 256;
        break;
    }

    //
    // Initialize the Cache Manager callbacks
    //

    CacheManagerCallbacks = &(Ext2Global->CacheManagerCallbacks);
    CacheManagerCallbacks->AcquireForLazyWrite  = Ext2AcquireForLazyWrite;
    CacheManagerCallbacks->ReleaseFromLazyWrite = Ext2ReleaseFromLazyWrite;
    CacheManagerCallbacks->AcquireForReadAhead  = Ext2AcquireForReadAhead;
    CacheManagerCallbacks->ReleaseFromReadAhead = Ext2ReleaseFromReadAhead;

    Ext2Global->CacheManagerNoOpCallbacks.AcquireForLazyWrite  = Ext2NoOpAcquire;
    Ext2Global->CacheManagerNoOpCallbacks.ReleaseFromLazyWrite = Ext2NoOpRelease;
    Ext2Global->CacheManagerNoOpCallbacks.AcquireForReadAhead  = Ext2NoOpAcquire;
    Ext2Global->CacheManagerNoOpCallbacks.ReleaseFromReadAhead = Ext2NoOpRelease;


#ifndef _WIN2K_TARGET_
    //
    // Initialize FS Filter callbacks
    //

    RtlZeroMemory(&Ext2Global->FilterCallbacks,  sizeof(FS_FILTER_CALLBACKS));
    Ext2Global->FilterCallbacks.SizeOfFsFilterCallbacks = sizeof(FS_FILTER_CALLBACKS);
    Ext2Global->FilterCallbacks.PreAcquireForSectionSynchronization = Ext2PreAcquireForCreateSection;
    FsRtlRegisterFileSystemFilterCallbacks(DriverObject,  &Ext2Global->FilterCallbacks );

#endif

    //
    // Initialize the global data
    //

    ExInitializeNPagedLookasideList( &(Ext2Global->Ext2IrpContextLookasideList),
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(EXT2_IRP_CONTEXT),
                                     'PRIE',
                                     0 );

    ExInitializeNPagedLookasideList( &(Ext2Global->Ext2FcbLookasideList),
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(EXT2_FCB),
                                     'BCFE',
                                     0 );

    ExInitializeNPagedLookasideList( &(Ext2Global->Ext2CcbLookasideList),
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(EXT2_CCB),
                                    'BCCE',
                                    0 );

    ExInitializeNPagedLookasideList( &(Ext2Global->Ext2McbLookasideList),
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(EXT2_MCB),
                                    'BCME',
                                    0 );

    ExInitializeNPagedLookasideList( &(Ext2Global->Ext2ExtLookasideList),
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(EXT2_EXTENT),
                                    'STXE',
                                    0 );

    ExInitializeNPagedLookasideList( &(Ext2Global->Ext2DentryLookasideList),
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(struct dentry),
                                    'TNED',
                                    0 );

    RtlInitUnicodeString(&DosDeviceName, DOS_DEVICE_NAME);
    IoCreateSymbolicLink(&DosDeviceName, &DeviceName);

#if EXT2_DEBUG
    ProcessNameOffset = Ext2GetProcessNameOffset();
#endif

    Ext2LoadAllNls();

    Ext2Global->Codepage.PageTable =
        load_nls(Ext2Global->Codepage.AnsiName);

    /* register file system devices for disk and cdrom */
    IoRegisterFileSystem(DiskdevObject);
    ObReferenceObject(DiskdevObject);

    IoRegisterFileSystem(CdromdevObject);
    ObReferenceObject(CdromdevObject);

errorout:

    if (!NT_SUCCESS(Status)) {

        /*
         *  stop reaper thread ...
         */


        /*
         *  cleanup resources ...
         */

        if (Ext2Global) {
            ExDeleteResourceLite(&Ext2Global->Resource);
            Ext2FreePool(Ext2Global, 'LG2E');
        }

        if (CdromdevObject) {
            IoDeleteDevice(CdromdevObject);
        }

        if (DiskdevObject) {
            IoDeleteDevice(DiskdevObject);
        }

        if (journal_module_inited) {
            /* cleanup journal related caches */
            UNLOAD_MODULE(journal_exit);
        }

        if (linux_lib_inited) {
            /* cleanup linux lib */
            ext2_destroy_linux();
        }
    }

    return Status;
}
