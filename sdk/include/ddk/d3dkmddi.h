/*
 * PROJECT:     ReactOS Display Driver Model
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Header file for WDDM style DDIs
 * COPYRIGHT:   Copyright 2024 Justin Miller <justin.miller@reactos.org>
 */

#ifndef _D3DKMDDI_H_
#define _D3DKMDDI_H_

#include <d3dkmdt.h>
#include <windef.h>

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
typedef enum _DXGK_PRESENT_DISPLAY_ONLY_PROGRESS_ID
{
    DXGK_PRESENT_DISPLAYONLY_PROGRESS_ID_COMPLETE = 0,
    DXGK_PRESENT_DISPLAYONLY_PROGRESS_ID_FAILED = 1,
} DXGK_PRESENT_DISPLAY_ONLY_PROGRESS_ID;

typedef struct _DXGKARGCB_PRESENT_DISPLAYONLY_PROGRESS
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    DXGK_PRESENT_DISPLAY_ONLY_PROGRESS_ID ProgressId;
} DXGKARGCB_PRESENT_DISPLAYONLY_PROGRESS;

typedef
VOID
(APIENTRY *DXGKCB_PRESENT_DISPLAYONLY_PROGRESS)(
    _In_ const HANDLE hAdapter,
    _In_ const DXGKARGCB_PRESENT_DISPLAYONLY_PROGRESS* pArgs
    );

typedef struct _DXGK_MULTIPLANE_OVERLAY_VSYNC_INFO DXGK_MULTIPLANE_OVERLAY_VSYNC_INFO;
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)

typedef struct _D3DKMT_PRESENT_DISPLAY_ONLY_FLAGS
{
    union
    {
        struct
        {
            UINT Rotate : 1;
            UINT Reserved : 31;
        };
        UINT Value;
    };
} D3DKMT_PRESENT_DISPLAY_ONLY_FLAGS;

typedef struct _DXGKARG_PRESENT_DISPLAYONLY
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    VOID* pSource;
    ULONG BytesPerPixel;
    LONG Pitch;
    D3DKMT_PRESENT_DISPLAY_ONLY_FLAGS Flags;
    ULONG NumMoves;
    _Field_size_(NumMoves) D3DKMT_MOVE_RECT* pMoves;
    ULONG NumDirtyRects;
    _Field_size_(NumDirtyRects) RECT* pDirtyRect;
    DXGKCB_PRESENT_DISPLAYONLY_PROGRESS pfnPresentDisplayOnlyProgress;
} DXGKARG_PRESENT_DISPLAYONLY;

typedef
_Check_return_
NTSTATUS
APIENTRY
DXGKDDI_PRESENTDISPLAYONLY(
    _In_ const HANDLE hAdapter,
    _In_ const DXGKARG_PRESENT_DISPLAYONLY* pPresentDisplayOnly
    );

typedef DXGKDDI_PRESENTDISPLAYONLY *PDXGKDDI_PRESENTDISPLAYONLY;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)

typedef struct _DXGKARG_ESCAPE
{
    HANDLE hDevice;
    D3DDDI_ESCAPEFLAGS Flags;
    VOID* pPrivateDriverData;
    UINT PrivateDriverDataSize;
    HANDLE hContext;
    HANDLE hKmdProcessHandle;
} DXGKARG_ESCAPE, *PDXGKARG_ESCAPE;

typedef
_Check_return_
NTSTATUS
APIENTRY
DXGKDDI_ESCAPE(
    _In_ const HANDLE hAdapter,
    _In_ const DXGKARG_ESCAPE* pEscape
    );

typedef DXGKDDI_ESCAPE *PDXGKDDI_ESCAPE;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
typedef struct _DXGK_MULTIPLANE_OVERLAY_VSYNC_INFO2 DXGK_MULTIPLANE_OVERLAY_VSYNC_INFO2;
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
typedef struct _DXGK_MULTIPLANE_OVERLAY_VSYNC_INFO3 DXGK_MULTIPLANE_OVERLAY_VSYNC_INFO3;
#endif

typedef enum _DXGK_HANDLE_TYPE
{
    DXGK_HANDLE_ALLOCATION = 1,
    DXGK_HANDLE_RESOURCE = 2,
} DXGK_HANDLE_TYPE;

typedef enum _DXGK_INTERRUPT_TYPE
{
    DXGK_INTERRUPT_DMA_COMPLETED = 1,
    DXGK_INTERRUPT_DMA_PREEMPTED = 2,
    DXGK_INTERRUPT_CRTC_VSYNC = 3,
    DXGK_INTERRUPT_DMA_FAULTED = 4,

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    DXGK_INTERRUPT_DISPLAYONLY_VSYNC = 5,
    DXGK_INTERRUPT_DISPLAYONLY_PRESENT_PROGRESS = 6,
    DXGK_INTERRUPT_CRTC_VSYNC_WITH_MULTIPLANE_OVERLAY = 7,
#endif // DXGKDDI_INTERFACE_VERSION

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    DXGK_INTERRUPT_MICACAST_CHUNK_PROCESSING_COMPLETE = 8,
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    DXGK_INTERRUPT_DMA_PAGE_FAULTED = 9,
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
    DXGK_INTERRUPT_CRTC_VSYNC_WITH_MULTIPLANE_OVERLAY2 = 10,
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
    DXGK_INTERRUPT_MONITORED_FENCE_SIGNALED = 11,
    DXGK_INTERRUPT_HWQUEUE_PAGE_FAULTED = 12,
    DXGK_INTERRUPT_HWCONTEXTLIST_SWITCH_COMPLETED = 13,
    DXGK_INTERRUPT_PERIODIC_MONITORED_FENCE_SIGNALED = 14,
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
    DXGK_INTERRUPT_SCHEDULING_LOG_INTERRUPT = 15,
    DXGK_INTERRUPT_GPU_ENGINE_TIMEOUT = 16,
    DXGK_INTERRUPT_SUSPEND_CONTEXT_COMPLETED = 17,
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
    DXGK_INTERRUPT_CRTC_VSYNC_WITH_MULTIPLANE_OVERLAY3 = 18,
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_1)
    DXGK_INTERRUPT_NATIVE_FENCE_SIGNALED = 19,
    DXGK_INTERRUPT_GPU_ENGINE_STATE_CHANGE = 20
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_1)

} DXGK_INTERRUPT_TYPE;

typedef enum _DXGK_MONITOR_INTERFACE_VERSION
{
    DXGK_MONITOR_INTERFACE_VERSION_UNINITIALIZED = 0,
    DXGK_MONITOR_INTERFACE_VERSION_V1 = 1,
    DXGK_MONITOR_INTERFACE_VERSION_V2 = 2,
} DXGK_MONITOR_INTERFACE_VERSION;

typedef enum _DXGK_VIDPN_INTERFACE_VERSION
{
    DXGK_VIDPN_INTERFACE_VERSION_UNINITIALIZED = 0,
    DXGK_VIDPN_INTERFACE_VERSION_V1 = 1,

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
    DXGK_VIDPN_INTERFACE_VERSION_V2 = 2,
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
} DXGK_VIDPN_INTERFACE_VERSION;

typedef enum _DXGK_ENGINE_STATE
{
    DXGK_ENGINE_STATE_ACTIVE = 0,
    DXGK_ENGINE_STATE_IDLE = 1,
    DXGK_ENGINE_STATE_HUNG = 2
} DXGK_ENGINE_STATE;

typedef enum _DXGK_QUERYADAPTERINFOTYPE
{
    DXGKQAITYPE_UMDRIVERPRIVATE = 0,
    DXGKQAITYPE_DRIVERCAPS = 1,
    DXGKQAITYPE_QUERYSEGMENT = 2,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7)
    DXGKQAITYPE_RESERVED = 3,
    DXGKQAITYPE_QUERYSEGMENT2 = 4,
#endif // DXGKDDI_INTERFACE_VERSION_WIN7
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    DXGKQAITYPE_QUERYSEGMENT3 = 5,
    DXGKQAITYPE_NUMPOWERCOMPONENTS = 6,
    DXGKQAITYPE_POWERCOMPONENTINFO = 7,
    DXGKQAITYPE_PREFERREDGPUNODE = 8,
#endif // DXGKDDI_INTERFACE_VERSION_WIN8
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    DXGKQAITYPE_POWERCOMPONENTPSTATEINFO = 9,
    DXGKQAITYPE_HISTORYBUFFERPRECISION = 10,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM1_3
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    DXGKQAITYPE_QUERYSEGMENT4 = 11,
    DXGKQAITYPE_SEGMENTMEMORYSTATE = 12,
    DXGKQAITYPE_GPUMMUCAPS = 13,
    DXGKQAITYPE_PAGETABLELEVELDESC = 14,
    DXGKQAITYPE_PHYSICALADAPTERCAPS = 15,
    DXGKQAITYPE_DISPLAY_DRIVERCAPS_EXTENSION = 16,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_0
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
    DXGKQAITYPE_INTEGRATED_DISPLAY_DESCRIPTOR = 17,
    DXGKQAITYPE_UEFIFRAMEBUFFERRANGES = 18,
    DXGKQAITYPE_QUERYCOLORIMETRYOVERRIDES = 19,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_2
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
    DXGKQAITYPE_DISPLAYID_DESCRIPTOR = 20,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_3
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
    DXGKQAITYPE_FRAMEBUFFERSAVESIZE = 21,
    DXGKQAITYPE_HARDWARERESERVEDRANGES = 22,
    DXGKQAITYPE_INTEGRATED_DISPLAY_DESCRIPTOR2 = 23,
    DXGKQAITYPE_NODEPERFDATA = 24,
    DXGKQAITYPE_ADAPTERPERFDATA = 25,
    DXGKQAITYPE_ADAPTERPERFDATA_CAPS = 26,
    DXGKQAITYPE_GPUVERSION = 27,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_4
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)
    DXGKQAITYPE_DEVICE_TYPE_CAPS = 28,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_5
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)
    DXGKQAITYPE_WDDMDEVICECAPS = 29,
    DXGKQAITYPE_GPUPCAPS = 30,
    DXGKQAITYPE_QUERYTARGETGAMMACAPS = 31,
    DXGKQAITYPE_SCANOUT_CAPS = 33,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_6
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
    DXGKQAITYPE_PHYSICAL_MEMORY_CAPS = 34,
    DXGKQAITYPE_IOMMU_CAPS = 35,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_9
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_1)
    DXGKQAITYPE_HARDWARERESERVEDRANGES2 = 36,
    DXGKQAITYPE_NATIVE_FENCE_CAPS = 37,
    DXGKQAITYPE_USERMODESUBMISSION_CAPS = 38,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM3_1

} DXGK_QUERYADAPTERINFOTYPE;

/*
 * Driver capability structures used with DXGKQAITYPE_DRIVERCAPS and
 * DXGKQAITYPE_DISPLAY_DRIVERCAPS_EXTENSION.
 * ReactOS currently only needs a minimal subset for display-only bring-up.
 */

