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

typedef enum _D3DKMT_QUEUEDLIMIT_TYPE {
  D3DKMT_SET_QUEUEDLIMIT_PRESENT,
  D3DKMT_GET_QUEUEDLIMIT_PRESENT
} D3DKMT_QUEUEDLIMIT_TYPE;

typedef enum _D3DKMT_SCHEDULINGPRIORITYCLASS {
  D3DKMT_SCHEDULINGPRIORITYCLASS_IDLE,
  D3DKMT_SCHEDULINGPRIORITYCLASS_BELOW_NORMAL,
  D3DKMT_SCHEDULINGPRIORITYCLASS_NORMAL,
  D3DKMT_SCHEDULINGPRIORITYCLASS_ABOVE_NORMAL,
  D3DKMT_SCHEDULINGPRIORITYCLASS_HIGH,
  D3DKMT_SCHEDULINGPRIORITYCLASS_REALTIME
} D3DKMT_SCHEDULINGPRIORITYCLASS;

typedef struct _D3DKMT_RENDERFLAGS {
  UINT ResizeCommandBuffer : 1;
  UINT ResizeAllocationList : 1;
  UINT ResizePatchLocationList : 1;
  UINT NullRendering : 1;
  UINT PresentRedirected : 1;
  UINT RenderKm : 1;
  UINT RenderKmReadback : 1;
  UINT Reserved : 25;
} D3DKMT_RENDERFLAGS;

typedef enum _D3DKMT_QUERYSTATISTICS_TYPE
{
    D3DKMT_QUERYSTATISTICS_ADAPTER                = 0,
    D3DKMT_QUERYSTATISTICS_PROCESS                = 1,
    D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER        = 2,
    D3DKMT_QUERYSTATISTICS_SEGMENT                = 3,
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT        = 4,
    D3DKMT_QUERYSTATISTICS_NODE                   = 5,
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE           = 6,
    D3DKMT_QUERYSTATISTICS_VIDPNSOURCE            = 7,
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE    = 8,
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_GROUP  = 9,
    D3DKMT_QUERYSTATISTICS_PHYSICAL_ADAPTER       = 10,
    D3DKMT_QUERYSTATISTICS_ADAPTER2               = 11,
    D3DKMT_QUERYSTATISTICS_SEGMENT2               = 12,
    D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER2       = 13,
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT2       = 14,
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_GROUP2 = 15,
    D3DKMT_QUERYSTATISTICS_SEGMENT_USAGE          = 16,
    D3DKMT_QUERYSTATISTICS_SEGMENT_GROUP_USAGE    = 17,
    D3DKMT_QUERYSTATISTICS_NODE2                  = 18,
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE2          = 19
} D3DKMT_QUERYSTATISTICS_TYPE;

