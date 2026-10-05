/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UCX object handle types
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

WDF_EXTERN_C_START

DECLARE_HANDLE(UCXCONTROLLER);
DECLARE_HANDLE(UCXROOTHUB);
DECLARE_HANDLE(UCXUSBDEVICE);
DECLARE_HANDLE(UCXENDPOINT);
DECLARE_HANDLE(UCXSSTREAMS);

/* Opaque init blocks */
typedef struct _UCXUSBDEVICE_INIT *PUCXUSBDEVICE_INIT;
typedef struct _UCXENDPOINT_INIT *PUCXENDPOINT_INIT;
typedef struct _UCXSSTREAMS_INIT *PUCXSSTREAMS_INIT;

WDF_EXTERN_C_END