typedef enum _DXGK_WDDMVERSION
{
    DXGKDDI_WDDMv1_0 = 0x1000,
    DXGKDDI_WDDMv1_1 = 0x1100,
    DXGKDDI_WDDMv1_2 = 0x1200,
} DXGK_WDDMVERSION;

typedef struct _DXGK_DRIVERCAPS
{
    DXGK_WDDMVERSION WDDMVersion;
    LARGE_INTEGER HighestAcceptableAddress;
    BOOLEAN SupportNonVGA;
    BOOLEAN SupportSmoothRotation;
    UCHAR Reserved[2];
} DXGK_DRIVERCAPS;

typedef struct _DXGK_DISPLAY_DRIVERCAPS_EXTENSION
{
    union
    {
        struct
        {
            UINT VirtualModeSupport : 1;
            UINT Reserved : 31;
        };
        UINT Value;
    };
} DXGK_DISPLAY_DRIVERCAPS_EXTENSION;

typedef struct _DXGKCB_GETHANDLEDATAFLAGS
{
    union
    {
        struct
        {
            UINT DeviceSpecific : 1;
            UINT Reserved : 31;
        };
        UINT Value;
    };
} DXGKCB_GETHANDLEDATAFLAGS;

typedef struct _DXGK_ALLOCATIONINFOFLAGS_WDDM2_0
{
    union
    {
        struct
        {
            UINT    CpuVisible                      : 1;
            UINT    PermanentSysMem                 : 1;
            UINT    Cached                          : 1;
            UINT    Protected                       : 1;
            UINT    ExistingSysMem                  : 1;
            UINT    ExistingKernelSysMem            : 1;
            UINT    FromEndOfSegment                : 1;
            UINT    DisableLargePageMapping         : 1;
            UINT    Overlay                         : 1;
            UINT    Capture                         : 1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
            UINT    CreateInVpr                     : 1;
#else
            UINT    Reserved00                      : 1;
#endif
            UINT    DXGK_ALLOC_RESERVED17           : 1;
            UINT    Reserved02                      : 1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
            UINT    MapApertureCpuVisible           : 1;
#else
            UINT    Reserved03                      : 1;
#endif
            UINT    HistoryBuffer                   : 1;
            UINT    AccessedPhysically              : 1;
            UINT    ExplicitResidencyNotification   : 1;
            UINT    HardwareProtected               : 1;
            UINT    CpuVisibleOnDemand              : 1;
            UINT    DXGK_ALLOC_RESERVED16           : 1;
            UINT    DXGK_ALLOC_RESERVED15           : 1;
            UINT    DXGK_ALLOC_RESERVED14           : 1;
            UINT    DXGK_ALLOC_RESERVED13           : 1;
            UINT    DXGK_ALLOC_RESERVED12           : 1;
            UINT    DXGK_ALLOC_RESERVED11           : 1;
            UINT    DXGK_ALLOC_RESERVED10           : 1;
            UINT    DXGK_ALLOC_RESERVED9            : 1;
            UINT    DXGK_ALLOC_RESERVED4            : 1;
            UINT    DXGK_ALLOC_RESERVED3            : 1;
            UINT    DXGK_ALLOC_RESERVED2            : 1;
            UINT    DXGK_ALLOC_RESERVED1            : 1;
            UINT    DXGK_ALLOC_RESERVED0            : 1;
        };
        UINT Value;
    };
} DXGK_ALLOCATIONINFOFLAGS_WDDM2_0;
C_ASSERT(sizeof(DXGK_ALLOCATIONINFOFLAGS_WDDM2_0) == 0x4);

typedef struct _DXGK_SEGMENTPREFERENCE
{
    union
    {
        struct
        {
            UINT SegmentId0 : 5;
            UINT Direction0 : 1;
            UINT SegmentId1 : 5;
            UINT Direction1 : 1;
            UINT SegmentId2 : 5;
            UINT Direction2 : 1;
            UINT SegmentId3 : 5;
            UINT Direction3 : 1;
            UINT SegmentId4 : 5;
            UINT Direction4 : 1;
            UINT Reserved   : 2;
        };
        UINT Value;
    };
} DXGK_SEGMENTPREFERENCE, *PDXGK_SEGMENTPREFERENCE;
C_ASSERT(sizeof(DXGK_SEGMENTPREFERENCE) == 0x4);

typedef struct _DXGK_SEGMENTBANKPREFERENCE
{
    union
    {
        struct
        {
            UINT Bank0          : 7;
            UINT Direction0     : 1;
            UINT Bank1          : 7;
            UINT Direction1     : 1;
            UINT Bank2          : 7;
            UINT Direction2     : 1;
            UINT Bank3          : 7;
            UINT Direction3     : 1;
        };
        UINT Value;
    };
} DXGK_SEGMENTBANKPREFERENCE;
C_ASSERT(sizeof(DXGK_SEGMENTBANKPREFERENCE) == 0x4);

typedef struct _DXGK_ALLOCATIONINFOFLAGS
{
    union
    {
        struct
        {
            UINT CpuVisible              : 1;
            UINT PermanentSysMem         : 1;
            UINT Cached                  : 1;
            UINT Protected               : 1;
            UINT ExistingSysMem          : 1;
            UINT ExistingKernelSysMem    : 1;
            UINT FromEndOfSegment        : 1;
            UINT Swizzled                : 1;
            UINT Overlay                 : 1;
            UINT Capture                 : 1;
            UINT UseAlternateVA          : 1;
            UINT SynchronousPaging       : 1;
            UINT LinkMirrored            : 1;
            UINT LinkInstanced           : 1;
            UINT HistoryBuffer           : 1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
            UINT AccessedPhysically      : 1;
            UINT ExplicitResidencyNotification : 1;
            UINT HardwareProtected       : 1;
            UINT CpuVisibleOnDemand      : 1;
#else
            UINT Reserved                : 4;
#endif
            UINT DXGK_ALLOC_RESERVED16   : 1;
            UINT DXGK_ALLOC_RESERVED15   : 1;
            UINT DXGK_ALLOC_RESERVED14   : 1;
            UINT DXGK_ALLOC_RESERVED13   : 1;
            UINT DXGK_ALLOC_RESERVED12   : 1;
            UINT DXGK_ALLOC_RESERVED11   : 1;
            UINT DXGK_ALLOC_RESERVED10   : 1;
            UINT DXGK_ALLOC_RESERVED9    : 1;
            UINT DXGK_ALLOC_RESERVED4    : 1;
            UINT DXGK_ALLOC_RESERVED3    : 1;
            UINT DXGK_ALLOC_RESERVED2    : 1;
            UINT DXGK_ALLOC_RESERVED1    : 1;
            UINT DXGK_ALLOC_RESERVED0    : 1;
        };
        UINT Value;
    };
} DXGK_ALLOCATIONINFOFLAGS;
C_ASSERT(sizeof(DXGK_ALLOCATIONINFOFLAGS) == 0x4);

typedef struct _DXGK_ALLOCATIONUSAGEINFO1
{
    union
    {
        struct
        {
            UINT PrivateFormat  : 1;
            UINT Swizzled       : 1;
            UINT MipMap         : 1;
            UINT Cube           : 1;
            UINT Volume         : 1;
            UINT Vertex         : 1;
            UINT Index          : 1;
            UINT Reserved       : 25;
        };
        UINT Value;
    } Flags;
    union
    {
        D3DDDIFORMAT Format;
        UINT         PrivateFormat;
    };
    UINT SwizzledFormat;
    UINT ByteOffset;
    UINT Width;
    UINT Height;
    UINT Pitch;
    UINT Depth;
    UINT SlicePitch;
} DXGK_ALLOCATIONUSAGEINFO1;
C_ASSERT(sizeof(DXGK_ALLOCATIONUSAGEINFO1) == 0x24);

typedef struct _DXGK_ALLOCATIONUSAGEHINT
{
    UINT                      Version;
    DXGK_ALLOCATIONUSAGEINFO1 v1;
} DXGK_ALLOCATIONUSAGEHINT;
C_ASSERT(sizeof(DXGK_ALLOCATIONUSAGEHINT) == 0x28);

typedef struct _DXGK_ALLOCATIONINFO
{
    VOID*                      pPrivateDriverData;
    UINT                       PrivateDriverDataSize;
    UINT                       Alignment;
    SIZE_T                     Size;
    SIZE_T                     PitchAlignedSize;
    DXGK_SEGMENTBANKPREFERENCE HintedBank;
    DXGK_SEGMENTPREFERENCE     PreferredSegment;
    UINT                       SupportedReadSegmentSet;
    UINT                       SupportedWriteSegmentSet;
    UINT                       EvictionSegmentSet;
    union
    {
        UINT MaximumRenamingListLength;
        UINT PhysicalAdapterIndex;
    };
    HANDLE hAllocation;
    union
    {
        DXGK_ALLOCATIONINFOFLAGS Flags;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
        DXGK_ALLOCATIONINFOFLAGS_WDDM2_0 FlagsWddm2;
#endif
    };
    DXGK_ALLOCATIONUSAGEHINT* pAllocationUsageHint;
    UINT                      AllocationPriority;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)
    DXGK_ALLOCATIONINFOFLAGS2 Flags2;
#endif
} DXGK_ALLOCATIONINFO;

#ifdef _WIN64
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)
C_ASSERT(sizeof(DXGK_ALLOCATIONINFO) == 0x5C);
#else
C_ASSERT(sizeof(DXGK_ALLOCATIONINFO) == 0x58);
#endif
#else
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)
C_ASSERT(sizeof(DXGK_ALLOCATIONINFO) == 0x40);
#else
C_ASSERT(sizeof(DXGK_ALLOCATIONINFO) == 0x3C);
#endif
#endif

typedef struct _DXGK_CREATEALLOCATIONFLAGS
{
    union
    {
        struct
        {
            UINT Resource : 1;
            UINT Reserved : 31;
        };
        UINT Value;
    };
} DXGK_CREATEALLOCATIONFLAGS;

typedef struct _DXGKARG_CREATEALLOCATION
{
    CONST VOID* pPrivateDriverData;
    UINT PrivateDriverDataSize;
    UINT NumAllocations;
    DXGK_ALLOCATIONINFO* pAllocationInfo;
    HANDLE hResource;
    DXGK_CREATEALLOCATIONFLAGS Flags;
} DXGKARG_CREATEALLOCATION, *PDXGKARG_CREATEALLOCATION;

typedef
_Check_return_
NTSTATUS
APIENTRY
DXGKDDI_CREATEALLOCATION(
    _In_ const HANDLE hAdapter,
    _Inout_ PDXGKARG_CREATEALLOCATION pCreateAllocation
    );

typedef DXGKDDI_CREATEALLOCATION *PDXGKDDI_CREATEALLOCATION;