typedef enum _KMTQUERYADAPTERINFOTYPE {
  KMTQAITYPE_UMDRIVERPRIVATE,
  KMTQAITYPE_UMDRIVERNAME,
  KMTQAITYPE_UMOPENGLINFO,
  KMTQAITYPE_GETSEGMENTSIZE,
  KMTQAITYPE_ADAPTERGUID,
  KMTQAITYPE_FLIPQUEUEINFO,
  KMTQAITYPE_ADAPTERADDRESS,
  KMTQAITYPE_SETWORKINGSETINFO,
  KMTQAITYPE_ADAPTERREGISTRYINFO,
  KMTQAITYPE_CURRENTDISPLAYMODE,
  KMTQAITYPE_MODELIST,
  KMTQAITYPE_CHECKDRIVERUPDATESTATUS,
  KMTQAITYPE_VIRTUALADDRESSINFO,
  KMTQAITYPE_DRIVERVERSION,
  KMTQAITYPE_ADAPTERTYPE,
  KMTQAITYPE_OUTPUTDUPLCONTEXTSCOUNT,
  KMTQAITYPE_WDDM_1_2_CAPS,
  KMTQAITYPE_UMD_DRIVER_VERSION,
  KMTQAITYPE_DIRECTFLIP_SUPPORT,
  KMTQAITYPE_MULTIPLANEOVERLAY_SUPPORT,
  KMTQAITYPE_DLIST_DRIVER_NAME,
  KMTQAITYPE_WDDM_1_3_CAPS,
  KMTQAITYPE_MULTIPLANEOVERLAY_HUD_SUPPORT,
  KMTQAITYPE_WDDM_2_0_CAPS,
  KMTQAITYPE_NODEMETADATA,
  KMTQAITYPE_CPDRIVERNAME,
  KMTQAITYPE_XBOX,
  KMTQAITYPE_INDEPENDENTFLIP_SUPPORT,
  KMTQAITYPE_MIRACASTCOMPANIONDRIVERNAME,
  KMTQAITYPE_PHYSICALADAPTERCOUNT,
  KMTQAITYPE_PHYSICALADAPTERDEVICEIDS,
  KMTQAITYPE_DRIVERCAPS_EXT,
  KMTQAITYPE_QUERY_MIRACAST_DRIVER_TYPE,
  KMTQAITYPE_QUERY_GPUMMU_CAPS,
  KMTQAITYPE_QUERY_MULTIPLANEOVERLAY_DECODE_SUPPORT,
  KMTQAITYPE_QUERY_HW_PROTECTION_TEARDOWN_COUNT,
  KMTQAITYPE_QUERY_ISBADDRIVERFORHWPROTECTIONDISABLED,
  KMTQAITYPE_MULTIPLANEOVERLAY_SECONDARY_SUPPORT,
  KMTQAITYPE_INDEPENDENTFLIP_SECONDARY_SUPPORT,
  KMTQAITYPE_PANELFITTER_SUPPORT,
  KMTQAITYPE_PHYSICALADAPTERPNPKEY,
  KMTQAITYPE_GETSEGMENTGROUPSIZE,
  KMTQAITYPE_MPO3DDI_SUPPORT,
  KMTQAITYPE_HWDRM_SUPPORT,
  KMTQAITYPE_MPOKERNELCAPS_SUPPORT,
  KMTQAITYPE_MULTIPLANEOVERLAY_STRETCH_SUPPORT,
  KMTQAITYPE_GET_DEVICE_VIDPN_OWNERSHIP_INFO,
  KMTQAITYPE_QUERYREGISTRY,
  KMTQAITYPE_KMD_DRIVER_VERSION,
  KMTQAITYPE_BLOCKLIST_KERNEL,
  KMTQAITYPE_BLOCKLIST_RUNTIME,
  KMTQAITYPE_ADAPTERGUID_RENDER,
  KMTQAITYPE_ADAPTERADDRESS_RENDER,
  KMTQAITYPE_ADAPTERREGISTRYINFO_RENDER,
  KMTQAITYPE_CHECKDRIVERUPDATESTATUS_RENDER,
  KMTQAITYPE_DRIVERVERSION_RENDER,
  KMTQAITYPE_ADAPTERTYPE_RENDER,
  KMTQAITYPE_WDDM_1_2_CAPS_RENDER,
  KMTQAITYPE_WDDM_1_3_CAPS_RENDER,
  KMTQAITYPE_QUERY_ADAPTER_UNIQUE_GUID,
  KMTQAITYPE_NODEPERFDATA,
  KMTQAITYPE_ADAPTERPERFDATA,
  KMTQAITYPE_ADAPTERPERFDATA_CAPS,
  KMTQUITYPE_GPUVERSION,
  KMTQAITYPE_DRIVER_DESCRIPTION,
  KMTQAITYPE_DRIVER_DESCRIPTION_RENDER,
  KMTQAITYPE_SCANOUT_CAPS,
  KMTQAITYPE_DISPLAY_UMDRIVERNAME,
  KMTQAITYPE_PARAVIRTUALIZATION_RENDER,
  KMTQAITYPE_SERVICENAME,
  KMTQAITYPE_WDDM_2_7_CAPS,
  KMTQAITYPE_TRACKEDWORKLOAD_SUPPORT,
  KMTQAITYPE_HYBRID_DLIST_DLL_SUPPORT,
  KMTQAITYPE_DISPLAY_CAPS,
  KMTQAITYPE_WDDM_2_9_CAPS,
  KMTQAITYPE_CROSSADAPTERRESOURCE_SUPPORT,
  KMTQAITYPE_WDDM_3_0_CAPS,
  KMTQAITYPE_WSAUMDIMAGENAME,
  KMTQAITYPE_VGPUINTERFACEID,
  KMTQAITYPE_WDDM_3_1_CAPS,
  KMTQAITYPE_HYBRID_DLIST_DLL_MUX_SUPPORT
} KMTQUERYADAPTERINFOTYPE;


