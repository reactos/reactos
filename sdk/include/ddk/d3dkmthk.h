/*
 * Copyright 2016 Henri Verbeet for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_D3DKMTHK_H
#define __WINE_D3DKMTHK_H

#include <d3dukmdt.h>

typedef enum _D3DKMT_CLIENTHINT
{
    D3DKMT_CLIENTHINT_UNKNOWN        = 0,
    D3DKMT_CLIENTHINT_OPENGL         = 1,
    D3DKMT_CLIENTHINT_CDD            = 2,
    D3DKMT_CLIENTHINT_OPENCL         = 3,
    D3DKMT_CLIENTHINT_VULKAN         = 4,
    D3DKMT_CLIENTHINT_CUDA           = 5,
    D3DKMT_CLIENTHINT_RESERVED       = 6,
    D3DKMT_CLIENTHINT_DX7            = 7,
    D3DKMT_CLIENTHINT_DX8            = 8,
    D3DKMT_CLIENTHINT_DX9            = 9,
    D3DKMT_CLIENTHINT_DX10           = 10,
    D3DKMT_CLIENTHINT_DX11           = 11,
    D3DKMT_CLIENTHINT_DX12           = 12,
    D3DKMT_CLIENTHINT_9ON12          = 13,
    D3DKMT_CLIENTHINT_11ON12         = 14,
    D3DKMT_CLIENTHINT_MFT_ENCODE     = 15,
    D3DKMT_CLIENTHINT_GLON12         = 16,
    D3DKMT_CLIENTHINT_CLON12         = 17,
    D3DKMT_CLIENTHINT_DML_TENSORFLOW = 18,
    D3DKMT_CLIENTHINT_ONEAPI_LEVEL0  = 19,
    D3DKMT_CLIENTHINT_DML_PYTORCH    = 20,
    D3DKMT_CLIENTHINT_MAX
} D3DKMT_CLIENTHINT;

typedef struct _D3DKMT_CREATEDCFROMMEMORY
{
    void *pMemory;
    D3DDDIFORMAT Format;
    UINT Width;
    UINT Height;
    UINT Pitch;
    HDC hDeviceDc;
    PALETTEENTRY *pColorTable;
    HDC hDc;
    HANDLE hBitmap;
} D3DKMT_CREATEDCFROMMEMORY;

typedef struct _D3DKMT_DESTROYDCFROMMEMORY
{
    HDC hDc;
    HANDLE hBitmap;
} D3DKMT_DESTROYDCFROMMEMORY;

typedef struct _D3DKMT_CHECKMONITORPOWERSTATE
{
    D3DKMT_HANDLE    hAdapter;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
} D3DKMT_CHECKMONITORPOWERSTATE, *PD3DKMT_CHECKMONITORPOWERSTATE;

typedef struct _D3DKMT_CHECKOCCLUSION
{
    D3DKMT_PTR(HWND,            hWindow);
} D3DKMT_CHECKOCCLUSION, *PD3DKMT_CHECKOCCLUSION;

typedef struct _D3DKMT_CLOSEADAPTER
{
    D3DKMT_HANDLE   hAdapter;
} D3DKMT_CLOSEADAPTER, *PD3DKMT_CLOSEADAPTER;

typedef struct _D3DKMT_CREATECONTEXT
{
    D3DKMT_HANDLE               hDevice;
    UINT                        NodeOrdinal;
    UINT                        EngineAffinity;
    D3DDDI_CREATECONTEXTFLAGS   Flags;
    D3DKMT_PTR(VOID*,           pPrivateDriverData);
    UINT                        PrivateDriverDataSize;
    D3DKMT_CLIENTHINT           ClientHint;
    D3DKMT_HANDLE               hContext;
    D3DKMT_PTR(VOID*,           pCommandBuffer);
    UINT                        CommandBufferSize;
    D3DKMT_PTR(D3DDDI_ALLOCATIONLIST*, pAllocationList);
    UINT                        AllocationListSize;
    D3DKMT_PTR(D3DDDI_PATCHLOCATIONLIST*, pPatchLocationList);
    UINT                        PatchLocationListSize;
    D3DGPU_VIRTUAL_ADDRESS      CommandBuffer;
} D3DKMT_CREATECONTEXT, *PD3DKMT_CREATECONTEXT;

typedef struct _D3DKMT_CREATEDEVICEFLAGS
{
    UINT    LegacyMode               :  1;
    UINT    RequestVSync             :  1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    UINT    DisableGpuTimeout        :  1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_1)
    UINT    TestDevice               :  1;
    UINT    Reserved                 : 28;
#else
    UINT    Reserved                 : 29;
#endif
#else
    UINT    Reserved                 : 30;
#endif
} D3DKMT_CREATEDEVICEFLAGS;

typedef struct _D3DKMT_CREATEDEVICE
{
    union
    {
        D3DKMT_HANDLE           hAdapter;
        VOID*                   pAdapter;
        D3DKMT_PTR_HELPER(pAdapter_Align)
    };

    D3DKMT_CREATEDEVICEFLAGS    Flags;

    D3DKMT_HANDLE               hDevice;
    D3DKMT_PTR(VOID*,           pCommandBuffer);
    UINT                        CommandBufferSize;
    D3DKMT_PTR(D3DDDI_ALLOCATIONLIST*, pAllocationList);
    UINT                        AllocationListSize;
    D3DKMT_PTR(D3DDDI_PATCHLOCATIONLIST*, pPatchLocationList);
    UINT                        PatchLocationListSize;
} D3DKMT_CREATEDEVICE, *PD3DKMT_CREATEDEVICE;

typedef struct _D3DKMT_CREATEOVERLAY
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
    D3DKMT_HANDLE                   hDevice;
    D3DDDI_KERNELOVERLAYINFO        OverlayInfo;
    D3DKMT_HANDLE                   hOverlay;
} D3DKMT_CREATEOVERLAY, *PD3DKMT_CREATEOVERLAY;

typedef struct _D3DDDI_ALLOCATIONINFO
{
    D3DKMT_HANDLE                   hAllocation;
    D3DKMT_PTR(CONST VOID*,         pSystemMem);
    D3DKMT_PTR(VOID*,               pPrivateDriverData);
    UINT                            PrivateDriverDataSize;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
    union
    {
        struct
        {
            UINT    Primary         : 1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
            UINT    Stereo          : 1;
            UINT    Reserved        :30;
#else
            UINT    Reserved        :31;
#endif
        };
        UINT        Value;
    } Flags;
} D3DDDI_ALLOCATIONINFO;

typedef struct _D3DDDI_ALLOCATIONINFO2
{
    D3DKMT_HANDLE                   hAllocation;
    union D3DKMT_ALIGN64
    {
        D3DKMT_PTR_HELPER(pSystemMem_hSection_Align)
        HANDLE                      hSection;
        CONST VOID*                 pSystemMem;
    };
    D3DKMT_PTR(VOID*,               pPrivateDriverData);
    UINT                            PrivateDriverDataSize;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
    union
    {
        struct
        {
            UINT    Primary          : 1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
            UINT    Stereo           : 1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
            UINT    OverridePriority : 1;
            UINT    Reserved         : 29;
#else
            UINT    Reserved         : 30;
#endif
#else
            UINT    Reserved          :31;
#endif
        };
        UINT        Value;
    } Flags;
    D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS GpuVirtualAddress;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
    union
    {
        UINT                        Priority;
        D3DKMT_ALIGN64 ULONG_PTR    Unused;
    };
    D3DKMT_ALIGN64 ULONG_PTR        Reserved[5];
#else
    D3DKMT_ALIGN64 ULONG_PTR        Reserved[6];
#endif
} D3DDDI_ALLOCATIONINFO2;
typedef enum _D3DKMT_STANDARDALLOCATIONTYPE
{
    D3DKMT_STANDARDALLOCATIONTYPE_EXISTINGHEAP = 1,
    D3DKMT_STANDARDALLOCATIONTYPE_INTERNALBACKINGSTORE = 2,
    D3DKMT_STANDARDALLOCATIONTYPE_MAX,
} D3DKMT_STANDARDALLOCATIONTYPE;

typedef struct _D3DKMT_STANDARDALLOCATION_EXISTINGHEAP
{
    D3DKMT_ALIGN64 D3DKMT_SIZE_T Size;
} D3DKMT_STANDARDALLOCATION_EXISTINGHEAP;

typedef struct _D3DKMT_CREATESTANDARDALLOCATIONFLAGS
{
    union
    {
        struct
        {
            UINT Reserved : 32;
        };
        UINT Value;
    };
} D3DKMT_CREATESTANDARDALLOCATIONFLAGS;
\

typedef struct _D3DKMT_CREATESTANDARDALLOCATION
{
    D3DKMT_STANDARDALLOCATIONTYPE Type;
    union
    {
        D3DKMT_STANDARDALLOCATION_EXISTINGHEAP ExistingHeapData;
    };
    D3DKMT_CREATESTANDARDALLOCATIONFLAGS Flags;
} D3DKMT_CREATESTANDARDALLOCATION;

typedef struct _D3DKMT_CREATEALLOCATIONFLAGS
{
    UINT    CreateResource              :  1;
    UINT    CreateShared                :  1;
    UINT    NonSecure                   :  1;
    UINT    CreateProtected             :  1;
    UINT    RestrictSharedAccess        :  1;
    UINT    ExistingSysMem              :  1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    UINT    NtSecuritySharing           :  1;
    UINT    ReadOnly                    :  1;
    UINT    CreateWriteCombined         :  1;
    UINT    CreateCached                :  1;
    UINT    SwapChainBackBuffer         :  1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    UINT    CrossAdapter                :  1;
    UINT    OpenCrossAdapter            :  1;
    UINT    PartialSharedCreation       :  1;
    UINT    Zeroed                      :  1;
    UINT    WriteWatch                  :  1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
    UINT    StandardAllocation          :  1;
    UINT    ExistingSection             :  1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)
    UINT    AllowNotZeroed              :  1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_7)
    UINT    PhysicallyContiguous        :  1;
    UINT    NoKmdAccess                 :  1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)
    UINT    SharedDisplayable           :  1;
    UINT    Reserved                    : 10;
#else
    UINT    Reserved                    : 11;
#endif
#else
    UINT    Reserved                    : 13;
#endif
#else
    UINT    Reserved                    : 14;
#endif
#else
    UINT    Reserved                    : 16;
#endif
#else
    UINT    Reserved                    : 21;
#endif
#else
    UINT    Reserved                    : 26;
#endif
} D3DKMT_CREATEALLOCATIONFLAGS;

typedef struct _D3DKMT_CREATEALLOCATION
{
    D3DKMT_HANDLE                   hDevice;
    D3DKMT_HANDLE                   hResource;
    D3DKMT_HANDLE                   hGlobalShare;
    D3DKMT_PTR(_Field_size_bytes_(PrivateRuntimeDataSize)
    CONST VOID*,                    pPrivateRuntimeData);
    UINT                            PrivateRuntimeDataSize;
    union
    {
        D3DKMT_CREATESTANDARDALLOCATION* pStandardAllocation;
        _Field_size_bytes_(PrivateDriverDataSize)
        CONST VOID*                      pPrivateDriverData;
        D3DKMT_PTR_HELPER(               AlignUnionTo64_1)
    };
    UINT                            PrivateDriverDataSize;
    UINT                            NumAllocations;
    union
    {
        _Field_size_(NumAllocations) D3DDDI_ALLOCATIONINFO*   pAllocationInfo;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7)
        _Field_size_(NumAllocations) D3DDDI_ALLOCATIONINFO2*  pAllocationInfo2;
#endif
        D3DKMT_PTR_HELPER(                                          AlignUnionTo64_2)
    };
    D3DKMT_CREATEALLOCATIONFLAGS    Flags;
    D3DKMT_PTR(HANDLE,              hPrivateRuntimeResourceHandle);
} D3DKMT_CREATEALLOCATION, *PD3DKMT_CREATEALLOCATION;


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

DWORD APIENTRY D3DKMTCreateDCFromMemory(_Inout_ D3DKMT_CREATEDCFROMMEMORY*);
DWORD APIENTRY D3DKMTDestroyDCFromMemory(_In_ CONST D3DKMT_DESTROYDCFROMMEMORY*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WINE_D3DKMTHK_H */
