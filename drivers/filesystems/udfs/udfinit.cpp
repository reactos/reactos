////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: UDFinit.cpp
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*     This file contains the initialization code for the kernel mode
*     UDF FSD module. The DriverEntry() routine is called by the I/O
*     sub-system to initialize the FSD.
*
*************************************************************************/

#include            "udffs.h"

// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID                UDF_FILE_INIT

// global variables are declared here
UDFData                 UDFGlobalData;

#define KD_PREFIX

struct UDF_MEDIA_CLASS_NAMES UDFMediaClassName[] = {
    {MediaUnknown, REG_DEFAULT_UNKNOWN},
    {MediaHdd    , REG_DEFAULT_HDD},
    {MediaCdr    , REG_DEFAULT_CDR},
    {MediaCdrw   , REG_DEFAULT_CDRW},
    {MediaCdrom  , REG_DEFAULT_CDROM},
    {MediaZip    , REG_DEFAULT_ZIP},
    {MediaFloppy , REG_DEFAULT_FLOPPY},
    {MediaDvdr   , REG_DEFAULT_DVDR},
    {MediaDvdrw  , REG_DEFAULT_DVDRW}
};
/*
ULONG  MajorVersion = 0;
ULONG  MinorVersion = 0;
ULONG  BuildNumber  = 0;
*/
ULONG  FsRegistered = FALSE;

WORK_QUEUE_ITEM    RemountWorkQueueItem;

//ptrFsRtlNotifyVolumeEvent FsRtlNotifyVolumeEvent = NULL;

HANDLE  FsNotification_ThreadId = (HANDLE)(-1);

NTSTATUS
UDFCreateFsDeviceObject(
    PCWSTR          FsDeviceName,
    PDRIVER_OBJECT  DriverObject,
    DEVICE_TYPE     DeviceType,
    PDEVICE_OBJECT  *DeviceObject);

NTSTATUS
UDFDismountDevice(
    PUNICODE_STRING unicodeCdRomDeviceName);

VOID
UDFRemountAll(
    IN PVOID Context);

