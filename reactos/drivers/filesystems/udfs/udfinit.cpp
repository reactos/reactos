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

#ifdef EVALUATION_TIME_LIMIT

ULONG   PresentDateMask = 0;
BOOLEAN TrialEndOnStart = FALSE;

#define UDF_MD5Init      UDF_MD5Init2
#define UDF_MD5Update    UDF_MD5Update2
#define UDF_MD5Pad       UDF_MD5Pad2
#define UDF_MD5Final     UDF_MD5Final2
#define UDF_MD5End       UDF_MD5End2
#define UDF_MD5Transform UDF_MD5Transform2
#define UDF_Encode       UDF_Encode2
#define UDF_Decode       UDF_Decode2
#define PADDING          PADDING2

#define ROTATE_LEFT      ROTATE_LEFT2
#define FF               FF2
#define GG               GG2
#define HH               HH2
#define II               II2

#define UDF_MD5Transform_dwords   UDF_MD5Transform_dwords2
#define UDF_MD5Transform_idx      UDF_MD5Transform_idx2
#define UDF_MD5Transform_Sxx      UDF_MD5Transform_Sxx2
#define UDF_MD5Rotate             UDF_MD5Rotate2

#include "..\Include\md5.h"
#include "..\Include\md5c.c"

#define UDF_FibonachiNum UDF_FibonachiNum2
#define XPEHb            XPEHb2
#define UDF_build_long_key    UDF_build_long_key2
#define UDF_build_hash_by_key UDF_build_hash_by_key2

#include "..\Include\key_lib.h"
#include "..\Include\key_lib.cpp"

extern ULONG UDFNumberOfKeys;
extern PCHAR pUDFLongKey;
extern PUDF_KEY_LIST pUDFKeyList;
extern PUCHAR pRegKeyName0;

#include "..\Include\protect.h"
#define INCLUDE_XOR_DECL_ONLY
#include "..\Include\protect.h"
#undef INCLUDE_XOR_DECL_ONLY

#endif //EVALUATION_TIME_LIMIT

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
#ifdef EVALUATION_TIME_LIMIT
    WCHAR           RegPath[128];
    WCHAR           RegKeyName[64];
    CHAR            LicenseKey[16+1];
    ULONG           iVer;

    ULONG i, j;
    int checksum[4] = {0,0,0,0};
#else
    LARGE_INTEGER   delay;
#endif //EVALUATION_TIME_LIMIT

//    KdPrint(("UDF: Entered " VER_STR_PRODUCT_NAME " UDF DriverEntry \n"));
//    KdPrint((KD_PREFIX "Build " VER_STR_PRODUCT "\n"));

#ifdef EVALUATION_TIME_LIMIT
    BrutePoint();
