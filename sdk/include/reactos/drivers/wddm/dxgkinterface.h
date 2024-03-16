#pragma once

#include <d3dkmddi.h>

/*
 * Every structure in here is shared across two or modules and doesn't currently match
 * a single windows version/update.
 *
 * These structures DO have varients in windows, I just would like to track what
 * we don't match 1:1 yet. Or haven't bother attempting to do so.
 */

/* REACTOS_WIN32K_DXGKRNL_INTERFACE function Pointers: */

typedef
NTSTATUS
DXGADAPTER_CREATEALLOCATION(_Inout_ PD3DKMT_CREATEALLOCATION unnamedParam1);

typedef DXGADAPTER_CREATEALLOCATION *PDXGADAPTER_CREATEALLOCATION;

typedef
NTSTATUS
DXGADAPTER_CHECKMONITORPOWERSTATE(_In_ PD3DKMT_CHECKMONITORPOWERSTATE unnamedParam1);

typedef DXGADAPTER_CHECKMONITORPOWERSTATE *PDXGADAPTER_CHECKMONITORPOWERSTATE;

typedef
NTSTATUS
DXGADAPTER_CHECKOCCLUSION(_In_ PD3DKMT_CHECKOCCLUSION unnamedParam1);

typedef DXGADAPTER_CHECKOCCLUSION *PDXGADAPTER_CHECKOCCLUSION;

typedef
NTSTATUS
DXGADAPTER_CLOSEADAPTER(_In_ PD3DKMT_CLOSEADAPTER unnamedParam1);

typedef DXGADAPTER_CLOSEADAPTER *PDXGADAPTER_CLOSEADAPTER;

typedef
NTSTATUS
DXGADAPTER_CREATECONTEXT(_Inout_ PD3DKMT_CREATECONTEXT unnamedParam1);

typedef DXGADAPTER_CREATECONTEXT *PDXGADAPTER_CREATECONTEXT;

typedef
NTSTATUS
DXGADAPTER_CREATEDEVICE(_Inout_ PD3DKMT_CREATEDEVICE unnamedParam1);

typedef DXGADAPTER_CREATEDEVICE *PDXGADAPTER_CREATEDEVICE;

typedef
NTSTATUS
DXGADAPTER_CREATEOVERLAY(_Inout_ PD3DKMT_CREATEOVERLAY unnamedParam1);

typedef DXGADAPTER_CREATEOVERLAY *PDXGADAPTER_CREATEOVERLAY;

/*
 * The goal of this structure to be the list of callback that exist between DXGKNRL
 * and Win32k. This private interface is undocumented and changes with every windows update that
 * remotely touches WDDM.
 *
 * Reversing this isn't possible until we can throw our DxgKrnl into vista or above at runtime
 * But this cannot happen without us first supporting watchdog.
 *
 */
typedef struct _REACTOS_WIN32K_DXGKRNL_INTERFACE
{
    PDXGADAPTER_CREATEALLOCATION DxgkIntPfnCreateAllocation;
    PDXGADAPTER_CHECKMONITORPOWERSTATE DxgkIntPfnCheckMonitorPowerState;
    PDXGADAPTER_CHECKOCCLUSION DxgkIntPfnCheckOcclusion;
    PDXGADAPTER_CLOSEADAPTER DxgkIntPfnCloseAdapter;
    PDXGADAPTER_CREATECONTEXT DxgkIntPfnCreateContext;
    PDXGADAPTER_CREATEDEVICE DxgkIntPfnCreateDevice;
    PDXGADAPTER_CREATEOVERLAY DxgkIntPfnCreateOverlay;
} REACTOS_WIN32K_DXGKRNL_INTERFACE, *PREACTOS_WIN32K_DXGKRNL_INTERFACE;