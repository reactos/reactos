/*++

Copyright (c) Microsoft Corporation

Module Name:

    corepriv.hpp

Abstract:

    This is the main driver framework.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#pragma once

#if ((FX_CORE_MODE)==(FX_CORE_USER_MODE))
#define FX_IS_USER_MODE (TRUE)
#define FX_IS_KERNEL_MODE (FALSE)
#elif ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
#define FX_IS_USER_MODE (FALSE)
#define FX_IS_KERNEL_MODE (TRUE)
#endif

extern "C" {
#include "mx.h"
}

#include "FxMin.hpp"



#include "wdfmemory.h"
#include "wdfrequest.h"
#include "wdfdevice.h"
#include "wdfWmi.h"
#include "wdfChildList.h"
#include "wdfpdo.h"
#include "wdffdo.h"
#include "wdfiotarget.h"
#include "wdfcontrol.h"
#include "wdfcx.h"
#include "wdfio.h"
#include "wdfqueryinterface.h"

#include "FxIrpQueue.hpp"
#include "FxCallback.hpp"
#if (FX_CORE_MODE == FX_CORE_USER_MODE)
#include "FxIrpUm.hpp"
#else if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
#include "FxIrpKm.hpp"
#endif
#include "FxTransactionedList.hpp"

#include "FxCollection.hpp"
#include "FxDeviceInitShared.hpp"
#include "FxDeviceToMxInterface.hpp"
#include "FxRequestContext.hpp"
#include "FxRequestContextTypes.h"
#include "FxRequestBase.hpp"
#include "FxRequestBuffer.hpp"
#include "IfxMemory.hpp"
#include "FxIoTarget.hpp"
#include "FxIoTargetRemote.hpp"
#include "FxIoTargetSelf.hpp"







//
// Versioning of structures for wdfIoTarget.h
//
typedef struct _WDF_IO_TARGET_OPEN_PARAMS_V1_11 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // Indicates which fields of this structure are going to be used in
    // creating the WDFIOTARGET.
    //
    WDF_IO_TARGET_OPEN_TYPE Type;

    //
    // Notification when the target is being queried for removal.
    // If !NT_SUCCESS is returned, the query will fail and the target will
    // remain opened.
    //
    PFN_WDF_IO_TARGET_QUERY_REMOVE EvtIoTargetQueryRemove;

    //
    // The previous query remove has been canceled and the target can now be
    // reopened.
    //
    PFN_WDF_IO_TARGET_REMOVE_CANCELED EvtIoTargetRemoveCanceled;

    //
    // The query remove has succeeded and the target is now removed from the
    // system.
    //
    PFN_WDF_IO_TARGET_REMOVE_COMPLETE EvtIoTargetRemoveComplete;

    // ========== WdfIoTargetOpenUseExistingDevice begin ==========
    //
    // The device object to send requests to
    //
    PDEVICE_OBJECT TargetDeviceObject;

    //
    // File object representing the TargetDeviceObject.  The PFILE_OBJECT will
    // be passed as a parameter in all requests sent to the resulting
    // WDFIOTARGET.
    //
    PFILE_OBJECT TargetFileObject;

    // ========== WdfIoTargetOpenUseExistingDevice end ==========
    //
    // ========== WdfIoTargetOpenByName begin ==========
    //
    // Name of the device to open.
    //
    UNICODE_STRING TargetDeviceName;

    //
    // The access desired on the device being opened up, ie WDM FILE_XXX_ACCESS
    // such as FILE_ANY_ACCESS, FILE_SPECIAL_ACCESS, FILE_READ_ACCESS, or
    // FILE_WRITE_ACCESS or you can use values such as GENERIC_READ,
    // GENERIC_WRITE, or GENERIC_ALL.
    //
    ACCESS_MASK DesiredAccess;

    //
    // Share access desired on the target being opened, ie WDM FILE_SHARE_XXX
    // values such as FILE_SHARE_READ, FILE_SHARE_WRITE, FILE_SHARE_DELETE.
    //
    // A zero value means exclusive access to the target.
    //
    ULONG ShareAccess;

    //
    // File  attributes, see ZwCreateFile in the DDK for a list of valid
    // values and their meaning.
    //
    ULONG FileAttributes;

    //
    // Create disposition, see ZwCreateFile in the DDK for a list of valid
    // values and their meaning.
    //
    ULONG CreateDisposition;

    //
    // Options for opening the device, see CreateOptions for ZwCreateFile in the
    // DDK for a list of valid values and their meaning.
    //
    ULONG CreateOptions;

    PVOID EaBuffer;

    ULONG EaBufferLength;

    PLONGLONG AllocationSize;

    // ========== WdfIoTargetOpenByName end ==========
    //
    //
    // On return for a create by name, this will contain one of the following
    // values:  FILE_CREATED, FILE_OPENED, FILE_OVERWRITTEN, FILE_SUPERSEDED,
    // FILE_EXISTS, FILE_DOES_NOT_EXIST
    //
    ULONG FileInformation;

} WDF_IO_TARGET_OPEN_PARAMS_V1_11, *PWDF_IO_TARGET_OPEN_PARAMS_V1_11;