#endif //EVALUATION_TIME_LIMIT

    _SEH2_TRY {
        _SEH2_TRY {

#ifdef EVALUATION_TIME_LIMIT
            LARGE_INTEGER cTime;
#endif //EVALUATION_TIME_LIMIT

/*
            CrNtInit(DriverObject, RegistryPath);

            //PsGetVersion(&MajorVersion, &MinorVersion, &BuildNumber, NULL);
            KdPrint(("UDF: OS Version Major: %x, Minor: %x, Build number: %d\n",
                                MajorVersion, MinorVersion, BuildNumber));
*/
#ifdef __REACTOS__
            KdPrint(("UDF Init: OS should be ReactOS\n"));
#endif

            // initialize the global data structure
            RtlZeroMemory(&UDFGlobalData, sizeof(UDFGlobalData));

            // initialize some required fields
            UDFGlobalData.NodeIdentifier.NodeType = UDF_NODE_TYPE_GLOBAL_DATA;
            UDFGlobalData.NodeIdentifier.NodeSize = sizeof(UDFGlobalData);

#ifdef EVALUATION_TIME_LIMIT

            KeQuerySystemTime((PLARGE_INTEGER)&cTime);

            {
                uint64 t, t2;
                t = cTime.QuadPart / 100;
                t /= 60;
                t /= 200*120*24;
                t /= 250;
                t2 = (int32)t;
                KdPrint((KD_PREFIX "t2 = %x\n", t2));
                if(t2-TIME_JAN_1_2003 > UDF_MAX_DATE ||
                   t2-TIME_JAN_1_2003 < UDF_MIN_DATE) {
                    KdPrint((KD_PREFIX "Ssytem time is out of range: %x <= %x <= %x\n",
                            UDF_MIN_DATE, (uint32)t2-TIME_JAN_1_2003, UDF_MAX_DATE));
                    UDFGlobalData.UDFFlags |= UDF_DATA_FLAGS_UNREGISTERED;
                }
            }

            if(UDFGetTrialEnd(&iVer)) {
                TrialEndOnStart = TRUE;
                KdPrint((KD_PREFIX "Eval time expired some time ago\n"));
                UDFGlobalData.UDFFlags = (UDFGlobalData.UDFFlags & ~UDF_DATA_FLAGS_UNREGISTERED) + UDF_DATA_FLAGS_UNREGISTERED;
            }
            cTime.QuadPart /= (10*1000*1000);
            cTime.QuadPart /= (60*60*24);

            KdPrint((KD_PREFIX "cTime = %x, jTime = %x\n", cTime.LowPart, TIME_JAN_1_2003));
            if(cTime.QuadPart < TIME_JAN_1_2003) {
                KdPrint((KD_PREFIX "System time %x < TIME_JAN_1_2003\n", cTime.LowPart));
                UDFGlobalData.UDFFlags |= UDF_DATA_FLAGS_UNREGISTERED;
            }
            cTime.LowPart -= TIME_JAN_1_2003;
            KdPrint(("cTime = %x\n", cTime.LowPart));

            if(!UDFGetInstallVersion((PULONG)&iVer) ||
               !UDFGetInstallTime((PULONG)&cTime.HighPart)) {
                KdPrint((KD_PREFIX "UDFGetInstallTime() or UDFGetInstallVersion() failed\n"));
                UDFGlobalData.UDFFlags |= UDF_DATA_FLAGS_UNREGISTERED;
            } else {
                KdPrint((KD_PREFIX "cTime = %x, iTime = %x\n", cTime.LowPart, cTime.HighPart));
                if(iVer > UDF_CURRENT_BUILD) {
                    KdPrint(("Init: Detected newer build (0)\n"));
                    UDFGlobalData.UDFFlags |= UDF_DATA_FLAGS_UNREGISTERED;
                }
                if((ULONG)cTime.LowPart < (ULONG)cTime.HighPart) {
                    KdPrint(("Eval time expired: current (%x) < install (%x)\n",
                             cTime.LowPart, cTime.HighPart));
                    UDFGlobalData.UDFFlags |= UDF_DATA_FLAGS_UNREGISTERED;
                }
                if((ULONG)cTime.LowPart > (ULONG)cTime.HighPart + EVALUATION_TERM) {
                    KdPrint(("Eval time expired above EVALUATION_TERM\n"));
                    UDFGlobalData.UDFFlags |= UDF_DATA_FLAGS_UNREGISTERED;
                }
                if((cTime.HighPart >> 16) & 0x8000) {
                    KdPrint(("Eval time expired (negative install time)\n"));
                    UDFGlobalData.UDFFlags |= UDF_DATA_FLAGS_UNREGISTERED;
                }
            }
            if(UDFGlobalData.UDFFlags & UDF_DATA_FLAGS_UNREGISTERED) {
                WCHAR s[16];
                ULONG type, sz;
                ULONG d;
                PVOID pdata;

                KdPrint(("Init: unregistered (3). Write BIAKAs to Registry\n"));
                UDFGlobalData.UDFFlags |= UDF_DATA_FLAGS_UNREGISTERED;

                // End of trial
                d = 1 ^ XOR_VAR(TrialEnd, 0);
                swprintf(s, L"0x%8.8x\0", d);
                GET_TRIAL_REG_KEY_NAME(RegPath, 0);
                GET_TRIAL_REG_VAL_NAME(RegKeyName, 0);
                type = GET_XXX_REG_VAL_TYPE(TRIAL, 0) ? REG_SZ : REG_DWORD;
                pdata = GET_XXX_REG_VAL_TYPE(TRIAL, 0) ? (PVOID)s : (PVOID)&d;
                sz = GET_XXX_REG_VAL_TYPE(TRIAL, 0) ? (10+1+1)*sizeof(WCHAR) : sizeof(d);
                KdPrint(("%ws\n  %ws\n", RegPath, RegKeyName));
                RC = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE /*| RTL_REGISTRY_OPTIONAL*/,
                                      RegPath, RegKeyName,
                                      type, pdata, sz );
                KdPrint(("status %#x\n", RC));
                d = 1 ^ XOR_VAR(TrialEnd, 1);
                swprintf(s, L"0x%8.8x\0", d);
                GET_TRIAL_REG_KEY_NAME(RegPath, 1);
                GET_TRIAL_REG_VAL_NAME(RegKeyName, 1);
                type = GET_XXX_REG_VAL_TYPE(TRIAL, 1) ? REG_SZ : REG_DWORD;
                pdata = GET_XXX_REG_VAL_TYPE(TRIAL, 1) ? (PVOID)s : (PVOID)&d;
                sz = GET_XXX_REG_VAL_TYPE(TRIAL, 1) ? (10+1+1)*sizeof(WCHAR) : sizeof(d);
                KdPrint(("%ws\n  %ws\n", RegPath, RegKeyName));
                RC = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE /*| RTL_REGISTRY_OPTIONAL*/,
                                      RegPath, RegKeyName,
                                      type, pdata, sz );
                KdPrint(("status %#x\n", RC));
                // Install Date
                if(!TrialEndOnStart) {
                    d = UDFGlobalData.iTime ^ XOR_VAR(Date, 0);
                    swprintf(s, L"0x%8.8x\0", d);
                    GET_DATE_REG_KEY_NAME(RegPath, 0);
                    GET_DATE_REG_VAL_NAME(RegKeyName, 0);
                    type = GET_XXX_REG_VAL_TYPE(DATE, 0) ? REG_SZ : REG_DWORD;
                    pdata = GET_XXX_REG_VAL_TYPE(DATE, 0) ? (PVOID)s : (PVOID)&d;
                    sz = GET_XXX_REG_VAL_TYPE(DATE, 0) ? (10+1+1)*sizeof(WCHAR) : sizeof(d);
                    if(!(PresentDateMask & (1 << 0))) {
                        KdPrint(("%ws\n  %ws\n", RegPath, RegKeyName));
                        RC = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE /*| RTL_REGISTRY_OPTIONAL*/,
                                              RegPath, RegKeyName,
                                              type, pdata, sz );
                        KdPrint(("status %#x\n", RC));
                    }
                    d = UDFGlobalData.iTime ^ XOR_VAR(Date, 1);
                    swprintf(s, L"0x%8.8x\0", d);
                    GET_DATE_REG_KEY_NAME(RegPath, 1);
                    GET_DATE_REG_VAL_NAME(RegKeyName, 1);
                    type = GET_XXX_REG_VAL_TYPE(DATE, 1) ? REG_SZ : REG_DWORD;
                    pdata = GET_XXX_REG_VAL_TYPE(DATE, 1) ? (PVOID)s : (PVOID)&d;
                    sz = GET_XXX_REG_VAL_TYPE(DATE, 1) ? (10+1+1)*sizeof(WCHAR) : sizeof(d);
                    if(!(PresentDateMask & (1 << 1))) {
                        KdPrint(("%ws\n  %ws\n", RegPath, RegKeyName));
                        RC = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE /*| RTL_REGISTRY_OPTIONAL*/,
                                              RegPath, RegKeyName,
                                              type, pdata, sz );
                        KdPrint(("status %#x\n", RC));
                    }
                }
                // Highest version
                d = UDFGlobalData.iVer ^ XOR_VAR(Version, 0);
                swprintf(s, L"0x%8.8x\0", d);
                GET_VERSION_REG_KEY_NAME(RegPath, 0);
                GET_VERSION_REG_VAL_NAME(RegKeyName, 0);
                type = GET_XXX_REG_VAL_TYPE(VERSION, 0) ? REG_SZ : REG_DWORD;
                pdata = GET_XXX_REG_VAL_TYPE(VERSION, 0) ? (PVOID)s : (PVOID)&d;
                sz = GET_XXX_REG_VAL_TYPE(VERSION, 0) ? (10+1+1)*sizeof(WCHAR) : sizeof(d);
                KdPrint(("%ws\n  %ws\n", RegPath, RegKeyName));
                RC = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE /*| RTL_REGISTRY_OPTIONAL*/,
                                      RegPath, RegKeyName,
                                      type, pdata, sz );
                KdPrint(("status %#x\n", RC));
                d = UDFGlobalData.iVer ^ XOR_VAR(Version, 1);
                swprintf(s, L"0x%8.8x\0", d);
                GET_VERSION_REG_KEY_NAME(RegPath, 1);
                GET_VERSION_REG_VAL_NAME(RegKeyName, 1);
                type = GET_XXX_REG_VAL_TYPE(VERSION, 1) ? REG_SZ : REG_DWORD;
                pdata = GET_XXX_REG_VAL_TYPE(VERSION, 1) ? (PVOID)s : (PVOID)&d;
                sz = GET_XXX_REG_VAL_TYPE(VERSION, 1) ? (10+1+1)*sizeof(WCHAR) : sizeof(d);
                KdPrint(("%ws\n  %ws\n", RegPath, RegKeyName));
                RC = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE /*| RTL_REGISTRY_OPTIONAL*/,
                                      RegPath, RegKeyName,
                                      type, pdata, sz );
                KdPrint(("status %#x\n", RC));
            }