typedef enum _D3DKMT_ESCAPETYPE
{
    D3DKMT_ESCAPE_DRIVERPRIVATE                 =  0,
    D3DKMT_ESCAPE_VIDMM                         =  1,
    D3DKMT_ESCAPE_TDRDBGCTRL                    =  2,
    D3DKMT_ESCAPE_VIDSCH                        =  3,
    D3DKMT_ESCAPE_DEVICE                        =  4,
    D3DKMT_ESCAPE_DMM                           =  5,
    D3DKMT_ESCAPE_DEBUG_SNAPSHOT                =  6,
    D3DKMT_ESCAPE_DRT_TEST                      =  8,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    D3DKMT_ESCAPE_DIAGNOSTICS                   =  9,
    D3DKMT_ESCAPE_OUTPUTDUPL_SNAPSHOT           = 10,
    D3DKMT_ESCAPE_OUTPUTDUPL_DIAGNOSTICS        = 11,
    D3DKMT_ESCAPE_BDD_PNP                       = 12,
    D3DKMT_ESCAPE_BDD_FALLBACK                  = 13,
    D3DKMT_ESCAPE_ACTIVATE_SPECIFIC_DIAG        = 14,
    D3DKMT_ESCAPE_MODES_PRUNED_OUT              = 15,
    D3DKMT_ESCAPE_WHQL_INFO                     = 16,
    D3DKMT_ESCAPE_BRIGHTNESS                    = 17,
    D3DKMT_ESCAPE_EDID_CACHE                    = 18,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    D3DKMT_ESCAPE_MIRACAST_DISPLAY_REQUEST      = 20,
    D3DKMT_ESCAPE_HISTORY_BUFFER_STATUS         = 21,
    D3DKMT_ESCAPE_MIRACAST_ADAPTER_DIAG_INFO    = 23,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    D3DKMT_ESCAPE_FORCE_BDDFALLBACK_HEADLESS    = 24,
    D3DKMT_ESCAPE_REQUEST_MACHINE_CRASH         = 25,

    D3DKMT_ESCAPE_SOFTGPU_ENABLE_DISABLE_HMD    = 27,
    D3DKMT_ESCAPE_PROCESS_VERIFIER_OPTION       = 28,
    D3DKMT_ESCAPE_ADAPTER_VERIFIER_OPTION       = 29,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
    D3DKMT_ESCAPE_IDD_REQUEST                   = 30,
    D3DKMT_ESCAPE_DOD_SET_DIRTYRECT_MODE        = 31,
    D3DKMT_ESCAPE_LOG_CODEPOINT_PACKET          = 32,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
    D3DKMT_ESCAPE_LOG_USERMODE_DAIG_PACKET      = 33,
    D3DKMT_ESCAPE_GET_EXTERNAL_DIAGNOSTICS      = 34,
    // unused (35 previously was D3DKMT_ESCAPE_GET_PREFERRED_MODE)
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
    D3DKMT_ESCAPE_GET_DISPLAY_CONFIGURATIONS    = 36,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
    D3DKMT_ESCAPE_QUERY_IOMMU_STATUS            = 37,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)
    D3DKMT_ESCAPE_CCD_DATABASE                  = 38,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)
    D3DKMT_ESCAPE_QUERY_DMA_REMAPPING_STATUS    = 39,
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
    D3DKMT_ESCAPE_WIN32K_START                  = 1024,
    D3DKMT_ESCAPE_WIN32K_HIP_DEVICE_INFO        = 1024,
    D3DKMT_ESCAPE_WIN32K_QUERY_CD_ROTATION_BLOCK = 1025,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    D3DKMT_ESCAPE_WIN32K_DPI_INFO               = 1026,
    D3DKMT_ESCAPE_WIN32K_PRESENTER_VIEW_INFO    = 1027,
    D3DKMT_ESCAPE_WIN32K_SYSTEM_DPI             = 1028,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    D3DKMT_ESCAPE_WIN32K_BDD_FALLBACK           = 1029,
    D3DKMT_ESCAPE_WIN32K_DDA_TEST_CTL           = 1030,
    D3DKMT_ESCAPE_WIN32K_USER_DETECTED_BLACK_SCREEN = 1031,
    D3DKMT_ESCAPE_WIN32K_HMD_ENUM = 1032,
    D3DKMT_ESCAPE_WIN32K_HMD_CONTROL = 1033,
    D3DKMT_ESCAPE_WIN32K_LPMDISPLAY_CONTROL = 1034,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)
    D3DKMT_ESCAPE_WIN32K_DISPBROKER_TEST        = 1035,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)
    D3DKMT_ESCAPE_WIN32K_COLOR_PROFILE_INFO     = 1036,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_7)
    D3DKMT_ESCAPE_WIN32K_SET_DIMMED_STATE       = 1037,
    D3DKMT_ESCAPE_WIN32K_SPECIALIZED_DISPLAY_TEST = 1038,
#endif
#endif
#endif
#endif
#endif
#endif
} D3DKMT_ESCAPETYPE;
typedef enum _D3DKMDT_MODE_PRUNING_REASON
{
    D3DKMDT_MPR_UNINITIALIZED                               = 0,
    D3DKMDT_MPR_ALLCAPS                                     = 1,
    D3DKMDT_MPR_DESCRIPTOR_MONITOR_SOURCE_MODE              = 2,
    D3DKMDT_MPR_DESCRIPTOR_MONITOR_FREQUENCY_RANGE          = 3,
    D3DKMDT_MPR_DESCRIPTOR_OVERRIDE_MONITOR_SOURCE_MODE     = 4,
    D3DKMDT_MPR_DESCRIPTOR_OVERRIDE_MONITOR_FREQUENCY_RANGE = 5,
    D3DKMDT_MPR_DEFAULT_PROFILE_MONITOR_SOURCE_MODE         = 6,
    D3DKMDT_MPR_DRIVER_RECOMMENDED_MONITOR_SOURCE_MODE      = 7,
    D3DKMDT_MPR_MONITOR_FREQUENCY_RANGE_OVERRIDE            = 8,
    D3DKMDT_MPR_CLONE_PATH_PRUNED                           = 9,
    D3DKMDT_MPR_MAXVALID                                    = 10
}
D3DKMDT_MODE_PRUNING_REASON;
typedef enum _D3DKMT_ALLOCATIONRESIDENCYSTATUS
{
    D3DKMT_ALLOCATIONRESIDENCYSTATUS_RESIDENTINGPUMEMORY=1,
    D3DKMT_ALLOCATIONRESIDENCYSTATUS_RESIDENTINSHAREDMEMORY=2,
    D3DKMT_ALLOCATIONRESIDENCYSTATUS_NOTRESIDENT=3,
} D3DKMT_ALLOCATIONRESIDENCYSTATUS;

typedef enum _D3DKMT_VIDPNSOURCEOWNER_TYPE
{
     D3DKMT_VIDPNSOURCEOWNER_UNOWNED        = 0,
     D3DKMT_VIDPNSOURCEOWNER_SHARED         = 1,
     D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE      = 2,
     D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVEGDI   = 3,
     D3DKMT_VIDPNSOURCEOWNER_EMULATED       = 4,
} D3DKMT_VIDPNSOURCEOWNER_TYPE;