/*************************************************************************
*
* Function: DriverEntry()
*
* Description:
*   This routine is the standard entry point for all kernel mode drivers.
*   The routine is invoked at IRQL PASSIVE_LEVEL in the context of a
*   system worker thread.
*   All FSD specific data structures etc. are initialized here.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error (will cause driver to be unloaded).
*
*************************************************************************/
NTSTATUS
NTAPI
DriverEntry(
    PDRIVER_OBJECT  DriverObject,       // created by the I/O sub-system
    PUNICODE_STRING RegistryPath        // path to the registry key
    )
{
    NTSTATUS        RC = STATUS_SUCCESS;
    UNICODE_STRING  DriverDeviceName;
    UNICODE_STRING  unicodeDeviceName;
//    BOOLEAN         RegisteredShutdown = FALSE;
    BOOLEAN         InternalMMInitialized = FALSE;
//    BOOLEAN         DLDetectInitialized = FALSE;
//    ULONG           CdRomNumber;
//    CCHAR           deviceNameBuffer[MAXIMUM_FILENAME_LENGTH];
//    ANSI_STRING     deviceName;
//    UNICODE_STRING  unicodeCdRomDeviceName;
    PUDFFS_DEV_EXTENSION FSDevExt;
    HKEY            hUdfRootKey;
    LARGE_INTEGER   delay;

//    UDFPrint(("UDF: Entered " VER_STR_PRODUCT_NAME " UDF DriverEntry \n"));
//    UDFPrint((KD_PREFIX "Build " VER_STR_PRODUCT "\n"));

    _SEH2_TRY {
        _SEH2_TRY {

/*
            CrNtInit(DriverObject, RegistryPath);

            //PsGetVersion(&MajorVersion, &MinorVersion, &BuildNumber, NULL);
            UDFPrint(("UDF: OS Version Major: %x, Minor: %x, Build number: %d\n",
                                MajorVersion, MinorVersion, BuildNumber));
*/
#ifdef __REACTOS__
            UDFPrint(("UDF Init: OS should be ReactOS\n"));
#endif

            // initialize the global data structure
            RtlZeroMemory(&UDFGlobalData, sizeof(UDFGlobalData));

            // initialize some required fields
            UDFGlobalData.NodeIdentifier.NodeType = UDF_NODE_TYPE_GLOBAL_DATA;
            UDFGlobalData.NodeIdentifier.NodeSize = sizeof(UDFGlobalData);

            // initialize the global data resource and remember the fact that
            //  the resource has been initialized
            RC = UDFInitializeResourceLite(&(UDFGlobalData.GlobalDataResource));
            ASSERT(NT_SUCCESS(RC));
            UDFSetFlag(UDFGlobalData.UDFFlags, UDF_DATA_FLAGS_RESOURCE_INITIALIZED);

            RC = UDFInitializeResourceLite(&(UDFGlobalData.DelayedCloseResource));
            ASSERT(NT_SUCCESS(RC));
//            UDFSetFlag(UDFGlobalData.UDFFlags, UDF_DATA_FLAGS_RESOURCE_INITIALIZED);

            // keep a ptr to the driver object sent to us by the I/O Mgr
            UDFGlobalData.DriverObject = DriverObject;

            //SeEnableAccessToExports();

            // initialize the mounted logical volume list head
            InitializeListHead(&(UDFGlobalData.VCBQueue));

            UDFPrint(("UDF: Init memory manager\n"));
            // Initialize internal memory management
            if(!MyAllocInit()) {
                try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
            }
            InternalMMInitialized = TRUE;

#ifdef USE_DLD
            // Initialize Deadlock Detector
            DLDInit(1280);
            DLDetectInitialized = TRUE;
#endif
            // before we proceed with any more initialization, read in
            //  user supplied configurable values ...

            // Save RegistryPath
            RtlCopyMemory(&(UDFGlobalData.SavedRegPath), RegistryPath, sizeof(UNICODE_STRING));

            UDFGlobalData.SavedRegPath.Buffer = (PWSTR)MyAllocatePool__(NonPagedPool, RegistryPath->Length + 2);
            if(!UDFGlobalData.SavedRegPath.Buffer) try_return (RC = STATUS_INSUFFICIENT_RESOURCES);
            RtlCopyMemory(UDFGlobalData.SavedRegPath.Buffer, RegistryPath->Buffer, RegistryPath->Length + 2);

            RegTGetKeyHandle(NULL, UDFGlobalData.SavedRegPath.Buffer, &hUdfRootKey);

            RtlInitUnicodeString(&UDFGlobalData.UnicodeStrRoot, L"\\");
            RtlInitUnicodeString(&UDFGlobalData.UnicodeStrSDir, L":");
            RtlInitUnicodeString(&UDFGlobalData.AclName, UDF_SN_NT_ACL);

            UDFPrint(("UDF: Init delayed close queues\n"));
#ifdef UDF_DELAYED_CLOSE
            InitializeListHead( &UDFGlobalData.DelayedCloseQueue );
            InitializeListHead( &UDFGlobalData.DirDelayedCloseQueue );

            ExInitializeWorkItem( &UDFGlobalData.CloseItem,
                                  UDFDelayedClose,
                                  NULL );

            UDFGlobalData.DelayedCloseCount = 0;
            UDFGlobalData.DirDelayedCloseCount = 0;
#endif //UDF_DELAYED_CLOSE

            // we should have the registry data (if any), allocate zone memory ...
            //  This is an example of when FSD implementations __try to pre-allocate
            //  some fixed amount of memory to avoid internal fragmentation and/or waiting
            //  later during run-time ...

            UDFGlobalData.DefaultZoneSizeInNumStructs=10;

            UDFPrint(("UDF: Init zones\n"));
            if (!NT_SUCCESS(RC = UDFInitializeZones()))
                try_return(RC);

            UDFPrint(("UDF: Init pointers\n"));
            // initialize the IRP major function table, and the fast I/O table
            UDFInitializeFunctionPointers(DriverObject);

            UDFGlobalData.CPU_Count = KeNumberProcessors;

            // create a device object representing the driver itself
            //  so that requests can be targeted to the driver ...
            //  e.g. for a disk-based FSD, "mount" requests will be sent to
            //        this device object by the I/O Manager.
            //        For a redirector/server, you may have applications
            //        send "special" IOCTL's using this device object ...

            RtlInitUnicodeString(&DriverDeviceName, UDF_FS_NAME);

            UDFPrint(("UDF: Create Driver dev obj\n"));
            if (!NT_SUCCESS(RC = IoCreateDevice(
                    DriverObject,       // our driver object
                    sizeof(UDFFS_DEV_EXTENSION),  // don't need an extension for this object
                    &DriverDeviceName,  // name - can be used to "open" the driver
                                        // see the book for alternate choices
                    FILE_DEVICE_CD_ROM_FILE_SYSTEM,
                    0,                  // no special characteristics
                                        // do not want this as an exclusive device, though you might
                    FALSE,
                    &(UDFGlobalData.UDFDeviceObject)))) {
                        // failed to create a device object, leave ...
                try_return(RC);
            }

            FSDevExt = (PUDFFS_DEV_EXTENSION)((UDFGlobalData.UDFDeviceObject)->DeviceExtension);
            // Zero it out (typically this has already been done by the I/O
            // Manager but it does not hurt to do it again)!
            RtlZeroMemory(FSDevExt, sizeof(UDFFS_DEV_EXTENSION));

            // Initialize the signature fields
            FSDevExt->NodeIdentifier.NodeType = UDF_NODE_TYPE_UDFFS_DRVOBJ;
            FSDevExt->NodeIdentifier.NodeSize = sizeof(UDFFS_DEV_EXTENSION);

            RtlInitUnicodeString(&unicodeDeviceName, UDF_DOS_FS_NAME);
            IoCreateSymbolicLink(&unicodeDeviceName, &DriverDeviceName);

            UDFPrint(("UDF: Create CD dev obj\n"));
            if (!NT_SUCCESS(RC = UDFCreateFsDeviceObject(UDF_FS_NAME_CD,
                                    DriverObject,
                                    FILE_DEVICE_CD_ROM_FILE_SYSTEM,
                                    &(UDFGlobalData.UDFDeviceObject_CD)))) {
                // failed to create a device object, leave ...
                try_return(RC);
            }
#ifdef UDF_HDD_SUPPORT
            UDFPrint(("UDF: Create HDD dev obj\n"));
            if (!NT_SUCCESS(RC = UDFCreateFsDeviceObject(UDF_FS_NAME_HDD,
                                    DriverObject,
                                    FILE_DEVICE_DISK_FILE_SYSTEM,
                                    &(UDFGlobalData.UDFDeviceObject_HDD)))) {
                // failed to create a device object, leave ...
                try_return(RC);
            }
#endif //UDF_HDD_SUPPORT

/*            RtlInitUnicodeString(&DriverDeviceName, UDF_FS_NAME_OTHER);

            if (!NT_SUCCESS(RC = IoCreateDevice(
                    DriverObject,       // our driver object
                    0,                  // don't need an extension for this object
                    &DriverDeviceName,  // name - can be used to "open" the driver
                                        // see the book for alternate choices
                    FILE_DEVICE_FILE_SYSTEM,
                    0,                  // no special characteristics
                                        // do not want this as an exclusive device, though you might
                    FALSE,
                    &(UDFGlobalData.UDFDeviceObject_OTHER)))) {
                        // failed to create a device object, leave ...
                try_return(RC);
            }
            // register the driver with the I/O Manager, pretend as if this is
            //  a physical disk based FSD (or in order words, this FSD manages
            //  logical volumes residing on physical disk drives)
            IoRegisterFileSystem(UDFGlobalData.UDFDeviceObject_OTHER);

            RtlInitUnicodeString(&DriverDeviceName, UDF_FS_NAME_TAPE);

            if (!NT_SUCCESS(RC = IoCreateDevice(
                    DriverObject,       // our driver object
                    0,                  // don't need an extension for this object
                    &DriverDeviceName,  // name - can be used to "open" the driver
                                        // see the book for alternate choices
                    FILE_DEVICE_TAPE_FILE_SYSTEM,
                    0,                  // no special characteristics
                                        // do not want this as an exclusive device, though you might
                    FALSE,
                    &(UDFGlobalData.UDFDeviceObject_TAPE)))) {
                        // failed to create a device object, leave ...
                try_return(RC);
            }
            // register the driver with the I/O Manager, pretend as if this is
            //  a physical disk based FSD (or in order words, this FSD manages
            //  logical volumes residing on physical disk drives)
            IoRegisterFileSystem(UDFGlobalData.UDFDeviceObject_TAPE);
*/

            if (UDFGlobalData.UDFDeviceObject_CD) {
                UDFPrint(("UDFCreateFsDeviceObject: IoRegisterFileSystem() for CD\n"));
                IoRegisterFileSystem(UDFGlobalData.UDFDeviceObject_CD);
            }
#ifdef UDF_HDD_SUPPORT
            if (UDFGlobalData.UDFDeviceObject_HDD) {
                UDFPrint(("UDFCreateFsDeviceObject: IoRegisterFileSystem() for HDD\n"));
                IoRegisterFileSystem(UDFGlobalData.UDFDeviceObject_HDD);
            }
#endif // UDF_HDD_SUPPORT
            FsRegistered = TRUE;

            UDFPrint(("UDF: IoRegisterFsRegistrationChange()\n"));
            IoRegisterFsRegistrationChange( DriverObject, UDFFsNotification );

//            delay.QuadPart = -10000000;
//            KeDelayExecutionThread(KernelMode, FALSE, &delay);        //10 microseconds

           delay.QuadPart = -10000000; // 1 sec
           KeDelayExecutionThread(KernelMode, FALSE, &delay);

#if 0
            if(!WinVer_IsNT) {
                /*ExInitializeWorkItem(&RemountWorkQueueItem, UDFRemountAll, NULL);
                UDFPrint(("UDFDriverEntry: create remount thread\n"));
                ExQueueWorkItem(&RemountWorkQueueItem, DelayedWorkQueue);*/

                for(CdRomNumber = 0;true;CdRomNumber++) {
                    sprintf(deviceNameBuffer, "\\Device\\CdRom%d", CdRomNumber);
                    UDFPrint(( "UDF: DriverEntry : dismount %s\n", deviceNameBuffer));
                    RtlInitString(&deviceName, deviceNameBuffer);
                    RC = RtlAnsiStringToUnicodeString(&unicodeCdRomDeviceName, &deviceName, TRUE);

                    if (!NT_SUCCESS(RC)) {
                        RtlFreeUnicodeString(&unicodeCdRomDeviceName);
                        break;
                    }

                    RC = UDFDismountDevice(&unicodeCdRomDeviceName);
                    RtlFreeUnicodeString(&unicodeCdRomDeviceName);

                    if (!NT_SUCCESS(RC)) break;

                }

                PVOID ModuleBase = NULL;

                // get NTOSKRNL.EXE exports
                ModuleBase = CrNtGetModuleBase("NTOSKRNL.EXE");
                if(ModuleBase) {
                    FsRtlNotifyVolumeEvent = (ptrFsRtlNotifyVolumeEvent)CrNtGetProcAddress(ModuleBase, "FsRtlNotifyVolumeEvent");
                }

            }
#endif
            RC = STATUS_SUCCESS;

        } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
            // we encountered an exception somewhere, eat it up
            UDFPrint(("UDF: exception\n"));
            RC = _SEH2_GetExceptionCode();
        } _SEH2_END;

        InternalMMInitialized = FALSE;

        try_exit:   NOTHING;
    } _SEH2_FINALLY {
        // start unwinding if we were unsuccessful
        if (!NT_SUCCESS(RC)) {
            UDFPrint(("UDF: failed with status %x\n", RC));
            // Now, delete any device objects, etc. we may have created
/*            if (UDFGlobalData.UDFDeviceObject) {
                IoDeleteDevice(UDFGlobalData.UDFDeviceObject);
                UDFGlobalData.UDFDeviceObject = NULL;
            }*/
#ifdef USE_DLD
            if (DLDetectInitialized) DLDFree();
#endif
            if (InternalMMInitialized) {
                MyAllocRelease();
            }
            if (UDFGlobalData.UDFDeviceObject_CD) {
                IoDeleteDevice(UDFGlobalData.UDFDeviceObject_CD);
                UDFGlobalData.UDFDeviceObject_CD = NULL;
            }
#ifdef UDF_HDD_SUPPORT

            if (UDFGlobalData.UDFDeviceObject_HDD) {
                IoDeleteDevice(UDFGlobalData.UDFDeviceObject_HDD);
                UDFGlobalData.UDFDeviceObject_HDD = NULL;
            }
#endif // UDF_HDD_SUPPORT

/*
            if (UDFGlobalData.UDFDeviceObject_OTHER) {
                IoDeleteDevice(UDFGlobalData.UDFDeviceObject_CD);
                UDFGlobalData.UDFDeviceObject_CD = NULL;
            }

            if (UDFGlobalData.UDFDeviceObject_TAPE) {
                IoDeleteDevice(UDFGlobalData.UDFDeviceObject_CD);
                UDFGlobalData.UDFDeviceObject_CD = NULL;
            }
*/
            // free up any memory we might have reserved for zones/lookaside
            //  lists
            if (UDFGlobalData.UDFFlags & UDF_DATA_FLAGS_ZONES_INITIALIZED) {
                UDFDestroyZones();
            }

            // delete the resource we may have initialized
            if (UDFGlobalData.UDFFlags & UDF_DATA_FLAGS_RESOURCE_INITIALIZED) {
                // un-initialize this resource
                UDFDeleteResource(&(UDFGlobalData.GlobalDataResource));
                UDFClearFlag(UDFGlobalData.UDFFlags, UDF_DATA_FLAGS_RESOURCE_INITIALIZED);
            }
//        } else {
        }
    } _SEH2_END;

    return(RC);
} // end DriverEntry()