typedef struct _DXGKCB_NOTIFY_INTERRUPT_DATA_FLAGS
{
    union
    {
        struct
        {
            UINT ValidPhysicalAdapterMask : 1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
            UINT HsyncFlipCompletion : 1;
            UINT Reserved : 30;
#else
            UINT Reserved : 31;
#endif
        };
        UINT Value;
    };
} DXGKCB_NOTIFY_INTERRUPT_DATA_FLAGS;

typedef struct _DXGKARGCB_NOTIFY_INTERRUPT_DATA
{
    DXGK_INTERRUPT_TYPE InterruptType;
    union
    {
        struct
        {
            UINT SubmissionFenceId;
            UINT NodeOrdinal;
            UINT EngineOrdinal;
        } DmaCompleted;
        struct
        {
            UINT PreemptionFenceId;
            UINT LastCompletedFenceId;
            UINT NodeOrdinal;
            UINT EngineOrdinal;
        } DmaPreempted;
        struct
        {
            UINT FaultedFenceId;
            NTSTATUS Status;
            UINT NodeOrdinal;
            UINT EngineOrdinal;
        } DmaFaulted;
        struct
        {
            D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId;
            PHYSICAL_ADDRESS PhysicalAddress;
            UINT PhysicalAdapterMask;
        } CrtcVsync;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
        struct
        {
            D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId;
        } DisplayOnlyVsync;
        struct
        {
            D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId;
            UINT PhysicalAdapterMask;
            UINT MultiPlaneOverlayVsyncInfoCount;
            DXGK_MULTIPLANE_OVERLAY_VSYNC_INFO *pMultiPlaneOverlayVsyncInfo;
        } CrtcVsyncWithMultiPlaneOverlay;
        DXGKARGCB_PRESENT_DISPLAYONLY_PROGRESS DisplayOnlyPresentProgress;
#endif // DXGKDDI_INTERFACE_VERSION

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
        struct
        {
            D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId;
            DXGK_MIRACAST_CHUNK_INFO ChunkInfo;
            PVOID pPrivateDriverData;
            UINT PrivateDataDriverSize;
            NTSTATUS Status;
        } MiracastEncodeChunkCompleted;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
        struct
        {
            UINT FaultedFenceId;
            UINT64 FaultedPrimitiveAPISequenceNumber;
            DXGK_RENDER_PIPELINE_STAGE FaultedPipelineStage;
            UINT FaultedBindTableEntry;
            DXGK_PAGE_FAULT_FLAGS PageFaultFlags;
            D3DGPU_VIRTUAL_ADDRESS FaultedVirtualAddress;
            UINT NodeOrdinal;
            UINT EngineOrdinal;
            UINT PageTableLevel;
            DXGK_FAULT_ERROR_CODE FaultErrorCode;
            HANDLE FaultedProcessHandle;
        } DmaPageFaulted;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
        struct
        {
            D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId;
            UINT PhysicalAdapterMask;
            UINT MultiPlaneOverlayVsyncInfoCount;
            _Field_size_(MultiPlaneOverlayVsyncInfoCount) DXGK_MULTIPLANE_OVERLAY_VSYNC_INFO2 *pMultiPlaneOverlayVsyncInfo;
            ULONGLONG GpuFrequency;
            ULONGLONG GpuClockCounter;
        } CrtcVsyncWithMultiPlaneOverlay2;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
        struct
        {
            UINT NodeOrdinal;
            UINT EngineOrdinal;
        } MonitoredFenceSignaled;
        struct
        {
            UINT NodeOrdinal;
            UINT EngineOrdinal;
            UINT64 ContextSwitchFence;
        } HwContextListSwitchCompleted;
        struct
        {
            UINT64 FaultedFenceId;
            D3DGPU_VIRTUAL_ADDRESS FaultedVirtualAddress;
            UINT64 FaultedPrimitiveAPISequenceNumber;
            union
            {
                HANDLE FaultedHwQueue;
                HANDLE FaultedHwContext;
                HANDLE FaultedProcessHandle;
            };
            UINT NodeOrdinal;
            UINT EngineOrdinal;
            DXGK_RENDER_PIPELINE_STAGE FaultedPipelineStage;
            UINT FaultedBindTableEntry;
            DXGK_PAGE_FAULT_FLAGS PageFaultFlags;
            UINT PageTableLevel;
            DXGK_FAULT_ERROR_CODE FaultErrorCode;
        } HwQueuePageFaulted;
        struct
        {
            D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId;
            UINT NotificationID;
        } PeriodicMonitoredFenceSignaled;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
        struct
        {
            UINT NodeOrdinal;
            UINT EngineOrdinal;
        } SchedulingLogInterrupt;
        struct
        {
            UINT NodeOrdinal;
            UINT EngineOrdinal;
        } GpuEngineTimeout;
        struct
        {
            HANDLE hContext;
            UINT64 ContextSuspendFence;
        } SuspendContextCompleted;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
        struct
        {
            D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId;
            UINT PhysicalAdapterMask;
            UINT MultiPlaneOverlayVsyncInfoCount;
            _Field_size_(MultiPlaneOverlayVsyncInfoCount) DXGK_MULTIPLANE_OVERLAY_VSYNC_INFO3 *pMultiPlaneOverlayVsyncInfo;
            ULONGLONG GpuFrequency;
            ULONGLONG GpuClockCounter;
        } CrtcVsyncWithMultiPlaneOverlay3;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_1)
        struct
        {
            UINT NodeOrdinal;
            UINT EngineOrdinal;
            UINT SignaledNativeFenceCount;
            _Field_size_(SignaledNativeFenceCount) HANDLE* pSignaledNativeFenceArray;
        } NativeFenceSignaled;
        struct
        {
            UINT NodeOrdinal;
            UINT EngineOrdinal;
            DXGK_ENGINE_STATE NewState;
        } EngineStateChange;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_1)

        struct
        {
            UINT Reserved[16];
        } Reserved;
    };
    DXGKCB_NOTIFY_INTERRUPT_DATA_FLAGS Flags;
} DXGKARGCB_NOTIFY_INTERRUPT_DATA ,*PDXGKARGCB_NOTIFY_INTERRUPT_DATA;

typedef struct _DXGKARGCB_GETHANDLEDATA
{
    D3DKMT_HANDLE hObject;
    DXGK_HANDLE_TYPE Type;
    DXGKCB_GETHANDLEDATAFLAGS Flags;
} DXGKARGCB_GETHANDLEDATA , *PDXGKARGCB_GETHANDLEDATA;

typedef _In_ CONST DXGKARGCB_GETHANDLEDATA* IN_CONST_PDXGKARGCB_GETHANDLEDATA;

typedef struct _DXGKARGCB_GETCAPTUREADDRESS
{
    D3DKMT_HANDLE hAllocation;
    UINT SegmentId;
    PHYSICAL_ADDRESS PhysicalAddress;
} DXGKARGCB_GETCAPTUREADDRESS, *PDXGKARGCB_GETCAPTUREADDRESS;

typedef struct _DXGKARGCB_ENUMHANDLECHILDREN
{
    D3DKMT_HANDLE hObject;
    UINT Index;
} DXGKARGCB_ENUMHANDLECHILDREN, *PDXGKARGCB_ENUMHANDLECHILDREN;

typedef _In_ CONST DXGKARGCB_ENUMHANDLECHILDREN* IN_CONST_PDXGKARGCB_ENUMHANDLECHILDREN;

typedef struct _DXGK_ENUM_PIVOT
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId;
} DXGK_ENUM_PIVOT;

typedef struct _DXGKARG_ENUMVIDPNCOFUNCMODALITY
{
    _In_ D3DKMDT_HVIDPN hConstrainingVidPn;
    _In_ D3DKMDT_ENUMCOFUNCMODALITY_PIVOT_TYPE EnumPivotType;
    _In_ DXGK_ENUM_PIVOT EnumPivot;
} DXGKARG_ENUMVIDPNCOFUNCMODALITY;

typedef _In_ CONST DXGKARG_ENUMVIDPNCOFUNCMODALITY* CONST IN_CONST_PDXGKARG_ENUMVIDPNCOFUNCMODALITY_CONST;

typedef struct _DXGK_SETPOINTERPOSITION_FLAGS
{
    union
    {
        struct
        {
            UINT Visible : 1;
            UINT Reserved : 31;
        };
        UINT Value;
    };
} DXGK_SETPOINTERPOSITION_FLAGS;

typedef struct _DXGKARG_SETPOINTERPOSITION
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    DXGK_SETPOINTERPOSITION_FLAGS Flags;
    POINT Position;
} DXGKARG_SETPOINTERPOSITION;

typedef struct _DXGKARG_SETPOINTERSHAPE
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    UINT Width;
    UINT Height;
    UINT Pitch;
    POINT HotSpot;
    const VOID* pPixels;
} DXGKARG_SETPOINTERSHAPE;

typedef struct _DXGKARG_ISSUPPORTEDVIDPN
{
    D3DKMDT_HVIDPN hDesiredVidPn;
    BOOLEAN IsVidPnSupported;
    UCHAR Reserved[3];
} DXGKARG_ISSUPPORTEDVIDPN;

typedef struct _DXGKARG_RECOMMENDFUNCTIONALVIDPN
{
    D3DKMDT_HVIDPN hRecommendedFunctionalVidPn;
} DXGKARG_RECOMMENDFUNCTIONALVIDPN;

typedef struct _DXGKARG_RECOMMENDVIDPNTOPOLOGY
{
    D3DKMDT_HVIDPNTOPOLOGY hVidPnTopology;
} DXGKARG_RECOMMENDVIDPNTOPOLOGY;

typedef struct _DXGK_MONITORSOURCEMODESET_INTERFACE DXGK_MONITORSOURCEMODESET_INTERFACE;

typedef struct _DXGKARG_RECOMMENDMONITORMODES
{
    D3DDDI_VIDEO_PRESENT_TARGET_ID VideoPresentTargetId;
    D3DKMDT_HMONITORSOURCEMODESET hMonitorSourceModeSet;
    const DXGK_MONITORSOURCEMODESET_INTERFACE* pMonitorSourceModeSetInterface;
} DXGKARG_RECOMMENDMONITORMODES;

typedef struct _DXGKARG_SETVIDPNSOURCEVISIBILITY
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    BOOLEAN Visible;
    UCHAR Reserved[3];
} DXGKARG_SETVIDPNSOURCEVISIBILITY;

typedef struct _DXGK_COMMITVIDPN_FLAGS
{
    union
    {
        struct
        {
            UINT PathPowerTransition : 1;
            UINT PathPoweredOff : 1;
            UINT Reserved : 30;
        };
        UINT Value;
    };
} DXGK_COMMITVIDPN_FLAGS;

typedef struct _DXGKARG_COMMITVIDPN
{
    D3DKMDT_HVIDPN hFunctionalVidPn;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID AffectedVidPnSourceId;
    DXGK_COMMITVIDPN_FLAGS Flags;
    HANDLE hPrimaryAllocation;
    D3DKMDT_MONITOR_CONNECTIVITY_CHECKS MonitorConnectivityChecks;
} DXGKARG_COMMITVIDPN;