typedef struct _D3DKMDT_DISPLAYMODE_FLAGS
{
#if (DXGKDDI_INTERFACE_VERSION < DXGKDDI_INTERFACE_VERSION_WIN8)
    BOOLEAN                      ValidatedAgainstMonitorCaps  : 1;
    BOOLEAN                      RoundedFakeMode              : 1;
    D3DKMDT_MODE_PRUNING_REASON  ModePruningReason            : 4;
    UINT                         Reserved                     : 28;
#else
    UINT                         ValidatedAgainstMonitorCaps  : 1;
    UINT                         RoundedFakeMode              : 1;
    UINT                                                      : 0;
    D3DKMDT_MODE_PRUNING_REASON  ModePruningReason            : 4;
    UINT                         Stereo                       : 1;
    UINT                         AdvancedScanCapable          : 1;
#if (DXGKDDI_INTERFACE_VERSION < DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    UINT                         Reserved                     : 26;
#else
    UINT                         PreferredTiming              : 1;
    UINT                         PhysicalModeSupported        : 1;
#if (DXGKDDI_INTERFACE_VERSION < DXGKDDI_INTERFACE_VERSION_WDDM2_9)
    UINT                         Reserved                     : 24;
#else
    UINT                         VirtualRefreshRate           : 1;
    UINT                         Reserved                     : 23;
#endif
#endif
#endif
}
D3DKMDT_DISPLAYMODE_FLAGS;

typedef enum _D3DKMT_DEVICESTATE_TYPE
{
    D3DKMT_DEVICESTATE_EXECUTION = 1,
    D3DKMT_DEVICESTATE_PRESENT   = 2,
    D3DKMT_DEVICESTATE_RESET     = 3,
    D3DKMT_DEVICESTATE_PRESENT_DWM = 4,

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    D3DKMT_DEVICESTATE_PAGE_FAULT = 5,
#endif
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
    D3DKMT_DEVICESTATE_PRESENT_QUEUE = 6,
#endif
} D3DKMT_DEVICESTATE_TYPE;

typedef enum _D3DKMT_DEVICEEXECUTION_STATE
{
    D3DKMT_DEVICEEXECUTION_ACTIVE               = 1,
    D3DKMT_DEVICEEXECUTION_RESET                = 2,
    D3DKMT_DEVICEEXECUTION_HUNG                 = 3,
    D3DKMT_DEVICEEXECUTION_STOPPED              = 4,
    D3DKMT_DEVICEEXECUTION_ERROR_OUTOFMEMORY    = 5,
    D3DKMT_DEVICEEXECUTION_ERROR_DMAFAULT       = 6,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    D3DKMT_DEVICEEXECUTION_ERROR_DMAPAGEFAULT   = 7,
#endif
} D3DKMT_DEVICEEXECUTION_STATE;

typedef struct _D3DKMT_DEVICEPRESENT_QUEUE_STATE
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId; // in: present source id
    BOOLEAN bQueuedPresentLimitReached;           // out: whether the queued present limit has been reached
} D3DKMT_DEVICEPRESENT_QUEUE_STATE;

typedef struct _D3DKMT_DEVICERESET_STATE
{
    union
    {
        struct
        {
            UINT    DesktopSwitched : 1;
            UINT    Reserved        :31;
        };
        UINT    Value;
    };
} D3DKMT_DEVICERESET_STATE;

typedef struct _D3DKMT_PRESENT_STATS
{
    UINT PresentCount;
    UINT PresentRefreshCount;
    UINT SyncRefreshCount;
    LARGE_INTEGER SyncQPCTime;
    LARGE_INTEGER SyncGPUTime;
} D3DKMT_PRESENT_STATS;

typedef struct _D3DKMT_DEVICEPRESENT_STATE
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    D3DKMT_PRESENT_STATS           PresentStats;
} D3DKMT_DEVICEPRESENT_STATE;

typedef struct _D3DKMT_PRESENT_STATS_DWM
{
    UINT PresentCount;
    UINT PresentRefreshCount;
    LARGE_INTEGER PresentQPCTime;
    UINT SyncRefreshCount;
    LARGE_INTEGER SyncQPCTime;
    UINT CustomPresentDuration;
} D3DKMT_PRESENT_STATS_DWM;

typedef struct _D3DKMT_DEVICEPRESENT_STATE_DWM
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    D3DKMT_PRESENT_STATS_DWM       PresentStatsDWM;
} D3DKMT_DEVICEPRESENT_STATE_DWM;

typedef struct _D3DKMT_MULTISAMPLEMETHOD
{
    UINT    NumSamples;
    UINT    NumQualityLevels;
    UINT    Reserved;
} D3DKMT_MULTISAMPLEMETHOD;


typedef struct _D3DKMT_DISPLAYMODE
{
    UINT                                   Width;
    UINT                                   Height;
    D3DDDIFORMAT                           Format;
    UINT                                   IntegerRefreshRate;
    D3DDDI_RATIONAL                        RefreshRate;
    D3DDDI_VIDEO_SIGNAL_SCANLINE_ORDERING  ScanLineOrdering;
    D3DDDI_ROTATION                        DisplayOrientation;
    UINT                                   DisplayFixedOutput;
    D3DKMDT_DISPLAYMODE_FLAGS              Flags;
} D3DKMT_DISPLAYMODE;

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

typedef struct _D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP {
    D3DKMT_HANDLE                  hAdapter;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
} D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP, *PD3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP;