#endif //EVALUATION_TIME_LIMIT

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

            KdPrint(("UDF: Init memory manager\n"));
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

            UDFGlobalData.UnicodeStrRoot.Buffer = L"\\"; //(PWCHAR)&(UDFGlobalData.UnicodeStrRootBuffer);
            UDFGlobalData.UnicodeStrRoot.Length = sizeof(WCHAR);
            UDFGlobalData.UnicodeStrRoot.MaximumLength = 2*sizeof(WCHAR);

            UDFGlobalData.UnicodeStrSDir.Buffer = L":";
            UDFGlobalData.UnicodeStrSDir.Length = sizeof(WCHAR);
            UDFGlobalData.UnicodeStrSDir.MaximumLength = 2*sizeof(WCHAR);

            UDFGlobalData.AclName.Buffer = UDF_SN_NT_ACL;
            UDFGlobalData.AclName.Length =
            (UDFGlobalData.AclName.MaximumLength = sizeof(UDF_SN_NT_ACL)) - sizeof(WCHAR);

            KdPrint(("UDF: Init delayed close queues\n"));
#ifdef UDF_DELAYED_CLOSE
            InitializeListHead( &UDFGlobalData.DelayedCloseQueue );
            InitializeListHead( &UDFGlobalData.DirDelayedCloseQueue );

            ExInitializeWorkItem( &UDFGlobalData.CloseItem,
                                  (PWORKER_THREAD_ROUTINE) UDFDelayedClose,
                                  NULL );

            UDFGlobalData.DelayedCloseCount = 0;
            UDFGlobalData.DirDelayedCloseCount = 0;
