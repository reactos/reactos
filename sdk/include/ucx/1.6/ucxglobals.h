/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UCX client globals and function table storage
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

WDF_EXTERN_C_START

typedef struct _UCX_DRIVER_GLOBALS
{
    ULONG Size;
    PWDF_DRIVER_GLOBALS WdfDriverGlobals;
} UCX_DRIVER_GLOBALS, *PUCX_DRIVER_GLOBALS;

typedef UCX_DRIVER_GLOBALS UCX_GLOBALS;
typedef PUCX_DRIVER_GLOBALS PUCX_GLOBALS;

typedef VOID (*PFN_UCXFUNC)(VOID);

/* Owned by the client stub library, filled in by the class bind */
extern PFN_UCXFUNC UcxFunctions[];
extern PUCX_DRIVER_GLOBALS UcxDriverGlobals;

WDF_EXTERN_C_END