typedef struct _D3DKMT_CREATESYNCHRONIZATIONOBJECT {
    D3DKMT_HANDLE                    hDevice;
    D3DDDI_SYNCHRONIZATIONOBJECTINFO Info;
    D3DKMT_HANDLE                    hSyncObject;
} D3DKMT_CREATESYNCHRONIZATIONOBJECT, *PD3DKMT_CREATESYNCHRONIZATIONOBJECT;

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

typedef struct _D3DKMT_WAITFORVERTICALBLANKEVENT {
    D3DKMT_HANDLE                  hAdapter;
    D3DKMT_HANDLE                  hDevice;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
} D3DKMT_WAITFORVERTICALBLANKEVENT, *PD3DKMT_WAITFORVERTICALBLANKEVENT;

typedef struct _D3DKMT_WAITFORSYNCHRONIZATIONOBJECT
{
    D3DKMT_HANDLE             hContext;                   // in: Identifies the context that needs to wait.
    UINT                      ObjectCount;                // in: Specifies the number of object to wait on.
    D3DKMT_HANDLE             ObjectHandleArray[D3DDDI_MAX_OBJECT_WAITED_ON]; // in: Specifies the object to wait on.
} D3DKMT_WAITFORSYNCHRONIZATIONOBJECT, *PD3DKMT_WAITFORSYNCHRONIZATIONOBJECT;

typedef struct _D3DKMT_WAITFORIDLE {
    D3DKMT_HANDLE hDevice;
} D3DKMT_WAITFORIDLE, *PD3DKMT_WAITFORIDLE;

typedef struct _D3DKMT_UPDATEOVERLAY {
    D3DKMT_HANDLE            hDevice;
    D3DKMT_HANDLE            hOverlay;
    D3DDDI_KERNELOVERLAYINFO OverlayInfo;
} D3DKMT_UPDATEOVERLAY, *PD3DKMT_UPDATEOVERLAY;

typedef struct _D3DKMT_UNLOCK {
    D3DKMT_HANDLE hDevice;
    UINT          NumAllocations;
    D3DKMT_PTR(CONST D3DKMT_HANDLE*, phAllocations);
} D3DKMT_UNLOCK, *PD3DKMT_UNLOCK;

typedef struct _D3DKMT_SIGNALSYNCHRONIZATIONOBJECT {
    D3DKMT_HANDLE        hContext;
    UINT                 ObjectCount;
    D3DKMT_HANDLE        ObjectHandleArray[D3DDDI_MAX_OBJECT_SIGNALED];
    D3DDDICB_SIGNALFLAGS Flags;
} D3DKMT_SIGNALSYNCHRONIZATIONOBJECT, *PD3DKMT_SIGNALSYNCHRONIZATIONOBJECT;

typedef struct _D3DKMT_SHAREDPRIMARYUNLOCKNOTIFICATION {
    LUID                           AdapterLuid;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
} D3DKMT_SHAREDPRIMARYUNLOCKNOTIFICATION, *PD3DKMT_SHAREDPRIMARYUNLOCKNOTIFICATION;

typedef struct _D3DKMT_SHAREDPRIMARYLOCKNOTIFICATION {
    LUID                           AdapterLuid;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    RECTL                          LockRect;
} D3DKMT_SHAREDPRIMARYLOCKNOTIFICATION, *PD3DKMT_SHAREDPRIMARYLOCKNOTIFICATION;

typedef struct _D3DKMT_SETVIDPNSOURCEOWNER {
    D3DKMT_HANDLE hDevice;
    D3DKMT_PTR(CONST D3DKMT_VIDPNSOURCEOWNER_TYPE*, pType);
    D3DKMT_PTR(CONST D3DDDI_VIDEO_PRESENT_SOURCE_ID*, pVidPnSourceId);
    UINT          VidPnSourceCount;
} D3DKMT_SETVIDPNSOURCEOWNER, *PD3DKMT_SETVIDPNSOURCEOWNER;

typedef struct _D3DKMT_SETQUEUEDLIMIT {
     D3DKMT_HANDLE           hDevice;
     D3DKMT_QUEUEDLIMIT_TYPE Type;
    union {
      UINT QueuedPresentLimit;
      struct {
        D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
        UINT                           QueuedPendingFlipLimit;
      };
    };
} D3DKMT_SETQUEUEDLIMIT, *PD3DKMT_SETQUEUEDLIMIT;

typedef struct _D3DKMT_SETGAMMARAMP {
    D3DKMT_HANDLE                  hDevice;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    D3DDDI_GAMMARAMP_TYPE          Type;
  union {
    D3DDDI_GAMMA_RAMP_RGB256x3x16 *pGammaRampRgb256x3x16;
    D3DDDI_GAMMA_RAMP_DXGI_1      *pGammaRampDXGI1;
  };
    UINT                           Size;
} D3DKMT_SETGAMMARAMP, *PD3DKMT_SETGAMMARAMP;

typedef struct _D3DKMT_SETDISPLAYPRIVATEDRIVERFORMAT {
    D3DKMT_HANDLE                  hDevice;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    UINT                           PrivateDriverFormatAttribute;
} D3DKMT_SETDISPLAYPRIVATEDRIVERFORMAT, *PD3DKMT_SETDISPLAYPRIVATEDRIVERFORMAT;

