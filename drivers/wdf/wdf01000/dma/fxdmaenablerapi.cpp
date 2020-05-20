/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     DMA enabler api functions
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "wdf.h"


extern "C" {

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDmaEnablerCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    WDF_DMA_ENABLER_CONFIG * Config,
    __in_opt
    WDF_OBJECT_ATTRIBUTES * Attributes,
    __out
    WDFDMAENABLER * DmaEnablerHandle
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
size_t
WDFEXPORT(WdfDmaEnablerGetMaximumLength)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler
    )
{
    WDFNOTIMPLEMENTED();
    return 0;
}

__drv_maxIRQL(DISPATCH_LEVEL)
size_t
WDFEXPORT(WdfDmaEnablerGetMaximumScatterGatherElements)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler
    )
{
    WDFNOTIMPLEMENTED();
    return 0;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfDmaEnablerSetMaximumScatterGatherElements)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    __drv_when(MaximumElements == 0, __drv_reportError(MaximumElements cannot be zero))
    size_t MaximumElements
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
size_t
WDFEXPORT(WdfDmaEnablerGetFragmentLength)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    WDF_DMA_DIRECTION   DmaDirection
    )
{
    WDFNOTIMPLEMENTED();
    return 0;
}

__drv_maxIRQL(DISPATCH_LEVEL)
PDMA_ADAPTER
WDFEXPORT(WdfDmaEnablerWdmGetDmaAdapter)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    WDF_DMA_DIRECTION DmaDirection
    )
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

} // extern "C"