#endif //UDF_DELAYED_CLOSE

            // we should have the registry data (if any), allocate zone memory ...
            //  This is an example of when FSD implementations __try to pre-allocate
            //  some fixed amount of memory to avoid internal fragmentation and/or waiting
            //  later during run-time ...

            UDFGlobalData.DefaultZoneSizeInNumStructs=10;

            KdPrint(("UDF: Init zones\n"));
            if (!NT_SUCCESS(RC = UDFInitializeZones()))
                try_return(RC);

            KdPrint(("UDF: Init pointers\n"));
            // initialize the IRP major function table, and the fast I/O table
            UDFInitializeFunctionPointers(DriverObject);

            UDFGlobalData.CPU_Count = KeNumberProcessors;
#ifdef EVALUATION_TIME_LIMIT
            UDFGlobalData.Saved_j = 4;
#endif //EVALUATION_TIME_LIMIT

            // create a device object representing the driver itself
            //  so that requests can be targeted to the driver ...
            //  e.g. for a disk-based FSD, "mount" requests will be sent to
            //        this device object by the I/O Manager.\
            //        For a redirector/server, you may have applications
            //        send "special" IOCTL's using this device object ...

            RtlInitUnicodeString(&DriverDeviceName, UDF_FS_NAME);

            KdPrint(("UDF: Create Driver dev obj\n"));
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

            KdPrint(("UDF: Create CD dev obj\n"));
            if (!NT_SUCCESS(RC = UDFCreateFsDeviceObject(UDF_FS_NAME_CD,
                                    DriverObject,
                                    FILE_DEVICE_CD_ROM_FILE_SYSTEM,
                                    &(UDFGlobalData.UDFDeviceObject_CD)))) {
                // failed to create a device object, leave ...
                try_return(RC);
            }