typedef struct _D3DKMT_SETDISPLAYMODE {
    D3DKMT_HANDLE                         hDevice;
    D3DKMT_HANDLE                         hPrimaryAllocation;
    D3DDDI_VIDEO_SIGNAL_SCANLINE_ORDERING ScanLineOrdering;
    D3DDDI_ROTATION                       DisplayOrientation;
    UINT                                  PrivateDriverFormatAttribute;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7)
    D3DKMT_SETDISPLAYMODE_FLAGS           Flags;
#endif
} D3DKMT_SETDISPLAYMODE, *PD3DKMT_SETDISPLAYMODE;

typedef struct _D3DKMT_SETCONTEXTSCHEDULINGPRIORITY {
    D3DKMT_HANDLE hContext;
    INT           Priority;
} D3DKMT_SETCONTEXTSCHEDULINGPRIORITY, *PD3DKMT_SETCONTEXTSCHEDULINGPRIORITY;

typedef struct _D3DKMT_SETALLOCATIONPRIORITY {
    D3DKMT_HANDLE hDevice;
    D3DKMT_HANDLE hResource;
    D3DKMT_PTR(CONST D3DKMT_HANDLE*, phAllocationList);
    UINT                    AllocationCount;
    D3DKMT_PTR(CONST UINT*, pPriorities);
} D3DKMT_SETALLOCATIONPRIORITY, *PD3DKMT_SETALLOCATIONPRIORITY;

typedef struct _D3DKMT_RENDER {
 union
    {
        D3DKMT_HANDLE               hDevice;
        D3DKMT_HANDLE               hContext;
    };
    UINT                            CommandOffset;
    UINT                            CommandLength;
    UINT                            AllocationCount;
    UINT                            PatchLocationCount;
    D3DKMT_PTR(VOID*,               pNewCommandBuffer);

    UINT                            NewCommandBufferSize;

    D3DKMT_PTR(D3DDDI_ALLOCATIONLIST*, pNewAllocationList);

    UINT                            NewAllocationListSize;

    D3DKMT_PTR(D3DDDI_PATCHLOCATIONLIST*, pNewPatchLocationList);
    UINT                            NewPatchLocationListSize;

    D3DKMT_RENDERFLAGS              Flags;
    ULONGLONG        PresentHistoryToken;
    ULONG                           BroadcastContextCount;

    D3DKMT_HANDLE                   BroadcastContext[D3DDDI_MAX_BROADCAST_CONTEXT];

    ULONG                           QueuedBufferCount;
     D3DGPU_VIRTUAL_ADDRESS NewCommandBuffer;
    D3DKMT_PTR(VOID*,               pPrivateDriverData);
    UINT                            PrivateDriverDataSize;
} D3DKMT_RENDER, *PD3DKMT_RENDER;

typedef struct _D3DKMT_QUERYSTATISTICS {
  D3DKMT_QUERYSTATISTICS_TYPE   Type;
  LUID                          AdapterLuid;
  D3DKMT_PTR(HANDLE,            hProcess);
//TODO: incomplete, will come up with more for this later.
} D3DKMT_QUERYSTATISTICS, *PD3DKMT_QUERYSTATISTICS;

typedef struct _D3DKMT_QUERYRESOURCEINFO {
   D3DKMT_HANDLE      hDevice;
    D3DKMT_PTR(HANDLE, hNtHandle);
    D3DKMT_PTR(VOID*,  pPrivateRuntimeData);
    UINT               PrivateRuntimeDataSize;
    UINT               TotalPrivateDriverDataSize;
    UINT               ResourcePrivateDriverDataSize;
    UINT               NumAllocations;
} D3DKMT_QUERYRESOURCEINFO, *PD3DKMT_QUERYRESOURCEINFO;

typedef struct _D3DKMT_QUERYALLOCATIONRESIDENCY {
    D3DKMT_HANDLE hDevice;
    D3DKMT_HANDLE hResource;
    D3DKMT_PTR(CONST D3DKMT_HANDLE*,    phAllocationList);
    UINT          AllocationCount;
    D3DKMT_PTR(D3DKMT_ALLOCATIONRESIDENCYSTATUS*, pResidencyStatus);
} D3DKMT_QUERYALLOCATIONRESIDENCY, *PD3DKMT_QUERYALLOCATIONRESIDENCY;

typedef struct _D3DKMT_QUERYADAPTERINFO
{
    D3DKMT_HANDLE           hAdapter;
    KMTQUERYADAPTERINFOTYPE Type;
    D3DKMT_PTR(VOID*,       pPrivateDriverData);
    UINT                    PrivateDriverDataSize;
} D3DKMT_QUERYADAPTERINFO, *PD3DKMT_QUERYADAPTERINFO;

typedef struct _D3DKMT_PRESENT
{
    union
    {
        D3DKMT_HANDLE               hDevice;
        D3DKMT_HANDLE               hContext;
    };
    D3DKMT_PTR(HWND,                hWindow);
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
    D3DKMT_HANDLE                   hSource;
    D3DKMT_HANDLE                   hDestination;
    UINT                            Color;
    RECT                            DstRect;
    RECT                            SrcRect;
    UINT                            SubRectCnt;
    D3DKMT_PTR(CONST RECT*,         pSrcSubRects);
    UINT                            PresentCount;
    D3DDDI_FLIPINTERVAL_TYPE        FlipInterval;
    D3DKMT_PRESENTFLAGS             Flags;
    ULONG                           BroadcastContextCount;

    D3DKMT_HANDLE                   BroadcastContext[D3DDDI_MAX_BROADCAST_CONTEXT];
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7)
    HANDLE                          PresentLimitSemaphore;
    D3DKMT_PRESENTHISTORYTOKEN      PresentHistoryToken;