/*************************************************************************
*
* Function: UDFInitializeFunctionPointers()
*
* Description:
*   Initialize the IRP... function pointer array in the driver object
*   structure. Also initialize the fast-io function ptr array ...
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
VOID
NTAPI
UDFInitializeFunctionPointers(
    PDRIVER_OBJECT      DriverObject       // created by the I/O sub-system
    )
{
    PFAST_IO_DISPATCH    PtrFastIoDispatch = NULL;

    // initialize the function pointers for the IRP major
    //  functions that this FSD is prepared to  handle ...
    //  NT Version 4.0 has 28 possible functions that a
    //  kernel mode driver can handle.
    //  NT Version 3.51 and before has only 22 such functions,
    //  of which 18 are typically interesting to most FSD's.

    //  The only interesting new functions that a FSD might
    //  want to respond to beginning with Version 4.0 are the
    //  IRP_MJ_QUERY_QUOTA and the IRP_MJ_SET_QUOTA requests.

    //  The code below does not handle quota manipulation, neither
    //  does the NT Version 4.0 operating system (or I/O Manager).
    //  However, you should be on the lookout for any such new
    //  functionality that the FSD might have to implement in
    //  the near future.

    DriverObject->MajorFunction[IRP_MJ_CREATE]              = UDFCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]               = UDFClose;
    DriverObject->MajorFunction[IRP_MJ_READ]                = UDFRead;
#ifndef UDF_READ_ONLY_BUILD
    DriverObject->MajorFunction[IRP_MJ_WRITE]               = UDFWrite;
#endif //UDF_READ_ONLY_BUILD

    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]   = UDFFileInfo;
#ifndef UDF_READ_ONLY_BUILD
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]     = UDFFileInfo;
#endif //UDF_READ_ONLY_BUILD

#ifndef UDF_READ_ONLY_BUILD
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]       = UDFFlush;
#endif //UDF_READ_ONLY_BUILD
    // To implement support for querying and modifying volume attributes
    // (volume information query/set operations), enable initialization
    // of the following two function pointers and then implement the supporting
    // functions.
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = UDFQueryVolInfo;
#ifndef UDF_READ_ONLY_BUILD
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION] = UDFSetVolInfo;
#endif //UDF_READ_ONLY_BUILD
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]   = UDFDirControl;
    // To implement support for file system IOCTL calls, enable initialization
    // of the following function pointer and implement appropriate support.
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = UDFFSControl;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]      = UDFDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]            = UDFShutdown;
    // For byte-range lock support, enable initialization of the following
    // function pointer and implement appropriate support.
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL]        = UDFLockControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]             = UDFCleanup;
#ifdef UDF_HANDLE_EAS
    DriverObject->MajorFunction[IRP_MJ_QUERY_EA]            = UDFQuerySetEA;
#ifndef UDF_READ_ONLY_BUILD
    DriverObject->MajorFunction[IRP_MJ_SET_EA]              = UDFQuerySetEA;
#endif //UDF_READ_ONLY_BUILD
#endif //UDF_HANDLE_EAS
    // If the FSD supports security attributes, we should provide appropriate
    // dispatch entry points and initialize the function pointers as given below.

#ifdef UDF_ENABLE_SECURITY
    DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY]   = UDFGetSecurity;
#ifndef UDF_READ_ONLY_BUILD
    DriverObject->MajorFunction[IRP_MJ_SET_SECURITY]     = UDFSetSecurity;
#endif //UDF_READ_ONLY_BUILD
#endif //UDF_ENABLE_SECURITY

//    if(MajorVersion >= 0x05) {
        // w2k and higher
//        DriverObject->MajorFunction[IRP_MJ_PNP]           = UDFPnp;
//    }

    // Now, it is time to initialize the fast-io stuff ...
    PtrFastIoDispatch = DriverObject->FastIoDispatch = &(UDFGlobalData.UDFFastIoDispatch);

    // initialize the global fast-io structure
    //  NOTE: The fast-io structure has undergone a substantial revision
    //  in Windows NT Version 4.0. The structure has been extensively expanded.
    //  Therefore, if the driver needs to work on both V3.51 and V4.0+,
    //  we will have to be able to distinguish between the two versions at compile time.

    RtlZeroMemory(PtrFastIoDispatch, sizeof(FAST_IO_DISPATCH));

    PtrFastIoDispatch->SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
    PtrFastIoDispatch->FastIoCheckIfPossible    = UDFFastIoCheckIfPossible;
    PtrFastIoDispatch->FastIoRead               = FsRtlCopyRead;
#ifndef UDF_READ_ONLY_BUILD
    PtrFastIoDispatch->FastIoWrite              = UDFFastIoCopyWrite /*FsRtlCopyWrite*/;
