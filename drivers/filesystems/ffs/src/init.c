/* 
 * FFS File System Driver for Windows
 *
 * init.c
 *
 * 2004.5.6 ~
 *
 * Lee Jae-Hong, http://www.pyrasis.com
 *
 * See License.txt
 *
 */

#include <ntifs.h>
#ifndef __REACTOS__
#include <wdmsec.h>
#endif
#include "ffsdrv.h"

/* Globals */

PFFS_GLOBAL FFSGlobal = NULL;


/* Definitions */

NTSTATUS NTAPI
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, FFSQueryParameters)
#pragma alloc_text(INIT, DriverEntry)
#if FFS_UNLOAD
#pragma alloc_text(PAGE, DriverUnload)
#endif
#endif


#if FFS_UNLOAD

/*
 * FUNCTION: Called by the system to unload the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 * RETURNS:  None
 */

VOID NTAPI
DriverUnload(
	IN PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING  DosDeviceName;

    PAGED_CODE();

	FFSPrint((DBG_FUNC, "ffsdrv: Unloading routine.\n"));

	RtlInitUnicodeString(&DosDeviceName, DOS_DEVICE_NAME);
	IoDeleteSymbolicLink(&DosDeviceName);

	ExDeleteResourceLite(&FFSGlobal->LAResource);
	ExDeleteResourceLite(&FFSGlobal->CountResource);
	ExDeleteResourceLite(&FFSGlobal->Resource);

	ExDeletePagedLookasideList(&(FFSGlobal->FFSMcbLookasideList));
	ExDeleteNPagedLookasideList(&(FFSGlobal->FFSCcbLookasideList));
	ExDeleteNPagedLookasideList(&(FFSGlobal->FFSFcbLookasideList));
	ExDeleteNPagedLookasideList(&(FFSGlobal->FFSIrpContextLookasideList));

	IoDeleteDevice(FFSGlobal->DeviceObject);
}

#endif

