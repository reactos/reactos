/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxUm.h

Abstract:

    This file includes standard NT headers and
    user mode versions of mode agnostic headers

    It also contains definitions pulled out from wdm.h

Author:



Revision History:



--*/

#pragma once

#ifndef UMDF_USING_NTSTATUS
#define UMDF_USING_NTSTATUS
#endif

#include <windows.h>
#include <devpropdef.h>
#include <winioctl.h>




#ifdef UMDF_INFRASTRUCTURE
#ifndef WUDF_KERNEL
typedef PVOID PIRP;
typedef PVOID PIO_REMOVE_LOCK;
#endif
#endif

#include "wdmdefs.h"

#define FX_PLUGPLAY_REGKEY_DEVICEMAP 0x8

//
// Define the callback function to be supplied by a user-mode user of MxTimer
// It has the extra parameters Reserved1, Reserved2 and Reserved3 to make it
// look like the KDEFERRED_ROUTINE that used as the callback function for the
// kernel mode version of MxTimer. The user-mode user of MxTimer should not
// use these ReservedX parameters.
//

typedef
VOID
TIMER_CALLBACK_ROUTINE(
    __in     PKDPC Reserved1,
    __in_opt PVOID Context,
    __in_opt PVOID Reserved2,
    __in_opt PVOID Reserved3
    );

typedef PVOID PEX_TIMER;

typedef
VOID
TIMER_CALLBACK_ROUTINE_EX(
    __in     PEX_TIMER Reserved1,
    __in_opt PVOID Context
    );

typedef TIMER_CALLBACK_ROUTINE MdDeferredRoutineType, *MdDeferredRoutine;
typedef TIMER_CALLBACK_ROUTINE_EX MdExtCallbackType, *MdExtCallback;

//
// Forward defines
//
struct IFxMessageDispatch;
struct IUnknown;
struct IWudfIrp;
struct IWudfIoIrp;
struct IWudfFile;
struct IWDFObject;
struct IObjectCleanup;
struct IWudfDeviceStack;
struct IWudfDeviceStack2;
struct IWudfTargetCallbackDeviceChange;
struct IWudfIoDispatcher;
struct IWudfRemoteDispatcher;
struct IWudfDevice;
struct IWudfDevice2;
struct IWudfHost;
struct IWudfHost2;

//
// typedefs
//
typedef IWudfDevice *           MdDeviceObject;
typedef IWudfIrp*               MdIrp;
typedef LPCSTR                  MxFuncName;
typedef PVOID                   MxThread;
typedef PVOID                   MdEThread;
typedef PWUDF_IO_REMOVE_LOCK    MdRemoveLock;
typedef PVOID                   MdInterrupt;

typedef struct _STACK_DEVICE_CAPABILITIES *PSTACK_DEVICE_CAPABILITIES;
typedef UINT64 WUDF_INTERFACE_CONTEXT;
typedef enum _WDF_REQUEST_TYPE WDF_REQUEST_TYPE;
typedef struct _WDF_INTERRUPT_INFO *PWDF_INTERRUPT_INFO;
typedef enum _WDF_INTERRUPT_POLICY WDF_INTERRUPT_POLICY;
typedef enum _WDF_INTERRUPT_PRIORITY WDF_INTERRUPT_PRIORITY;
typedef struct _WDF_OBJECT_ATTRIBUTES *PWDF_OBJECT_ATTRIBUTES;
typedef enum _WDF_DEVICE_IO_BUFFER_RETRIEVAL WDF_DEVICE_IO_BUFFER_RETRIEVAL;
typedef enum RdWmiPowerAction;
typedef struct _WDF_REQUEST_PARAMETERS *PWDF_REQUEST_PARAMETERS;
typedef enum _WDF_EVENT_TYPE WDF_EVENT_TYPE;
typedef enum _WDF_FILE_INFORMATION_CLASS WDF_FILE_INFORMATION_CLASS;
typedef WDF_FILE_INFORMATION_CLASS *PWDF_FILE_INFORMATION_CLASS;