#endif //UDF_READ_ONLY_BUILD
    PtrFastIoDispatch->FastIoQueryBasicInfo     = UDFFastIoQueryBasicInfo;
    PtrFastIoDispatch->FastIoQueryStandardInfo  = UDFFastIoQueryStdInfo;
    PtrFastIoDispatch->FastIoLock               = UDFFastLock;         // Lock
    PtrFastIoDispatch->FastIoUnlockSingle       = UDFFastUnlockSingle; // UnlockSingle
    PtrFastIoDispatch->FastIoUnlockAll          = UDFFastUnlockAll;    // UnlockAll
    PtrFastIoDispatch->FastIoUnlockAllByKey     =  (unsigned char (__stdcall *)(struct _FILE_OBJECT *,
        PVOID ,unsigned long,struct _IO_STATUS_BLOCK *,struct _DEVICE_OBJECT *))UDFFastUnlockAllByKey;     //  UnlockAllByKey

    PtrFastIoDispatch->AcquireFileForNtCreateSection = UDFFastIoAcqCreateSec;
    PtrFastIoDispatch->ReleaseFileForNtCreateSection = UDFFastIoRelCreateSec;

//    PtrFastIoDispatch->FastIoDeviceControl = UDFFastIoDeviceControl;

    // the remaining are only valid under NT Version 4.0 and later
