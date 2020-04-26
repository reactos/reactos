/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    WdfDmaEnabler.h

Abstract:

    WDF DMA Enabler support

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#ifndef _WDFDMAENABLER_H_
#define _WDFDMAENABLER_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)

typedef enum _WDF_DMA_PROFILE {
    WdfDmaProfileInvalid = 0,
    WdfDmaProfilePacket,
    WdfDmaProfileScatterGather,
    WdfDmaProfilePacket64,
    WdfDmaProfileScatterGather64,
    WdfDmaProfileScatterGatherDuplex,
    WdfDmaProfileScatterGather64Duplex,
} WDF_DMA_PROFILE;

typedef enum _WDF_DMA_DIRECTION {
    WdfDmaDirectionReadFromDevice = FALSE,
    WdfDmaDirectionWriteToDevice = TRUE,
} WDF_DMA_DIRECTION;



//
// DMA power event callbacks
//
typedef
__drv_functionClass(EVT_WDF_DMA_ENABLER_FILL)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
EVT_WDF_DMA_ENABLER_FILL(
    __in
    WDFDMAENABLER DmaEnabler
    );

typedef EVT_WDF_DMA_ENABLER_FILL *PFN_WDF_DMA_ENABLER_FILL;

typedef
__drv_functionClass(EVT_WDF_DMA_ENABLER_FLUSH)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
EVT_WDF_DMA_ENABLER_FLUSH(
    __in
    WDFDMAENABLER DmaEnabler
    );

typedef EVT_WDF_DMA_ENABLER_FLUSH *PFN_WDF_DMA_ENABLER_FLUSH;

typedef
__drv_functionClass(EVT_WDF_DMA_ENABLER_ENABLE)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
EVT_WDF_DMA_ENABLER_ENABLE(
    __in
    WDFDMAENABLER DmaEnabler
    );

typedef EVT_WDF_DMA_ENABLER_ENABLE *PFN_WDF_DMA_ENABLER_ENABLE;

typedef
__drv_functionClass(EVT_WDF_DMA_ENABLER_DISABLE)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
EVT_WDF_DMA_ENABLER_DISABLE(
    __in
    WDFDMAENABLER DmaEnabler
    );

typedef EVT_WDF_DMA_ENABLER_DISABLE *PFN_WDF_DMA_ENABLER_DISABLE;

typedef
__drv_functionClass(EVT_WDF_DMA_ENABLER_SELFMANAGED_IO_START)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
EVT_WDF_DMA_ENABLER_SELFMANAGED_IO_START(
    __in
    WDFDMAENABLER DmaEnabler
    );

typedef EVT_WDF_DMA_ENABLER_SELFMANAGED_IO_START *PFN_WDF_DMA_ENABLER_SELFMANAGED_IO_START;

typedef
__drv_functionClass(EVT_WDF_DMA_ENABLER_SELFMANAGED_IO_STOP)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
EVT_WDF_DMA_ENABLER_SELFMANAGED_IO_STOP(
    __in
    WDFDMAENABLER DmaEnabler
    );

typedef EVT_WDF_DMA_ENABLER_SELFMANAGED_IO_STOP *PFN_WDF_DMA_ENABLER_SELFMANAGED_IO_STOP;


#define  WDF_DMA_ENABLER_UNLIMITED_FRAGMENTS ((ULONG) -1)

typedef struct _WDF_DMA_ENABLER_CONFIG {
    //
    // Size of this structure in bytes
    //
    ULONG                Size;

    //
    // One of the above WDF_DMA_PROFILES
    //
    WDF_DMA_PROFILE      Profile;

    //
    // Maximum DMA Transfer handled in bytes.
    //
    size_t               MaximumLength;

    //
    // The various DMA PnP/Power event callbacks
    //
    PFN_WDF_DMA_ENABLER_FILL                  EvtDmaEnablerFill;
    PFN_WDF_DMA_ENABLER_FLUSH                 EvtDmaEnablerFlush;
    PFN_WDF_DMA_ENABLER_DISABLE               EvtDmaEnablerDisable;
    PFN_WDF_DMA_ENABLER_ENABLE                EvtDmaEnablerEnable;
    PFN_WDF_DMA_ENABLER_SELFMANAGED_IO_START  EvtDmaEnablerSelfManagedIoStart;
    PFN_WDF_DMA_ENABLER_SELFMANAGED_IO_STOP   EvtDmaEnablerSelfManagedIoStop;

} WDF_DMA_ENABLER_CONFIG, *PWDF_DMA_ENABLER_CONFIG;