#endif
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    D3DKMT_PRESENT_RGNS*            pPresentRegions;
#endif
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    union
    {
        D3DKMT_HANDLE               hAdapter;
        D3DKMT_HANDLE               hIndirectContext;

    };
    UINT                            Duration;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    D3DKMT_PTR(_Field_size_(BroadcastContextCount)
    D3DKMT_HANDLE*,                 BroadcastSrcAllocation);
    D3DKMT_PTR(_Field_size_opt_(BroadcastContextCount)
    D3DKMT_HANDLE*,                 BroadcastDstAllocation);
    UINT                            PrivateDriverDataSize;
    D3DKMT_PTR(_Field_size_bytes_(PrivateDriverDataSize)
    PVOID,                          pPrivateDriverData);
    BOOLEAN                         bOptimizeForComposition;
#endif
#endif
} D3DKMT_PRESENT, *PD3DKMT_PRESENT;

typedef struct _D3DKMT_POLLDISPLAYCHILDREN {
    D3DKMT_HANDLE hAdapter;
    UINT          NonDestructiveOnly : 1;
    UINT          SynchronousPolling : 1;
    UINT          DisableModeReset : 1;
    UINT          PollAllAdapters : 1;
    UINT          PollInterruptible : 1;
    UINT          Reserved : 27;
} D3DKMT_POLLDISPLAYCHILDREN, *PD3DKMT_POLLDISPLAYCHILDREN;


typedef struct _D3DKMT_OPENRESOURCE
{
    D3DKMT_HANDLE               hDevice;                            // in : Indentifies the device
    D3DKMT_HANDLE               hGlobalShare;                       // in : Shared resource handle
    UINT                        NumAllocations;                     // in : Number of allocations associated with the resource
    union {
        D3DDDI_OPENALLOCATIONINFO*  pOpenAllocationInfo;
#if (DXGKDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7)
        D3DDDI_OPENALLOCATIONINFO2* pOpenAllocationInfo2;
#endif
    };
    D3DKMT_PTR(VOID*, pPrivateRuntimeData);
    UINT              PrivateRuntimeDataSize;
    D3DKMT_PTR(VOID*, pResourcePrivateDriverData);
    UINT              ResourcePrivateDriverDataSize;
    D3DKMT_PTR(VOID*, pTotalPrivateDriverDataBuffer);
    UINT              TotalPrivateDriverDataBufferSize;
    D3DKMT_HANDLE     hResource;
} D3DKMT_OPENRESOURCE, *PD3DKMT_OPENRESOURCE;

typedef struct _D3DKMT_OPENADAPTERFROMHDC {
    D3DKMT_PTR(HDC,                 hdc);
    D3DKMT_HANDLE                  hAdapter;
    LUID                           AdapterLuid;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
} D3DKMT_OPENADAPTERFROMHDC, *PD3DKMT_OPENADAPTERFROMHDC;

typedef struct _D3DKMT_OPENADAPTERFROMDEVICENAME {
    D3DKMT_PTR(PCWSTR,              pDeviceName);
    D3DKMT_HANDLE hAdapter;
    LUID          AdapterLuid;
} D3DKMT_OPENADAPTERFROMDEVICENAME, *PD3DKMT_OPENADAPTERFROMDEVICENAME;

typedef struct _D3DKMT_LOCK {
    D3DKMT_HANDLE          hDevice;
    D3DKMT_HANDLE          hAllocation;
    UINT                   PrivateDriverData;
    UINT                   NumPages;
    D3DKMT_PTR(CONST UINT*, pPages);
    D3DKMT_PTR(VOID*,   pData);
    D3DDDICB_LOCKFLAGS     Flags;
    D3DGPU_VIRTUAL_ADDRESS GpuVirtualAddress;
} D3DKMT_LOCK, *PD3DKMT_LOCK;

typedef struct _D3DKMT_INVALIDATEACTIVEVIDPN {
  D3DKMT_HANDLE hAdapter;
  D3DKMT_PTR(VOID*, pPrivateDriverData);
  UINT          PrivateDriverDataSize;
} D3DKMT_INVALIDATEACTIVEVIDPN, *PD3DKMT_INVALIDATEACTIVEVIDPN;

typedef struct _D3DKMT_GETSHAREDPRIMARYHANDLE {
    D3DKMT_HANDLE                  hAdapter;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    D3DKMT_HANDLE                  hSharedPrimary;
} D3DKMT_GETSHAREDPRIMARYHANDLE, *PD3DKMT_GETSHAREDPRIMARYHANDLE;

typedef struct _D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME {
    WCHAR                          DeviceName[32];
    D3DKMT_HANDLE                  hAdapter;
    LUID                           AdapterLuid;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
} D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME, *PD3DKMT_OPENADAPTERFROMGDIDISPLAYNAME;

typedef struct _D3DKMT_GETSCANLINE {
    D3DKMT_HANDLE                  hAdapter;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    BOOLEAN                        InVerticalBlank;
    UINT                           ScanLine;
} D3DKMT_GETSCANLINE, *PD3DKMT_GETSCANLINE;

