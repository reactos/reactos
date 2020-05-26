/*
* PROJECT:     ReactOS Kernel
* LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
* PURPOSE:     Kernel Shim Engine types
* COPYRIGHT:   Copyright 2020 Hervé Poussineau (hpoussin@reactos.org)
* COPYRIGHT:   Copyright 2020 Mark Jansen (mark.jansen@reactos.org)
*/


#define KseHookFunction 0
#define KseHookIRPCallback 1
#define KseHookInvalid 2

#define KseHookCallbackDriverInit       1
#define KseHookCallbackDriverStartIo    2
#define KseHookCallbackDriverUnload     3
#define KseHookCallbackAddDevice        4
#define KseHookCallbackMajorFunction    100

typedef struct _KSE_HOOK
{
    ULONG Type;
    union
    {
        PCHAR FunctionName;     // if Type == KseHookFunction
        ULONG CallbackId;       // if Type == KseHookIRPCallback, KseHookCallback..
    };
    PVOID HookFunction;
    PVOID OriginalFunction;     // if Type == KseHookFunction
} KSE_HOOK, *PKSE_HOOK;


#define KseCollectionNtExport 0
#define KseCollectionHalExport 1
#define KseCollectionDriverExport 2
#define KseCollectionCallback 3
#define KseCollectionInvalid 4

typedef struct _KSE_HOOK_COLLECTION
{
    ULONG Type;
    PWCHAR ExportDriverName;    // if Type == KseCollectionDriverExport
    PKSE_HOOK HookArray;
} KSE_HOOK_COLLECTION, *PKSE_HOOK_COLLECTION;


typedef VOID
(NTAPI *PKSE_HOOK_DRIVER_TARGETED)(
    IN PUNICODE_STRING BaseName,
    IN PVOID BaseAddress,
    IN ULONG SizeOfImage,
    IN ULONG TimeDateStamp,
    IN ULONG CheckSum);

typedef VOID
(NTAPI *PKSE_HOOK_DRIVER_UNTARGETED)(
    IN PVOID BaseAddress);


typedef struct _KSE_DRIVER_IO_CALLBACKS
{
    PDRIVER_INITIALIZE DriverInit;
    PDRIVER_STARTIO DriverStartIo;
    PDRIVER_UNLOAD DriverUnload;
    PDRIVER_ADD_DEVICE AddDevice;
    PDRIVER_DISPATCH MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
} KSE_DRIVER_IO_CALLBACKS, *PKSE_DRIVER_IO_CALLBACKS;


typedef PKSE_DRIVER_IO_CALLBACKS
(NTAPI KSE_GET_IO_CALLBACKS)(
    IN PDRIVER_OBJECT DriverObject);
typedef KSE_GET_IO_CALLBACKS *PKSE_GET_IO_CALLBACKS;

typedef NTSTATUS
(NTAPI KSE_SET_COMPLETION_HOOK)(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID Context);
typedef KSE_SET_COMPLETION_HOOK *PKSE_SET_COMPLETION_HOOK;


typedef struct _KSE_CALLBACK_ROUTINES
{
    PKSE_GET_IO_CALLBACKS KseGetIoCallbacksRoutine;
    PKSE_SET_COMPLETION_HOOK KseSetCompletionHookRoutine;
} KSE_CALLBACK_ROUTINES, *PKSE_CALLBACK_ROUTINES;

typedef struct _KSE_SHIM
{
    ULONG Size;
    const GUID* ShimGuid;
    PWCHAR ShimName;
    PKSE_CALLBACK_ROUTINES KseCallbackRoutines;
    PKSE_HOOK_DRIVER_TARGETED ShimmedDriverTargetedNotification;
    PKSE_HOOK_DRIVER_UNTARGETED ShimmedDriverUntargetedNotification;
    PKSE_HOOK_COLLECTION HookCollectionsArray;
} *PKSE_SHIM, KSE_SHIM;


/* Exported functions */
//KseQueryDeviceData
//KseQueryDeviceDataList
//KseQueryDeviceFlags
//KseSetDeviceFlags

NTSTATUS
NTAPI
KseRegisterShim(
    IN PKSE_SHIM Shim,
    IN PVOID Unknown,
    IN ULONG Flags);

NTSTATUS
NTAPI
KseRegisterShimEx(
    IN PKSE_SHIM Shim,
    IN PVOID Unknown,
    IN ULONG Flags,
    IN PVOID DriverObject OPTIONAL);

NTSTATUS
NTAPI
KseUnregisterShim(
    IN PKSE_SHIM Shim,
    IN PVOID Unknown1,
    IN PVOID Unknown2);

/******************************************************* PRIVATE STUFF *****************************************/

NTSTATUS
NTAPI
KseInitialize(
    IN ULONG BootPhase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock);

NTSTATUS
NTAPI
KseShimDriverIoCallbacks(
    IN PDRIVER_OBJECT DriverObject);

NTSTATUS
NTAPI
KseDriverLoadImage(
    IN PLDR_DATA_TABLE_ENTRY LdrEntry);

NTSTATUS
NTAPI
KseVersionLieInitialize(VOID);

NTSTATUS
NTAPI
KseDriverScopeInitialize(VOID);