VOID
FORCEINLINE
WDF_DMA_ENABLER_CONFIG_INIT(
    __out PWDF_DMA_ENABLER_CONFIG Config,
    __in  WDF_DMA_PROFILE    Profile,
    __in  size_t             MaximumLength
    )
{
    RtlZeroMemory(Config, sizeof(WDF_DMA_ENABLER_CONFIG));

    Config->Size = sizeof(WDF_DMA_ENABLER_CONFIG);
    Config->Profile = Profile;
    Config->MaximumLength = MaximumLength;
}

//
// WDF Function: WdfDmaEnablerCreate
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFDMAENABLERCREATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_DMA_ENABLER_CONFIG Config,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFDMAENABLER* DmaEnablerHandle
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfDmaEnablerCreate(
    __in
    WDFDEVICE Device,
    __in
    PWDF_DMA_ENABLER_CONFIG Config,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFDMAENABLER* DmaEnablerHandle
    )
{
    return ((PFN_WDFDMAENABLERCREATE) WdfFunctions[WdfDmaEnablerCreateTableIndex])(WdfDriverGlobals, Device, Config, Attributes, DmaEnablerHandle);
}

//
// WDF Function: WdfDmaEnablerGetMaximumLength
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
size_t
(*PFN_WDFDMAENABLERGETMAXIMUMLENGTH)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler
    );

__drv_maxIRQL(DISPATCH_LEVEL)
size_t
FORCEINLINE
WdfDmaEnablerGetMaximumLength(
    __in
    WDFDMAENABLER DmaEnabler
    )
{
    return ((PFN_WDFDMAENABLERGETMAXIMUMLENGTH) WdfFunctions[WdfDmaEnablerGetMaximumLengthTableIndex])(WdfDriverGlobals, DmaEnabler);
}

//
// WDF Function: WdfDmaEnablerGetMaximumScatterGatherElements
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
size_t
(*PFN_WDFDMAENABLERGETMAXIMUMSCATTERGATHERELEMENTS)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler
    );

__drv_maxIRQL(DISPATCH_LEVEL)
size_t
FORCEINLINE
WdfDmaEnablerGetMaximumScatterGatherElements(
    __in
    WDFDMAENABLER DmaEnabler
    )
{
    return ((PFN_WDFDMAENABLERGETMAXIMUMSCATTERGATHERELEMENTS) WdfFunctions[WdfDmaEnablerGetMaximumScatterGatherElementsTableIndex])(WdfDriverGlobals, DmaEnabler);
}

//
// WDF Function: WdfDmaEnablerSetMaximumScatterGatherElements
//
typedef
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
VOID
(*PFN_WDFDMAENABLERSETMAXIMUMSCATTERGATHERELEMENTS)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    __drv_when(MaximumFragments == 0, __drv_reportError(MaximumFragments cannot be zero))
    size_t MaximumFragments
    );

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
FORCEINLINE
WdfDmaEnablerSetMaximumScatterGatherElements(
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    __drv_when(MaximumFragments == 0, __drv_reportError(MaximumFragments cannot be zero))
    size_t MaximumFragments
    )
{
    ((PFN_WDFDMAENABLERSETMAXIMUMSCATTERGATHERELEMENTS) WdfFunctions[WdfDmaEnablerSetMaximumScatterGatherElementsTableIndex])(WdfDriverGlobals, DmaEnabler, MaximumFragments);
}

//
// WDF Function: WdfDmaEnablerGetFragmentLength
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
size_t
(*PFN_WDFDMAENABLERGETFRAGMENTLENGTH)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    WDF_DMA_DIRECTION DmaDirection
    );

__drv_maxIRQL(DISPATCH_LEVEL)
size_t
FORCEINLINE
WdfDmaEnablerGetFragmentLength(
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    WDF_DMA_DIRECTION DmaDirection
    )
{
    return ((PFN_WDFDMAENABLERGETFRAGMENTLENGTH) WdfFunctions[WdfDmaEnablerGetFragmentLengthTableIndex])(WdfDriverGlobals, DmaEnabler, DmaDirection);
}

//
// WDF Function: WdfDmaEnablerWdmGetDmaAdapter
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
PDMA_ADAPTER
(*PFN_WDFDMAENABLERWDMGETDMAADAPTER)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    WDF_DMA_DIRECTION DmaDirection
    );

__drv_maxIRQL(DISPATCH_LEVEL)
PDMA_ADAPTER
FORCEINLINE
WdfDmaEnablerWdmGetDmaAdapter(
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    WDF_DMA_DIRECTION DmaDirection
    )
{
    return ((PFN_WDFDMAENABLERWDMGETDMAADAPTER) WdfFunctions[WdfDmaEnablerWdmGetDmaAdapterTableIndex])(WdfDriverGlobals, DmaEnabler, DmaDirection);
}



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFDMAENABLER_H_