typedef struct _DXGKARG_UPDATEACTIVEVIDPNPRESENTPATH
{
    D3DKMDT_VIDPN_PRESENT_PATH VidPnPresentPathInfo;
} DXGKARG_UPDATEACTIVEVIDPNPRESENTPATH;

typedef struct _DXGK_SETVIDPNSOURCEADDRESS_FLAGS
{
    union
    {
        struct
        {
            UINT ModeChange : 1;
            UINT FlipImmediate : 1;
            UINT FlipOnNextVSync : 1;
            UINT Reserved : 29;
        };
        UINT Value;
    };
} DXGK_SETVIDPNSOURCEADDRESS_FLAGS;

typedef struct _DXGKARG_SETVIDPNSOURCEADDRESS
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    UINT PrimarySegment;
    PHYSICAL_ADDRESS PrimaryAddress;
    HANDLE hAllocation;
    UINT ContextCount;
    HANDLE Context[1 + D3DDDI_MAX_BROADCAST_CONTEXT];
    DXGK_SETVIDPNSOURCEADDRESS_FLAGS Flags;
} DXGKARG_SETVIDPNSOURCEADDRESS;

typedef _Inout_ DXGKARGCB_NOTIFY_INTERRUPT_DATA* IN_CONST_PDXGKARGCB_NOTIFY_INTERRUPT_DATA;

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNSOURCEMODESET_GETNUMMODES)(
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET hVidPnSourceModeSet,
    _Out_ SIZE_T* const pNumSourceModes);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNSOURCEMODESET_ACQUIREFIRSTMODEINFO)(
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET hVidPnSourceModeSet,
    _Outptr_ const D3DKMDT_VIDPN_SOURCE_MODE** ppFirstVidPnSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNSOURCEMODESET_ACQUIRENEXTMODEINFO)(
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET hVidPnSourceModeSet,
    _In_ const D3DKMDT_VIDPN_SOURCE_MODE* pVidPnSourceModeInfo,
    _Outptr_ const D3DKMDT_VIDPN_SOURCE_MODE** ppNextVidPnSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNSOURCEMODESET_ACQUIREPINNEDMODEINFO)(
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET hVidPnSourceModeSet,
    _Outptr_ const D3DKMDT_VIDPN_SOURCE_MODE** ppPinnedVidPnSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNSOURCEMODESET_RELEASEMODEINFO)(
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET hVidPnSourceModeSet,
    _In_ const D3DKMDT_VIDPN_SOURCE_MODE* pVidPnSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNSOURCEMODESET_CREATENEWMODEINFO)(
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET hVidPnSourceModeSet,
    _Outptr_ D3DKMDT_VIDPN_SOURCE_MODE** ppNewVidPnSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNSOURCEMODESET_ADDMODE)(
    _In_ D3DKMDT_HVIDPNSOURCEMODESET hVidPnSourceModeSet,
    _In_ D3DKMDT_VIDPN_SOURCE_MODE* pVidPnSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNSOURCEMODESET_PINMODE)(
    _In_ D3DKMDT_HVIDPNSOURCEMODESET hVidPnSourceModeSet,
    _In_ const D3DKMDT_VIDEO_PRESENT_SOURCE_MODE_ID VidPnSourceModeId);

typedef struct _DXGK_VIDPNSOURCEMODESET_INTERFACE
{
    DXGKDDI_VIDPNSOURCEMODESET_GETNUMMODES pfnGetNumModes;
    DXGKDDI_VIDPNSOURCEMODESET_ACQUIREFIRSTMODEINFO pfnAcquireFirstModeInfo;
    DXGKDDI_VIDPNSOURCEMODESET_ACQUIRENEXTMODEINFO pfnAcquireNextModeInfo;
    DXGKDDI_VIDPNSOURCEMODESET_ACQUIREPINNEDMODEINFO pfnAcquirePinnedModeInfo;
    DXGKDDI_VIDPNSOURCEMODESET_RELEASEMODEINFO pfnReleaseModeInfo;
    DXGKDDI_VIDPNSOURCEMODESET_CREATENEWMODEINFO pfnCreateNewModeInfo;
    DXGKDDI_VIDPNSOURCEMODESET_ADDMODE pfnAddMode;
    DXGKDDI_VIDPNSOURCEMODESET_PINMODE pfnPinMode;
} DXGK_VIDPNSOURCEMODESET_INTERFACE;

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTARGETMODESET_GETNUMMODES)(
    _In_ const D3DKMDT_HVIDPNTARGETMODESET hVidPnTargetModeSet,
    _Out_ SIZE_T* const pNumTargetModes);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTARGETMODESET_ACQUIREFIRSTMODEINFO)(
    _In_ const D3DKMDT_HVIDPNTARGETMODESET hVidPnTargetModeSet,
    _Outptr_ const D3DKMDT_VIDPN_TARGET_MODE** ppFirstVidPnTargetModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTARGETMODESET_ACQUIRENEXTMODEINFO)(
    _In_ const D3DKMDT_HVIDPNTARGETMODESET hVidPnTargetModeSet,
    _In_ const D3DKMDT_VIDPN_TARGET_MODE* pVidPnTargetModeInfo,
    _Outptr_ const D3DKMDT_VIDPN_TARGET_MODE** ppNextVidPnTargetModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTARGETMODESET_ACQUIREPINNEDMODEINFO)(
    _In_ const D3DKMDT_HVIDPNTARGETMODESET hVidPnTargetModeSet,
    _Outptr_ const D3DKMDT_VIDPN_TARGET_MODE** ppPinnedVidPnTargetModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTARGETMODESET_RELEASEMODEINFO)(
    _In_ const D3DKMDT_HVIDPNTARGETMODESET hVidPnTargetModeSet,
    _In_ const D3DKMDT_VIDPN_TARGET_MODE* pVidPnTargetModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTARGETMODESET_CREATENEWMODEINFO)(
    _In_ const D3DKMDT_HVIDPNTARGETMODESET hVidPnTargetModeSet,
    _Outptr_ D3DKMDT_VIDPN_TARGET_MODE** ppNewVidPnTargetModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTARGETMODESET_ADDMODE)(
    _In_ D3DKMDT_HVIDPNTARGETMODESET hVidPnTargetModeSet,
    _In_ D3DKMDT_VIDPN_TARGET_MODE* pVidPnTargetModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTARGETMODESET_PINMODE)(
    _In_ D3DKMDT_HVIDPNTARGETMODESET hVidPnTargetModeSet,
    _In_ const D3DKMDT_VIDEO_PRESENT_TARGET_MODE_ID VidPnTargetModeId);

typedef struct _DXGK_VIDPNTARGETMODESET_INTERFACE
{
    DXGKDDI_VIDPNTARGETMODESET_GETNUMMODES pfnGetNumModes;
    DXGKDDI_VIDPNTARGETMODESET_ACQUIREFIRSTMODEINFO pfnAcquireFirstModeInfo;
    DXGKDDI_VIDPNTARGETMODESET_ACQUIRENEXTMODEINFO pfnAcquireNextModeInfo;
    DXGKDDI_VIDPNTARGETMODESET_ACQUIREPINNEDMODEINFO pfnAcquirePinnedModeInfo;
    DXGKDDI_VIDPNTARGETMODESET_RELEASEMODEINFO pfnReleaseModeInfo;
    DXGKDDI_VIDPNTARGETMODESET_CREATENEWMODEINFO pfnCreateNewModeInfo;
    DXGKDDI_VIDPNTARGETMODESET_ADDMODE pfnAddMode;
    DXGKDDI_VIDPNTARGETMODESET_PINMODE pfnPinMode;
} DXGK_VIDPNTARGETMODESET_INTERFACE;

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_GETNUMPATHS)(
    _In_ const D3DKMDT_HVIDPNTOPOLOGY hVidPnTopology,
    _Out_ PSIZE_T pNumPaths);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_GETNUMPATHSFROMSOURCE)(
    _In_ const D3DKMDT_HVIDPNTOPOLOGY hVidPnTopology,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId,
    _Out_ PSIZE_T pNumPathsFromSource);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_ENUMPATHTARGETSFROMSOURCE)(
    _In_ const D3DKMDT_HVIDPNTOPOLOGY hVidPnTopology,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId,
    _In_ const D3DKMDT_VIDPN_PRESENT_PATH_INDEX VidPnPresentPathIndex,
    _Out_ D3DDDI_VIDEO_PRESENT_TARGET_ID* pVidPnTargetId);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_GETPATHSOURCEFROMTARGET)(
    _In_ const D3DKMDT_HVIDPNTOPOLOGY hVidTopology,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId,
    _Out_ D3DDDI_VIDEO_PRESENT_SOURCE_ID* pVidPnSourceId);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_ACQUIREPATHINFO)(
    _In_ const D3DKMDT_HVIDPNTOPOLOGY hVidPnTopology,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId,
    _Outptr_ const D3DKMDT_VIDPN_PRESENT_PATH** ppVidPnPresentPathInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_ACQUIREFIRSTPATHINFO)(
    _In_ const D3DKMDT_HVIDPNTOPOLOGY hVidPnTopology,
    _Outptr_ const D3DKMDT_VIDPN_PRESENT_PATH** ppFirstVidPnPresentPathInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_ACQUIRENEXTPATHINFO)(
    _In_ const D3DKMDT_HVIDPNTOPOLOGY hVidPnTopology,
    _In_ const D3DKMDT_VIDPN_PRESENT_PATH* pVidPnPresentPathInfo,
    _Outptr_ const D3DKMDT_VIDPN_PRESENT_PATH** ppNextVidPnPresentPathInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_UPDATEPATHSUPPORTINFO)(
     _In_ const D3DKMDT_HVIDPNTOPOLOGY i_hVidPnTopology,
     _In_ const D3DKMDT_VIDPN_PRESENT_PATH* i_pVidPnPresentPathInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_RELEASEPATHINFO)(
     _In_ const D3DKMDT_HVIDPNTOPOLOGY hVidPnTopology,
     _In_ const D3DKMDT_VIDPN_PRESENT_PATH* pVidPnPresentPathInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_CREATENEWPATHINFO)(
     _In_ const D3DKMDT_HVIDPNTOPOLOGY hVidPnTopology,
     _Outptr_ D3DKMDT_VIDPN_PRESENT_PATH** ppNewVidPnPresentPathInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_ADDPATH)(
     _In_ D3DKMDT_HVIDPNTOPOLOGY hVidPnTopology,
     _In_ D3DKMDT_VIDPN_PRESENT_PATH* pVidPnPresentPath);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_REMOVEPATH)(
    _In_ const D3DKMDT_HVIDPNTOPOLOGY hVidPnTopology,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId);