#if(_WIN32_WINNT >= 0x0400)

    PtrFastIoDispatch->FastIoQueryNetworkOpenInfo = UDFFastIoQueryNetInfo;

    PtrFastIoDispatch->AcquireForModWrite       = UDFFastIoAcqModWrite;
    PtrFastIoDispatch->ReleaseForModWrite       = UDFFastIoRelModWrite;
    PtrFastIoDispatch->AcquireForCcFlush        = UDFFastIoAcqCcFlush;
    PtrFastIoDispatch->ReleaseForCcFlush        = UDFFastIoRelCcFlush;

/*    // MDL functionality

    PtrFastIoDispatch->MdlRead                  = UDFFastIoMdlRead;
    PtrFastIoDispatch->MdlReadComplete          = UDFFastIoMdlReadComplete;
    PtrFastIoDispatch->PrepareMdlWrite          = UDFFastIoPrepareMdlWrite;
    PtrFastIoDispatch->MdlWriteComplete         = UDFFastIoMdlWriteComplete;*/

    //  this FSD does not support compressed read/write functionality,
    //  NTFS does, and if we design a FSD that can provide such functionality,
    //  we should consider initializing the fast io entry points for reading
    //  and/or writing compressed data ...
#endif  // (_WIN32_WINNT >= 0x0400)

    // last but not least, initialize the Cache Manager callback functions
    //  which are used in CcInitializeCacheMap()

    UDFGlobalData.CacheMgrCallBacks.AcquireForLazyWrite  = UDFAcqLazyWrite;
    UDFGlobalData.CacheMgrCallBacks.ReleaseFromLazyWrite = UDFRelLazyWrite;
    UDFGlobalData.CacheMgrCallBacks.AcquireForReadAhead  = UDFAcqReadAhead;
    UDFGlobalData.CacheMgrCallBacks.ReleaseFromReadAhead = UDFRelReadAhead;

    DriverObject->DriverUnload = UDFDriverUnload;

    return;
} // end UDFInitializeFunctionPointers()