typedef
NTSTATUS
WUDF_IO_COMPLETION_ROUTINE (
     __in MdDeviceObject DeviceObject,
     __in MdIrp Irp,
     __in PVOID Context
    );

typedef WUDF_IO_COMPLETION_ROUTINE *PWUDF_IO_COMPLETION_ROUTINE;

typedef
VOID
WUDF_DRIVER_CANCEL (
     __in MdDeviceObject DeviceObject,
     __in MdIrp Irp
    );

typedef WUDF_DRIVER_CANCEL *PWUDF_DRIVER_CANCEL;
typedef WUDF_IO_COMPLETION_ROUTINE MdCompletionRoutineType, *MdCompletionRoutine;
typedef WUDF_DRIVER_CANCEL MdCancelRoutineType, *MdCancelRoutine;

//
// From wdm.h
//

typedef
__drv_functionClass(REQUEST_POWER_COMPLETE)
__drv_sameIRQL
VOID
REQUEST_POWER_COMPLETE (
    __in MdDeviceObject DeviceObject,
    __in UCHAR MinorFunction,
    __in POWER_STATE PowerState,
    __in_opt PVOID Context,
    __in PIO_STATUS_BLOCK IoStatus
    );

typedef REQUEST_POWER_COMPLETE *PREQUEST_POWER_COMPLETE;
typedef REQUEST_POWER_COMPLETE MdRequestPowerCompleteType, *MdRequestPowerComplete;

typedef enum _WDF_DEVICE_IO_TYPE WDF_DEVICE_IO_TYPE;
typedef struct _DRIVER_OBJECT_UM *PDRIVER_OBJECT_UM;

//
// Driver object's basic interface.
//
typedef
NTSTATUS
DRIVER_ADD_DEVICE_UM (
    _In_  PDRIVER_OBJECT_UM         DriverObject,
    _In_  PVOID                     Context,
    _In_  IWudfDeviceStack *        DevStack,
    _In_  LPCWSTR                   KernelDeviceName,
    _In_opt_ HKEY                   hPdoKey,
    _In_  LPCWSTR                   pwszServiceName,
    _In_  LPCWSTR                   pwszDevInstanceID,
    _In_  ULONG                     ulDriverID
    );

typedef DRIVER_ADD_DEVICE_UM *PFN_DRIVER_ADD_DEVICE_UM;

typedef
VOID
DRIVER_DISPATCH_UM (
    _In_ IWudfDevice *              DeviceObject,
    _In_ IWudfIrp *                 Irp,
    _In_opt_ IUnknown *             Context
    );

typedef DRIVER_DISPATCH_UM *PFN_DRIVER_DISPATCH_UM;

typedef
VOID
DRIVER_UNLOAD_UM (
    _In_ PDRIVER_OBJECT_UM          DriverObject
    );

typedef DRIVER_UNLOAD_UM *PFN_DRIVER_UNLOAD_UM;




#ifdef UMDF_INFRASTRUCTURE
#ifndef WUDF_KERNEL
typedef CCHAR KPROCESSOR_MODE;
typedef PVOID PMDL;
typedef
_IRQL_requires_same_
_Function_class_(ALLOCATE_FUNCTION)
PVOID
ALLOCATE_FUNCTION (
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Tag
    );
typedef ALLOCATE_FUNCTION *PALLOCATE_FUNCTION;
typedef
_IRQL_requires_same_
_Function_class_(FREE_FUNCTION)
VOID
FREE_FUNCTION (
    _In_ __drv_freesMem(Mem) PVOID Buffer
    );
typedef FREE_FUNCTION *PFREE_FUNCTION;
#endif
#endif

//===============================================================================
#include <limits.h>
#include <driverspecs.h>

#include "ErrToStatus.h"

#include "MxDriverObjectUm.h"
#include "MxDeviceObjectUm.h"
#include "MxFileObjectUm.h"
#include "MxGeneralUm.h"
#include "MxLockUm.h"
#include "MxPagedLockUm.h"
#include "MxEventUm.h"
#include "MxMemoryUm.h"
#include "MxTimerUm.h"
#include "MxWorkItemUm.h"