#ifdef UDF_HDD_SUPPORT
            KdPrint(("UDF: Create HDD dev obj\n"));
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
                KdPrint(("UDFCreateFsDeviceObject: IoRegisterFileSystem() for CD\n"));
                IoRegisterFileSystem(UDFGlobalData.UDFDeviceObject_CD);
            }
#ifdef UDF_HDD_SUPPORT
            if (UDFGlobalData.UDFDeviceObject_HDD) {
                KdPrint(("UDFCreateFsDeviceObject: IoRegisterFileSystem() for HDD\n"));
                IoRegisterFileSystem(UDFGlobalData.UDFDeviceObject_HDD);
            }
#endif // UDF_HDD_SUPPORT
            FsRegistered = TRUE;

            KdPrint(("UDF: IoRegisterFsRegistrationChange()\n"));
            IoRegisterFsRegistrationChange( DriverObject, (PDRIVER_FS_NOTIFICATION)UDFFsNotification );

//            delay.QuadPart = -10000000;
//            KeDelayExecutionThread(KernelMode, FALSE, &delay);        //10 microseconds

#ifdef EVALUATION_TIME_LIMIT
            // Build Registry Value name for License Key 
            for(i=0; i<UDFNumberOfKeys; i++) {
                for(j=0; j<4; j++) {
                    checksum[j] += pUDFKeyList[i].d[j];
                }
            }

            // Read Key
            for(i=0; i<sizeof(UDF_LICENSE_KEY_USER)-1; i++) {
                RegKeyName[i] = pRegKeyName0[(i*sizeof(UDF_LICENSE_KEY_USER))] ^ (UCHAR)(checksum[i%4] ^ (checksum[i%4] >> 16));
            }
            RegKeyName[i] = 0;

//            RtlZeroMemory(UDFGlobalData.LicenseKeyW, sizeof(UDFGlobalData.LicenseKeyW));
            RegTGetStringValue(hUdfRootKey, NULL,
                                              RegKeyName, UDFGlobalData.LicenseKeyW, (16+1)*sizeof(WCHAR));
//                UDFGlobalData.UDFFlags |= UDF_DATA_FLAGS_UNREGISTERED;
//            }
            RegTCloseKeyHandle(hUdfRootKey);

            UDFGlobalData.LicenseKeyW[16] = 0;
            // convert WCHAR Key to CHAR key
            for(i=0; i<16; i++) {
                LicenseKey[i] = (UCHAR)(UDFGlobalData.LicenseKeyW[i]);
            }

            // build hash
            UDF_build_hash_by_key(pUDFLongKey, UDF_LONG_KEY_SIZE, (PCHAR)&(UDFGlobalData.CurrentKeyHash), LicenseKey);
            // check if it is correct
            for(i=0; i<UDFNumberOfKeys; i++) {
                for(j=0; j<4; j++) {
                    if(pUDFKeyList[i].d[j] ^ UDFGlobalData.CurrentKeyHash.d[j]) {
                        break;
                    }
                }
                if(j==4)
                    break;
            }
            if((UDFGlobalData.Saved_j = j) == 4) {
                UDFGlobalData.UDFFlags &= ~UDF_DATA_FLAGS_UNREGISTERED;
            } else {
                if(!(UDFGlobalData.UDFFlags & UDF_DATA_FLAGS_UNREGISTERED)) {
                    UDFGlobalData.Saved_j = 4;
                }
            }
#else //EVALUATION_TIME_LIMIT
           delay.QuadPart = -10000000; // 1 sec
           KeDelayExecutionThread(KernelMode, FALSE, &delay);
#endif //EVALUATION_TIME_LIMIT

