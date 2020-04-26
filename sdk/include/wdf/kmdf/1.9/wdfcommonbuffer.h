/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    WdfCommonBuffer.h

Abstract:

    WDF CommonBuffer support

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#ifndef _WDFCOMMONBUFFER_H_
#define _WDFCOMMONBUFFER_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)



typedef struct _WDF_COMMON_BUFFER_CONFIG {
    //
    // Size of this structure in bytes
    //
    ULONG   Size;

    //
    // Alignment requirement of the buffer address
    //
    ULONG   AlignmentRequirement;

} WDF_COMMON_BUFFER_CONFIG, *PWDF_COMMON_BUFFER_CONFIG;

VOID
FORCEINLINE
WDF_COMMON_BUFFER_CONFIG_INIT(
    __out PWDF_COMMON_BUFFER_CONFIG Config,
    __in  ULONG  AlignmentRequirement
    )
{
    RtlZeroMemory(Config, sizeof(WDF_COMMON_BUFFER_CONFIG));

    Config->Size = sizeof(WDF_COMMON_BUFFER_CONFIG);
    Config->AlignmentRequirement = AlignmentRequirement;
}

//
// WDF Function: WdfCommonBufferCreate
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFCOMMONBUFFERCREATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    __drv_when(Length == 0, __drv_reportError(Length cannot be zero))
    size_t Length,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFCOMMONBUFFER* CommonBuffer
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfCommonBufferCreate(
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    __drv_when(Length == 0, __drv_reportError(Length cannot be zero))
    size_t Length,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFCOMMONBUFFER* CommonBuffer
    )
{
    return ((PFN_WDFCOMMONBUFFERCREATE) WdfFunctions[WdfCommonBufferCreateTableIndex])(WdfDriverGlobals, DmaEnabler, Length, Attributes, CommonBuffer);
}

//
// WDF Function: WdfCommonBufferCreateWithConfig
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFCOMMONBUFFERCREATEWITHCONFIG)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    __drv_when(Length == 0, __drv_reportError(Length cannot be zero))
    size_t Length,
    __in
    PWDF_COMMON_BUFFER_CONFIG Config,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFCOMMONBUFFER* CommonBuffer
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfCommonBufferCreateWithConfig(
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    __drv_when(Length == 0, __drv_reportError(Length cannot be zero))
    size_t Length,
    __in
    PWDF_COMMON_BUFFER_CONFIG Config,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFCOMMONBUFFER* CommonBuffer
    )
{
    return ((PFN_WDFCOMMONBUFFERCREATEWITHCONFIG) WdfFunctions[WdfCommonBufferCreateWithConfigTableIndex])(WdfDriverGlobals, DmaEnabler, Length, Config, Attributes, CommonBuffer);
}

//
// WDF Function: WdfCommonBufferGetAlignedVirtualAddress
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
PVOID
(*PFN_WDFCOMMONBUFFERGETALIGNEDVIRTUALADDRESS)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOMMONBUFFER CommonBuffer
    );

__drv_maxIRQL(DISPATCH_LEVEL)
PVOID
FORCEINLINE
WdfCommonBufferGetAlignedVirtualAddress(
    __in
    WDFCOMMONBUFFER CommonBuffer
    )
{
    return ((PFN_WDFCOMMONBUFFERGETALIGNEDVIRTUALADDRESS) WdfFunctions[WdfCommonBufferGetAlignedVirtualAddressTableIndex])(WdfDriverGlobals, CommonBuffer);
}

//
// WDF Function: WdfCommonBufferGetAlignedLogicalAddress
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
PHYSICAL_ADDRESS
(*PFN_WDFCOMMONBUFFERGETALIGNEDLOGICALADDRESS)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOMMONBUFFER CommonBuffer
    );

__drv_maxIRQL(DISPATCH_LEVEL)
PHYSICAL_ADDRESS
FORCEINLINE
WdfCommonBufferGetAlignedLogicalAddress(
    __in
    WDFCOMMONBUFFER CommonBuffer
    )
{
    return ((PFN_WDFCOMMONBUFFERGETALIGNEDLOGICALADDRESS) WdfFunctions[WdfCommonBufferGetAlignedLogicalAddressTableIndex])(WdfDriverGlobals, CommonBuffer);
}

//
// WDF Function: WdfCommonBufferGetLength
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
size_t
(*PFN_WDFCOMMONBUFFERGETLENGTH)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOMMONBUFFER CommonBuffer
    );

__drv_maxIRQL(DISPATCH_LEVEL)
size_t
FORCEINLINE
WdfCommonBufferGetLength(
    __in
    WDFCOMMONBUFFER CommonBuffer
    )
{
    return ((PFN_WDFCOMMONBUFFERGETLENGTH) WdfFunctions[WdfCommonBufferGetLengthTableIndex])(WdfDriverGlobals, CommonBuffer);
}



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFCOMMONBUFFER_H_