typedef struct _DXGK_VIDPNTOPOLOGY_INTERFACE
{
    DXGKDDI_VIDPNTOPOLOGY_GETNUMPATHS pfnGetNumPaths;
    DXGKDDI_VIDPNTOPOLOGY_GETNUMPATHSFROMSOURCE pfnGetNumPathsFromSource;
    DXGKDDI_VIDPNTOPOLOGY_ENUMPATHTARGETSFROMSOURCE pfnEnumPathTargetsFromSource;
    DXGKDDI_VIDPNTOPOLOGY_GETPATHSOURCEFROMTARGET pfnGetPathSourceFromTarget;
    DXGKDDI_VIDPNTOPOLOGY_ACQUIREPATHINFO pfnAcquirePathInfo;
    DXGKDDI_VIDPNTOPOLOGY_ACQUIREFIRSTPATHINFO pfnAcquireFirstPathInfo;
    DXGKDDI_VIDPNTOPOLOGY_ACQUIRENEXTPATHINFO pfnAcquireNextPathInfo;
    DXGKDDI_VIDPNTOPOLOGY_UPDATEPATHSUPPORTINFO pfnUpdatePathSupportInfo;
    DXGKDDI_VIDPNTOPOLOGY_RELEASEPATHINFO pfnReleasePathInfo;
    DXGKDDI_VIDPNTOPOLOGY_CREATENEWPATHINFO pfnCreateNewPathInfo;
    DXGKDDI_VIDPNTOPOLOGY_ADDPATH pfnAddPath;
    DXGKDDI_VIDPNTOPOLOGY_REMOVEPATH pfnRemovePath;
} DXGK_VIDPNTOPOLOGY_INTERFACE, *PDXGK_VIDPNTOPOLOGY_INTERFACE;

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_GETTOPOLOGY)(
    _In_ const D3DKMDT_HVIDPN hVidPn,
    _Out_ D3DKMDT_HVIDPNTOPOLOGY* phVidPnTopology,
    _Outptr_ const DXGK_VIDPNTOPOLOGY_INTERFACE** ppVidPnTopologyInterface);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_ACQUIRESOURCEMODESET)(
    _In_ const D3DKMDT_HVIDPN hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId,
    _Out_ D3DKMDT_HVIDPNSOURCEMODESET * phVidPnSourceModeSet,
    _Outptr_ const DXGK_VIDPNSOURCEMODESET_INTERFACE** ppVidPnSourceModeSetInterface);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_RELEASESOURCEMODESET)(
    _In_ const D3DKMDT_HVIDPN hVidPn,
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET hVidPnSourceModeSet);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_CREATENEWSOURCEMODESET)(
    _In_ const D3DKMDT_HVIDPN hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId,
    _Out_ D3DKMDT_HVIDPNSOURCEMODESET* phNewVidPnSourceModeSet,
    _Outptr_ const DXGK_VIDPNSOURCEMODESET_INTERFACE** ppVidPnSourceModeSetInterface);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_ASSIGNSOURCEMODESET)(
    _In_ D3DKMDT_HVIDPN hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId,
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET hVidPnSourceModeSet
    );

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_ASSIGNMULTISAMPLINGMETHODSET)(
    _In_ D3DKMDT_HVIDPN hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId,
    _In_ const SIZE_T NumMethods,
    _In_reads_(NumMethods) CONST D3DDDI_MULTISAMPLINGMETHOD* pSupportedMethodSet
    );

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_ACQUIRETARGETMODESET)(
    _In_ const D3DKMDT_HVIDPN hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId,
    _Out_ D3DKMDT_HVIDPNTARGETMODESET* phVidPnTargetModeSet,
    _Outptr_ const DXGK_VIDPNTARGETMODESET_INTERFACE** ppVidPnTargetModeSetInterface
    );

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_RELEASETARGETMODESET)(
    _In_ const D3DKMDT_HVIDPN hVidPn,
    _In_ const D3DKMDT_HVIDPNTARGETMODESET hVidPnTargetModeSet
    );

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_CREATENEWTARGETMODESET)(
    _In_ const D3DKMDT_HVIDPN hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId,
    _Out_ D3DKMDT_HVIDPNTARGETMODESET* phNewVidPnTargetModeSet,
    _Outptr_ const DXGK_VIDPNTARGETMODESET_INTERFACE** ppVidPnTargetModeSetInterace
    );

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_ASSIGNTARGETMODESET)(
    _In_ D3DKMDT_HVIDPN hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId,
    _In_ const D3DKMDT_HVIDPNTARGETMODESET hVidPnTargetModeSet);

typedef struct _DXGK_VIDPN_INTERFACE
{
    DXGK_VIDPN_INTERFACE_VERSION Version;
    DXGKDDI_VIDPN_GETTOPOLOGY pfnGetTopology;
    DXGKDDI_VIDPN_ACQUIRESOURCEMODESET pfnAcquireSourceModeSet;
    DXGKDDI_VIDPN_RELEASESOURCEMODESET pfnReleaseSourceModeSet;
    DXGKDDI_VIDPN_CREATENEWSOURCEMODESET pfnCreateNewSourceModeSet;
    DXGKDDI_VIDPN_ASSIGNSOURCEMODESET pfnAssignSourceModeSet;
    DXGKDDI_VIDPN_ASSIGNMULTISAMPLINGMETHODSET pfnAssignMultisamplingMethodSet;
    DXGKDDI_VIDPN_ACQUIRETARGETMODESET pfnAcquireTargetModeSet;
    DXGKDDI_VIDPN_RELEASETARGETMODESET pfnReleaseTargetModeSet;
    DXGKDDI_VIDPN_CREATENEWTARGETMODESET pfnCreateNewTargetModeSet;
    DXGKDDI_VIDPN_ASSIGNTARGETMODESET pfnAssignTargetModeSet;
} DXGK_VIDPN_INTERFACE, *PDXGK_VIDPN_INTERFACE;

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORSOURCEMODESET_GETNUMMODES)(
    _In_ const D3DKMDT_HMONITORSOURCEMODESET hMonitorSourceModeSet,
    _Out_ const PSIZE_T pNumMonitorSourceModes);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORSOURCEMODESET_ACQUIREPREFERREDMODEINFO)(
    _In_ const D3DKMDT_HMONITORSOURCEMODESET hMonitorSourceModeSet,
    _Outptr_ const D3DKMDT_MONITOR_SOURCE_MODE** ppFirstMonitorSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORSOURCEMODESET_ACQUIREFIRSTMODEINFO)(
    _In_ const D3DKMDT_HMONITORSOURCEMODESET hMonitorSourceModeSet,
    _Outptr_ const D3DKMDT_MONITOR_SOURCE_MODE** ppFirstMonitorSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORSOURCEMODESET_ACQUIRENEXTMODEINFO)(
    _In_ const D3DKMDT_HMONITORSOURCEMODESET hMonitorSourceModeSet,
    _In_ const D3DKMDT_MONITOR_SOURCE_MODE* pMonitorSourceModeInfo,
    _Outptr_ const D3DKMDT_MONITOR_SOURCE_MODE** ppNextMonitorSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORSOURCEMODESET_CREATENEWMODEINFO)(
    _In_ const D3DKMDT_HMONITORSOURCEMODESET hMonitorSourceModeSet,
    _Outptr_ D3DKMDT_MONITOR_SOURCE_MODE** ppNewMonitorSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORSOURCEMODESET_ADDMODE)(
    _In_ const D3DKMDT_HMONITORSOURCEMODESET hMonitorSourceModeSet,
    _In_ const D3DKMDT_MONITOR_SOURCE_MODE* pMonitorSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORSOURCEMODESET_RELEASEMODEINFO)(
     _In_ const D3DKMDT_HMONITORSOURCEMODESET hMonitorSourceModeSet,
     _In_ const D3DKMDT_MONITOR_SOURCE_MODE* pMonitorSourceModeInfo);

typedef struct _DXGK_MONITORSOURCEMODESET_INTERFACE
{
    DXGKDDI_MONITORSOURCEMODESET_GETNUMMODES pfnGetNumModes;
    DXGKDDI_MONITORSOURCEMODESET_ACQUIREPREFERREDMODEINFO pfnAcquirePreferredModeInfo;
    DXGKDDI_MONITORSOURCEMODESET_ACQUIREFIRSTMODEINFO pfnAcquireFirstModeInfo;
    DXGKDDI_MONITORSOURCEMODESET_ACQUIRENEXTMODEINFO pfnAcquireNextModeInfo;
    DXGKDDI_MONITORSOURCEMODESET_CREATENEWMODEINFO pfnCreateNewModeInfo;
    DXGKDDI_MONITORSOURCEMODESET_ADDMODE pfnAddMode;
    DXGKDDI_MONITORSOURCEMODESET_RELEASEMODEINFO pfnReleaseModeInfo;
} DXGK_MONITORSOURCEMODESET_INTERFACE, *PDXGK_MONITORSOURCEMODESET_INTERFACE;

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORFREQUENCYRANGESET_GETNUMFREQUENCYRANGES)(
    _In_ const D3DKMDT_HMONITORFREQUENCYRANGESET hMonitorFrequencyRangeSet,
    _Out_ const PSIZE_T pNumMonitorFrequencyRanges);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORFREQUENCYRANGESET_ACQUIREFIRSTFREQUENCYRANGEINFO)(
    _In_ const D3DKMDT_HMONITORFREQUENCYRANGESET hMonitorFrequencyRangeSet,
    _Outptr_ const D3DKMDT_MONITOR_FREQUENCY_RANGE** ppFirstMonitorFrequencyRangeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORFREQUENCYRANGESET_ACQUIRENEXTFREQUENCYRANGEINFO)(
    _In_ const D3DKMDT_HMONITORFREQUENCYRANGESET hMonitorFrequencyRangeSet,
    _In_ const D3DKMDT_MONITOR_FREQUENCY_RANGE* pMonitorFrequencyRangeInfo,
    _Outptr_ const D3DKMDT_MONITOR_FREQUENCY_RANGE** ppNextMonitorFrequencyRangeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORFREQUENCYRANGESET_RELEASEFREQUENCYRANGEINFO)(
     _In_ const D3DKMDT_HMONITORFREQUENCYRANGESET hMonitorFrequencyRangeSet,
     _In_ const D3DKMDT_MONITOR_FREQUENCY_RANGE* pMonitorFrequencyRangeInfo);