#if 0
            if(!WinVer_IsNT) {
                /*ExInitializeWorkItem(&RemountWorkQueueItem, UDFRemountAll, NULL);
                KdPrint(("UDFDriverEntry: create remount thread\n"));
                ExQueueWorkItem(&RemountWorkQueueItem, DelayedWorkQueue);*/

                for(CdRomNumber = 0;true;CdRomNumber++) {
                    sprintf(deviceNameBuffer, "\\Device\\CdRom%d", CdRomNumber);
                    KdPrint(( "UDF: DriverEntry : dismount %s\n", deviceNameBuffer));
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
            KdPrint(("UDF: exception\n"));
            RC = _SEH2_GetExceptionCode();
        } _SEH2_END;

        InternalMMInitialized = FALSE;

        try_exit:   NOTHING;
    } _SEH2_FINALLY {
        // start unwinding if we were unsuccessful
        if (!NT_SUCCESS(RC)) {
            KdPrint(("UDF: failed with status %x\n", RC));
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
#ifndef DEMO
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION] = UDFSetVolInfo;
#endif //DEMO
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
#ifndef DEMO
    DriverObject->MajorFunction[IRP_MJ_SET_SECURITY]     = UDFSetSecurity;
#endif //DEMO
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

    KdPrint(("UDFCreateFsDeviceObject: create dev\n"));

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
/*    KdPrint(("UDFCreateFsDeviceObject: IoRegisterFileSystem()\n"));
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
                                     OBJ_CASE_INSENSITIVE,
                                     NULL,
                                     NULL );

        KdPrint(("\n*** UDFDismountDevice: Create\n"));
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
                  
        KdPrint(("\n*** UDFDismountDevice: QueryVolInfo\n"));
        RC = ZwQueryVolumeInformationFile( NtFileHandle,
                                           &IoStatus,
                                           Buffer,
                                           sizeof(FILE_FS_ATTRIBUTE_INFORMATION)+2*sizeof(UDF_FS_TITLE_DVDRAM),
                                           FileFsAttributeInformation);

#define UDF_CHECK_FS_NAME(name) \
    (Buffer->FileSystemNameLength+sizeof(WCHAR) == sizeof(name) && \
            DbgCompareMemory(&Buffer->FileSystemName[0],name  , sizeof(name)) == sizeof(name))
                    
        if (NT_SUCCESS(RC) &&
           (UDF_CHECK_FS_NAME(UDF_FS_TITLE_CDR)    ||
            UDF_CHECK_FS_NAME(UDF_FS_TITLE_CDRW)   ||
            UDF_CHECK_FS_NAME(UDF_FS_TITLE_DVDR)   ||
            UDF_CHECK_FS_NAME(UDF_FS_TITLE_DVDRW)  ||
            UDF_CHECK_FS_NAME(UDF_FS_TITLE_DVDpR)  ||
            UDF_CHECK_FS_NAME(UDF_FS_TITLE_DVDpRW) ||
            UDF_CHECK_FS_NAME(UDF_FS_TITLE_DVDRAM) )) try_return(STATUS_SUCCESS);
        
        KdPrint(("\n*** UDFDismountDevice: LockVolume\n"));
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

        KdPrint(("\n*** UDFDismountDevice: DismountVolume\n"));
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

        KdPrint(("\n*** UDFDismountDevice: NotifyMediaChange\n"));
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
        
        
        KdPrint(("\n*** UDFDismountDevice: UnlockVolume\n"));
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

        KdPrint(("\n*** UDFDismountDevice: Close\n"));
        ZwClose( NtFileHandle );
        
        NtFileHandle = (HANDLE)-1;

        KdPrint(("\n*** UDFDismountDevice: Create 2\n"));
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

    KdPrint(("\n*** UDFDismountDevice: RC=%x\n",RC));
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
        KdPrint(("\n*** UDFFSNotification \n\n"));

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
            KdPrint(("\n*** recursive UDFFSNotification call,\n can't become top-level UDF FSD \n\n"));
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

/*    delay.QuadPart = -80*10000000;
    KeDelayExecutionThread(KernelMode, FALSE, &delay);        //10 seconds*/
    
/*    for(CdRomNumber = 0;true;CdRomNumber++) {
        sprintf(deviceNameBuffer, "\\Device\\CdRom%d", CdRomNumber);
        KdPrint(( "UDF: UDFRemountAll : dismount %s\n", deviceNameBuffer));
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


#ifdef EVALUATION_TIME_LIMIT

BOOLEAN
UDFGetInstallVersion(
    PULONG iVer
    )
{
    WCHAR RegPath[128];
    WCHAR RegVal[64];
    WCHAR Str[32];
    ULONG i, j;
    ULONG a, v;
    ULONG type;
    ULONG sz;

    KdPrint((KD_PREFIX "UDFGetInstallVersion:\n"));

    (*iVer) = UDF_CURRENT_BUILD;
    for(j=0; j<UDF_INSTALL_INFO_LOCATIONS; j++) {
        RtlZeroMemory(Str, sizeof(Str));
        switch(j) {
        case 0: GET_VERSION_REG_KEY_NAME(RegPath, 0); break;
        case 1: GET_VERSION_REG_KEY_NAME(RegPath, 1); break;
        }
        v = 0;
        switch(j) {
        case 0: GET_VERSION_REG_VAL_NAME(RegVal, 0); break;
        case 1: GET_VERSION_REG_VAL_NAME(RegVal, 1); break;
        }
        type = 0;
        if(j) {
            type = GET_XXX_REG_VAL_TYPE(VERSION, 1);
        } else {
            type = GET_XXX_REG_VAL_TYPE(VERSION, 0);
        }
        if(!type &&
            RegTGetDwordValue(NULL, RegPath, RegVal, &v)) {
            // ok
            KdPrint((KD_PREFIX "val: %x\n", v));
            goto de_xor;
        } else
        if(type &&
           (sz = 30) &&
           RegTGetStringValue(NULL, RegPath, RegVal,
                                         &Str[0], sz) &&
           wcslen(Str) == 10)
        {
            for(i=2; i<10; i++) {
                if(Str[i] >= 'a' && Str[i] <= 'f') {
                    a = 10 + Str[i] - 'a';
                } else {
                    a = Str[i] - '0';
                }
                KdPrint((KD_PREFIX "val: %x\n", a));
                v *= 16;
                v += a;
            }
de_xor:
            switch(j) {
            case 0: v ^= XOR_VAR(Version, 0); break;
            case 1: v ^= XOR_VAR(Version, 1); break;
            }
            if(v & 0x80000000)
                continue;
            if((*iVer) == -1 || (*iVer) < v) {
                (*iVer) = v;
            }
            UDFGlobalData.iVer = (*iVer);
        }
    }
    UDFGlobalData.iVer = (*iVer);
/*    if(UDFGlobalData.iVer == -1)
        return FALSE;*/
    KdPrint((KD_PREFIX "ret val: %x\n", *iVer));
    if((*iVer) > UDF_CURRENT_BUILD) {
        return FALSE;
    }
    return TRUE;
} // end UDFGetInstallVersion()