NTSTATUS
UDFCreateFsDeviceObject(
    PCWSTR          FsDeviceName,
    PDRIVER_OBJECT  DriverObject,
    DEVICE_TYPE     DeviceType,
    PDEVICE_OBJECT  *DeviceObject
    )
{
    NTSTATUS RC = STATUS_SUCCESS;
    UNICODE_STRING  DriverDeviceName;
    PUDFFS_DEV_EXTENSION FSDevExt;
    RtlInitUnicodeString(&DriverDeviceName, FsDeviceName);
    *DeviceObject = NULL;

    UDFPrint(("UDFCreateFsDeviceObject: create dev\n"));

    if (!NT_SUCCESS(RC = IoCreateDevice(
            DriverObject,                   // our driver object
            sizeof(UDFFS_DEV_EXTENSION),    // don't need an extension for this object
            &DriverDeviceName,              // name - can be used to "open" the driver
                                // see the book for alternate choices
            DeviceType,
            0,                  // no special characteristics
                                // do not want this as an exclusive device, though you might
            FALSE,
            DeviceObject))) {
                // failed to create a device object, leave ...
        return(RC);
    }
    FSDevExt = (PUDFFS_DEV_EXTENSION)((*DeviceObject)->DeviceExtension);
    // Zero it out (typically this has already been done by the I/O
    // Manager but it does not hurt to do it again)!
    RtlZeroMemory(FSDevExt, sizeof(UDFFS_DEV_EXTENSION));

    // Initialize the signature fields
    FSDevExt->NodeIdentifier.NodeType = UDF_NODE_TYPE_UDFFS_DEVOBJ;
    FSDevExt->NodeIdentifier.NodeSize = sizeof(UDFFS_DEV_EXTENSION);
    // register the driver with the I/O Manager, pretend as if this is
    //  a physical disk based FSD (or in order words, this FSD manages
    //  logical volumes residing on physical disk drives)
/*    UDFPrint(("UDFCreateFsDeviceObject: IoRegisterFileSystem()\n"));
    IoRegisterFileSystem(*DeviceObject);*/
    return(RC);
} // end UDFCreateFsDeviceObject()