typedef struct _DXGK_MONITORFREQUENCYRANGESET_INTERFACE
{
    DXGKDDI_MONITORFREQUENCYRANGESET_GETNUMFREQUENCYRANGES pfnGetNumFrequencyRanges;
    DXGKDDI_MONITORFREQUENCYRANGESET_ACQUIREFIRSTFREQUENCYRANGEINFO pfnAcquireFirstFrequencyRangeInfo;
    DXGKDDI_MONITORFREQUENCYRANGESET_ACQUIRENEXTFREQUENCYRANGEINFO pfnAcquireNextFrequencyRangeInfo;
    DXGKDDI_MONITORFREQUENCYRANGESET_RELEASEFREQUENCYRANGEINFO pfnReleaseFrequencyRangeInfo;
} DXGK_MONITORFREQUENCYRANGESET_INTERFACE, *PDXGK_MONITORFREQUENCYRANGESET_INTERFACE;

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORDESCRIPTORSET_GETNUMDESCRIPTORS)(
    _In_ const D3DKMDT_HMONITORDESCRIPTORSET hMonitorDescriptorSet,
    _Out_ const PSIZE_T pNumMonitorDescriptors);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORDESCRIPTORSET_ACQUIREFIRSTDESCRIPTORINFO)(
    _In_ const D3DKMDT_HMONITORDESCRIPTORSET hMonitorDescriptorSet,
    _Outptr_ const D3DKMDT_MONITOR_DESCRIPTOR** ppFirstMonitorDescriptorInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORDESCRIPTORSET_ACQUIRENEXTDESCRIPTORINFO)(
    _In_ const D3DKMDT_HMONITORDESCRIPTORSET hMonitorDescriptorSet,
    _In_ const D3DKMDT_MONITOR_DESCRIPTOR* pMonitorDescriptorInfo,
    _Outptr_ const D3DKMDT_MONITOR_DESCRIPTOR** ppNextMonitorDescriptorInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORDESCRIPTORSET_RELEASEDESCRIPTORINFO)(
     _In_ const D3DKMDT_HMONITORDESCRIPTORSET hMonitorDescriptorSet,
     _In_ const D3DKMDT_MONITOR_DESCRIPTOR* pMonitorDescriptorInfo);

typedef struct _DXGK_MONITORDESCRIPTORSET_INTERFACE
{
    DXGKDDI_MONITORDESCRIPTORSET_GETNUMDESCRIPTORS pfnGetNumDescriptors;
    DXGKDDI_MONITORDESCRIPTORSET_ACQUIREFIRSTDESCRIPTORINFO pfnAcquireFirstDescriptorInfo;
    DXGKDDI_MONITORDESCRIPTORSET_ACQUIRENEXTDESCRIPTORINFO pfnAcquireNextDescriptorInfo;
    DXGKDDI_MONITORDESCRIPTORSET_RELEASEDESCRIPTORINFO pfnReleaseDescriptorInfo;
} DXGK_MONITORDESCRIPTORSET_INTERFACE, *PDXGK_MONITORDESCRIPTORSET_INTERFACE;

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITOR_ACQUIREMONITORSOURCEMODESET)(
    _In_ const D3DKMDT_ADAPTER hAdapter,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID VideoPresentTargetId,
    _Out_ D3DKMDT_HMONITORSOURCEMODESET* phMonitorSourceModeSet,
    _Outptr_ const PDXGK_MONITORSOURCEMODESET_INTERFACE* ppMonitorSourceModeSetInterface);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITOR_RELEASEMONITORSOURCEMODESET)(
    _In_ const D3DKMDT_ADAPTER hAdapter,
    _In_ const D3DKMDT_HMONITORSOURCEMODESET hMonitorSourceModeSet);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITOR_GETMONITORFREQUENCYRANGESET)(
    _In_ const D3DKMDT_ADAPTER hAdapter,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID VideoPresentTargetId,
    _Out_ D3DKMDT_HMONITORFREQUENCYRANGESET* phMonitorFrequencyRangeSet,
    _Outptr_ const PDXGK_MONITORFREQUENCYRANGESET_INTERFACE* ppMonitorFrequencyRangeSetInterface);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITOR_GETMONITORDESCRIPTORSET)(
    _In_ const D3DKMDT_ADAPTER hAdapter,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID VideoPresentTargetId,
    _Out_ D3DKMDT_HMONITORDESCRIPTORSET* phMonitorDescriptorSet,
    _Outptr_ const PDXGK_MONITORDESCRIPTORSET_INTERFACE* ppMonitorDescriptorSetInterface);

typedef struct _DXGK_MONITOR_INTERFACE
{
    DXGK_MONITOR_INTERFACE_VERSION Version;
    DXGKDDI_MONITOR_ACQUIREMONITORSOURCEMODESET pfnAcquireMonitorSourceModeSet;
    DXGKDDI_MONITOR_RELEASEMONITORSOURCEMODESET pfnReleaseMonitorSourceModeSet;
    DXGKDDI_MONITOR_GETMONITORFREQUENCYRANGESET pfnGetMonitorFrequencyRangeSet;
    DXGKDDI_MONITOR_GETMONITORDESCRIPTORSET pfnGetMonitorDescriptorSet;
}
DXGK_MONITOR_INTERFACE, *PDXGK_MONITOR_INTERFACE;

typedef
VOID*
(APIENTRY *DXGKCB_GETHANDLEDATA)(IN_CONST_PDXGKARGCB_GETHANDLEDATA);

typedef
D3DKMT_HANDLE
(APIENTRY *DXGKCB_GETHANDLEPARENT)(_In_ D3DKMT_HANDLE hAllocation);

typedef
D3DKMT_HANDLE
(APIENTRY *DXGKCB_ENUMHANDLECHILDREN)(IN_CONST_PDXGKARGCB_ENUMHANDLECHILDREN);

typedef
VOID
(APIENTRY *DXGKCB_NOTIFY_INTERRUPT)(
    _In_ const HANDLE hAdapter, IN_CONST_PDXGKARGCB_NOTIFY_INTERRUPT_DATA);

typedef
VOID
(APIENTRY *DXGKCB_NOTIFY_DPC)(
    _In_ const HANDLE hAdapter);

typedef
NTSTATUS
(APIENTRY *DXGKCB_QUERYVIDPNINTERFACE)(
    _In_ const D3DKMDT_HVIDPN hVidPn,
    _In_ const DXGK_VIDPN_INTERFACE_VERSION VidPnInterfaceVersion,
    _Outptr_ const DXGK_VIDPN_INTERFACE** ppVidPnInterface);

typedef
NTSTATUS
(APIENTRY *DXGKCB_QUERYMONITORINTERFACE)(
    _In_ const HANDLE hAdapter,
    _In_ const DXGK_MONITOR_INTERFACE_VERSION MonitorInterfaceVersion,
    _Outptr_ const DXGK_MONITOR_INTERFACE** ppMonitorInterface);

typedef
NTSTATUS
(APIENTRY *DXGKCB_GETCAPTUREADDRESS)(_Inout_ PDXGKARGCB_GETCAPTUREADDRESS);

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)

typedef struct _DXGK_CREATECONTEXTALLOCATIONFLAGS
{
    union
    {
        struct
        {
            UINT SharedAcrossContexts : 1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
            UINT MapGpuVirtualAddress : 1;
            UINT Reserved : 30;
#else
            UINT Reserved : 31;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
        };
        UINT Value;
    };
} DXGK_CREATECONTEXTALLOCATIONFLAGS;

typedef struct _DXGKARGCB_CREATECONTEXTALLOCATION
{
    DXGK_CREATECONTEXTALLOCATIONFLAGS ContextAllocationFlags;
    HANDLE hAdapter;
    HANDLE hDevice;
    HANDLE hContext;
    HANDLE hDriverAllocation;
    SIZE_T Size;
    UINT Alignment;
    UINT SupportedSegmentSet;
    UINT EvictionSegmentSet;
    DXGK_SEGMENTPREFERENCE PreferredSegment;
    DXGK_SEGMENTBANKPREFERENCE HintedBank;
    DXGK_ALLOCATIONINFOFLAGS Flags;
    HANDLE hAllocation;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    UINT PhysicalAdapterIndex;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
} DXGKARGCB_CREATECONTEXTALLOCATION, *PDXGKARGCB_CREATECONTEXTALLOCATION;

typedef
NTSTATUS
(APIENTRY *DXGKCB_CREATECONTEXTALLOCATION)(
    _Inout_ PDXGKARGCB_CREATECONTEXTALLOCATION
    );

typedef
NTSTATUS
(APIENTRY *DXGKCB_DESTROYCONTEXTALLOCATION)(
    _In_ const HANDLE hAdapter,
    _In_ const HANDLE hAllocation
    );

typedef
VOID
(APIENTRY *DXGKCB_SETPOWERCOMPONENTACTIVE)(
    _In_ const HANDLE hAdapter,
    _In_ UINT ComponentIndex
    );

typedef
NTSTATUS
(APIENTRY *DXGKCB_POWERRUNTIMECONTROLREQUEST)(
    _In_ const HANDLE hAdapter,
    _In_ const LPCGUID PowerControlCode,
    _In_opt_ PVOID InBuffer,
    _In_ SIZE_T InBufferSize,
    _Out_opt_ PVOID OutBuffer,
    _In_ SIZE_T OutBufferSize,
    _Out_opt_ PSIZE_T BytesReturned
    );

typedef
VOID
(APIENTRY *DXGKCB_SETPOWERCOMPONENTIDLE)(
    _In_ const HANDLE hAdapter,
    _In_ UINT ComponentIndex
    );

typedef
VOID
(APIENTRY *DXGKCB_SETPOWERCOMPONENTLATENCY)(
    _In_ const HANDLE hAdapter,
    _In_ UINT ComponentIndex,
    _In_ ULONGLONG Latency
    );

typedef
VOID
(APIENTRY *DXGKCB_SETPOWERCOMPONENTRESIDENCY)(
    _In_ const HANDLE hAdapter,
    _In_ UINT ComponentIndex,
    _In_ ULONGLONG Residency
    );

typedef
VOID
(APIENTRY *DXGKCB_COMPLETEFSTATETRANSITION)(
    _In_ const HANDLE hAdapter,
    _In_ UINT ComponentIndex
    );

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)

typedef
VOID
(APIENTRY *DXGKCB_COMPLETEPSTATETRANSITION)(
    _In_ const HANDLE hAdapter,
    _In_ UINT ComponentIndex,
    _In_ UINT CompletedPState
    );

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

typedef VOID* DXGKARG_RELEASE_HANDLE;
typedef DXGKARG_RELEASE_HANDLE* PDXGKARG_RELEASE_HANDLE;

typedef
VOID*
(APIENTRY *DXGKCB_ACQUIREHANDLEDATA)(
    _In_ const PDXGKARGCB_GETHANDLEDATA pGetHandleData,
    _Out_ PDXGKARG_RELEASE_HANDLE pReleaseHandle
    );

typedef struct _DXGKARGCB_RELEASEHANDLEDATA
{
    DXGKARG_RELEASE_HANDLE ReleaseHandle;
    DXGK_HANDLE_TYPE Type;
} DXGKARGCB_RELEASEHANDLEDATA;

typedef
VOID
(APIENTRY *DXGKCB_RELEASEHANDLEDATA)(
    _In_ const DXGKARGCB_RELEASEHANDLEDATA pArgs
    );