BOOLEAN
FFSQueryParameters(
	IN PUNICODE_STRING  RegistryPath)
{
	NTSTATUS                    Status;
	UNICODE_STRING              ParameterPath;
	RTL_QUERY_REGISTRY_TABLE    QueryTable[2];

	ULONG                       WritingSupport;
	ULONG                       CheckingBitmap;
	ULONG                       PartitionNumber;

	ParameterPath.Length = 0;

	ParameterPath.MaximumLength =
		RegistryPath->Length + sizeof(PARAMETERS_KEY) + sizeof(WCHAR);

	ParameterPath.Buffer =
		(PWSTR) ExAllocatePoolWithTag(PagedPool, ParameterPath.MaximumLength, FFS_POOL_TAG);

	if (!ParameterPath.Buffer)
	{
		return FALSE;
	}

	WritingSupport = 0;
	CheckingBitmap = 0;
	PartitionNumber = 0;

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
				NULL);

	FFSPrint((DBG_USER, "FFSQueryParameters: WritingSupport=%xh\n", WritingSupport));

	RtlZeroMemory(&QueryTable[0], sizeof(RTL_QUERY_REGISTRY_TABLE) * 2);

	QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
	QueryTable[0].Name = CHECKING_BITMAP;
	QueryTable[0].EntryContext = &CheckingBitmap;

	Status = RtlQueryRegistryValues(
				RTL_REGISTRY_ABSOLUTE,
				ParameterPath.Buffer,
				&QueryTable[0],
				NULL,
				NULL);

	FFSPrint((DBG_USER, "FFSQueryParameters: CheckingBitmap=%xh\n", CheckingBitmap));

	RtlZeroMemory(&QueryTable[0], sizeof(RTL_QUERY_REGISTRY_TABLE) * 2);

	QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
	QueryTable[0].Name = PARTITION_NUMBER;
	QueryTable[0].EntryContext = &PartitionNumber;

	Status = RtlQueryRegistryValues(
				RTL_REGISTRY_ABSOLUTE,
				ParameterPath.Buffer,
				&QueryTable[0],
				NULL,
				NULL);

	FFSPrint((DBG_USER, "FFSQueryParameters: PartitionNumber=%xh\n", PartitionNumber));

	{
		if (WritingSupport)
		{
			SetFlag(FFSGlobal->Flags, FFS_SUPPORT_WRITING);
		}
		else
		{
			ClearFlag(FFSGlobal->Flags, FFS_SUPPORT_WRITING);
		}

		if (CheckingBitmap)
		{
			SetFlag(FFSGlobal->Flags, FFS_CHECKING_BITMAP);
		}
		else
		{
			ClearFlag(FFSGlobal->Flags, FFS_CHECKING_BITMAP);
		}

		if (PartitionNumber)
		{
			FFSGlobal->PartitionNumber = PartitionNumber;
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
DriverEntry(
	IN PDRIVER_OBJECT   DriverObject,
	IN PUNICODE_STRING  RegistryPath)
{
	PDEVICE_OBJECT              DeviceObject;
	PFAST_IO_DISPATCH           FastIoDispatch;
	PCACHE_MANAGER_CALLBACKS    CacheManagerCallbacks;
	PFFS_EXT                    DeviceExt;
	UNICODE_STRING              DeviceName;
#ifndef __REACTOS__
    UNICODE_STRING              Sddl;
#endif
	NTSTATUS                    Status;
#if FFS_UNLOAD
	UNICODE_STRING              DosDeviceName;
#endif

	DbgPrint(
			"ffsdrv --"
			" Version " 
			FFSDRV_VERSION
#if FFS_READ_ONLY
			" (ReadOnly)"
#endif // FFS_READ_ONLY
#if DBG
			" Checked"
#else
			" Free" 
#endif
			" - Built at "
			__DATE__" "
			__TIME__".\n");

	FFSPrint((DBG_FUNC, "FFS DriverEntry ...\n"));

	RtlInitUnicodeString(&DeviceName, DEVICE_NAME);
#ifndef __REACTOS__
    RtlInitUnicodeString(&Sddl, L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;BU)");

	Status = IoCreateDeviceSecure(
				DriverObject,
				sizeof(FFS_EXT),
				&DeviceName,
				FILE_DEVICE_DISK_FILE_SYSTEM,
				0,
				FALSE,
                &Sddl,
                NULL,
				&DeviceObject);
#else

	Status = IoCreateDevice(
				DriverObject,
				sizeof(FFS_EXT),
				&DeviceName,
				FILE_DEVICE_DISK_FILE_SYSTEM,
				0,
				FALSE,
				&DeviceObject);
#endif
	if (!NT_SUCCESS(Status))
	{
		FFSPrint((DBG_ERROR, "IoCreateDevice fs object error.\n"));
		return Status;
	}

	DeviceExt = (PFFS_EXT)DeviceObject->DeviceExtension;
	RtlZeroMemory(DeviceExt, sizeof(FFS_EXT));

	FFSGlobal = &(DeviceExt->FFSGlobal);

	FFSGlobal->Identifier.Type = FFSFGD;
	FFSGlobal->Identifier.Size = sizeof(FFS_GLOBAL);
	FFSGlobal->DeviceObject = DeviceObject;
	FFSGlobal->DriverObject = DriverObject;
	FFSGlobal->PartitionNumber = 0;

	FFSQueryParameters(RegistryPath);

	DriverObject->MajorFunction[IRP_MJ_CREATE]              = FFSBuildRequest;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]               = FFSBuildRequest;
	DriverObject->MajorFunction[IRP_MJ_READ]                = FFSBuildRequest;
#if !FFS_READ_ONLY
	DriverObject->MajorFunction[IRP_MJ_WRITE]               = FFSBuildRequest;
#endif // !FFS_READ_ONLY

	DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]       = FFSBuildRequest;
	DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]            = FFSBuildRequest;

	DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]   = FFSBuildRequest;
	DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]     = FFSBuildRequest;

	DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION]    = FFSBuildRequest;
#if !FFS_READ_ONLY
	DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION]      = FFSBuildRequest;
#endif // !FFS_READ_ONLY

	DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]   = FFSBuildRequest;
	DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = FFSBuildRequest;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]      = FFSBuildRequest;
	DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL]        = FFSBuildRequest;

	DriverObject->MajorFunction[IRP_MJ_CLEANUP]             = FFSBuildRequest;

#if (_WIN32_WINNT >= 0x0500)
	DriverObject->MajorFunction[IRP_MJ_PNP]                 = FFSBuildRequest;
#endif //(_WIN32_WINNT >= 0x0500)

#if FFS_UNLOAD
#ifdef _MSC_VER
#pragma prefast( suppress: 28175, "allowed to unload" )
#endif
	DriverObject->DriverUnload                              = DriverUnload;
#else
#ifdef _MSC_VER
#pragma prefast( suppress: 28175, "allowed to unload" )
#endif
	DriverObject->DriverUnload                              = NULL;
#endif

	//
	// Initialize the fast I/O entry points
	//

	FastIoDispatch = &(FFSGlobal->FastIoDispatch);

	FastIoDispatch->SizeOfFastIoDispatch        = sizeof(FAST_IO_DISPATCH);
	FastIoDispatch->FastIoCheckIfPossible       = FFSFastIoCheckIfPossible;
