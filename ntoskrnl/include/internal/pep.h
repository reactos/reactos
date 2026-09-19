/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Internal header for the Power Manager Platform Extension Plug-ins (PEPs)
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

//
// Platform Extension Plugin (PEP) handle
//
typedef struct _PEPHANDLE__
{
    LONG unused;
} PEPHANDLE__, *PPEPHANDLE__;

//
// PEP crash dump information
//
typedef struct _PEP_CRASHDUMP_INFORMATION
{
    PPEPHANDLE__ DeviceHandle;
    PVOID DeviceContext;
} PEP_CRASHDUMP_INFORMATION, *PPEP_CRASHDUMP_INFORMATION;

/* EOF */