typedef struct _DXGKARGCB_MAPCONTEXTALLOCATION
{
    D3DGPU_VIRTUAL_ADDRESS BaseAddress;
    D3DGPU_VIRTUAL_ADDRESS MinimumAddress;
    D3DGPU_VIRTUAL_ADDRESS MaximumAddress;
    HANDLE hAllocation;
    D3DGPU_SIZE_T OffsetInPages;
    D3DGPU_SIZE_T SizeInPages;
    D3DDDIGPUVIRTUALADDRESS_PROTECTION_TYPE Protection;
    UINT64 DriverProtection;
} DXGKARGCB_MAPCONTEXTALLOCATION;

typedef
D3DGPU_VIRTUAL_ADDRESS
(APIENTRY *DXGKCB_MAPCONTEXTALLOCATION)(
    _In_ const HANDLE hAdapter,
    _In_ const DXGKARGCB_MAPCONTEXTALLOCATION* pArgs
    );

typedef struct _DXGKARGCB_UPDATECONTEXTALLOCATION
{
    HANDLE hAllocation;
    PVOID pPrivateDriverData;
    UINT PrivateDriverDataSize;
} DXGKARGCB_UPDATECONTEXTALLOCATION;

typedef
NTSTATUS
(APIENTRY *DXGKCB_UPDATECONTEXTALLOCATION)(
    _In_ const HANDLE hAdapter,
    _In_ const DXGKARGCB_UPDATECONTEXTALLOCATION* pArgs
    );

typedef struct _DXGKARGCB_RESERVEGPUVIRTUALADDRESSRANGE
{
    HANDLE hDxgkProcess;
    UINT64 SizeInBytes;
    UINT Alignment;
    UINT64 StartVirtualAddress;
    UINT64 BaseAddress;
    union
    {
        struct
        {
            UINT AllowUserModeMapping : 1;
        };
        UINT Flags;
    };
} DXGKARGCB_RESERVEGPUVIRTUALADDRESSRANGE;

typedef
NTSTATUS
(APIENTRY *DXGKCB_RESERVEGPUVIRTUALADDRESSRANGE)(
    _In_ const HANDLE hAdapter,
    _Inout_ DXGKARGCB_RESERVEGPUVIRTUALADDRESSRANGE* pArgs
    );

typedef enum _DXGK_HARDWARE_CONTENT_PROTECTION_TEARDOWN_FLAGS
{
    DXGK_HARDWARE_CONTENT_PROTECTION_TEARDOWN_FLAG_PREEMPTIVE = 1
} DXGK_HARDWARE_CONTENT_PROTECTION_TEARDOWN_FLAGS;

typedef
VOID
(APIENTRY *DXGKCB_HARDWARECONTENTPROTECTIONTEARDOWN)(
    _In_ const HANDLE hAdapter,
    _In_ UINT Flags
    );

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
typedef struct _DXGK_QUERYADAPTERINFOFLAGS
{
    union
    {
        struct
        {
            UINT VirtualMachineData : 1;
            UINT SecureVirtualMachine : 1;
            UINT Reserved : 30;
        };
        UINT Value;
    };
 } DXGK_QUERYADAPTERINFOFLAGS;
 #endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

typedef struct _DXGKARG_QUERYADAPTERINFO
{
    DXGK_QUERYADAPTERINFOTYPE Type;
    VOID* pInputData;
    UINT InputDataSize;
    VOID* pOutputData;
    UINT OutputDataSize;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
    DXGK_QUERYADAPTERINFOFLAGS Flags;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
    HANDLE hKmdProcessHandle;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
} DXGKARG_QUERYADAPTERINFO, *PDXGKARG_QUERYADAPTERINFO;

typedef struct _DXGK_DEVICEINFOFLAGS
{
    union
    {
        struct
        {
            UINT GuaranteedDmaBufferContract : 1;
            UINT Reserved : 31;
        };
        UINT Value;
    };
 } DXGK_DEVICEINFOFLAGS;

typedef struct _DXGK_DEVICEINFO
{
    UINT DmaBufferSize;
    UINT DmaBufferSegmentSet;
    UINT DmaBufferPrivateDataSize;
    UINT AllocationListSize;
    UINT PatchLocationListSize;
    DXGK_DEVICEINFOFLAGS Flags;
} DXGK_DEVICEINFO;

typedef struct _DXGK_CREATEDEVICEFLAGS
{
    union
    {
        struct
        {
            UINT SystemDevice : 1;
            UINT GdiDevice : 1;
            UINT Reserved : 29;
            UINT DXGK_DEVICE_RESERVED0 : 1;
        };
        UINT Value;
    };
} DXGK_CREATEDEVICEFLAGS;

typedef struct _DXGKARG_CREATEDEVICE
{
    HANDLE hDevice;
    union
    {
        DXGK_CREATEDEVICEFLAGS Flags;
        DXGK_DEVICEINFO* pInfo;
    };
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    ULONG Pasid;
    HANDLE hKmdProcess;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
} DXGKARG_CREATEDEVICE, *PDXGKARG_CREATEDEVICE;

typedef
NTSTATUS
APIENTRY
DXGKDDI_QUERYADAPTERINFO(
    _In_ const HANDLE hAdapter,
    _In_ const DXGKARG_QUERYADAPTERINFO* pQueryAdapterInfo);

typedef
NTSTATUS
APIENTRY
DXGKDDI_CREATEDEVICE(_In_ const HANDLE hAdapter,
                     _Inout_ PDXGKARG_CREATEDEVICE pCreateDevice);

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7)

typedef struct _DXGKARG_QUERYVIDPNHWCAPABILITY
{
    _In_ D3DKMDT_HVIDPN hFunctionalVidPn;
    _In_ D3DDDI_VIDEO_PRESENT_SOURCE_ID SourceId;
    _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId;
    _Out_ D3DKMDT_VIDPN_HW_CAPABILITY VidPnHWCaps;
} DXGKARG_QUERYVIDPNHWCAPABILITY;

typedef
_Check_return_
NTSTATUS
APIENTRY
DXGKDDI_QUERYVIDPNHWCAPABILITY(
    _In_ const HANDLE hAdapter,
    _Inout_ DXGKARG_QUERYVIDPNHWCAPABILITY* pVidPnHWCaps);

typedef DXGKDDI_QUERYVIDPNHWCAPABILITY *PDXGKDDI_QUERYVIDPNHWCAPABILITY;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7)

typedef DXGKDDI_QUERYADAPTERINFO *PDXGKDDI_QUERYADAPTERINFO;
typedef DXGKDDI_CREATEDEVICE *PDXGKDDI_CREATEDEVICE;
#if 0
typedef DXGKDDI_DESTROYALLOCATION *PDXGKDDI_DESTROYALLOCATION;
typedef DXGKDDI_DESCRIBEALLOCATION *PDXGKDDI_DESCRIBEALLOCATION;
typedef DXGKDDI_GETSTANDARDALLOCATIONDRIVERDATA *PDXGKDDI_GETSTANDARDALLOCATIONDRIVERDATA;
typedef DXGKDDI_ACQUIRESWIZZLINGRANGE *PDXGKDDI_ACQUIRESWIZZLINGRANGE;
typedef DXGKDDI_RELEASESWIZZLINGRANGE *PDXGKDDI_RELEASESWIZZLINGRANGE;
typedef DXGKDDI_PATCH *PDXGKDDI_PATCH;
typedef DXGKDDI_SUBMITCOMMAND *PDXGKDDI_SUBMITCOMMAND;
typedef DXGKDDI_PREEMPTCOMMAND *PDXGKDDI_PREEMPTCOMMAND;
typedef DXGKDDI_CANCELCOMMAND *PDXGKDDI_CANCELCOMMAND;
typedef DXGKDDI_BUILDPAGINGBUFFER *PDXGKDDI_BUILDPAGINGBUFFER;
typedef DXGKDDI_SETPALETTE *PDXGKDDI_SETPALETTE;
typedef DXGKDDI_SETPOINTERPOSITION *PDXGKDDI_SETPOINTERPOSITION;
typedef DXGKDDI_SETPOINTERSHAPE *PDXGKDDI_SETPOINTERSHAPE;
typedef DXGKDDI_RESETFROMTIMEOUT *PDXGKDDI_RESETFROMTIMEOUT;
typedef DXGKDDI_RESTARTFROMTIMEOUT *PDXGKDDI_RESTARTFROMTIMEOUT;
typedef DXGKDDI_ESCAPE *PDXGKDDI_ESCAPE;
typedef DXGKDDI_COLLECTDBGINFO *PDXGKDDI_COLLECTDBGINFO;
typedef DXGKDDI_QUERYCURRENTFENCE *PDXGKDDI_QUERYCURRENTFENCE;
typedef DXGKDDI_ISSUPPORTEDVIDPN *PDXGKDDI_ISSUPPORTEDVIDPN;
typedef DXGKDDI_RECOMMENDFUNCTIONALVIDPN *PDXGKDDI_RECOMMENDFUNCTIONALVIDPN;
typedef DXGKDDI_ENUMVIDPNCOFUNCMODALITY *PDXGKDDI_ENUMVIDPNCOFUNCMODALITY;
typedef DXGKDDI_SETVIDPNSOURCEADDRESS *PDXGKDDI_SETVIDPNSOURCEADDRESS;
typedef DXGKDDI_SETVIDPNSOURCEVISIBILITY *PDXGKDDI_SETVIDPNSOURCEVISIBILITY;
typedef DXGKDDI_COMMITVIDPN *PDXGKDDI_COMMITVIDPN;
typedef DXGKDDI_UPDATEACTIVEVIDPNPRESENTPATH *PDXGKDDI_UPDATEACTIVEVIDPNPRESENTPATH;
typedef DXGKDDI_RECOMMENDMONITORMODES *PDXGKDDI_RECOMMENDMONITORMODES;
typedef DXGKDDI_RECOMMENDVIDPNTOPOLOGY *PDXGKDDI_RECOMMENDVIDPNTOPOLOGY;
typedef DXGKDDI_GETSCANLINE *PDXGKDDI_GETSCANLINE;
typedef DXGKDDI_STOPCAPTURE *PDXGKDDI_STOPCAPTURE;
typedef DXGKDDI_CONTROLINTERRUPT *PDXGKDDI_CONTROLINTERRUPT;
typedef DXGKDDI_CREATEOVERLAY *PDXGKDDI_CREATEOVERLAY;
typedef DXGKDDI_DESTROYDEVICE *PDXGKDDI_DESTROYDEVICE;
typedef DXGKDDI_OPENALLOCATIONINFO *PDXGKDDI_OPENALLOCATIONINFO;
typedef DXGKDDI_CLOSEALLOCATION *PDXGKDDI_CLOSEALLOCATION;
typedef DXGKDDI_RENDER *PDXGKDDI_RENDER;
typedef DXGKDDI_PRESENT *PDXGKDDI_PRESENT;
typedef DXGKDDI_UPDATEOVERLAY *PDXGKDDI_UPDATEOVERLAY;
typedef DXGKDDI_FLIPOVERLAY *PDXGKDDI_FLIPOVERLAY;
typedef DXGKDDI_DESTROYOVERLAY *PDXGKDDI_DESTROYOVERLAY;
typedef DXGKDDI_CREATECONTEXT *PDXGKDDI_CREATECONTEXT;
typedef DXGKDDI_DESTROYCONTEXT *PDXGKDDI_DESTROYCONTEXT;
typedef DXGKDDI_SETDISPLAYPRIVATEDRIVERFORMAT *PDXGKDDI_SETDISPLAYPRIVATEDRIVERFORMAT;
#endif
/*
 * NOTE:
 * dispmprt.h's DRIVER_INITIALIZATION_DATA relies on a large set of PDXGKDDI_*
 * function pointer typedefs.
 *
 * Historically this header used UINT32* placeholders for many of these types.
 * That is dangerous because it silently turns function pointers into data
 * pointers, obscuring ABI and making debugging/interop harder (even if pointer
 * sizes match on x64).
 *
 * Only keep placeholders for DDIs we truly don't model yet. For DDIs we do
 * model (Escape, OpenAllocation, Present, CreateContext, DestroyContext, etc)
 * we must NOT override the real prototypes.
 */