#if DBG
	FastIoDispatch->FastIoRead                  = FFSFastIoRead;
#if !FFS_READ_ONLY
	FastIoDispatch->FastIoWrite                 = FFSFastIoWrite;
#endif // !FFS_READ_ONLY
#else
#ifdef _MSC_VER
#pragma prefast( suppress: 28155, "allowed in file system drivers" )
#endif
	FastIoDispatch->FastIoRead                  = FsRtlCopyRead;
#if !FFS_READ_ONLY
#ifdef _MSC_VER
#pragma prefast( suppress: 28155, "allowed in file system drivers" )
#endif
	FastIoDispatch->FastIoWrite                 = FsRtlCopyWrite;
#endif // !FFS_READ_ONLY
#endif
	FastIoDispatch->FastIoQueryBasicInfo        = FFSFastIoQueryBasicInfo;
	FastIoDispatch->FastIoQueryStandardInfo     = FFSFastIoQueryStandardInfo;
	FastIoDispatch->FastIoLock                  = FFSFastIoLock;
	FastIoDispatch->FastIoUnlockSingle          = FFSFastIoUnlockSingle;
	FastIoDispatch->FastIoUnlockAll             = FFSFastIoUnlockAll;
	FastIoDispatch->FastIoUnlockAllByKey        = FFSFastIoUnlockAllByKey;
	FastIoDispatch->FastIoQueryNetworkOpenInfo  = FFSFastIoQueryNetworkOpenInfo;

#ifdef _MSC_VER
#pragma prefast( suppress: 28175, "allowed in file system drivers" )
#endif
	DriverObject->FastIoDispatch = FastIoDispatch;

	switch (MmQuerySystemSize())
	{
		case MmSmallSystem:

			FFSGlobal->MaxDepth = 16;
			break;

		case MmMediumSystem:

			FFSGlobal->MaxDepth = 64;
			break;

		case MmLargeSystem:

			FFSGlobal->MaxDepth = 256;
			break;
	}

	//
	// Initialize the Cache Manager callbacks
	//

	CacheManagerCallbacks = &(FFSGlobal->CacheManagerCallbacks);
	CacheManagerCallbacks->AcquireForLazyWrite  = FFSAcquireForLazyWrite;
	CacheManagerCallbacks->ReleaseFromLazyWrite = FFSReleaseFromLazyWrite;
	CacheManagerCallbacks->AcquireForReadAhead  = FFSAcquireForReadAhead;
	CacheManagerCallbacks->ReleaseFromReadAhead = FFSReleaseFromReadAhead;

	FFSGlobal->CacheManagerNoOpCallbacks.AcquireForLazyWrite  = FFSNoOpAcquire;
	FFSGlobal->CacheManagerNoOpCallbacks.ReleaseFromLazyWrite = FFSNoOpRelease;
	FFSGlobal->CacheManagerNoOpCallbacks.AcquireForReadAhead  = FFSNoOpAcquire;
	FFSGlobal->CacheManagerNoOpCallbacks.ReleaseFromReadAhead = FFSNoOpRelease;


	//
	// Initialize the global data
	//

	InitializeListHead(&(FFSGlobal->VcbList));
	ExInitializeResourceLite(&(FFSGlobal->Resource));
	ExInitializeResourceLite(&(FFSGlobal->CountResource));
	ExInitializeResourceLite(&(FFSGlobal->LAResource));

	ExInitializeNPagedLookasideList(&(FFSGlobal->FFSIrpContextLookasideList),
			NULL,
			NULL,
			0,
			sizeof(FFS_IRP_CONTEXT),
			' SFF',
			0);

	ExInitializeNPagedLookasideList(&(FFSGlobal->FFSFcbLookasideList),
			NULL,
			NULL,
			0,
			sizeof(FFS_FCB),
			' SFF',
			0);

	ExInitializeNPagedLookasideList(&(FFSGlobal->FFSCcbLookasideList),
			NULL,
			NULL,
			0,
			sizeof(FFS_CCB),
			' SFF',
			0);

	ExInitializePagedLookasideList(&(FFSGlobal->FFSMcbLookasideList),
			NULL,
			NULL,
			0,
			sizeof(FFS_MCB),
			' SFF',
			0);

#if FFS_UNLOAD
	RtlInitUnicodeString(&DosDeviceName, DOS_DEVICE_NAME);
	IoCreateSymbolicLink(&DosDeviceName, &DeviceName);
#endif

#if DBG
	ProcessNameOffset = FFSGetProcessNameOffset();
#endif

	IoRegisterFileSystem(DeviceObject);

	return Status;
}
