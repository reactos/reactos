/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    wdfbugcodes.h

Abstract:

    Lists bugcheck subcode values for the WDF_VIOLATION bugcheck code

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _WDFBUGCODES_H_
#define _WDFBUGCODES_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)

// 
// These values are used in Parameter 1 in the bugcheck data
// 
// NOTE: Do not change these codes, only add to the end.
// The OCA analysis and debugger tools will look at
// these codes to perform fault analysis.
// 
typedef enum _WDF_BUGCHECK_CODES {
    WDF_POWER_ROUTINE_TIMED_OUT = 0x1,
    WDF_RECURSIVE_LOCK = 0x2,
    WDF_VERIFIER_FATAL_ERROR = 0x3,
    WDF_REQUIRED_PARAMETER_IS_NULL = 0x4,
    WDF_INVALID_HANDLE = 0x5,
    WDF_REQUEST_FATAL_ERROR = 0x6,
    WDF_OBJECT_ERROR = 0x7,
    WDF_DMA_FATAL_ERROR = 0x8,
    WDF_INVALID_INTERRUPT = 0x9,
    WDF_QUEUE_FATAL_ERROR = 0xA,
    WDF_INVALID_LOCK_OPERATION = 0xB,
    WDF_PNP_FATAL_ERROR = 0xC,
    WDF_POWER_MULTIPLE_PPO = 0xD,
    WDF_VERIFIER_IRQL_MISMATCH = 0xE,
    WDF_VERIFIER_CRITICAL_REGION_MISMATCH = 0xF,
} WDF_BUGCHECK_CODES;

typedef enum _WDF_REQUEST_FATAL_ERROR_CODES {
    WDF_REQUEST_FATAL_ERROR_NO_MORE_STACK_LOCATIONS = 0x1,
    WDF_REQUEST_FATAL_ERROR_NULL_IRP = 0x2,
    WDF_REQUEST_FATAL_ERROR_REQUEST_ALREADY_SENT = 0x3,
    WDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH = 0x4,
} WDF_REQUEST_FATAL_ERROR_CODES;



typedef struct _WDF_POWER_ROUTINE_TIMED_OUT_DATA {
    //
    // Current power state associated with the timed out device
    //
    WDF_DEVICE_POWER_STATE PowerState;

    //
    // Current power policy state associated with the timed out device
    //
    WDF_DEVICE_POWER_POLICY_STATE PowerPolicyState;

    //
    // The device object for the timed out device
    //
    PDEVICE_OBJECT DeviceObject;

    //
    // The handle for the timed out device
    //
    WDFDEVICE Device;

    //
    // The thread which is stuck
    //
    PKTHREAD TimedOutThread;

} WDF_POWER_ROUTINE_TIMED_OUT_DATA;

typedef struct _WDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH_DATA {
    WDFREQUEST Request;

    PIRP Irp;

    ULONG OutputBufferLength;

    ULONG_PTR Information;

    UCHAR MajorFunction;

}   WDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH_DATA,
  *PWDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH_DATA;

typedef struct _WDF_QUEUE_FATAL_ERROR_DATA {
    WDFQUEUE Queue;

    WDFREQUEST Request;

    NTSTATUS Status;

} WDF_QUEUE_FATAL_ERROR_DATA, *PWDF_QUEUE_FATAL_ERROR_DATA;



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFBUGCODES_H_