typedef UINT32 *PDXGKDDI_DESTROYALLOCATION;
typedef UINT32 *PDXGKDDI_DESCRIBEALLOCATION;
typedef UINT32 *PDXGKDDI_GETSTANDARDALLOCATIONDRIVERDATA;
typedef UINT32 *PDXGKDDI_ACQUIRESWIZZLINGRANGE;
typedef UINT32 *PDXGKDDI_RELEASESWIZZLINGRANGE;
typedef UINT32 *PDXGKDDI_PATCH;
typedef UINT32 *PDXGKDDI_SUBMITCOMMAND;
typedef UINT32 *PDXGKDDI_PREEMPTCOMMAND;
typedef UINT32 *PDXGKDDI_CANCELCOMMAND;
typedef UINT32 *PDXGKDDI_BUILDPAGINGBUFFER;
typedef UINT32 *PDXGKDDI_SETPALETTE;
typedef
_Check_return_
NTSTATUS
(APIENTRY *PDXGKDDI_SETPOINTERPOSITION)(
    _In_ const HANDLE hAdapter,
    _In_ const DXGKARG_SETPOINTERPOSITION* pSetPointerPosition
    );

typedef
_Check_return_
NTSTATUS
(APIENTRY *PDXGKDDI_SETPOINTERSHAPE)(
    _In_ const HANDLE hAdapter,
    _In_ const DXGKARG_SETPOINTERSHAPE* pSetPointerShape
    );

typedef UINT32 *PDXGKDDI_RESETFROMTIMEOUT;
typedef UINT32 *PDXGKDDI_RESTARTFROMTIMEOUT;
typedef UINT32 *PDXGKDDI_COLLECTDBGINFO;
typedef UINT32 *PDXGKDDI_QUERYCURRENTFENCE;

typedef
_Check_return_
NTSTATUS
(APIENTRY *PDXGKDDI_ISSUPPORTEDVIDPN)(
    _In_ const HANDLE hAdapter,
    _Inout_ DXGKARG_ISSUPPORTEDVIDPN* pIsSupportedVidPn
    );

typedef
_Check_return_
NTSTATUS
(APIENTRY *PDXGKDDI_RECOMMENDFUNCTIONALVIDPN)(
    _In_ const HANDLE hAdapter,
    _In_ const DXGKARG_RECOMMENDFUNCTIONALVIDPN* pRecommendFunctionalVidPn
    );

typedef
_Check_return_
NTSTATUS
(APIENTRY *PDXGKDDI_ENUMVIDPNCOFUNCMODALITY)(
    _In_ const HANDLE hAdapter,
    _In_ IN_CONST_PDXGKARG_ENUMVIDPNCOFUNCMODALITY_CONST pEnumCofuncModality
    );
typedef
_Check_return_
NTSTATUS
(APIENTRY *PDXGKDDI_SETVIDPNSOURCEADDRESS)(
    _In_ const HANDLE hAdapter,
    _In_ const DXGKARG_SETVIDPNSOURCEADDRESS* pSetVidPnSourceAddress
    );
typedef
_Check_return_
NTSTATUS
(APIENTRY *PDXGKDDI_SETVIDPNSOURCEVISIBILITY)(
    _In_ const HANDLE hAdapter,
    _In_ const DXGKARG_SETVIDPNSOURCEVISIBILITY* pSetVidPnSourceVisibility
    );

typedef
_Check_return_
NTSTATUS
(APIENTRY *PDXGKDDI_COMMITVIDPN)(
    _In_ const HANDLE hAdapter,
    _In_ const DXGKARG_COMMITVIDPN* pCommitVidPn
    );

typedef
_Check_return_
NTSTATUS
(APIENTRY *PDXGKDDI_UPDATEACTIVEVIDPNPRESENTPATH)(
    _In_ const HANDLE hAdapter,
    _In_ const DXGKARG_UPDATEACTIVEVIDPNPRESENTPATH* pUpdateActiveVidPnPresentPath
    );

typedef
_Check_return_
NTSTATUS
(APIENTRY *PDXGKDDI_RECOMMENDMONITORMODES)(
    _In_ const HANDLE hAdapter,
    _In_ const DXGKARG_RECOMMENDMONITORMODES* pRecommendMonitorModes
    );

typedef
_Check_return_
NTSTATUS
(APIENTRY *PDXGKDDI_RECOMMENDVIDPNTOPOLOGY)(
    _In_ const HANDLE hAdapter,
    _In_ const DXGKARG_RECOMMENDVIDPNTOPOLOGY* pRecommendVidPnTopology
    );

typedef UINT32 *PDXGKDDI_GETSCANLINE;
typedef UINT32 *PDXGKDDI_STOPCAPTURE;
typedef UINT32 *PDXGKDDI_CONTROLINTERRUPT;
typedef UINT32 *PDXGKDDI_CREATEOVERLAY;
typedef UINT32 *PDXGKDDI_DESTROYDEVICE;
typedef UINT32 *PDXGKDDI_CLOSEALLOCATION;
typedef UINT32 *PDXGKDDI_UPDATEOVERLAY;
typedef UINT32 *PDXGKDDI_FLIPOVERLAY;
typedef UINT32 *PDXGKDDI_DESTROYOVERLAY;
typedef UINT32 *PDXGKDDI_SETDISPLAYPRIVATEDRIVERFORMAT;

/*
 * Real prototypes required by dispmprt.h's DRIVER_INITIALIZATION_DATA:
 * Define these as function pointer types (NOT UINT32* placeholders).
 * We only need forward declarations for the payload structs here.
 */

typedef struct _DXGKARG_OPENALLOCATION DXGKARG_OPENALLOCATION;
typedef struct _DXGKARG_PRESENT DXGKARG_PRESENT;
typedef struct _DXGKARG_RENDER DXGKARG_RENDER;
typedef struct _DXGKARG_CREATECONTEXT DXGKARG_CREATECONTEXT;

typedef
_Check_return_
NTSTATUS
APIENTRY
DXGKDDI_OPENALLOCATIONINFO(
    _In_ const HANDLE hDevice,
    _In_ const DXGKARG_OPENALLOCATION* pOpenAllocation
    );
typedef DXGKDDI_OPENALLOCATIONINFO *PDXGKDDI_OPENALLOCATIONINFO;

typedef
_Check_return_
NTSTATUS
APIENTRY
DXGKDDI_PRESENT(
    _In_ const HANDLE hContext,
    _Inout_ DXGKARG_PRESENT* pPresent
    );
typedef DXGKDDI_PRESENT *PDXGKDDI_PRESENT;

typedef
_Check_return_
NTSTATUS
APIENTRY
DXGKDDI_RENDER(
    _In_ const HANDLE hContext,
    _Inout_ DXGKARG_RENDER* pRender
    );
typedef DXGKDDI_RENDER *PDXGKDDI_RENDER;

typedef DXGKDDI_RENDER *PDXGKDDI_RENDERKM;

typedef
_Check_return_
NTSTATUS
APIENTRY
DXGKDDI_CREATECONTEXT(
    _In_ const HANDLE hDevice,
    _Inout_ DXGKARG_CREATECONTEXT* pCreateContext
    );
typedef DXGKDDI_CREATECONTEXT *PDXGKDDI_CREATECONTEXT;

typedef
_Check_return_
NTSTATUS
APIENTRY
DXGKDDI_DESTROYCONTEXT(
    _In_ const HANDLE hContext
    );
typedef DXGKDDI_DESTROYCONTEXT *PDXGKDDI_DESTROYCONTEXT;

/*
 * TODO:
 * dispmprt.h's DRIVER_INITIALIZATION_DATA references a number of additional
 * WDDM DDI entrypoints whose full prototypes are not yet declared yet.
 */

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
typedef VOID* PDXGKDDISETPOWERCOMPONENTFSTATE;
typedef VOID* PDXGKDDI_QUERYDEPENDENTENGINEGROUP;
typedef VOID* PDXGKDDI_QUERYENGINESTATUS;
typedef VOID* PDXGKDDI_RESETENGINE;
typedef VOID* PDXGKDDIPOWERRUNTIMECONTROLREQUEST;
typedef VOID* PDXGKDDI_SETVIDPNSOURCEADDRESSWITHMULTIPLANEOVERLAY;
#endif


#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
typedef VOID* PDXGKDDI_GETNODEMETADATA;
typedef VOID* PDXGKDDISETPOWERPSTATE;
typedef VOID* PDXGKDDI_CONTROLINTERRUPT2;
typedef VOID* PDXGKDDI_CHECKMULTIPLANEOVERLAYSUPPORT;
typedef VOID* PDXGKDDI_CALIBRATEGPUCLOCK;
typedef VOID* PDXGKDDI_FORMATHISTORYBUFFER;
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
typedef VOID* PDXGKDDI_RENDERGDI;
typedef VOID* PDXGKDDI_SUBMITCOMMANDVIRTUAL;
typedef VOID* PDXGKDDI_SETROOTPAGETABLE;
typedef VOID* PDXGKDDI_GETROOTPAGETABLESIZE;
typedef VOID* PDXGKDDI_MAPCPUHOSTAPERTURE;
typedef VOID* PDXGKDDI_UNMAPCPUHOSTAPERTURE;
typedef VOID* PDXGKDDI_CHECKMULTIPLANEOVERLAYSUPPORT2;
typedef VOID* PDXGKDDI_CREATEPROCESS;
typedef VOID* PDXGKDDI_DESTROYPROCESS;
typedef VOID* PDXGKDDI_SETVIDPNSOURCEADDRESSWITHMULTIPLANEOVERLAY2;
typedef VOID* PDXGKDDI_POWERRUNTIMESETDEVICEHANDLE;
typedef VOID* PDXGKDDI_SETSTABLEPOWERSTATE;
typedef VOID* PDXGKDDI_SETVIDEOPROTECTEDREGION;
#endif
#endif // _D3DKMDDI_H_