NTSTATUS
UDFDismountDevice(
    PUNICODE_STRING unicodeCdRomDeviceName
    )
{
    NTSTATUS RC;
    IO_STATUS_BLOCK IoStatus;
    HANDLE NtFileHandle = (HANDLE)-1;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NOTIFY_MEDIA_CHANGE_USER_IN buffer = { 0 };
    PFILE_FS_ATTRIBUTE_INFORMATION Buffer;

    _SEH2_TRY {

        Buffer = (PFILE_FS_ATTRIBUTE_INFORMATION)MyAllocatePool__(NonPagedPool,sizeof(FILE_FS_ATTRIBUTE_INFORMATION)+2*sizeof(UDF_FS_TITLE_DVDRAM));
        if (!Buffer) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);

        InitializeObjectAttributes ( &ObjectAttributes,
                                     unicodeCdRomDeviceName,
                                     OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                     NULL,
                                     NULL );

        UDFPrint(("\n*** UDFDismountDevice: Create\n"));
        RC = ZwCreateFile( &NtFileHandle,
                           GENERIC_READ,
                           &ObjectAttributes,
                           &IoStatus,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ,
                           FILE_OPEN,
                           FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                           NULL,
                           0 );


        if (!NT_SUCCESS(RC)) try_return(RC);

        UDFPrint(("\n*** UDFDismountDevice: QueryVolInfo\n"));
        RC = ZwQueryVolumeInformationFile( NtFileHandle,
                                           &IoStatus,
                                           Buffer,
                                           sizeof(FILE_FS_ATTRIBUTE_INFORMATION)+2*sizeof(UDF_FS_TITLE_DVDRAM),
                                           FileFsAttributeInformation);

#define UDF_CHECK_FS_NAME(name) \
    (Buffer->FileSystemNameLength+sizeof(WCHAR) == sizeof(name) && \
            DbgCompareMemory(&Buffer->FileSystemName[0],name  , sizeof(name)) == sizeof(name))

        if (NT_SUCCESS(RC) &&
           (UDF_CHECK_FS_NAME((PVOID)UDF_FS_TITLE_CDR)    ||
            UDF_CHECK_FS_NAME((PVOID)UDF_FS_TITLE_CDRW)   ||
            UDF_CHECK_FS_NAME((PVOID)UDF_FS_TITLE_DVDR)   ||
            UDF_CHECK_FS_NAME((PVOID)UDF_FS_TITLE_DVDRW)  ||
            UDF_CHECK_FS_NAME((PVOID)UDF_FS_TITLE_DVDpR)  ||
            UDF_CHECK_FS_NAME((PVOID)UDF_FS_TITLE_DVDpRW) ||
            UDF_CHECK_FS_NAME((PVOID)UDF_FS_TITLE_DVDRAM) )) try_return(STATUS_SUCCESS);

        UDFPrint(("\n*** UDFDismountDevice: LockVolume\n"));
        RC = ZwFsControlFile(NtFileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatus,
                             FSCTL_LOCK_VOLUME,
                             NULL,
                             NULL,
                             NULL,
                             NULL);

        if (!NT_SUCCESS(RC)) try_return(RC);

        UDFPrint(("\n*** UDFDismountDevice: DismountVolume\n"));
        RC = ZwFsControlFile(NtFileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatus,
                             FSCTL_DISMOUNT_VOLUME,
                             NULL,
                             NULL,
                             NULL,
                             NULL);

        if (!NT_SUCCESS(RC)) try_return(RC);

        UDFPrint(("\n*** UDFDismountDevice: NotifyMediaChange\n"));
        RC = ZwDeviceIoControlFile(NtFileHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatus,
                                   IOCTL_CDRW_NOTIFY_MEDIA_CHANGE,
                                   &buffer,
                                   sizeof(buffer),
                                   &buffer,
                                   sizeof(buffer));

        if (!NT_SUCCESS(RC)) try_return(RC);


        UDFPrint(("\n*** UDFDismountDevice: UnlockVolume\n"));
        RC = ZwFsControlFile(NtFileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatus,
                             FSCTL_UNLOCK_VOLUME,
                             NULL,
                             NULL,
                             NULL,
                             NULL);

        UDFPrint(("\n*** UDFDismountDevice: Close\n"));
        ZwClose( NtFileHandle );

        NtFileHandle = (HANDLE)-1;

        UDFPrint(("\n*** UDFDismountDevice: Create 2\n"));
        RC = ZwCreateFile( &NtFileHandle,
                           GENERIC_READ,
                           &ObjectAttributes,
                           &IoStatus,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ,
                           FILE_OPEN,
                           FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                           NULL,
                           0 );

try_exit: NOTHING;

    } _SEH2_FINALLY {
        if (Buffer) MyFreePool__(Buffer);
        if (NtFileHandle != (HANDLE)-1) ZwClose( NtFileHandle );
    } _SEH2_END;

    UDFPrint(("\n*** UDFDismountDevice: RC=%x\n",RC));
    return RC;
}