BOOLEAN
UDFGetInstallTime(
    PULONG iTime
    )
{
    WCHAR RegPath[128];
    WCHAR RegVal[64];
    WCHAR Str[32];
    ULONG i, j;
    ULONG a, v;
    ULONG type;
    ULONG sz;

    KdPrint((KD_PREFIX "UDFGetInstallTime:\n"));

    (*iTime) = -1;
    for(j=0; j<UDF_INSTALL_INFO_LOCATIONS; j++) {
        RtlZeroMemory(Str, sizeof(Str));
        switch(j) {
        case 0: GET_DATE_REG_KEY_NAME(RegPath, 0); break;
        case 1: GET_DATE_REG_KEY_NAME(RegPath, 1); break;
        }
        v = 0;
        switch(j) {
        case 0: GET_DATE_REG_VAL_NAME(RegVal, 0); break;
        case 1: GET_DATE_REG_VAL_NAME(RegVal, 1); break;
        }
        type = 0;
        if(j) {
            type = GET_XXX_REG_VAL_TYPE(DATE, 1);
        } else {
            type = GET_XXX_REG_VAL_TYPE(DATE, 0);
        }
        if(!type &&
           RegTGetDwordValue(NULL, RegPath, RegVal, &v)) {
           // ok
            KdPrint((KD_PREFIX "val: %x\n", v));
            goto de_xor;
        } else
        if(type &&
           (sz = 30) &&
           RegTGetStringValue(NULL, RegPath, RegVal,
                                         &Str[0], sz) &&
           wcslen(Str) == 10)
        {
            for(i=2; i<10; i++) {
                if(Str[i] >= 'a' && Str[i] <= 'f') {
                    a = 10 + Str[i] - 'a';
                } else {
                    a = Str[i] - '0';
                }
                KdPrint((KD_PREFIX "val: %x\n", a));
                v *= 16;
                v += a;
            }
de_xor:
            switch(j) {
            case 0: v ^= XOR_VAR(Date, 0); break;
            case 1: v ^= XOR_VAR(Date, 1); break;
            }
            PresentDateMask |= 0x00000001 << j;
            if(v & 0x80000000)
                continue;
            if((*iTime) == -1 || (*iTime) < v) {
                (*iTime) = v;
            }
            UDFGlobalData.iTime = (*iTime);
        }
    }
    UDFGlobalData.iTime = (*iTime);
    if(UDFGlobalData.iTime == -1) {

        LARGE_INTEGER cTime;
        KeQuerySystemTime((PLARGE_INTEGER)&cTime);

        cTime.QuadPart /= (10*1000*1000);
        cTime.QuadPart /= (60*60*24);

        KdPrint((KD_PREFIX "cTime = %x, jTime = %x\n", cTime.LowPart, TIME_JAN_1_2003));
        if(cTime.QuadPart < TIME_JAN_1_2003) {
            KdPrint((KD_PREFIX "Eval time expired (1)\n"));
            UDFGlobalData.UDFFlags |= UDF_DATA_FLAGS_UNREGISTERED;
        }
        cTime.LowPart -= TIME_JAN_1_2003;
        KdPrint(("cTime = %x\n", cTime.LowPart));

        UDFGlobalData.iTime = (*iTime) = cTime.LowPart;
        return FALSE;
    }
    UDFGlobalData.iTime = (*iTime);
    KdPrint((KD_PREFIX "ret val: %x\n", *iTime));
    return TRUE;
} // end UDFGetInstallTime()