typedef struct _D3DKMT_GETRUNTIMEDATA {
    D3DKMT_HANDLE hAdapter;
    D3DKMT_HANDLE hGlobalShare;
    D3DKMT_PTR(VOID*,   pRuntimeData);
    UINT          RuntimeDataSize;
} D3DKMT_GETRUNTIMEDATA, *PD3DKMT_GETRUNTIMEDATA;

typedef struct _D3DKMT_GETPRESENTHISTORY {
    D3DKMT_HANDLE hAdapter;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7)
    UINT          ProvidedSize;
    UINT          WrittenSize;
    D3DKMT_PTR(D3DKMT_PRESENTHISTORYTOKEN*, pTokens);
    UINT          NumTokens;
#endif
} D3DKMT_GETPRESENTHISTORY, *PD3DKMT_GETPRESENTHISTORY;

typedef struct _D3DKMT_GETMULTISAMPLEMETHODLIST
{
    D3DKMT_HANDLE                   hAdapter;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
    UINT                            Width;
    UINT                            Height;
    D3DDDIFORMAT                    Format;
    D3DKMT_PTR(D3DKMT_MULTISAMPLEMETHOD*, pMethodList);
    UINT                            MethodCount;
} D3DKMT_GETMULTISAMPLEMETHODLIST, *PD3DKMT_GETMULTISAMPLEMETHODLIST;

typedef struct _D3DKMT_GETDISPLAYMODELIST
{
    D3DKMT_HANDLE                   hAdapter;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
    D3DKMT_PTR(D3DKMT_DISPLAYMODE*, pModeList);
    UINT                            ModeCount;
} D3DKMT_GETDISPLAYMODELIST, *PD3DKMT_GETDISPLAYMODELIST;

typedef struct _D3DKMT_GETDEVICESTATE
{
    D3DKMT_HANDLE                   hDevice;
    D3DKMT_DEVICESTATE_TYPE         StateType;
    union
    {
        D3DKMT_DEVICEEXECUTION_STATE ExecutionState;
        D3DKMT_DEVICEPRESENT_STATE   PresentState;
        D3DKMT_DEVICERESET_STATE     ResetState;
        D3DKMT_DEVICEPRESENT_STATE_DWM  PresentStateDWM;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
        D3DKMT_DEVICEPAGEFAULT_STATE PageFaultState;
#endif
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
        D3DKMT_DEVICEPRESENT_QUEUE_STATE PresentQueueState;
#endif
    };
} D3DKMT_GETDEVICESTATE, *PD3DKMT_GETDEVICESTATE;

typedef struct _D3DKMT_GETCONTEXTSCHEDULINGPRIORITY {
    D3DKMT_HANDLE hContext;
    INT           Priority;
} D3DKMT_GETCONTEXTSCHEDULINGPRIORITY, *PD3DKMT_GETCONTEXTSCHEDULINGPRIORITY;

typedef struct _D3DKMT_FLIPOVERLAY
{
    D3DKMT_HANDLE        hDevice;
    D3DKMT_HANDLE        hOverlay;
    D3DKMT_HANDLE        hSource;
    D3DKMT_PTR(VOID*,    pPrivateDriverData);
    UINT                 PrivateDriverDataSize;
} D3DKMT_FLIPOVERLAY, *PD3DKMT_FLIPOVERLAY;

typedef struct _D3DKMT_ESCAPE
{
    D3DKMT_HANDLE       hAdapter;
    D3DKMT_HANDLE       hDevice;
    D3DKMT_ESCAPETYPE   Type;
    D3DDDI_ESCAPEFLAGS  Flags;
    D3DKMT_PTR(VOID*,   pPrivateDriverData);
    UINT                PrivateDriverDataSize;
    D3DKMT_HANDLE       hContext;
} D3DKMT_ESCAPE, *PD3DKMT_ESCAPE;

typedef struct _D3DKMT_DESTROYSYNCHRONIZATIONOBJECT
{
    D3DKMT_HANDLE               hSyncObject;
} D3DKMT_DESTROYSYNCHRONIZATIONOBJECT, *PD3DKMT_DESTROYSYNCHRONIZATIONOBJECT;

typedef struct _D3DKMT_DESTROYOVERLAY {
    D3DKMT_HANDLE hDevice;
    D3DKMT_HANDLE hOverlay;
} D3DKMT_DESTROYOVERLAY, *PD3DKMT_DESTROYOVERLAY;

typedef struct _D3DKMT_DESTROYDEVICE {
    D3DKMT_HANDLE hDevice;
} D3DKMT_DESTROYDEVICE, *PD3DKMT_DESTROYDEVICE;

typedef struct _D3DKMT_DESTROYCONTEXT {
    D3DKMT_HANDLE hContext;
} D3DKMT_DESTROYCONTEXT, *PD3DKMT_DESTROYCONTEXT;

typedef struct _D3DKMT_DESTROYALLOCATION
{
    D3DKMT_HANDLE           hDevice;
    D3DKMT_HANDLE           hResource;
    D3DKMT_PTR(CONST D3DKMT_HANDLE*, phAllocationList);
    UINT                    AllocationCount;
} D3DKMT_DESTROYALLOCATION, *PD3DKMT_DESTROYALLOCATION;

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