VOID
NTAPI
UDFFsNotification(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN FsActive
    )

/*

Routine Description:

    This routine is invoked whenever a file system has either registered or
    unregistered itself as an active file system.

    For the former case, this routine creates a device object and attaches it
    to the specified file system's device object.  This allows this driver
    to filter all requests to that file system.

    For the latter case, this file system's device object is located,
    detached, and deleted.  This removes this file system as a filter for
    the specified file system.

Arguments:

    DeviceObject - Pointer to the file system's device object.

    FsActive - bolean indicating whether the file system has registered
        (TRUE) or unregistered (FALSE) itself as an active file system.

Return Value:

    None.

*/

{
    // Begin by determine whether or not the file system is a cdrom-based file
    // system.  If not, then this driver is not concerned with it.
    if (!FsRegistered ||
        DeviceObject->DeviceType != FILE_DEVICE_CD_ROM_FILE_SYSTEM) {
        return;
    }

    // Begin by determining whether this file system is registering or
    // unregistering as an active file system.
    if (FsActive
        && UDFGlobalData.UDFDeviceObject_CD  != DeviceObject
#ifdef UDF_HDD_SUPPORT
        && UDFGlobalData.UDFDeviceObject_HDD != DeviceObject
#endif // UDF_HDD_SUPPORT
    ) {
        UDFPrint(("\n*** UDFFSNotification \n\n"));

        // Acquire GlobalDataResource
        UDFAcquireResourceExclusive(&(UDFGlobalData.GlobalDataResource), TRUE);

        if(FsNotification_ThreadId != PsGetCurrentThreadId()) {

            FsNotification_ThreadId = PsGetCurrentThreadId();

            IoUnregisterFileSystem(UDFGlobalData.UDFDeviceObject_CD);
            IoRegisterFileSystem(UDFGlobalData.UDFDeviceObject_CD);

#ifdef UDF_HDD_SUPPORT
            IoUnregisterFileSystem(UDFGlobalData.UDFDeviceObject_HDD);
            IoRegisterFileSystem(UDFGlobalData.UDFDeviceObject_HDD);
#endif // UDF_HDD_SUPPORT

            FsNotification_ThreadId = (HANDLE)(-1);

        } else {
            UDFPrint(("\n*** recursive UDFFSNotification call,\n can't become top-level UDF FSD \n\n"));
        }

        // Release the global resource.
        UDFReleaseResource( &(UDFGlobalData.GlobalDataResource) );


    }
}
/*VOID
UDFRemountAll(
    IN PVOID Context
    )
{
    NTSTATUS RC = STATUS_SUCCESS;
    ULONG           CdRomNumber;
    CCHAR           deviceNameBuffer[MAXIMUM_FILENAME_LENGTH];
    ANSI_STRING     deviceName;
    UNICODE_STRING  unicodeCdRomDeviceName;
    LARGE_INTEGER   delay;

*/
/*    delay.QuadPart = -80*10000000;
    KeDelayExecutionThread(KernelMode, FALSE, &delay);        //10 seconds*/

/*    for(CdRomNumber = 0;true;CdRomNumber++) {
        sprintf(deviceNameBuffer, "\\Device\\CdRom%d", CdRomNumber);
        UDFPrint(( "UDF: UDFRemountAll : dismount %s\n", deviceNameBuffer));
        RtlInitString(&deviceName, deviceNameBuffer);
        RC = RtlAnsiStringToUnicodeString(&unicodeCdRomDeviceName, &deviceName, TRUE);

        if (!NT_SUCCESS(RC)) {
            RtlFreeUnicodeString(&unicodeCdRomDeviceName);
            break;
        }

        RC = UDFDismountDevice(&unicodeCdRomDeviceName);
        RtlFreeUnicodeString(&unicodeCdRomDeviceName);

        if (!NT_SUCCESS(RC)) break;
    }
}*/