BOOLEAN
UDFGetTrialEnd(
    PULONG iTrial
    )
{
    WCHAR RegPath[128];
    WCHAR RegVal[64];
    WCHAR Str[32];
    ULONG i, j;
    ULONG a, v;
    ULONG type;
    ULONG sz;

    KdPrint((KD_PREFIX "UDFGetTrialEnd:\n"));

    (*iTrial) = 0;
    for(j=0; j<UDF_INSTALL_INFO_LOCATIONS; j++) {
        RtlZeroMemory(Str, sizeof(Str));
        switch(j) {
        case 0: GET_TRIAL_REG_KEY_NAME(RegPath, 0); break;
        case 1: GET_TRIAL_REG_KEY_NAME(RegPath, 1); break;
        }
        v = 0;
        switch(j) {
        case 0: GET_TRIAL_REG_VAL_NAME(RegVal, 0); break;
        case 1: GET_TRIAL_REG_VAL_NAME(RegVal, 1); break;
        }
        type = 0;
        if(j) {
            type = GET_XXX_REG_VAL_TYPE(TRIAL, 1);
        } else {
            type = GET_XXX_REG_VAL_TYPE(TRIAL, 0);
        }
        if(!type &&
           RegTGetDwordValue(NULL, RegPath, RegVal, &v)) {
           // ok
            KdPrint((KD_PREFIX "val: %x\n", v));
            goto de_xor;
        } else
        if(type &&
           (sz = 30) &&
           RegTGetStringValue(NULL, RegPath, RegVal,
                                         &Str[0], sz) &&
           wcslen(Str) == 10)
        {
            for(i=2; i<10; i++) {
                if(Str[i] >= 'a' && Str[i] <= 'f') {
                    a = 10 + Str[i] - 'a';
                } else {
                    a = Str[i] - '0';
                }
                KdPrint((KD_PREFIX "val: %x\n", a));
                v *= 16;
                v += a;
            }
de_xor:
            switch(j) {
            case 0: v ^= XOR_VAR(TrialEnd, 0); break;
            case 1: v ^= XOR_VAR(TrialEnd, 1); break;
            }
            if((*iTrial) < v) {
                (*iTrial) = v;
            }
            if(UDFGlobalData.iTrial = (*iTrial)) {
                return TRUE;
            }
        }
    }
    return FALSE;
} // end UDFGetTrialEnd()

#endif //EVALUATION_TRIAL_LIMIT



