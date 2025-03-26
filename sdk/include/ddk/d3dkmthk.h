/******************************Module*Header**********************************\
*
* Module Name: d3dkmthk.h
*
* Content: Windows Display Driver Model (WDDM) kernel mode thunk interfaces
*
* Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*
\*****************************************************************************/
#ifndef _D3DKMTHK_H_
#define _D3DKMTHK_H_

#include <d3dkmdt.h>

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#pragma warning(push)
#pragma warning(disable:4201) // anonymous unions warning
#pragma warning(disable:4200) // zero-sized array in struct/union
#pragma warning(disable:4214)   // nonstandard extension used: bit field types other than int


typedef struct _OBJECT_ATTRIBUTES OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;

//
// Available only for Vista (LONGHORN) and later and for
// multiplatform tools such as debugger extensions
//
#if defined(__REACTOS__) || ((NTDDI_VERSION >= NTDDI_LONGHORN) || defined(D3DKMDT_SPECIAL_MULTIPLATFORM_TOOL))

typedef struct _D3DKMT_CREATEDEVICEFLAGS
{
    UINT    LegacyMode               :  1;   // 0x00000001
    UINT    RequestVSync             :  1;   // 0x00000002
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    UINT    DisableGpuTimeout        :  1;   // 0x00000004
    UINT    Reserved                 : 29;   // 0xFFFFFFF8
#else
    UINT    Reserved                 : 30;   // 0xFFFFFFFC
#endif
} D3DKMT_CREATEDEVICEFLAGS;

typedef struct _D3DKMT_CREATEDEVICE
{
    union
    {
        D3DKMT_HANDLE           hAdapter;           // in: identifies the adapter for user-mode creation
        VOID*                   pAdapter;           // in: identifies the adapter for kernel-mode creation
        D3DKMT_PTR_HELPER(pAdapter_Align)
    };

    D3DKMT_CREATEDEVICEFLAGS    Flags;

    D3DKMT_HANDLE               hDevice;                // out: Identifies the device
    D3DKMT_PTR(VOID*,           pCommandBuffer);        // out: D3D10 compatibility.
    UINT                        CommandBufferSize;      // out: D3D10 compatibility.
    D3DKMT_PTR(D3DDDI_ALLOCATIONLIST*, pAllocationList); // out: D3D10 compatibility.
    UINT                        AllocationListSize;     // out: D3D10 compatibility.
    D3DKMT_PTR(D3DDDI_PATCHLOCATIONLIST*, pPatchLocationList); // out: D3D10 compatibility.
    UINT                        PatchLocationListSize;  // out: D3D10 compatibility.
} D3DKMT_CREATEDEVICE;

typedef struct _D3DKMT_DESTROYDEVICE
{
    D3DKMT_HANDLE     hDevice;              // in: Indentifies the device
}D3DKMT_DESTROYDEVICE;

typedef enum _D3DKMT_CLIENTHINT
{
    D3DKMT_CLIENTHINT_UNKNOWN        = 0,
    D3DKMT_CLIENTHINT_OPENGL         = 1,
    D3DKMT_CLIENTHINT_CDD            = 2,       // Internal
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
    D3DKMT_CLIENTHINT_MAX
} D3DKMT_CLIENTHINT;

typedef struct _D3DKMT_CREATECONTEXT
{
    D3DKMT_HANDLE               hDevice;                    // in:  Handle to the device owning this context.
    UINT                        NodeOrdinal;                // in:  Identifier for the node targetted by this context.
    UINT                        EngineAffinity;             // in:  Engine affinity within the specified node.
    D3DDDI_CREATECONTEXTFLAGS   Flags;                      // in:  Context creation flags.
    D3DKMT_PTR(VOID*,           pPrivateDriverData);        // in:  Private driver data
    UINT                        PrivateDriverDataSize;      // in:  Size of private driver data
    D3DKMT_CLIENTHINT           ClientHint;                 // in:  Hints which client is creating this
    D3DKMT_HANDLE               hContext;                   // out: Handle of the created context.
    D3DKMT_PTR(VOID*,           pCommandBuffer);            // out: Pointer to the first command buffer.
    UINT                        CommandBufferSize;          // out: Command buffer size (bytes).
    D3DKMT_PTR(D3DDDI_ALLOCATIONLIST*, pAllocationList);    // out: Pointer to the first allocation list.
    UINT                        AllocationListSize;         // out: Allocation list size (elements).
    D3DKMT_PTR(D3DDDI_PATCHLOCATIONLIST*, pPatchLocationList); // out: Pointer to the first patch location list.
    UINT                        PatchLocationListSize;      // out: Patch location list size (elements).
    D3DGPU_VIRTUAL_ADDRESS      CommandBuffer;              // out: GPU virtual address of the command buffer. _ADVSCH_
} D3DKMT_CREATECONTEXT;

typedef struct _D3DKMT_DESTROYCONTEXT
{
    D3DKMT_HANDLE               hContext;                   // in:  Identifies the context being destroyed.
} D3DKMT_DESTROYCONTEXT;

typedef struct _D3DKMT_CREATESYNCHRONIZATIONOBJECT
{
    D3DKMT_HANDLE                           hDevice;        // in:  Handle to the device.
    D3DDDI_SYNCHRONIZATIONOBJECTINFO        Info;           // in:  Attributes of the synchronization object.
    D3DKMT_HANDLE                           hSyncObject;    // out: Handle to the synchronization object created.
} D3DKMT_CREATESYNCHRONIZATIONOBJECT;

typedef struct _D3DKMT_CREATESYNCHRONIZATIONOBJECT2
{
    D3DKMT_HANDLE                           hDevice;        // in:  Handle to the device.
    D3DDDI_SYNCHRONIZATIONOBJECTINFO2       Info;           // in/out: Attributes of the synchronization object.
    D3DKMT_HANDLE                           hSyncObject;    // out: Handle to the synchronization object created.
} D3DKMT_CREATESYNCHRONIZATIONOBJECT2;

typedef struct _D3DKMT_DESTROYSYNCHRONIZATIONOBJECT
{
    D3DKMT_HANDLE               hSyncObject;                // in:  Identifies the synchronization objects being destroyed.
} D3DKMT_DESTROYSYNCHRONIZATIONOBJECT;

typedef struct _D3DKMT_OPENSYNCHRONIZATIONOBJECT
{
    D3DKMT_HANDLE               hSharedHandle;              // in: shared handle to synchronization object to be opened.
    D3DKMT_HANDLE               hSyncObject;                // out: Handle to sync object in this process.

    D3DKMT_ALIGN64 UINT64       Reserved[8];
} D3DKMT_OPENSYNCHRONIZATIONOBJECT;

typedef struct _D3DKMT_WAITFORSYNCHRONIZATIONOBJECT
{
    D3DKMT_HANDLE             hContext;                   // in: Identifies the context that needs to wait.
    UINT                      ObjectCount;                // in: Specifies the number of object to wait on.
    D3DKMT_HANDLE             ObjectHandleArray[D3DDDI_MAX_OBJECT_WAITED_ON]; // in: Specifies the object to wait on.
} D3DKMT_WAITFORSYNCHRONIZATIONOBJECT;

typedef struct _D3DKMT_WAITFORSYNCHRONIZATIONOBJECT2
{
    D3DKMT_HANDLE             hContext;                   // in: Identifies the context that needs to wait.
    UINT                      ObjectCount;                // in: Specifies the number of object to wait on.
    D3DKMT_HANDLE             ObjectHandleArray[D3DDDI_MAX_OBJECT_WAITED_ON]; // in: Specifies the object to wait on.
    union
    {
        struct {
            D3DKMT_ALIGN64 UINT64 FenceValue;             // in: fence value to be waited.
        } Fence;
        D3DKMT_ALIGN64 UINT64     Reserved[8];
    };
} D3DKMT_WAITFORSYNCHRONIZATIONOBJECT2;

typedef struct _D3DKMT_SIGNALSYNCHRONIZATIONOBJECT
{
    D3DKMT_HANDLE             hContext;           // in: Identifies the context that needs to signal.
    UINT                      ObjectCount;        // in: Specifies the number of object to signal.
    D3DKMT_HANDLE             ObjectHandleArray[D3DDDI_MAX_OBJECT_SIGNALED]; // in: Specifies the object to be signaled.
    D3DDDICB_SIGNALFLAGS      Flags;                                         // in: Specifies signal behavior.
} D3DKMT_SIGNALSYNCHRONIZATIONOBJECT;

typedef struct _D3DKMT_SIGNALSYNCHRONIZATIONOBJECT2
{
    D3DKMT_HANDLE             hContext;           // in: Identifies the context that needs to signal.
    UINT                      ObjectCount;        // in: Specifies the number of object to signal.
    D3DKMT_HANDLE             ObjectHandleArray[D3DDDI_MAX_OBJECT_SIGNALED]; // in: Specifies the object to be signaled.
    D3DDDICB_SIGNALFLAGS      Flags;                  // in: Specifies signal behavior.
    ULONG                     BroadcastContextCount;  // in: Specifies the number of context
                                                      //     to broadcast this command buffer to.
    D3DKMT_HANDLE             BroadcastContext[D3DDDI_MAX_BROADCAST_CONTEXT]; // in: Specifies the handle of the context to
                                                                              //     broadcast to.
    union
    {
        struct {
            D3DKMT_ALIGN64 UINT64 FenceValue;             // in: fence value to be signaled;
        } Fence;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
        HANDLE                CpuEventHandle;         // in: handle of a CPU event to be signaled
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
        D3DKMT_ALIGN64 UINT64 Reserved[8];
    };
} D3DKMT_SIGNALSYNCHRONIZATIONOBJECT2;

typedef struct _D3DKMT_LOCK
{
    D3DKMT_HANDLE       hDevice;            // in: identifies the device
    D3DKMT_HANDLE       hAllocation;        // in: allocation to lock
                                            // out: New handle representing the allocation after the lock.
    UINT                PrivateDriverData;  // in: Used by UMD for AcquireAperture
    UINT                NumPages;
    D3DKMT_PTR(CONST UINT*, pPages);
    D3DKMT_PTR(VOID*,   pData);             // out: pointer to memory
    D3DDDICB_LOCKFLAGS  Flags;              // in: Bit field defined by D3DDDI_LOCKFLAGS
    D3DGPU_VIRTUAL_ADDRESS GpuVirtualAddress; // out: GPU's Virtual Address of locked allocation. _ADVSCH_
} D3DKMT_LOCK;

typedef struct _D3DKMT_UNLOCK
{
    D3DKMT_HANDLE           hDevice;        // in: Identifies the device
    UINT                    NumAllocations; // in: Number of allocations in the array
    D3DKMT_PTR(CONST D3DKMT_HANDLE*, phAllocations); // in: array of allocations to unlock
} D3DKMT_UNLOCK;

typedef enum _D3DKMDT_MODE_PRUNING_REASON
{
    D3DKMDT_MPR_UNINITIALIZED                               = 0, // mode was pruned or is supported because of:
    D3DKMDT_MPR_ALLCAPS                                     = 1, //   all of the monitor caps (only used to imply lack of support - for support, specific reason is always indicated)
    D3DKMDT_MPR_DESCRIPTOR_MONITOR_SOURCE_MODE              = 2, //   monitor source mode in the monitor descriptor
    D3DKMDT_MPR_DESCRIPTOR_MONITOR_FREQUENCY_RANGE          = 3, //   monitor frequency range in the monitor descriptor
    D3DKMDT_MPR_DESCRIPTOR_OVERRIDE_MONITOR_SOURCE_MODE     = 4, //   monitor source mode in the monitor descriptor override
    D3DKMDT_MPR_DESCRIPTOR_OVERRIDE_MONITOR_FREQUENCY_RANGE = 5, //   monitor frequency range in the monitor descriptor override
    D3DKMDT_MPR_DEFAULT_PROFILE_MONITOR_SOURCE_MODE         = 6, //   monitor source mode in the default monitor profile
    D3DKMDT_MPR_DRIVER_RECOMMENDED_MONITOR_SOURCE_MODE      = 7, //   monitor source mode recommended by the driver
    D3DKMDT_MPR_MONITOR_FREQUENCY_RANGE_OVERRIDE            = 8, //   monitor frequency range override
    D3DKMDT_MPR_CLONE_PATH_PRUNED                           = 9, //   Mode is pruned because other path(s) in clone cluster has(have) no mode supported by monitor
    D3DKMDT_MPR_MAXVALID                                    = 10
}
D3DKMDT_MODE_PRUNING_REASON;

// This structure takes 8 bytes.
// The unnamed UINT of size 0 forces alignment of the structure to
// make it exactly occupy 8 bytes, see MSDN docs on C++ bitfields
// for more details
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
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_9
#endif
#endif
}
D3DKMDT_DISPLAYMODE_FLAGS;

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

typedef struct _D3DKMT_GETDISPLAYMODELIST
{
    D3DKMT_HANDLE                   hAdapter;       // in: adapter handle
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;  // in: adapter's VidPN source ID
    D3DKMT_PTR(D3DKMT_DISPLAYMODE*, pModeList);      // out:
    UINT                            ModeCount;      // in/out:
} D3DKMT_GETDISPLAYMODELIST;

typedef struct _D3DKMT_DISPLAYMODELIST
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
    UINT                            ModeCount;
    D3DKMT_DISPLAYMODE              pModeList[0];
} D3DKMT_DISPLAYMODELIST;

typedef struct _D3DKMT_SETDISPLAYMODE_FLAGS
{
    BOOLEAN  PreserveVidPn   : 1;
    UINT     Reserved       : 31;
}
D3DKMT_SETDISPLAYMODE_FLAGS;

typedef struct _D3DKMT_SETDISPLAYMODE
{
    D3DKMT_HANDLE                          hDevice;                         // in: Identifies the device
    D3DKMT_HANDLE                          hPrimaryAllocation;              // in:
    D3DDDI_VIDEO_SIGNAL_SCANLINE_ORDERING  ScanLineOrdering;                // in:
    D3DDDI_ROTATION                        DisplayOrientation;              // in:
    UINT                                   PrivateDriverFormatAttribute;    // out: Private Format Attribute of the current primary surface if DxgkSetDisplayMode failed with STATUS_GRAPHICS_INCOMPATIBLE_PRIVATE_FORMAT
    D3DKMT_SETDISPLAYMODE_FLAGS            Flags;                           // in:
} D3DKMT_SETDISPLAYMODE;


typedef struct _D3DKMT_MULTISAMPLEMETHOD
{
    UINT    NumSamples;
    UINT    NumQualityLevels;
    UINT    Reserved;   //workaround for NTRAID#Longhorn-1124385-2005/03/14-kanqiu
} D3DKMT_MULTISAMPLEMETHOD;

typedef struct _D3DKMT_GETMULTISAMPLEMETHODLIST
{
    D3DKMT_HANDLE                   hAdapter;       // in: adapter handle
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;  // in: adapter's VidPN source ID
    UINT                            Width;          // in:
    UINT                            Height;         // in:
    D3DDDIFORMAT                    Format;         // in:
    D3DKMT_PTR(D3DKMT_MULTISAMPLEMETHOD*, pMethodList); // out:
    UINT                            MethodCount;    // in/out:
} D3DKMT_GETMULTISAMPLEMETHODLIST;

typedef struct _D3DKMT_PRESENTFLAGS
{
    union
    {
        struct
        {
            UINT    Blt                         : 1;        // 0x00000001
            UINT    ColorFill                   : 1;        // 0x00000002
            UINT    Flip                        : 1;        // 0x00000004
            UINT    FlipDoNotFlip               : 1;        // 0x00000008
            UINT    FlipDoNotWait               : 1;        // 0x00000010
            UINT    FlipRestart                 : 1;        // 0x00000020
            UINT    DstRectValid                : 1;        // 0x00000040
            UINT    SrcRectValid                : 1;        // 0x00000080
            UINT    RestrictVidPnSource         : 1;        // 0x00000100
            UINT    SrcColorKey                 : 1;        // 0x00000200
            UINT    DstColorKey                 : 1;        // 0x00000400
            UINT    LinearToSrgb                : 1;        // 0x00000800
            UINT    PresentCountValid           : 1;        // 0x00001000
            UINT    Rotate                      : 1;        // 0x00002000
            UINT    PresentToBitmap             : 1;        // 0x00004000
            UINT    RedirectedFlip              : 1;        // 0x00008000
            UINT    RedirectedBlt               : 1;        // 0x00010000
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
            UINT    FlipStereo                  : 1;        // 0x00020000   // This is a flip from a stereo alloc. Used in addition to Flip.
            UINT    FlipStereoTemporaryMono     : 1;        // 0x00040000   // This is a flip from a stereo alloc. The left image should used to produce both images. Used in addition to Flip.
            UINT    FlipStereoPreferRight       : 1;        // 0x00080000   // This is a flip from a stereo alloc. Use the right image when cloning to a mono monitor. Used in addition to Flip.
            UINT    BltStereoUseRight           : 1;        // 0x00100000   // This is a Blt from a stereo alloc to a mono alloc. The right image should be used.
            UINT    PresentHistoryTokenOnly     : 1;        // 0x00200000   // Submit Present History Token only.
            UINT    PresentRegionsValid         : 1;        // 0x00400000   // Ptr to present regions is valid
            UINT    PresentDDA                  : 1;        // 0x00800000   // Present from a DDA swapchain
            UINT    ProtectedContentBlankedOut  : 1;        // 0x01000000
            UINT    RemoteSession               : 1;        // 0x02000000
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
            UINT    CrossAdapter                : 1;        // 0x04000000
            UINT    DurationValid               : 1;        // 0x08000000
            UINT    PresentIndirect             : 1;        // 0x10000000   // Present to an indirect-display adapter
            UINT    PresentHMD                  : 1;        // 0x20000000   // Present from an HMD swapchain.
            UINT    Reserved                    : 2;        // 0xC0000000
#else
            UINT    Reserved                    : 6;        // 0xFC000000
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
#else
            UINT    Reserved                    : 15;       // 0xFFFE0000
#endif
        };
        UINT    Value;
    };
} D3DKMT_PRESENTFLAGS;

typedef enum _D3DKMT_PRESENT_MODEL
{
    D3DKMT_PM_UNINITIALIZED       = 0,
    D3DKMT_PM_REDIRECTED_GDI       = 1,
    D3DKMT_PM_REDIRECTED_FLIP      = 2,
    D3DKMT_PM_REDIRECTED_BLT       = 3,
    D3DKMT_PM_REDIRECTED_VISTABLT  = 4,
    D3DKMT_PM_SCREENCAPTUREFENCE   = 5,
    D3DKMT_PM_REDIRECTED_GDI_SYSMEM  = 6,
    D3DKMT_PM_REDIRECTED_COMPOSITION = 7,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
    D3DKMT_PM_SURFACECOMPLETE        = 8,
#endif
    D3DKMT_PM_FLIPMANAGER            = 9,
} D3DKMT_PRESENT_MODEL;

typedef enum _D3DKMT_FLIPMODEL_INDEPENDENT_FLIP_STAGE
{
    D3DKMT_FLIPMODEL_INDEPENDENT_FLIP_STAGE_FLIP_SUBMITTED = 0,
    D3DKMT_FLIPMODEL_INDEPENDENT_FLIP_STAGE_FLIP_COMPLETE = 1
} D3DKMT_FLIPMODEL_INDEPENDENT_FLIP_STAGE;

typedef struct _D3DKMT_FLIPMODEL_PRESENTHISTORYTOKENFLAGS
{
    union
    {
        struct
        {
            UINT  Video                         :  1;   // 0x00000001
            UINT  RestrictedContent             :  1;   // 0x00000002
            UINT  ClipToView                    :  1;   // 0x00000004
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
            UINT  StereoPreferRight             :  1;   // 0x00000008
            UINT  TemporaryMono                 :  1;   // 0x00000010
            UINT  FlipRestart                   :  1;   // 0x00000020
            UINT  HDRMetaDataChanged            :  1;   // 0x00000040
            UINT  AlphaMode                     :  2;   // 0x00000180
            UINT  SignalLimitOnTokenCompletion  :  1;   // 0x00000200
            UINT  YCbCrFlags                    :  3;   // 0x00001C00
            UINT  IndependentFlip               :  1;   // 0x00002000
            D3DKMT_FLIPMODEL_INDEPENDENT_FLIP_STAGE IndependentFlipStage : 2;   // 0x0000C000
            UINT  IndependentFlipReleaseCount   :  2;   // 0x00030000
            UINT  IndependentFlipForceNotifyDwm :  1;   // 0x00040000
            UINT  UseCustomDuration             :  1;   // 0x00080000
            UINT  IndependentFlipRequestDwmConfirm:1;   // 0x00100000
            UINT  IndependentFlipCandidate      :  1;   // 0x00200000
            UINT  IndependentFlipCheckNeeded    :  1;   // 0x00400000
            UINT  IndependentFlipTrueImmediate  :  1;   // 0x00800000
            UINT  IndependentFlipRequestDwmExit :  1;   // 0x01000000
            UINT  CompSurfaceNotifiedEarly      :  1;   // 0x02000000
            UINT  IndependentFlipDoNotFlip      :  1;   // 0x04000000
            UINT  RequirePairedToken            :  1;   // 0x08000000
            UINT  VariableRefreshOverrideEligible :1;   // 0x10000000
            UINT  Reserved                      :  3;   // 0xE0000000
#else
            UINT  Reserved                      : 29;   // 0xFFFFFFF8
#endif
        };

        UINT  Value;
    };
} D3DKMT_FLIPMODEL_PRESENTHISTORYTOKENFLAGS;

#define D3DKMT_MAX_PRESENT_HISTORY_RECTS 16

typedef struct _D3DKMT_DIRTYREGIONS
{
    UINT  NumRects;
    RECT  Rects[D3DKMT_MAX_PRESENT_HISTORY_RECTS];
} D3DKMT_DIRTYREGIONS;

typedef struct _D3DKMT_COMPOSITION_PRESENTHISTORYTOKEN
{
    D3DKMT_ALIGN64 ULONG64 hPrivateData;
} D3DKMT_COMPOSITION_PRESENTHISTORYTOKEN;

typedef struct _D3DKMT_FLIPMANAGER_PRESENTHISTORYTOKEN
{
    D3DKMT_ALIGN64 ULONG64 hPrivateData;
    D3DKMT_ALIGN64 ULONGLONG PresentAtQpc;
    union
    {
        struct
        {
            UINT Discard   : 1;
            UINT PresentAt : 1;
            UINT hPrivateDataIsPointer : 1;
            UINT Reserved  : 29;
        };
        UINT Value;
    }Flags;
} D3DKMT_FLIPMANAGER_PRESENTHISTORYTOKEN;

typedef enum _D3DKMT_AUXILIARYPRESENTINFO_TYPE
{
    D3DKMT_AUXILIARYPRESENTINFO_TYPE_FLIPMANAGER = 0
} D3DKMT_AUXILIARYPRESENTINFO_TYPE;

typedef struct _D3DKMT_AUXILIARYPRESENTINFO
{
    UINT size;
    D3DKMT_AUXILIARYPRESENTINFO_TYPE type;
} D3DKMT_AUXILIARYPRESENTINFO;

typedef struct _D3DKMT_FLIPMANAGER_AUXILIARYPRESENTINFO
{
    // in: Base information
    D3DKMT_AUXILIARYPRESENTINFO auxiliaryPresentInfo;

    // in: Tracing ID of owner flip manager
    UINT flipManagerTracingId;

    // in: Whether or not the application requested a different custom duration
    // than the previous present
    BOOL customDurationChanged;

    // out: The adapter LUID/VidPn source of the flip output
    LUID FlipAdapterLuid;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;

    // out: Independent flip stage
    D3DKMT_FLIPMODEL_INDEPENDENT_FLIP_STAGE independentFlipStage;

    // out: The DPC frame time of the frame on which the flip was completed
    D3DKMT_ALIGN64 ULONGLONG FlipCompletedQpc;

    // out: The approved frame duration
    UINT HwPresentDurationQpc;

    // out: Whether or not the present was canceled in the scheduler
    BOOL WasCanceled;
} D3DKMT_FLIPMANAGER_AUXILIARYPRESENTINFO;

typedef struct _D3DKMT_GDIMODEL_PRESENTHISTORYTOKEN
{
    D3DKMT_ALIGN64 ULONG64 hLogicalSurface;
    D3DKMT_ALIGN64 ULONG64 hPhysicalSurface;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    RECT                 ScrollRect;
    POINT                ScrollOffset;
#endif
    D3DKMT_DIRTYREGIONS  DirtyRegions;
} D3DKMT_GDIMODEL_PRESENTHISTORYTOKEN;

typedef struct _D3DKMT_GDIMODEL_SYSMEM_PRESENTHISTORYTOKEN
{
    D3DKMT_ALIGN64 ULONG64 hlsurf;
    DWORD dwDirtyFlags;
    D3DKMT_ALIGN64 UINT64 uiCookie;
} D3DKMT_GDIMODEL_SYSMEM_PRESENTHISTORYTOKEN;

typedef ULONGLONG  D3DKMT_VISTABLTMODEL_PRESENTHISTORYTOKEN;

typedef struct _D3DKMT_FENCE_PRESENTHISTORYTOKEN
{
    D3DKMT_ALIGN64 UINT64 Key;
} D3DKMT_FENCE_PRESENTHISTORYTOKEN;

typedef struct _D3DKMT_BLTMODEL_PRESENTHISTORYTOKEN
{
    D3DKMT_ALIGN64 ULONG64 hLogicalSurface;
    D3DKMT_ALIGN64 ULONG64 hPhysicalSurface;
    D3DKMT_ALIGN64 ULONG64 EventId;
    D3DKMT_DIRTYREGIONS                 DirtyRegions;
} D3DKMT_BLTMODEL_PRESENTHISTORYTOKEN;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
#define D3DKMT_MAX_PRESENT_HISTORY_SCATTERBLTS 12

typedef struct _D3DKMT_SCATTERBLT
{
    D3DKMT_ALIGN64 ULONG64 hLogicalSurfaceDestination;
    D3DKMT_ALIGN64 LONG64  hDestinationCompSurfDWM;
    D3DKMT_ALIGN64 UINT64  DestinationCompositionBindingId;
    RECT    SourceRect;
    POINT   DestinationOffset;
} D3DKMT_SCATTERBLT;

typedef struct _D3DKMT_SCATTERBLTS
{
    UINT NumBlts;
    D3DKMT_SCATTERBLT Blts[D3DKMT_MAX_PRESENT_HISTORY_SCATTERBLTS];
} D3DKMT_SCATTERBLTS;
#endif

typedef struct _D3DKMT_FLIPMODEL_PRESENTHISTORYTOKEN
{
    D3DKMT_ALIGN64 UINT64                      FenceValue;
    D3DKMT_ALIGN64 ULONG64                     hLogicalSurface;
    D3DKMT_ALIGN64 D3DKMT_UINT_PTR             dxgContext;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID             VidPnSourceId;
    UINT                                       SwapChainIndex;
    D3DKMT_ALIGN64 UINT64                      PresentLimitSemaphoreId;
    D3DDDI_FLIPINTERVAL_TYPE                   FlipInterval;
    D3DKMT_FLIPMODEL_PRESENTHISTORYTOKENFLAGS  Flags;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    D3DKMT_ALIGN64 LONG64                      hCompSurf;
    LUID                                       compSurfLuid;
    D3DKMT_ALIGN64 UINT64                      confirmationCookie;
    D3DKMT_ALIGN64 UINT64                      CompositionSyncKey;
    UINT                                       RemainingTokens;
    RECT                                       ScrollRect;
    POINT                                      ScrollOffset;
    UINT                                       PresentCount;
    FLOAT                                      RevealColor[4]; // index 0 == R, ... , 3 == A
    D3DDDI_ROTATION                            Rotation;
    union
    {
        D3DKMT_SCATTERBLTS                     ScatterBlts;     // Unused
        struct
        {
            HANDLE                             hSyncObject;     // NT handle to FlipEx fence.
            D3DDDI_HDR_METADATA_TYPE           HDRMetaDataType;
            union
            {
                D3DDDI_HDR_METADATA_HDR10      HDRMetaDataHDR10;
                D3DDDI_HDR_METADATA_HDR10PLUS  HDRMetaDataHDR10Plus;
            };
        };
    };
    UINT                                       InkCookie;
    RECT                                       SourceRect;
    UINT                                       DestWidth;
    UINT                                       DestHeight;
    RECT                                       TargetRect;
    // DXGI_MATRIX_3X2_F: _11 _12 _21 _22 _31 _32
    FLOAT                                      Transform[6];
    UINT                                       CustomDuration;
    D3DDDI_FLIPINTERVAL_TYPE                   CustomDurationFlipInterval;
    UINT                                       PlaneIndex;
#endif
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    D3DDDI_COLOR_SPACE_TYPE                    ColorSpace;
#endif
    D3DKMT_DIRTYREGIONS                        DirtyRegions;
} D3DKMT_FLIPMODEL_PRESENTHISTORYTOKEN;

// User mode timeout is in milliseconds, kernel mode timeout is in 100 nanoseconds
#define FLIPEX_TIMEOUT_USER     (2000)
#define FLIPEX_TIMEOUT_KERNEL   (FLIPEX_TIMEOUT_USER*10000)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
typedef struct _D3DKMT_SURFACECOMPLETE_PRESENTHISTORYTOKEN
{
    D3DKMT_ALIGN64 ULONG64 hLogicalSurface;
} D3DKMT_SURFACECOMPLETE_PRESENTHISTORYTOKEN;
#endif

typedef struct _D3DKMT_PRESENTHISTORYTOKEN
{
    D3DKMT_PRESENT_MODEL  Model;
    // The size of the present history token in bytes including Model.
    // Should be set to zero by when submitting a token.
    // It will be initialized when reading present history and can be used to
    // go to the next token in the present history buffer.
    UINT                  TokenSize;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    // The binding id as specified by the Composition Surface
    UINT64                CompositionBindingId;
#endif

    union
    {
        D3DKMT_FLIPMODEL_PRESENTHISTORYTOKEN        Flip;
        D3DKMT_BLTMODEL_PRESENTHISTORYTOKEN         Blt;
        D3DKMT_VISTABLTMODEL_PRESENTHISTORYTOKEN    VistaBlt;
        D3DKMT_GDIMODEL_PRESENTHISTORYTOKEN         Gdi;
        D3DKMT_FENCE_PRESENTHISTORYTOKEN            Fence;
        D3DKMT_GDIMODEL_SYSMEM_PRESENTHISTORYTOKEN  GdiSysMem;
        D3DKMT_COMPOSITION_PRESENTHISTORYTOKEN      Composition;
        D3DKMT_FLIPMANAGER_PRESENTHISTORYTOKEN      FlipManager;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
        D3DKMT_SURFACECOMPLETE_PRESENTHISTORYTOKEN  SurfaceComplete;
#endif
    }
    Token;
} D3DKMT_PRESENTHISTORYTOKEN;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
typedef struct _D3DKMT_PRESENT_RGNS
{
    UINT DirtyRectCount;
    D3DKMT_PTR(_Field_size_( DirtyRectCount ) const RECT*, pDirtyRects);
    UINT MoveRectCount;
    D3DKMT_PTR(_Field_size_( MoveRectCount ) const D3DKMT_MOVE_RECT*, pMoveRects);
}D3DKMT_PRESENT_RGNS;
#endif

typedef struct _D3DKMT_PRESENT
{
    union
    {
        D3DKMT_HANDLE               hDevice;            // in: D3D10 compatibility.
        D3DKMT_HANDLE               hContext;           // in: Indentifies the context
    };
    D3DKMT_PTR(HWND,                hWindow);           // in: window to present to
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;      // in: VidPn source ID if RestrictVidPnSource is flagged
    D3DKMT_HANDLE                   hSource;            // in: Source allocation to present from
    D3DKMT_HANDLE                   hDestination;       // in: Destination allocation whenever non-zero
    UINT                            Color;              // in: color value in ARGB 32 bit format
    RECT                            DstRect;            // in: unclipped dest rect
    RECT                            SrcRect;            // in: unclipped src rect
    UINT                            SubRectCnt;         // in: count of sub rects
    D3DKMT_PTR(CONST RECT*,         pSrcSubRects);      // in: sub rects in source space
    UINT                            PresentCount;       // in: present counter
    D3DDDI_FLIPINTERVAL_TYPE        FlipInterval;       // in: flip interval
    D3DKMT_PRESENTFLAGS             Flags;              // in:
    ULONG                           BroadcastContextCount;                          // in: Specifies the number of context
                                                                                    //     to broadcast this command buffer to.
    D3DKMT_HANDLE                   BroadcastContext[D3DDDI_MAX_BROADCAST_CONTEXT]; // in: Specifies the handle of the context to
                                                                                    //     broadcast to.
    HANDLE                          PresentLimitSemaphore;
    D3DKMT_PRESENTHISTORYTOKEN      PresentHistoryToken;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    D3DKMT_PRESENT_RGNS*            pPresentRegions;
#endif
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    union
    {
        D3DKMT_HANDLE               hAdapter;           // in: iGpu adapter for PHT redirection. Valid only when the CrossAdapter flag is set.
        D3DKMT_HANDLE               hIndirectContext;   // in: indirect adapter context for redirecting through the DoD present path. Only
                                                        //     valid if PresentIndirect flag is set.
    };
    UINT                            Duration;           // in: Per-present duration. Valid only when the DurationValid flag is set.
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    D3DKMT_PTR(_Field_size_(BroadcastContextCount)
    D3DKMT_HANDLE*,                 BroadcastSrcAllocation);                    // in: LDA
    D3DKMT_PTR(_Field_size_opt_(BroadcastContextCount)
    D3DKMT_HANDLE*,                 BroadcastDstAllocation);                    // in: LDA
    UINT                            PrivateDriverDataSize;                      // in:
    D3DKMT_PTR(_Field_size_bytes_(PrivateDriverDataSize)
    PVOID,                          pPrivateDriverData);                        // in: Private driver data to pass to DdiPresent and DdiSetVidPnSourceAddress
    BOOLEAN                         bOptimizeForComposition;                    // out: DWM is involved in composition
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
} D3DKMT_PRESENT;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
typedef struct _D3DKMT_PRESENT_REDIRECTEDS_FLAGS
{
    union
    {
        struct
        {
            UINT Reserved : 32; // 0xFFFFFFFF
        };
        UINT Value;
    };
}D3DKMT_PRESENT_REDIRECTED_FLAGS;

typedef struct _D3DKMT_PRESENT_REDIRECTED
{
    D3DKMT_HANDLE                   hSyncObj;              // in: Sync object PHT waits on
    D3DKMT_HANDLE                   hDevice;               // in: Device associated with the present
    D3DKMT_ALIGN64 ULONGLONG        WaitedFenceValue;      // in: Fence value of hSyncObj that PHT waits on
    D3DKMT_PRESENTHISTORYTOKEN      PresentHistoryToken;
    D3DKMT_PRESENT_REDIRECTED_FLAGS Flags;
    D3DKMT_HANDLE                   hSource;               // in: Source allocation to present from
    UINT                            PrivateDriverDataSize; // in:
    D3DKMT_PTR(_Field_size_bytes_(PrivateDriverDataSize)
    PVOID,                          pPrivateDriverData);   // in: Private driver data to pass to DdiPresent and DdiSetVidPnSourceAddress
}D3DKMT_PRESENT_REDIRECTED;
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)
typedef struct _D3DKMT_CANCEL_PRESENTS_FLAGS
{
    union
    {
        // D3DKMT_CANCEL_PRESENTS_OPERATION_REPROGRAM_INTERRUPT flags
        struct
        {
            UINT NewVSyncInterruptState :  1;
            UINT Reserved               : 31;
        } ReprogramInterrupt;

        UINT Value;
    };
}D3DKMT_CANCEL_PRESENTS_FLAGS;


typedef enum D3DKMT_CANCEL_PRESENTS_OPERATION
{
    D3DKMT_CANCEL_PRESENTS_OPERATION_CANCEL_FROM            = 0,
    D3DKMT_CANCEL_PRESENTS_OPERATION_REPROGRAM_INTERRUPT    = 1
} D3DKMT_CANCEL_PRESENTS_OPERATION;

typedef struct _D3DKMT_CANCEL_PRESENTS
{
    UINT                                cbSize;
    D3DKMT_HANDLE                       hDevice;
    D3DKMT_CANCEL_PRESENTS_FLAGS        Flags;
    D3DKMT_CANCEL_PRESENTS_OPERATION    Operation;
    D3DKMT_ALIGN64 UINT64               CancelFromPresentId;
    LUID                                CompSurfaceLuid;
    D3DKMT_ALIGN64 UINT64               BindId;
}D3DKMT_CANCEL_PRESENTS;
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)

typedef struct _D3DKMT_SUBMITPRESENTBLTTOHWQUEUE
{
    D3DKMT_HANDLE         hHwQueue;
    D3DKMT_ALIGN64 UINT64 HwQueueProgressFenceId;
    D3DKMT_PRESENT        PrivatePresentData;
} D3DKMT_SUBMITPRESENTBLTTOHWQUEUE;

#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)

typedef struct _D3DKMT_SUBMITPRESENTTOHWQUEUE
{
    D3DKMT_PTR(_Field_size_(PrivatePresentData.BroadcastContextCount + 1)
    D3DKMT_HANDLE*, hHwQueues);
    D3DKMT_PRESENT  PrivatePresentData;
} D3DKMT_SUBMITPRESENTTOHWQUEUE;

#endif

#define D3DKMT_MAX_MULTIPLANE_OVERLAY_PLANES                   8
#define D3DKMT_MAX_MULTIPLANE_OVERLAY_ALLOCATIONS_PER_PLANE   256

typedef enum D3DKMT_MULTIPLANE_OVERLAY_FLAGS
{
    D3DKMT_MULTIPLANE_OVERLAY_FLAG_VERTICAL_FLIP               = 0x1,
    D3DKMT_MULTIPLANE_OVERLAY_FLAG_HORIZONTAL_FLIP             = 0x2,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)
    D3DKMT_MULTIPLANE_OVERLAY_FLAG_STATIC_CHECK                = 0x4,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM3_0
} D3DKMT_MULTIPLANE_OVERLAY_FLAGS;

typedef enum D3DKMT_MULTIPLANE_OVERLAY_BLEND
{
    D3DKMT_MULTIPLANE_OVERLAY_BLEND_OPAQUE     = 0x0,
    D3DKMT_MULTIPLANE_OVERLAY_BLEND_ALPHABLEND = 0x1,
} D3DKMT_MULTIPLANE_OVERLAY_BLEND;

typedef enum D3DKMT_MULTIPLANE_OVERLAY_VIDEO_FRAME_FORMAT
{
    D3DKMT_MULIIPLANE_OVERLAY_VIDEO_FRAME_FORMAT_PROGRESSIVE  = 0,
    D3DKMT_MULTIPLANE_OVERLAY_VIDEO_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST   = 1,
    D3DKMT_MULTIPLANE_OVERLAY_VIDEO_FRAME_FORMAT_INTERLACED_BOTTOM_FIELD_FIRST    = 2
} D3DKMT_MULTIPLANE_OVERLAY_VIDEO_FRAME_FORMAT;

typedef enum D3DKMT_MULTIPLANE_OVERLAY_YCbCr_FLAGS
{
    D3DKMT_MULTIPLANE_OVERLAY_YCbCr_FLAG_NOMINAL_RANGE = 0x1, // 16 - 235 vs. 0 - 255
    D3DKMT_MULTIPLANE_OVERLAY_YCbCr_FLAG_BT709         = 0x2, // BT.709 vs. BT.601
    D3DKMT_MULTIPLANE_OVERLAY_YCbCr_FLAG_xvYCC         = 0x4, // xvYCC vs. conventional YCbCr
} D3DKMT_MULTIPLANE_OVERLAY_YCbCr_FLAGS;

typedef enum D3DKMT_MULTIPLANE_OVERLAY_STEREO_FORMAT
{
    DXGKMT_MULTIPLANE_OVERLAY_STEREO_FORMAT_MONO               = 0,
    D3DKMT_MULTIPLANE_OVERLAY_STEREO_FORMAT_HORIZONTAL         = 1,
    D3DKMT_MULTIPLANE_OVERLAY_STEREO_FORMAT_VERTICAL           = 2,
    DXGKMT_MULTIPLANE_OVERLAY_STEREO_FORMAT_SEPARATE           = 3,
    DXGKMT_MULTIPLANE_OVERLAY_STEREO_FORMAT_MONO_OFFSET        = 4,
    DXGKMT_MULTIPLANE_OVERLAY_STEREO_FORMAT_ROW_INTERLEAVED    = 5,
    DXGKMT_MULTIPLANE_OVERLAY_STEREO_FORMAT_COLUMN_INTERLEAVED = 6,
    DXGKMT_MULTIPLANE_OVERLAY_STEREO_FORMAT_CHECKERBOARD       = 7
} D3DKMT_MULTIPLANE_OVERLAY_STEREO_FORMAT;

typedef enum _DXGKMT_MULTIPLANE_OVERLAY_STEREO_FLIP_MODE
{
    DXGKMT_MULTIPLANE_OVERLAY_STEREO_FLIP_NONE   = 0,
    DXGKMT_MULTIPLANE_OVERLAY_STEREO_FLIP_FRAME0 = 1,
    DXGKMT_MULTIPLANE_OVERLAY_STEREO_FLIP_FRAME1 = 2,
} DXGKMT_MULTIPLANE_OVERLAY_STEREO_FLIP_MODE;

typedef enum _DXGKMT_MULTIPLANE_OVERLAY_STRETCH_QUALITY
{
    DXGKMT_MULTIPLANE_OVERLAY_STRETCH_QUALITY_BILINEAR        = 0x1,  // Bilinear
    DXGKMT_MULTIPLANE_OVERLAY_STRETCH_QUALITY_HIGH            = 0x2,  // Maximum
} DXGKMT_MULTIPLANE_OVERLAY_STRETCH_QUALITY;

typedef struct D3DKMT_MULTIPLANE_OVERLAY_ATTRIBUTES
{
    UINT                                         Flags;      // D3DKMT_MULTIPLANE_OVERLAY_FLAGS
    RECT                                         SrcRect;
    RECT                                         DstRect;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    RECT                                         ClipRect;
#endif
    D3DDDI_ROTATION                              Rotation;
    D3DKMT_MULTIPLANE_OVERLAY_BLEND              Blend;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    UINT                                         DirtyRectCount;
    D3DKMT_PTR(RECT*,                            pDirtyRects);
#else
    UINT                                         NumFilters;
    D3DKMT_PTR(void*,                            pFilters);
#endif
    D3DKMT_MULTIPLANE_OVERLAY_VIDEO_FRAME_FORMAT VideoFrameFormat;
    UINT                                         YCbCrFlags; // D3DKMT_MULTIPLANE_OVERLAY_YCbCr_FLAGS
    D3DKMT_MULTIPLANE_OVERLAY_STEREO_FORMAT      StereoFormat;
    BOOL                                         StereoLeftViewFrame0;
    BOOL                                         StereoBaseViewFrame0;
    DXGKMT_MULTIPLANE_OVERLAY_STEREO_FLIP_MODE   StereoFlipMode;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    DXGKMT_MULTIPLANE_OVERLAY_STRETCH_QUALITY    StretchQuality;
#endif
} D3DKMT_MULTIPLANE_OVERLAY_ATTRIBUTES;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM_1_3)
typedef struct D3DKMT_CHECK_MULTIPLANE_OVERLAY_PLANE
{
    D3DKMT_HANDLE                        hResource;
    LUID                                 CompSurfaceLuid;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID       VidPnSourceId;
    D3DKMT_MULTIPLANE_OVERLAY_ATTRIBUTES PlaneAttributes;
} D3DKMT_CHECK_MULTIPLANE_OVERLAY_PLANE;

typedef struct D3DKMT_CHECK_MULTIPLANE_OVERLAY_SUPPORT_RETURN_INFO
{
    union
    {
        struct
        {
            UINT    FailingPlane        : 4;   // The 0 based index of the first plane that could not be supported
            UINT    TryAgain            : 1;   // The configuration is not supported due to a transition condition, which should shortly go away
            UINT    Reserved            : 27;
        };
        UINT    Value;
    };
} D3DKMT_CHECK_MULTIPLANE_OVERLAY_SUPPORT_RETURN_INFO;

typedef struct _D3DKMT_CHECKMULTIPLANEOVERLAYSUPPORT
{
    D3DKMT_HANDLE                                       hDevice;            // in : Indentifies the device
    UINT                                                PlaneCount;         // in : Number of resources to pin
    D3DKMT_PTR(D3DKMT_CHECK_MULTIPLANE_OVERLAY_PLANE*,  pOverlayPlanes);    // in : Array of resource handles to pin
    BOOL                                                Supported;
    D3DKMT_CHECK_MULTIPLANE_OVERLAY_SUPPORT_RETURN_INFO ReturnInfo;
} D3DKMT_CHECKMULTIPLANEOVERLAYSUPPORT;
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM_2_0)
typedef struct _D3DKMT_MULTIPLANE_OVERLAY_ATTRIBUTES2
{
    UINT                                         Flags;     // D3DKMT_MULTIPLANE_OVERLAY_FLAGS
    RECT                                         SrcRect;   // Specifies the source rectangle, of type RECT, relative to the source resource.
    RECT                                         DstRect;   // Specifies the destination rectangle, of type RECT, relative to the monitor resolution.
    RECT                                         ClipRect;  // Specifies any additional clipping, of type RECT, relative to the DstRect rectangle,
                                                            // after the data has been stretched according to the values of SrcRect and DstRect.

                                                            // The driver and hardware can use the ClipRect member to apply a common stretch factor
                                                            // as the clipping changes when an app occludes part of the DstRect destination rectangle.
    D3DDDI_ROTATION                              Rotation;  // Specifies the clockwise rotation of the overlay plane, given as a value from the D3DDDI_ROTATION enumeration.
    D3DKMT_MULTIPLANE_OVERLAY_BLEND              Blend;     // Specifies the blend mode that applies to this overlay plane and the plane beneath it, given as a value from the DXGK_MULTIPLANE_OVERLAY_BLEND enumeration.
    UINT                                         DirtyRectCount;
    D3DKMT_PTR(RECT*,                            pDirtyRects);
    D3DKMT_MULTIPLANE_OVERLAY_VIDEO_FRAME_FORMAT VideoFrameFormat;  // DXGK_MULTIPLANE_OVERLAY_VIDEO_FRAME_FORMAT
    D3DDDI_COLOR_SPACE_TYPE                      ColorSpace;
    D3DKMT_MULTIPLANE_OVERLAY_STEREO_FORMAT      StereoFormat;      // DXGK_MULTIPLANE_OVERLAY_STEREO_FORMAT
    BOOL                                         StereoLeftViewFrame0;  // Reserved for system use. Must always be FALSE.
    BOOL                                         StereoBaseViewFrame0;  // Reserved for system use. Must always be FALSE.
    DXGKMT_MULTIPLANE_OVERLAY_STEREO_FLIP_MODE   StereoFlipMode;        // DXGK_MULTIPLANE_OVERLAY_STEREO_FLIP_MODE
    DXGKMT_MULTIPLANE_OVERLAY_STRETCH_QUALITY    StretchQuality;        // DXGK_MULTIPLANE_OVERLAY_STRETCH_QUALITY
    UINT                                         Reserved1;
} D3DKMT_MULTIPLANE_OVERLAY_ATTRIBUTES2;

typedef struct _D3DKMT_CHECK_MULTIPLANE_OVERLAY_PLANE2
{
    UINT                                  LayerIndex;
    D3DKMT_HANDLE                         hResource;
    LUID                                  CompSurfaceLuid;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID        VidPnSourceId;
    D3DKMT_MULTIPLANE_OVERLAY_ATTRIBUTES2 PlaneAttributes;
} D3DKMT_CHECK_MULTIPLANE_OVERLAY_PLANE2;

typedef struct _D3DKMT_CHECKMULTIPLANEOVERLAYSUPPORT2
{
    D3DKMT_HANDLE                                       hAdapter;           // in: adapter handle
    D3DKMT_HANDLE                                       hDevice;            // in : Indentifies the device
    UINT                                                PlaneCount;         // in : Number of resources to pin
    D3DKMT_PTR(D3DKMT_CHECK_MULTIPLANE_OVERLAY_PLANE2*, pOverlayPlanes);    // in : Array of resource handles to pin
    BOOL                                                Supported;
    D3DKMT_CHECK_MULTIPLANE_OVERLAY_SUPPORT_RETURN_INFO ReturnInfo;
} D3DKMT_CHECKMULTIPLANEOVERLAYSUPPORT2;

typedef struct _D3DKMT_MULTIPLANE_OVERLAY2
{
    UINT                                  LayerIndex;
    BOOL                                  Enabled;
    D3DKMT_HANDLE                         hAllocation;
    D3DKMT_MULTIPLANE_OVERLAY_ATTRIBUTES2 PlaneAttributes;
} D3DKMT_MULTIPLANE_OVERLAY2;

typedef struct _D3DKMT_PRESENT_MULTIPLANE_OVERLAY2
{
    D3DKMT_HANDLE                   hAdapter;           // in: adapter handle
    union
    {
        D3DKMT_HANDLE               hDevice;            // in: D3D10 compatibility.
        D3DKMT_HANDLE               hContext;           // in: Indentifies the context
    };
    ULONG                           BroadcastContextCount;                          // in: Specifies the number of context
                                                                                    //     to broadcast this command buffer to.
    D3DKMT_HANDLE                   BroadcastContext[D3DDDI_MAX_BROADCAST_CONTEXT]; // in: Specifies the handle of the context to
                                                                                    //     broadcast to.

    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;      // in: VidPn source ID if RestrictVidPnSource is flagged
    UINT                            PresentCount;       // in: present counter
    D3DDDI_FLIPINTERVAL_TYPE        FlipInterval;       // in: flip interval
    D3DKMT_PRESENTFLAGS             Flags;              // in:

    UINT                            PresentPlaneCount;
    D3DKMT_PTR(D3DKMT_MULTIPLANE_OVERLAY2*, pPresentPlanes);
    UINT                            Duration;
} D3DKMT_PRESENT_MULTIPLANE_OVERLAY2;
#endif  // DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
typedef struct _D3DKMT_MULTIPLANE_OVERLAY_ATTRIBUTES3
{
    UINT                                         Flags;     // D3DKMT_MULTIPLANE_OVERLAY_FLAGS
    RECT                                         SrcRect;   // Specifies the source rectangle, of type RECT, relative to the source resource.
    RECT                                         DstRect;   // Specifies the destination rectangle, of type RECT, relative to the monitor resolution.
    RECT                                         ClipRect;  // Specifies any additional clipping, of type RECT, relative to the DstRect rectangle,
                                                            // after the data has been stretched according to the values of SrcRect and DstRect.

                                                            // The driver and hardware can use the ClipRect member to apply a common stretch factor
                                                            // as the clipping changes when an app occludes part of the DstRect destination rectangle.
    D3DDDI_ROTATION                              Rotation;  // Specifies the clockwise rotation of the overlay plane, given as a value from the D3DDDI_ROTATION enumeration.
    D3DKMT_MULTIPLANE_OVERLAY_BLEND              Blend;     // Specifies the blend mode that applies to this overlay plane and the plane beneath it, given as a value from the DXGK_MULTIPLANE_OVERLAY_BLEND enumeration.
    UINT                                         DirtyRectCount;
    D3DKMT_PTR(_Field_size_(DirtyRectCount) RECT*, pDirtyRects);
    D3DDDI_COLOR_SPACE_TYPE                      ColorSpace;
    DXGKMT_MULTIPLANE_OVERLAY_STRETCH_QUALITY    StretchQuality;        // DXGK_MULTIPLANE_OVERLAY_STRETCH_QUALITY
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
    UINT                                         SDRWhiteLevel;
#endif
} D3DKMT_MULTIPLANE_OVERLAY_ATTRIBUTES3;

typedef struct _D3DKMT_CHECK_MULTIPLANE_OVERLAY_PLANE3
{
    UINT                                  LayerIndex;
    D3DKMT_HANDLE                         hResource;
    LUID                                  CompSurfaceLuid;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID        VidPnSourceId;
    D3DKMT_PTR(D3DKMT_MULTIPLANE_OVERLAY_ATTRIBUTES3*, pPlaneAttributes);
} D3DKMT_CHECK_MULTIPLANE_OVERLAY_PLANE3;

typedef struct _D3DKMT_MULTIPLANE_OVERLAY_POST_COMPOSITION_FLAGS
{
    union
    {
        struct
        {
            UINT VerticalFlip              : 1;   // 0x00000001
            UINT HorizontalFlip            : 1;   // 0x00000002
            UINT Reserved                  :30;   // 0xFFFFFFFC
        };
        UINT Value;
    };
} D3DKMT_MULTIPLANE_OVERLAY_POST_COMPOSITION_FLAGS;

typedef struct _D3DKMT_MULTIPLANE_OVERLAY_POST_COMPOSITION
{
    D3DKMT_MULTIPLANE_OVERLAY_POST_COMPOSITION_FLAGS    Flags;
    RECT                                                SrcRect;
    RECT                                                DstRect;
    D3DDDI_ROTATION                                     Rotation;
} D3DKMT_MULTIPLANE_OVERLAY_POST_COMPOSITION;

typedef struct _D3DKMT_MULTIPLANE_OVERLAY_POST_COMPOSITION_WITH_SOURCE
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID             VidPnSourceId;
    D3DKMT_MULTIPLANE_OVERLAY_POST_COMPOSITION PostComposition;
} D3DKMT_MULTIPLANE_OVERLAY_POST_COMPOSITION_WITH_SOURCE;

typedef struct _D3DKMT_CHECKMULTIPLANEOVERLAYSUPPORT3
{
    D3DKMT_HANDLE                                            hAdapter;           // in: adapter handle
    D3DKMT_HANDLE                                            hDevice;            // in : Indentifies the device
    UINT                                                     PlaneCount;         // in : Number of resources to pin
    // Note: Array-of-pointers don't work in 32bit WSL
    _Field_size_(PlaneCount)
    D3DKMT_CHECK_MULTIPLANE_OVERLAY_PLANE3**                 ppOverlayPlanes;    // in : Array of pointers to overlay planes
    UINT                                                     PostCompositionCount; // in : Number of resources to pin
    _Field_size_(PostCompositionCount)
    D3DKMT_MULTIPLANE_OVERLAY_POST_COMPOSITION_WITH_SOURCE** ppPostComposition;    // in : Array of pointers to overlay planes
    BOOL                                                     Supported;
    D3DKMT_CHECK_MULTIPLANE_OVERLAY_SUPPORT_RETURN_INFO      ReturnInfo;
} D3DKMT_CHECKMULTIPLANEOVERLAYSUPPORT3;

typedef struct _D3DKMT_PLANE_SPECIFIC_INPUT_FLAGS
{
    union
    {
        struct
        {
            UINT Enabled                   : 1;   // 0x00000001
            UINT Reserved                  :31;   // 0xFFFFFFFE
        };
        UINT Value;
    };
} D3DKMT_PLANE_SPECIFIC_INPUT_FLAGS;

typedef struct _D3DKMT_PLANE_SPECIFIC_OUTPUT_FLAGS
{
    union
    {
        struct
        {
            UINT FlipConvertedToImmediate  : 1;   // 0x00000001
            UINT Reserved                  :31;   // 0xFFFFFFFE
        };
        UINT Value;
    };
} D3DKMT_PLANE_SPECIFIC_OUTPUT_FLAGS;

typedef struct _D3DKMT_MULTIPLANE_OVERLAY3
{
    UINT                                         LayerIndex;
    D3DKMT_PLANE_SPECIFIC_INPUT_FLAGS            InputFlags;
    D3DDDI_FLIPINTERVAL_TYPE                     FlipInterval;
    UINT                                         MaxImmediateFlipLine;
    UINT                                         AllocationCount;
    D3DKMT_PTR(_Field_size_(AllocationCount)
    D3DKMT_HANDLE*,                              pAllocationList);
    UINT                                         DriverPrivateDataSize;
    D3DKMT_PTR(_Field_size_bytes_(DriverPrivateDataSize)
    VOID*,                                       pDriverPrivateData);
    D3DKMT_PTR(const D3DKMT_MULTIPLANE_OVERLAY_ATTRIBUTES3*, pPlaneAttributes);
    D3DKMT_HANDLE                                hFlipToFence;
    D3DKMT_HANDLE                                hFlipAwayFence;
    D3DKMT_ALIGN64 UINT64                        FlipToFenceValue;
    D3DKMT_ALIGN64 UINT64                        FlipAwayFenceValue;
} D3DKMT_MULTIPLANE_OVERLAY3;

typedef struct _D3DKMT_PRESENT_MULTIPLANE_OVERLAY_FLAGS
{
    union
    {
        struct
        {
            UINT FlipStereo                 : 1;    // 0x00000001 This is a flip from a stereo alloc. Used in addition to FlipImmediate or FlipOnNextVSync.
            UINT FlipStereoTemporaryMono    : 1;    // 0x00000002 This is a flip from a stereo alloc. The left image should used. Used in addition to FlipImmediate or FlipOnNextVSync.
            UINT FlipStereoPreferRight      : 1;    // 0x00000004 This is a flip from a stereo alloc. The right image should used when cloning to a mono monitor. Used in addition to FlipImmediate or FlipOnNextVSync.
            UINT FlipDoNotWait              : 1;    // 0x00000008
            UINT FlipDoNotFlip              : 1;    // 0x00000010
            UINT FlipRestart                : 1;    // 0x00000020
            UINT DurationValid              : 1;    // 0x00000040
            UINT HDRMetaDataValid           : 1;    // 0x00000080
            UINT HMD                        : 1;    // 0x00000100
            UINT TrueImmediate              : 1;    // 0x00000200 If a present interval is 0, allow tearing rather than override a previously queued flip
            UINT Reserved                   :22;    // 0xFFFFFE00
        };
        UINT Value;
    };
} D3DKMT_PRESENT_MULTIPLANE_OVERLAY_FLAGS;

typedef struct _D3DKMT_PRESENT_MULTIPLANE_OVERLAY3
{
    D3DKMT_HANDLE                               hAdapter;           // in: adapter handle
    UINT                                        ContextCount;
    D3DKMT_PTR(_Field_size_(ContextCount)
    D3DKMT_HANDLE*,                             pContextList);

    D3DDDI_VIDEO_PRESENT_SOURCE_ID              VidPnSourceId;      // in: VidPn source ID if RestrictVidPnSource is flagged
    UINT                                        PresentCount;       // in: present counter
    D3DKMT_PRESENT_MULTIPLANE_OVERLAY_FLAGS     Flags;              // in:

    UINT                                        PresentPlaneCount;
    // Note: Array-of-pointers don't work in 32bit WSL
    _Field_size_(PresentPlaneCount)
    D3DKMT_MULTIPLANE_OVERLAY3**                ppPresentPlanes;
    D3DKMT_PTR(D3DKMT_MULTIPLANE_OVERLAY_POST_COMPOSITION*, pPostComposition);
    UINT                                        Duration;
    D3DDDI_HDR_METADATA_TYPE                    HDRMetaDataType;
    UINT                                        HDRMetaDataSize;
    D3DKMT_PTR(_Field_size_bytes_(HDRMetaDataSize)
    const VOID*,                                pHDRMetaData);
    UINT                                        BoostRefreshRateMultiplier;
} D3DKMT_PRESENT_MULTIPLANE_OVERLAY3;
#endif  // DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
typedef struct _D3DKMT_MULTIPLANE_OVERLAY_CAPS
{
    union
    {
        struct
        {
            UINT Rotation                        : 1;    // Full rotation
            UINT RotationWithoutIndependentFlip  : 1;    // Rotation, but without simultaneous IndependentFlip support
            UINT VerticalFlip                    : 1;    // Can flip the data vertically
            UINT HorizontalFlip                  : 1;    // Can flip the data horizontally
            UINT StretchRGB                      : 1;    // Supports stretching RGB formats
            UINT StretchYUV                      : 1;    // Supports stretching YUV formats
            UINT BilinearFilter                  : 1;    // Blinear filtering
            UINT HighFilter                      : 1;    // Better than bilinear filtering
            UINT Shared                          : 1;    // MPO resources are shared across VidPnSources
            UINT Immediate                       : 1;    // Immediate flip support
            UINT Plane0ForVirtualModeOnly        : 1;    // Stretching plane 0 will also stretch the HW cursor and should only be used for virtual mode support
            UINT Version3DDISupport              : 1;    // Driver supports the 2.2 MPO DDIs
            UINT Reserved                        : 20;
        };
        UINT Value;
    };
} D3DKMT_MULTIPLANE_OVERLAY_CAPS;

typedef struct _D3DKMT_GET_MULTIPLANE_OVERLAY_CAPS
{
    D3DKMT_HANDLE                          hAdapter;             // in: adapter handle
    D3DDDI_VIDEO_PRESENT_SOURCE_ID         VidPnSourceId;        // in
    UINT                                   MaxPlanes;            // out: Total number of planes currently supported
    UINT                                   MaxRGBPlanes;         // out: Number of RGB planes currently supported
    UINT                                   MaxYUVPlanes;         // out: Number of YUV planes currently supported
    D3DKMT_MULTIPLANE_OVERLAY_CAPS         OverlayCaps;          // out: Overlay capabilities
    float                                  MaxStretchFactor;     // out
    float                                  MaxShrinkFactor;      // out
} D3DKMT_GET_MULTIPLANE_OVERLAY_CAPS;

typedef struct _D3DKMT_GET_POST_COMPOSITION_CAPS
{
    D3DKMT_HANDLE                          hAdapter;             // in: adapter handle
    D3DDDI_VIDEO_PRESENT_SOURCE_ID         VidPnSourceId;        // in
    float                                  MaxStretchFactor;     // out
    float                                  MaxShrinkFactor;      // out
} D3DKMT_GET_POST_COMPOSITION_CAPS;

typedef struct _D3DKMT_MULTIPLANEOVERLAY_STRETCH_SUPPORT
{
    UINT VidPnSourceId;
    BOOL Update;
    BOOL Supported;
} D3DKMT_MULTIPLANEOVERLAY_STRETCH_SUPPORT;
#endif  // DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2

typedef struct D3DKMT_MULTIPLANE_OVERLAY
{
    UINT                                 LayerIndex;
    BOOL                                 Enabled;
    D3DKMT_HANDLE                        hAllocation;
    D3DKMT_MULTIPLANE_OVERLAY_ATTRIBUTES PlaneAttributes;
} D3DKMT_MULTIPLANE_OVERLAY;

typedef struct D3DKMT_PRESENT_MULTIPLANE_OVERLAY
{
    union
    {
        D3DKMT_HANDLE               hDevice;            // in: D3D10 compatibility.
        D3DKMT_HANDLE               hContext;           // in: Indentifies the context
    };
    ULONG                           BroadcastContextCount;                          // in: Specifies the number of context
                                                                                    //     to broadcast this command buffer to.
    D3DKMT_HANDLE                   BroadcastContext[D3DDDI_MAX_BROADCAST_CONTEXT]; // in: Specifies the handle of the context to
                                                                                    //     broadcast to.

    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;      // in: VidPn source ID if RestrictVidPnSource is flagged
    UINT                            PresentCount;       // in: present counter
    D3DDDI_FLIPINTERVAL_TYPE        FlipInterval;       // in: flip interval
    D3DKMT_PRESENTFLAGS             Flags;              // in:

    UINT                            PresentPlaneCount;
    D3DKMT_MULTIPLANE_OVERLAY*      pPresentPlanes;
    UINT                            Duration;
} D3DKMT_PRESENT_MULTIPLANE_OVERLAY;

typedef struct _D3DKMT_RENDERFLAGS
{
    UINT    ResizeCommandBuffer     :  1;  // 0x00000001
    UINT    ResizeAllocationList    :  1;  // 0x00000002
    UINT    ResizePatchLocationList :  1;  // 0x00000004
    UINT    NullRendering           :  1;  // 0x00000008
    UINT    PresentRedirected       :  1;  // 0x00000010
    UINT    RenderKm                :  1;  // 0x00000020    Cannot be used with DxgkRender
    UINT    RenderKmReadback        :  1;  // 0x00000040    Cannot be used with DxgkRender
    UINT    Reserved                : 25;  // 0xFFFFFF80
} D3DKMT_RENDERFLAGS;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
typedef struct _D3DKMT_OUTPUTDUPLPRESENTFLAGS
{
    union
    {
        struct
        {
            UINT                ProtectedContentBlankedOut  :  1;
            UINT                RemoteSession               :  1;
            UINT                FullScreenPresent           :  1;
            UINT                PresentIndirect             :  1;
            UINT                Reserved                    : 28;
        };
        UINT    Value;
    };
}D3DKMT_OUTPUTDUPLPRESENTFLAGS;

typedef struct _D3DKMT_OUTPUTDUPLPRESENT
{
    D3DKMT_HANDLE                   hContext;           // in: Indentifies the context
    D3DKMT_HANDLE                   hSource;            // in: Source allocation to present from
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
    ULONG                           BroadcastContextCount;                          // in: Specifies the number of context
    D3DKMT_HANDLE                   BroadcastContext[D3DDDI_MAX_BROADCAST_CONTEXT]; // in: Specifies the handle of the context to
    D3DKMT_PRESENT_RGNS             PresentRegions;     // in: Dirty and move regions
    D3DKMT_OUTPUTDUPLPRESENTFLAGS   Flags;
    D3DKMT_HANDLE                   hIndirectContext;
} D3DKMT_OUTPUTDUPLPRESENT;
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)

typedef struct _D3DKMT_OUTPUTDUPLPRESENTTOHWQUEUE
{
    D3DKMT_HANDLE                   hSource;            // in: Source allocation to present from
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
    ULONG                           BroadcastHwQueueCount;
    D3DKMT_PTR(_Field_size_(BroadcastHwQueueCount)
    D3DKMT_HANDLE*,                 hHwQueues);
    D3DKMT_PRESENT_RGNS             PresentRegions;     // in: Dirty and move regions
    D3DKMT_OUTPUTDUPLPRESENTFLAGS   Flags;
    D3DKMT_HANDLE                   hIndirectHwQueue;
} D3DKMT_OUTPUTDUPLPRESENTTOHWQUEUE;

#endif

typedef struct _D3DKMT_RENDER
{
    union
    {
        D3DKMT_HANDLE               hDevice;                    // in: D3D10 compatibility.
        D3DKMT_HANDLE               hContext;                   // in: Indentifies the context
    };
    UINT                            CommandOffset;              // in: offset in bytes from start
    UINT                            CommandLength;              // in: number of bytes
    UINT                            AllocationCount;            // in: Number of allocations in allocation list.
    UINT                            PatchLocationCount;         // in: Number of patch locations in patch allocation list.
    D3DKMT_PTR(VOID*,               pNewCommandBuffer);         // out: Pointer to the next command buffer to use.
                                                                // in: When RenderKm flag is set, it points to a command buffer.
    UINT                            NewCommandBufferSize;       // in: Size requested for the next command buffer.
                                                                // out: Size of the next command buffer to use.
    D3DKMT_PTR(D3DDDI_ALLOCATIONLIST*, pNewAllocationList);     // out: Pointer to the next allocation list to use.
                                                                // in: When RenderKm flag is set, it points to an allocation list.
    UINT                            NewAllocationListSize;      // in: Size requested for the next allocation list.
                                                                // out: Size of the new allocation list.
    D3DKMT_PTR(D3DDDI_PATCHLOCATIONLIST*, pNewPatchLocationList); // out: Pointer to the next patch location list.
    UINT                            NewPatchLocationListSize;   // in: Size requested for the next patch location list.
                                                                // out: Size of the new patch location list.
    D3DKMT_RENDERFLAGS              Flags;                      // in:
    D3DKMT_ALIGN64 ULONGLONG        PresentHistoryToken;        // in: Present history token for redirected present calls
    ULONG                           BroadcastContextCount;                          // in: Specifies the number of context
                                                                                    //     to broadcast this command buffer to.
    D3DKMT_HANDLE                   BroadcastContext[D3DDDI_MAX_BROADCAST_CONTEXT]; // in: Specifies the handle of the context to
                                                                                    //     broadcast to.
    ULONG                           QueuedBufferCount;          // out: Number of DMA buffer queued to this context after this submission.
    D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS NewCommandBuffer;     // out: GPU virtual address of next command buffer to use. _ADVSCH_
    D3DKMT_PTR(VOID*,               pPrivateDriverData);        // in: pointer to private driver data. _ADVSCH_
    UINT                            PrivateDriverDataSize;      // in: size of private driver data. _ADVSCH_
} D3DKMT_RENDER;


typedef enum _D3DKMT_STANDARDALLOCATIONTYPE
{
    D3DKMT_STANDARDALLOCATIONTYPE_EXISTINGHEAP = 1,
    D3DKMT_STANDARDALLOCATIONTYPE_INTERNALBACKINGSTORE = 2,
    D3DKMT_STANDARDALLOCATIONTYPE_MAX,
} D3DKMT_STANDARDALLOCATIONTYPE;

typedef struct _D3DKMT_STANDARDALLOCATION_EXISTINGHEAP
{
    D3DKMT_ALIGN64 D3DKMT_SIZE_T Size;        // in: Size in bytes of existing heap
} D3DKMT_STANDARDALLOCATION_EXISTINGHEAP;

typedef struct _D3DKMT_CREATESTANDARDALLOCATIONFLAGS
{
    union
    {
        struct
        {
            UINT Reserved : 32; // 0xFFFFFFFF
        };
        UINT Value;
    };
} D3DKMT_CREATESTANDARDALLOCATIONFLAGS;

typedef struct _D3DKMT_CREATESTANDARDALLOCATION
{
    //
    // update onecoreuap/windows/core/ntuser/inc/whwin32.tpl when adding new memeber
    // to this struct
    //
    D3DKMT_STANDARDALLOCATIONTYPE Type;
    union
    {
        D3DKMT_STANDARDALLOCATION_EXISTINGHEAP ExistingHeapData;
    };
    D3DKMT_CREATESTANDARDALLOCATIONFLAGS Flags;
} D3DKMT_CREATESTANDARDALLOCATION;

typedef struct _D3DKMT_CREATEALLOCATIONFLAGS
{
    UINT    CreateResource              :  1;    // 0x00000001
    UINT    CreateShared                :  1;    // 0x00000002
    UINT    NonSecure                   :  1;    // 0x00000004
    UINT    CreateProtected             :  1;    // 0x00000008 Cannot be used when allocation is created from the user mode.
    UINT    RestrictSharedAccess        :  1;    // 0x00000010
    UINT    ExistingSysMem              :  1;    // 0x00000020 Cannot be used when allocation is created from the user mode.
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    UINT    NtSecuritySharing           :  1;    // 0x00000040
    UINT    ReadOnly                    :  1;    // 0x00000080
    UINT    CreateWriteCombined         :  1;    // 0x00000100 Cannot be used when allocation is created from the user mode.
    UINT    CreateCached                :  1;    // 0x00000200 Cannot be used when allocation is created from the user mode.
    UINT    SwapChainBackBuffer         :  1;    // 0x00000400 Specifies whether an allocation corresponds to a swap chain back buffer.
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    UINT    CrossAdapter                :  1;    // 0x00000800
    UINT    OpenCrossAdapter            :  1;    // 0x00001000 Cannot be used when allocation is created from the user mode.
    UINT    PartialSharedCreation       :  1;    // 0x00002000
    UINT    Zeroed                      :  1;    // 0x00004000  // out: set when allocation fulfilled by zero pages
    UINT    WriteWatch                  :  1;    // 0x00008000  // in: request Mm to track writes to pages of this allocation
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
    UINT    StandardAllocation          :  1;    // 0x00010000  // in: use pStandardAllocation instead of pPrivateDriverData
    UINT    ExistingSection             :  1;    // 0x00020000  // in: Use Section Handle instead of SysMem in D3DDI_ALLOCATIONINFO2
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)
    UINT    AllowNotZeroed              :  1;    // 0x00040000  // in: indicate zeroed pages are not required
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_7)
    UINT    PhysicallyContiguous        :  1;    // 0x00080000  // in: indicate allocation must be physically contguous
    UINT    NoKmdAccess                 :  1;    // 0x00100000  // in: KMD is not notified about the allocation
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)
    UINT    SharedDisplayable           :  1;    // 0x00200000
    UINT    Reserved                    : 10;    // 0xFFC00000
#else
    UINT    Reserved                    : 11;    // 0xFFE00000
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)
#else
    UINT    Reserved                    : 13;    // 0xFFF80000
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_7)
#else
    UINT    Reserved                    : 14;    // 0xFFFC0000
#endif //(DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)
#else
    UINT    Reserved                    : 16;    // 0xFFFF0000
#endif //(DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
#else
    UINT    Reserved                    : 21;    // 0xFFFFF800
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
#else
    UINT    Reserved                    : 26;    // 0xFFFFFFC0
#endif
} D3DKMT_CREATEALLOCATIONFLAGS;

typedef struct _D3DKMT_CREATEALLOCATION
{
    D3DKMT_HANDLE                   hDevice;
    D3DKMT_HANDLE                   hResource;      //in/out:valid only within device
    D3DKMT_HANDLE                   hGlobalShare;   //out:Shared handle if CreateShared and not NtSecuritySharing
    D3DKMT_PTR(_Field_size_bytes_(PrivateRuntimeDataSize)
    CONST VOID*,                    pPrivateRuntimeData);
    UINT                            PrivateRuntimeDataSize;
    union
    {
        //
        // update onecoreuap/windows/core/ntuser/inc/whwin32.tpl when adding new memeber
        // to this union
        //
        D3DKMT_CREATESTANDARDALLOCATION* pStandardAllocation;
        _Field_size_bytes_(PrivateDriverDataSize)
        CONST VOID*                      pPrivateDriverData;
        D3DKMT_PTR_HELPER(               AlignUnionTo64_1)
    };
    UINT                            PrivateDriverDataSize;
    UINT                            NumAllocations;
    union
    {
        _Field_size_(NumAllocations)       D3DDDI_ALLOCATIONINFO*   pAllocationInfo;
#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN7))
        _Field_size_(NumAllocations)       D3DDDI_ALLOCATIONINFO2*  pAllocationInfo2; // _ADVSCH_
#endif
        D3DKMT_PTR_HELPER(                                          AlignUnionTo64_2)
    };
    D3DKMT_CREATEALLOCATIONFLAGS    Flags;
    D3DKMT_PTR(HANDLE,              hPrivateRuntimeResourceHandle); // opaque handle used for event tracing
} D3DKMT_CREATEALLOCATION;

typedef struct _D3DKMT_OPENRESOURCE
{
                                                        D3DKMT_HANDLE               hDevice;                            // in : Indentifies the device
                                                        D3DKMT_HANDLE               hGlobalShare;                       // in : Shared resource handle
                                                        UINT                        NumAllocations;                     // in : Number of allocations associated with the resource
   union {
    _Field_size_(NumAllocations)                      D3DDDI_OPENALLOCATIONINFO*  pOpenAllocationInfo;                // in : Array of open allocation structs
#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN7))
    _Field_size_(NumAllocations)                      D3DDDI_OPENALLOCATIONINFO2* pOpenAllocationInfo2;                // in : Array of open allocation structs // _ADVSCH_
#endif
    D3DKMT_PTR_HELPER(AlignUnionTo64)
   };
    D3DKMT_PTR(_Field_size_bytes_(PrivateRuntimeDataSize) VOID*,                    pPrivateRuntimeData);               // in : Caller supplied buffer where the runtime private data associated with this resource will be copied
                                                        UINT                        PrivateRuntimeDataSize;             // in : Size in bytes of the pPrivateRuntimeData buffer
    D3DKMT_PTR(_Field_size_bytes_(ResourcePrivateDriverDataSize) VOID*,             pResourcePrivateDriverData);        // in : Caller supplied buffer where the driver private data associated with the resource will be copied
                                                        UINT                        ResourcePrivateDriverDataSize;      // in : Size in bytes of the pResourcePrivateDriverData buffer
    D3DKMT_PTR(_Field_size_bytes_(TotalPrivateDriverDataBufferSize) VOID*,          pTotalPrivateDriverDataBuffer);     // in : Caller supplied buffer where the Driver private data will be stored
                                                        UINT                        TotalPrivateDriverDataBufferSize;   // in/out : Size in bytes of pTotalPrivateDriverDataBuffer / Size in bytes of data written to pTotalPrivateDriverDataBuffer
                                                        D3DKMT_HANDLE               hResource;                          // out : Handle for this resource in this process
}D3DKMT_OPENRESOURCE;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
typedef struct _D3DKMT_OPENRESOURCEFROMNTHANDLE
{
                                                        D3DKMT_HANDLE               hDevice;                            // in : Indentifies the device
    D3DKMT_PTR(                                         HANDLE,                     hNtHandle);                         // in : Process's NT handle
                                                        UINT                        NumAllocations;                     // in : Number of allocations associated with the resource
    D3DKMT_PTR(_Field_size_(NumAllocations)             D3DDDI_OPENALLOCATIONINFO2*, pOpenAllocationInfo2);             // in : Array of open allocation structs // _ADVSCH_
                                                        UINT                        PrivateRuntimeDataSize;             // in : Size in bytes of the pPrivateRuntimeData buffer
    D3DKMT_PTR(_Field_size_bytes_(PrivateRuntimeDataSize) VOID*,                    pPrivateRuntimeData);               // in : Caller supplied buffer where the runtime private data associated with this resource will be copied
                                                        UINT                        ResourcePrivateDriverDataSize;      // in : Size in bytes of the pResourcePrivateDriverData buffer
    D3DKMT_PTR(_Field_size_bytes_(ResourcePrivateDriverDataSize) VOID*,             pResourcePrivateDriverData);        // in : Caller supplied buffer where the driver private data associated with the resource will be copied
                                                        UINT                        TotalPrivateDriverDataBufferSize;   // in/out : Size in bytes of pTotalPrivateDriverDataBuffer / Size in bytes of data written to pTotalPrivateDriverDataBuffer
    D3DKMT_PTR(_Field_size_bytes_(TotalPrivateDriverDataBufferSize) VOID*,          pTotalPrivateDriverDataBuffer);     // in : Caller supplied buffer where the Driver private data will be stored
                                                        D3DKMT_HANDLE               hResource;                          // out : Handle for this resource in this process

                                                        D3DKMT_HANDLE               hKeyedMutex;                        // out: Handle to the keyed mutex in this process
    D3DKMT_PTR(_In_reads_bytes_opt_(PrivateRuntimeDataSize) VOID*,                  pKeyedMutexPrivateRuntimeData);     // in:  Buffer containing initial private data.
                                                                                                                        //      If NULL then PrivateRuntimeDataSize must be 0.
                                                                                                                        //      It will only be copied if the keyed mutex does not already have private data.
                                                        UINT                        KeyedMutexPrivateRuntimeDataSize;   // in:  Size in bytes of pPrivateRuntimeData.
                                                        D3DKMT_HANDLE               hSyncObject;                        // out: Handle to sync object in this process.
} D3DKMT_OPENRESOURCEFROMNTHANDLE;

typedef struct _D3DKMT_OPENSYNCOBJECTFROMNTHANDLE
{
    D3DKMT_PTR(HANDLE, hNtHandle);                      // in : NT handle for the sync object.
    D3DKMT_HANDLE   hSyncObject;                        // out: Handle to sync object in this process.
} D3DKMT_OPENSYNCOBJECTFROMNTHANDLE;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

typedef struct _D3DKMT_OPENSYNCOBJECTFROMNTHANDLE2
{
    D3DKMT_PTR(HANDLE,                  hNtHandle);     // in : NT handle for the sync object.
    D3DKMT_HANDLE                       hDevice;        // in : Device handle to use this sync object on.
    D3DDDI_SYNCHRONIZATIONOBJECT_FLAGS  Flags;          // in : specifies sync object behavior for this device.
    D3DKMT_HANDLE                       hSyncObject;    // out: Handle to sync object in this process.

    union
    {

        struct
        {
            D3DKMT_PTR(VOID*,       FenceValueCPUVirtualAddress);           // out: Read-only mapping of the fence value for the CPU
            D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS FenceValueGPUVirtualAddress; // out: Read/write mapping of the fence value for the GPU
            UINT                    EngineAffinity;                         // in: Defines physical adapters where the GPU VA should be mapped
        } MonitoredFence;

        D3DKMT_ALIGN64 UINT64       Reserved[8];
    };

} D3DKMT_OPENSYNCOBJECTFROMNTHANDLE2;

typedef struct _D3DKMT_OPENSYNCOBJECTNTHANDLEFROMNAME
{
    DWORD                          dwDesiredAccess;
    D3DKMT_PTR(OBJECT_ATTRIBUTES*, pObjAttrib);
    D3DKMT_PTR(HANDLE,             hNtHandle);
} D3DKMT_OPENSYNCOBJECTNTHANDLEFROMNAME;

#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_0

typedef struct _D3DKMT_OPENNTHANDLEFROMNAME
{
    DWORD                          dwDesiredAccess;
    D3DKMT_PTR(OBJECT_ATTRIBUTES*, pObjAttrib);
    D3DKMT_PTR(HANDLE,             hNtHandle);
} D3DKMT_OPENNTHANDLEFROMNAME;

#define SHARED_ALLOCATION_WRITE         0x1
#define SHARED_ALLOCATION_ALL_ACCESS    (STANDARD_RIGHTS_REQUIRED | SHARED_ALLOCATION_WRITE)

typedef struct _D3DKMT_QUERYRESOURCEINFOFROMNTHANDLE
{
    D3DKMT_HANDLE      hDevice;                        // in : Indentifies the device
    D3DKMT_PTR(HANDLE, hNtHandle);                     // in : Global resource handle to open
    D3DKMT_PTR(VOID*,  pPrivateRuntimeData);           // in : Ptr to buffer that will receive runtime private data for the resource
    UINT               PrivateRuntimeDataSize;         // in/out : Size in bytes of buffer passed in for runtime private data / If pPrivateRuntimeData was NULL then size in bytes of buffer required for the runtime private data otherwise size in bytes of runtime private data copied into the buffer
    UINT               TotalPrivateDriverDataSize;     // out : Size in bytes of buffer required to hold all the DriverPrivate data for all of the allocations associated withe the resource
    UINT               ResourcePrivateDriverDataSize;  // out : Size in bytes of the driver's resource private data
    UINT               NumAllocations;                 // out : Number of allocations associated with this resource
}D3DKMT_QUERYRESOURCEINFOFROMNTHANDLE;

#endif

typedef struct _D3DKMT_QUERYRESOURCEINFO
{
    D3DKMT_HANDLE   hDevice;                        // in : Indentifies the device
    D3DKMT_HANDLE   hGlobalShare;                   // in : Global resource handle to open
    D3DKMT_PTR(VOID*, pPrivateRuntimeData);         // in : Ptr to buffer that will receive runtime private data for the resource
    UINT            PrivateRuntimeDataSize;         // in/out : Size in bytes of buffer passed in for runtime private data / If pPrivateRuntimeData was NULL then size in bytes of buffer required for the runtime private data otherwise size in bytes of runtime private data copied into the buffer
    UINT            TotalPrivateDriverDataSize;     // out : Size in bytes of buffer required to hold all the DriverPrivate data for all of the allocations associated withe the resource
    UINT            ResourcePrivateDriverDataSize;  // out : Size in bytes of the driver's resource private data
    UINT            NumAllocations;                 // out : Number of allocations associated with this resource
}D3DKMT_QUERYRESOURCEINFO;

typedef struct _D3DKMT_DESTROYALLOCATION
{
    D3DKMT_HANDLE           hDevice;            // in: Indentifies the device
    D3DKMT_HANDLE           hResource;
    D3DKMT_PTR(CONST D3DKMT_HANDLE*, phAllocationList);   // in: pointer to an array allocation handles to destroy
    UINT                    AllocationCount;    // in: Number of allocations in phAllocationList
} D3DKMT_DESTROYALLOCATION;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

typedef struct _D3DKMT_DESTROYALLOCATION2
{
    D3DKMT_HANDLE                       hDevice;            // in: Indentifies the device
    D3DKMT_HANDLE                       hResource;
    D3DKMT_PTR(CONST D3DKMT_HANDLE*,    phAllocationList);  // in: pointer to an array allocation handles to destroy
    UINT                                AllocationCount;    // in: Number of allocations in phAllocationList
    D3DDDICB_DESTROYALLOCATION2FLAGS    Flags;              // in: Bit field defined by D3DDDICB_DESTROYALLOCATION2FLAGS
} D3DKMT_DESTROYALLOCATION2;

#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_0

typedef struct _D3DKMT_SETALLOCATIONPRIORITY
{
    D3DKMT_HANDLE           hDevice;            // in: Indentifies the device
    D3DKMT_HANDLE           hResource;          // in: Specify the resource to set priority to.
    D3DKMT_PTR(CONST D3DKMT_HANDLE*, phAllocationList);   // in: pointer to an array allocation handles to destroy
    UINT                    AllocationCount;    // in: Number of allocations in phAllocationList
    D3DKMT_PTR(CONST UINT*, pPriorities);       // in: New priority for each of the allocation in the array.
} D3DKMT_SETALLOCATIONPRIORITY;

typedef enum _D3DKMT_ALLOCATIONRESIDENCYSTATUS
{
    D3DKMT_ALLOCATIONRESIDENCYSTATUS_RESIDENTINGPUMEMORY=1,
    D3DKMT_ALLOCATIONRESIDENCYSTATUS_RESIDENTINSHAREDMEMORY=2,
    D3DKMT_ALLOCATIONRESIDENCYSTATUS_NOTRESIDENT=3,
} D3DKMT_ALLOCATIONRESIDENCYSTATUS;

typedef struct _D3DKMT_QUERYALLOCATIONRESIDENCY
{
    D3DKMT_HANDLE                       hDevice;            // in: Indentifies the device
    D3DKMT_HANDLE                       hResource;          // in: pointer to resource owning the list of allocation.
    D3DKMT_PTR(CONST D3DKMT_HANDLE*,    phAllocationList);  // in: pointer to an array allocation to get residency status.
    UINT                                AllocationCount;    // in: Number of allocations in phAllocationList
    D3DKMT_PTR(D3DKMT_ALLOCATIONRESIDENCYSTATUS*, pResidencyStatus); // out: Residency status of each allocation in the array.
} D3DKMT_QUERYALLOCATIONRESIDENCY;

typedef struct _D3DKMT_GETRUNTIMEDATA
{
    D3DKMT_HANDLE       hAdapter;
    D3DKMT_HANDLE       hGlobalShare;       // in: shared handle
    D3DKMT_PTR(VOID*,   pRuntimeData);      // out: in: for a version?
    UINT                RuntimeDataSize;    // in:
} D3DKMT_GETRUNTIMEDATA;

typedef enum _KMTUMDVERSION
{
    KMTUMDVERSION_DX9 = 0,
    KMTUMDVERSION_DX10,
    KMTUMDVERSION_DX11,
    KMTUMDVERSION_DX12,
    NUM_KMTUMDVERSIONS
} KMTUMDVERSION;

typedef struct _D3DKMT_UMDFILENAMEINFO
{
    KMTUMDVERSION       Version;                // In: UMD version
    WCHAR               UmdFileName[MAX_PATH];  // Out: UMD file name
} D3DKMT_UMDFILENAMEINFO;

#define D3DKMT_COMPONENTIZED_INDICATOR  L'#'
#define D3DKMT_SUBKEY_DX9               L"DX9"
#define D3DKMT_SUBKEY_OPENGL            L"OpenGL"

typedef struct _D3DKMT_OPENGLINFO
{
    WCHAR               UmdOpenGlIcdFileName[MAX_PATH];
    ULONG               Version;
    ULONG               Flags;
} D3DKMT_OPENGLINFO;

typedef struct _D3DKMT_SEGMENTSIZEINFO
{
    D3DKMT_ALIGN64 ULONGLONG           DedicatedVideoMemorySize;
    D3DKMT_ALIGN64 ULONGLONG           DedicatedSystemMemorySize;
    D3DKMT_ALIGN64 ULONGLONG           SharedSystemMemorySize;
} D3DKMT_SEGMENTSIZEINFO;

typedef struct _D3DKMT_SEGMENTGROUPSIZEINFO
{
    UINT32 PhysicalAdapterIndex;
    D3DKMT_SEGMENTSIZEINFO LegacyInfo;
    D3DKMT_ALIGN64 ULONGLONG LocalMemory;
    D3DKMT_ALIGN64 ULONGLONG NonLocalMemory;
    D3DKMT_ALIGN64 ULONGLONG NonBudgetMemory;
} D3DKMT_SEGMENTGROUPSIZEINFO;

typedef struct _D3DKMT_WORKINGSETFLAGS
{
    UINT    UseDefault   :  1;   // 0x00000001
    UINT    Reserved     : 31;   // 0xFFFFFFFE
} D3DKMT_WORKINGSETFLAGS;

typedef struct _D3DKMT_WORKINGSETINFO
{
    D3DKMT_WORKINGSETFLAGS Flags;
    ULONG MinimumWorkingSetPercentile;
    ULONG MaximumWorkingSetPercentile;
} D3DKMT_WORKINGSETINFO;

typedef struct _D3DKMT_FLIPINFOFLAGS
{
    UINT                FlipInterval :  1; // 0x00000001 // Set when kmd driver support FlipInterval natively
    UINT                Reserved     : 31; // 0xFFFFFFFE
} D3DKMT_FLIPINFOFLAGS;

typedef struct _D3DKMT_FLIPQUEUEINFO
{
    UINT                 MaxHardwareFlipQueueLength; // Max flip can be queued for hardware flip queue.
    UINT                 MaxSoftwareFlipQueueLength; // Max flip can be queued for software flip queue for non-legacy device.
    D3DKMT_FLIPINFOFLAGS FlipFlags;
} D3DKMT_FLIPQUEUEINFO;

typedef struct _D3DKMT_ADAPTERADDRESS
{
    UINT   BusNumber;              // Bus number on which the physical device is located.
    UINT   DeviceNumber;           // Index of the physical device on the bus.
    UINT   FunctionNumber;         // Function number of the adapter on the physical device.
} D3DKMT_ADAPTERADDRESS;

typedef struct _D3DKMT_ADAPTERREGISTRYINFO
{
    WCHAR   AdapterString[MAX_PATH];
    WCHAR   BiosString[MAX_PATH];
    WCHAR   DacType[MAX_PATH];
    WCHAR   ChipType[MAX_PATH];
} D3DKMT_ADAPTERREGISTRYINFO;

typedef struct _D3DKMT_CURRENTDISPLAYMODE
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    D3DKMT_DISPLAYMODE DisplayMode;
} D3DKMT_CURRENTDISPLAYMODE;

typedef struct _D3DKMT_VIRTUALADDRESSFLAGS // _ADVSCH_
{
    UINT   VirtualAddressSupported :  1;
    UINT   Reserved                : 31;
} D3DKMT_VIRTUALADDRESSFLAGS;

typedef struct _D3DKMT_VIRTUALADDRESSINFO // _ADVSCH_
{
    D3DKMT_VIRTUALADDRESSFLAGS VirtualAddressFlags;
} D3DKMT_VIRTUALADDRESSINFO;

typedef enum _QAI_DRIVERVERSION
{
    KMT_DRIVERVERSION_WDDM_1_0               = 1000,
    KMT_DRIVERVERSION_WDDM_1_1_PRERELEASE    = 1102,
    KMT_DRIVERVERSION_WDDM_1_1               = 1105,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    KMT_DRIVERVERSION_WDDM_1_2               = 1200,
#endif
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    KMT_DRIVERVERSION_WDDM_1_3               = 1300,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM1_3
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    KMT_DRIVERVERSION_WDDM_2_0               = 2000,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_0
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
    KMT_DRIVERVERSION_WDDM_2_1               = 2100,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_1
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
    KMT_DRIVERVERSION_WDDM_2_2 = 2200,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_2
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
    KMT_DRIVERVERSION_WDDM_2_3 = 2300,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_3
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
    KMT_DRIVERVERSION_WDDM_2_4 = 2400,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_4
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)
    KMT_DRIVERVERSION_WDDM_2_5 = 2500,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_5
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)
    KMT_DRIVERVERSION_WDDM_2_6 = 2600,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_6
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_7)
    KMT_DRIVERVERSION_WDDM_2_7 = 2700,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_7
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_8)
    KMT_DRIVERVERSION_WDDM_2_8 = 2800,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_8
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
    KMT_DRIVERVERSION_WDDM_2_9 = 2900,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_9
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)
    KMT_DRIVERVERSION_WDDM_3_0 = 3000
#endif // DXGKDDI_INTERFACE_VERSION_WDDM3_0
} D3DKMT_DRIVERVERSION;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
typedef struct _D3DKMT_ADAPTERTYPE
{
    union
    {
        struct
        {
            UINT   RenderSupported       :  1;
            UINT   DisplaySupported      :  1;
            UINT   SoftwareDevice        :  1;
            UINT   PostDevice            :  1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
            UINT   HybridDiscrete        :  1;
            UINT   HybridIntegrated      :  1;
            UINT   IndirectDisplayDevice :  1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
            UINT   Paravirtualized              :  1;
            UINT   ACGSupported                 :  1;
            UINT   SupportSetTimingsFromVidPn   :  1;
            UINT   Detachable                   :  1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)
            UINT   ComputeOnly                  :  1;
            UINT   Prototype                    :  1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
            UINT   RuntimePowerManagement       :  1;
            UINT   Reserved                     : 18;
#else
            UINT   Reserved                     : 19;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
#else
            UINT   Reserved              : 21;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)
#else
            UINT   Reserved              : 25;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
#else
            UINT   Reserved              : 28;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
        };
        UINT Value;
    };
} D3DKMT_ADAPTERTYPE;

typedef struct _D3DKMT_OUTPUTDUPLCONTEXTSCOUNT
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    UINT OutputDuplicationCount;
} D3DKMT_OUTPUTDUPLCONTEXTSCOUNT;

typedef struct _D3DKMT_UMD_DRIVER_VERSION
{
    D3DKMT_ALIGN64 LARGE_INTEGER DriverVersion;
} D3DKMT_UMD_DRIVER_VERSION;

typedef struct _D3DKMT_KMD_DRIVER_VERSION
{
    D3DKMT_ALIGN64 LARGE_INTEGER DriverVersion;
} D3DKMT_KMD_DRIVER_VERSION;

typedef struct _D3DKMT_DIRECTFLIP_SUPPORT
{
    BOOL Supported;
} D3DKMT_DIRECTFLIP_SUPPORT;

typedef struct _D3DKMT_MULTIPLANEOVERLAY_SUPPORT
{
    BOOL Supported;
} D3DKMT_MULTIPLANEOVERLAY_SUPPORT;
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3_PATH_INDEPENDENT_ROTATION)
typedef struct _D3DKMT_MULTIPLANEOVERLAY_HUD_SUPPORT
{
    UINT VidPnSourceId; // Not yet used.
    BOOL Update;
    BOOL KernelSupported;
    BOOL HudSupported;
} D3DKMT_MULTIPLANEOVERLAY_HUD_SUPPORT;
#endif // DXGKDDI_INTERFACE_VERSION_WDDM1_3_PATH_INDEPENDENT_ROTATION

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)

typedef struct _D3DKMT_DLIST_DRIVER_NAME
{
    WCHAR DListFileName[MAX_PATH];  // Out: DList driver file name
} D3DKMT_DLIST_DRIVER_NAME;

typedef struct _D3DKMT_CPDRIVERNAME
{
    WCHAR  ContentProtectionFileName[MAX_PATH];
} D3DKMT_CPDRIVERNAME;

typedef struct _D3DKMT_MIRACASTCOMPANIONDRIVERNAME
{
    WCHAR  MiracastCompanionDriverName[MAX_PATH];
} D3DKMT_MIRACASTCOMPANIONDRIVERNAME;

#endif // DXGKDDI_INTERFACE_VERSION_WDDM1_3

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

typedef struct _D3DKMT_XBOX
{
    BOOL IsXBOX;
} D3DKMT_XBOX;

typedef struct _D3DKMT_INDEPENDENTFLIP_SUPPORT
{
    BOOL Supported;
} D3DKMT_INDEPENDENTFLIP_SUPPORT;

typedef struct _D3DKMT_MULTIPLANEOVERLAY_DECODE_SUPPORT
{
    BOOL Supported;
} D3DKMT_MULTIPLANEOVERLAY_DECODE_SUPPORT;

typedef struct _D3DKMT_ISBADDRIVERFORHWPROTECTIONDISABLED
{
    BOOL Disabled;
} D3DKMT_ISBADDRIVERFORHWPROTECTIONDISABLED;

typedef struct _D3DKMT_MULTIPLANEOVERLAY_SECONDARY_SUPPORT
{
    BOOL Supported;
} D3DKMT_MULTIPLANEOVERLAY_SECONDARY_SUPPORT;

typedef struct _D3DKMT_INDEPENDENTFLIP_SECONDARY_SUPPORT
{
    BOOL Supported;
} D3DKMT_INDEPENDENTFLIP_SECONDARY_SUPPORT;

typedef struct _D3DKMT_PANELFITTER_SUPPORT
{
    BOOL Supported;
} D3DKMT_PANELFITTER_SUPPORT;

typedef struct _D3DKMT_PHYSICAL_ADAPTER_COUNT
{
    UINT Count;
} D3DKMT_PHYSICAL_ADAPTER_COUNT;

typedef struct _D3DKMT_DEVICE_IDS
{
    UINT VendorID;
    UINT DeviceID;
    UINT SubVendorID;
    UINT SubSystemID;
    UINT RevisionID;
    UINT BusType;
} D3DKMT_DEVICE_IDS;

typedef struct _D3DKMT_QUERY_DEVICE_IDS
{
    UINT              PhysicalAdapterIndex; // in:
    D3DKMT_DEVICE_IDS DeviceIds;            // out:
} D3DKMT_QUERY_DEVICE_IDS;

typedef enum _D3DKMT_PNP_KEY_TYPE
{
    D3DKMT_PNP_KEY_HARDWARE = 1,
    D3DKMT_PNP_KEY_SOFTWARE = 2
} D3DKMT_PNP_KEY_TYPE;

typedef struct _D3DKMT_QUERY_PHYSICAL_ADAPTER_PNP_KEY
{
    UINT PhysicalAdapterIndex;
    D3DKMT_PNP_KEY_TYPE PnPKeyType;
    D3DKMT_PTR(_Field_size_opt_(*pCchDest) WCHAR*, pDest);
    D3DKMT_PTR(UINT*, pCchDest);
} D3DKMT_QUERY_PHYSICAL_ADAPTER_PNP_KEY;

typedef enum _D3DKMT_MIRACAST_DRIVER_TYPE
{
    D3DKMT_MIRACAST_DRIVER_NOT_SUPPORTED = 0,
    D3DKMT_MIRACAST_DRIVER_IHV = 1,
    D3DKMT_MIRACAST_DRIVER_MS = 2,
} D3DKMT_MIRACAST_DRIVER_TYPE;

typedef struct _D3DKMT_QUERY_MIRACAST_DRIVER_TYPE
{
    D3DKMT_MIRACAST_DRIVER_TYPE MiracastDriverType;
} D3DKMT_QUERY_MIRACAST_DRIVER_TYPE;

typedef struct _D3DKMT_GPUMMU_CAPS
{
    union
    {
        struct
        {
            UINT ReadOnlyMemorySupported                : 1;
            UINT NoExecuteMemorySupported               : 1;
            UINT CacheCoherentMemorySupported           : 1;
            UINT Reserved                               : 29;
        };
        UINT        Value;
    } Flags;
    UINT                        VirtualAddressBitCount;
} D3DKMT_GPUMMU_CAPS;

typedef struct _D3DKMT_QUERY_GPUMMU_CAPS
{
    UINT PhysicalAdapterIndex; // in:
    D3DKMT_GPUMMU_CAPS Caps;   // out:
} D3DKMT_QUERY_GPUMMU_CAPS;

#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_0

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

typedef struct _D3DKMT_MPO3DDI_SUPPORT
{
    BOOL Supported;
} D3DKMT_MPO3DDI_SUPPORT;

typedef struct _D3DKMT_HWDRM_SUPPORT
{
    BOOLEAN Supported;
} D3DKMT_HWDRM_SUPPORT;

typedef struct _D3DKMT_MPOKERNELCAPS_SUPPORT
{
    BOOL Supported;
} D3DKMT_MPOKERNELCAPS_SUPPORT;

typedef struct _D3DKMT_GET_DEVICE_VIDPN_OWNERSHIP_INFO
{
    D3DKMT_HANDLE hDevice;                           // in : Indentifies the device
    BOOLEAN bFailedDwmAcquireVidPn;                  // out : True if Dwm Acquire VidPn failed due to another Dwm device having ownership
} D3DKMT_GET_DEVICE_VIDPN_OWNERSHIP_INFO;

#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_2

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)

typedef struct _D3DKMT_BLOCKLIST_INFO
{
    UINT Size;
    WCHAR BlockList[1];
} D3DKMT_BLOCKLIST_INFO;

typedef struct _D3DKMT_QUERY_ADAPTER_UNIQUE_GUID
{
    WCHAR AdapterUniqueGUID[40];
} D3DKMT_QUERY_ADAPTER_UNIQUE_GUID;

typedef struct _D3DKMT_NODE_PERFDATA
{
    UINT32          NodeOrdinal;            // in: Node ordinal of the requested engine.
    UINT32          PhysicalAdapterIndex;   // in: The physical adapter index, in an LDA chain
    D3DKMT_ALIGN64 ULONGLONG Frequency;     // out: Clock frequency of the engine in hertz
    D3DKMT_ALIGN64 ULONGLONG MaxFrequency;  // out: Max engine clock frequency
    D3DKMT_ALIGN64 ULONGLONG MaxFrequencyOC;// out: Max engine over clock frequency
    ULONG           Voltage;                // out: Voltage of the engine in milli volts mV
    ULONG           VoltageMax;             // out: Max voltage levels in milli volts.
    ULONG           VoltageMaxOC;           // out: Max voltage level while overclocked in milli volts.
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)
    D3DKMT_ALIGN64 ULONGLONG MaxTransitionLatency;   // out: Max transition latency to change the frequency in 100 nanoseconds
#else
    D3DKMT_ALIGN64 ULONGLONG Reserved;
#endif
} D3DKMT_NODE_PERFDATA;

typedef struct _D3DKMT_ADAPTER_PERFDATA
{
    UINT32          PhysicalAdapterIndex;   // in: The physical adapter index, in an LDA chain
    D3DKMT_ALIGN64 ULONGLONG MemoryFrequency;        // out: Clock frequency of the memory in hertz
    D3DKMT_ALIGN64 ULONGLONG MaxMemoryFrequency;     // out: Max memory clock frequency
    D3DKMT_ALIGN64 ULONGLONG MaxMemoryFrequencyOC;   // out: Clock frequency of the memory while overclocked in hertz.
    D3DKMT_ALIGN64 ULONGLONG MemoryBandwidth;        // out: Amount of memory transferred in bytes
    D3DKMT_ALIGN64 ULONGLONG PCIEBandwidth;          // out: Amount of memory transferred over PCI-E in bytes
    ULONG           FanRPM;                 // out: Fan rpm
    ULONG           Power;                  // out: Power draw of the adapter in tenths of a percentage
    ULONG           Temperature;            // out: Temperature in deci-Celsius 1 = 0.1C
    UCHAR           PowerStateOverride;     // out: Overrides dxgkrnls power view of linked adapters.
} D3DKMT_ADAPTER_PERFDATA;

typedef struct _D3DKMT_ADAPTER_PERFDATACAPS
{
    UINT32      PhysicalAdapterIndex;   // in: The physical adapter index, in an LDA chain
    D3DKMT_ALIGN64 ULONGLONG MaxMemoryBandwidth;     // out: Max memory bandwidth in bytes for 1 second
    D3DKMT_ALIGN64 ULONGLONG MaxPCIEBandwidth;       // out: Max pcie bandwidth in bytes for 1 second
    ULONG       MaxFanRPM;              // out: Max fan rpm
    ULONG       TemperatureMax;         // out: Max temperature before damage levels
    ULONG       TemperatureWarning;     // out: The temperature level where throttling begins.
} D3DKMT_ADAPTER_PERFDATACAPS;

#define DXGK_MAX_GPUVERSION_NAME_LENGTH 32
typedef struct _D3DKMT_GPUVERSION
{
    UINT32          PhysicalAdapterIndex;                             // in: The physical adapter index, in an LDA chain
    WCHAR           BiosVersion[DXGK_MAX_GPUVERSION_NAME_LENGTH];     //out: The gpu bios version
    WCHAR           GpuArchitecture[DXGK_MAX_GPUVERSION_NAME_LENGTH]; //out: The gpu architectures name.
} D3DKMT_GPUVERSION;

#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_4

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)

typedef struct _D3DKMT_DRIVER_DESCRIPTION
{
    WCHAR           DriverDescription[4096];     //out: The driver description
} D3DKMT_DRIVER_DESCRIPTION;

typedef struct _D3DKMT_QUERY_SCANOUT_CAPS
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    UINT Caps;
} D3DKMT_QUERY_SCANOUT_CAPS;

typedef enum _KMT_DISPLAY_UMD_VERSION
{
    KMT_DISPLAY_UMDVERSION_1 = 0,
    NUM_KMT_DISPLAY_UMDVERSIONS
} KMT_DISPLAY_UMD_VERSION;

typedef struct _D3DKMT_DISPLAY_UMD_FILENAMEINFO
{
    KMT_DISPLAY_UMD_VERSION Version;                // In: UMD version
    WCHAR                   UmdFileName[MAX_PATH];  // Out: UMD file name
} D3DKMT_DISPLAY_UMD_FILENAMEINFO;

typedef struct _D3DKMT_PARAVIRTUALIZATION
{
    // This adapter property originates from the VM/ Container, and is currently replicated on adapters.
    // It precludes extended device functions (i.e. Escapes) for paravirtualized devices which not known at all,
    // and therefore assumed not to be secure enough for demanding server scenarios.
    BOOLEAN SecureContainer;
} D3DKMT_PARAVIRTUALIZATION;

#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_6

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_8)

typedef struct _D3DKMT_HYBRID_DLIST_DLL_SUPPORT
{
    BOOL Supported;
} D3DKMT_HYBRID_DLIST_DLL_SUPPORT;

#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_8

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)


typedef enum _D3DKMT_CROSSADAPTERRESOURCE_SUPPORT_TIER
{
    D3DKMT_CROSSADAPTERRESOURCE_SUPPORT_TIER_NONE    = 0,
    D3DKMT_CROSSADAPTERRESOURCE_SUPPORT_TIER_COPY    = 1,
    D3DKMT_CROSSADAPTERRESOURCE_SUPPORT_TIER_TEXTURE = 2,
    D3DKMT_CROSSADAPTERRESOURCE_SUPPORT_TIER_SCANOUT = 3,
} D3DKMT_CROSSADAPTERRESOURCE_SUPPORT_TIER;

typedef struct _D3DKMT_CROSSADAPTERRESOURCE_SUPPORT
{
    D3DKMT_CROSSADAPTERRESOURCE_SUPPORT_TIER SupportTier;
} D3DKMT_CROSSADAPTERRESOURCE_SUPPORT;

#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_9

typedef enum _KMTQUERYADAPTERINFOTYPE
{
     KMTQAITYPE_UMDRIVERPRIVATE         =  0,
     KMTQAITYPE_UMDRIVERNAME            =  1,
     KMTQAITYPE_UMOPENGLINFO            =  2,
     KMTQAITYPE_GETSEGMENTSIZE          =  3,
     KMTQAITYPE_ADAPTERGUID             =  4,
     KMTQAITYPE_FLIPQUEUEINFO           =  5,
     KMTQAITYPE_ADAPTERADDRESS          =  6,
     KMTQAITYPE_SETWORKINGSETINFO       =  7,
     KMTQAITYPE_ADAPTERREGISTRYINFO     =  8,
     KMTQAITYPE_CURRENTDISPLAYMODE      =  9,
     KMTQAITYPE_MODELIST                = 10,
     KMTQAITYPE_CHECKDRIVERUPDATESTATUS = 11,
     KMTQAITYPE_VIRTUALADDRESSINFO      = 12, // _ADVSCH_
     KMTQAITYPE_DRIVERVERSION           = 13,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
     KMTQAITYPE_ADAPTERTYPE             = 15,
     KMTQAITYPE_OUTPUTDUPLCONTEXTSCOUNT = 16,
     KMTQAITYPE_WDDM_1_2_CAPS           = 17,
     KMTQAITYPE_UMD_DRIVER_VERSION      = 18,
     KMTQAITYPE_DIRECTFLIP_SUPPORT      = 19,
#endif
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
     KMTQAITYPE_MULTIPLANEOVERLAY_SUPPORT = 20,
     KMTQAITYPE_DLIST_DRIVER_NAME       = 21,
     KMTQAITYPE_WDDM_1_3_CAPS           = 22,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM1_3
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3_PATH_INDEPENDENT_ROTATION)
     KMTQAITYPE_MULTIPLANEOVERLAY_HUD_SUPPORT = 23,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM1_3_PATH_INDEPENDENT_ROTATION
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
     KMTQAITYPE_WDDM_2_0_CAPS           = 24,
     KMTQAITYPE_NODEMETADATA            = 25,
     KMTQAITYPE_CPDRIVERNAME            = 26,
     KMTQAITYPE_XBOX                    = 27,
     KMTQAITYPE_INDEPENDENTFLIP_SUPPORT = 28,
     KMTQAITYPE_MIRACASTCOMPANIONDRIVERNAME = 29,
     KMTQAITYPE_PHYSICALADAPTERCOUNT    = 30,
     KMTQAITYPE_PHYSICALADAPTERDEVICEIDS = 31,
     KMTQAITYPE_DRIVERCAPS_EXT          = 32,
     KMTQAITYPE_QUERY_MIRACAST_DRIVER_TYPE = 33,
     KMTQAITYPE_QUERY_GPUMMU_CAPS       = 34,
     KMTQAITYPE_QUERY_MULTIPLANEOVERLAY_DECODE_SUPPORT = 35,
     KMTQAITYPE_QUERY_HW_PROTECTION_TEARDOWN_COUNT = 36,
     KMTQAITYPE_QUERY_ISBADDRIVERFORHWPROTECTIONDISABLED = 37,
     KMTQAITYPE_MULTIPLANEOVERLAY_SECONDARY_SUPPORT = 38,
     KMTQAITYPE_INDEPENDENTFLIP_SECONDARY_SUPPORT = 39,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_0
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
     KMTQAITYPE_PANELFITTER_SUPPORT     = 40,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_1
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
     KMTQAITYPE_PHYSICALADAPTERPNPKEY   = 41,
     KMTQAITYPE_GETSEGMENTGROUPSIZE     = 42,
     KMTQAITYPE_MPO3DDI_SUPPORT         = 43,
     KMTQAITYPE_HWDRM_SUPPORT           = 44,
     KMTQAITYPE_MPOKERNELCAPS_SUPPORT   = 45,
     KMTQAITYPE_MULTIPLANEOVERLAY_STRETCH_SUPPORT = 46,
     KMTQAITYPE_GET_DEVICE_VIDPN_OWNERSHIP_INFO = 47,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_2
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
     KMTQAITYPE_QUERYREGISTRY           = 48,
     KMTQAITYPE_KMD_DRIVER_VERSION      = 49,
     KMTQAITYPE_BLOCKLIST_KERNEL        = 50,
     KMTQAITYPE_BLOCKLIST_RUNTIME       = 51,
     KMTQAITYPE_ADAPTERGUID_RENDER              = 52,
     KMTQAITYPE_ADAPTERADDRESS_RENDER           = 53,
     KMTQAITYPE_ADAPTERREGISTRYINFO_RENDER      = 54,
     KMTQAITYPE_CHECKDRIVERUPDATESTATUS_RENDER  = 55,
     KMTQAITYPE_DRIVERVERSION_RENDER            = 56,
     KMTQAITYPE_ADAPTERTYPE_RENDER              = 57,
     KMTQAITYPE_WDDM_1_2_CAPS_RENDER            = 58,
     KMTQAITYPE_WDDM_1_3_CAPS_RENDER            = 59,
     KMTQAITYPE_QUERY_ADAPTER_UNIQUE_GUID = 60,
     KMTQAITYPE_NODEPERFDATA            = 61,
     KMTQAITYPE_ADAPTERPERFDATA         = 62,
     KMTQAITYPE_ADAPTERPERFDATA_CAPS    = 63,
     KMTQUITYPE_GPUVERSION              = 64,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_4
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)
     KMTQAITYPE_DRIVER_DESCRIPTION        = 65,
     KMTQAITYPE_DRIVER_DESCRIPTION_RENDER = 66,
     KMTQAITYPE_SCANOUT_CAPS              = 67,
     KMTQAITYPE_DISPLAY_UMDRIVERNAME      = 71, // Added in 19H2
     KMTQAITYPE_PARAVIRTUALIZATION_RENDER = 68,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_6
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_7)
     KMTQAITYPE_SERVICENAME = 69,
     KMTQAITYPE_WDDM_2_7_CAPS = 70,
     KMTQAITYPE_TRACKEDWORKLOAD_SUPPORT = 72,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_7
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_8)
     KMTQAITYPE_HYBRID_DLIST_DLL_SUPPORT = 73,
     KMTQAITYPE_DISPLAY_CAPS             = 74,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_8
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
     KMTQAITYPE_WDDM_2_9_CAPS                = 75,
     KMTQAITYPE_CROSSADAPTERRESOURCE_SUPPORT = 76,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_9
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)
     KMTQAITYPE_WDDM_3_0_CAPS                = 77,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM3_0
// If a new enum will be used by DXGI or D3D11 software driver code, update the test content in the area.
// Search for KMTQAITYPE_PARAVIRTUALIZATION_RENDER in directx\dxg\dxgi\unittests for references.
} KMTQUERYADAPTERINFOTYPE;

typedef struct _D3DKMT_QUERYADAPTERINFO
{
    D3DKMT_HANDLE           hAdapter;
    KMTQUERYADAPTERINFOTYPE Type;
    D3DKMT_PTR(VOID*,       pPrivateDriverData);
    UINT                    PrivateDriverDataSize;
} D3DKMT_QUERYADAPTERINFO;

typedef struct _D3DKMT_OPENADAPTERFROMHDC
{
    D3DKMT_PTR(HDC,                 hDc);           // in:  DC that maps to a single display
    D3DKMT_HANDLE                   hAdapter;       // out: adapter handle
    LUID                            AdapterLuid;    // out: adapter LUID
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;  // out: VidPN source ID for that particular display
} D3DKMT_OPENADAPTERFROMHDC;

typedef struct _D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME
{
    WCHAR                           DeviceName[32]; // in:  Name of GDI device from which to open an adapter instance
    D3DKMT_HANDLE                   hAdapter;       // out: adapter handle
    LUID                            AdapterLuid;    // out: adapter LUID
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;  // out: VidPN source ID for that particular display
} D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME;

typedef struct _D3DKMT_OPENADAPTERFROMDEVICENAME
{
    D3DKMT_PTR(PCWSTR,              pDeviceName);   // in:  NULL terminated string containing the device name to open
    D3DKMT_HANDLE                   hAdapter;       // out: adapter handle
    LUID                            AdapterLuid;    // out: adapter LUID
} D3DKMT_OPENADAPTERFROMDEVICENAME;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)

#define MAX_ENUM_ADAPTERS   16

typedef struct _D3DKMT_ADAPTERINFO
{
    D3DKMT_HANDLE       hAdapter;
    LUID                AdapterLuid;
    ULONG               NumOfSources;
    BOOL                bPrecisePresentRegionsPreferred;
} D3DKMT_ADAPTERINFO;

typedef struct _D3DKMT_ENUMADAPTERS
{
    _In_range_(0, MAX_ENUM_ADAPTERS) ULONG  NumAdapters;
    D3DKMT_ADAPTERINFO                      Adapters[MAX_ENUM_ADAPTERS];
} D3DKMT_ENUMADAPTERS;

typedef struct _D3DKMT_ENUMADAPTERS2
{
    ULONG                 NumAdapters;           // in/out: On input, the count of the pAdapters array buffer.  On output, the number of adapters enumerated.
    D3DKMT_PTR(D3DKMT_ADAPTERINFO*, pAdapters);  // out: Array of enumerated adapters containing NumAdapters elements
} D3DKMT_ENUMADAPTERS2;

typedef struct _D3DKMT_OPENADAPTERFROMLUID
{
    LUID            AdapterLuid;
    D3DKMT_HANDLE   hAdapter;
} D3DKMT_OPENADAPTERFROMLUID;

typedef struct _D3DKMT_QUERYREMOTEVIDPNSOURCEFROMGDIDISPLAYNAME
{
    WCHAR                           DeviceName[32]; // in:  Name of GDI device from which to open an adapter instance
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;  // out: VidPN source ID for that particular display
} D3DKMT_QUERYREMOTEVIDPNSOURCEFROMGDIDISPLAYNAME;
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_7)
typedef union _D3DKMT_ENUMADAPTERS_FILTER
{
    // Setting none of these flags will still enumerate adapters,
    // but there are fewer adapters than EnumAdapters2 enumerates.
    // ComputeOnly adapters are left out of the default enumeration, to avoid breaking applications.
    // DisplayOnly adapters are also left out of the default enumeration.
    struct
    {
        ULONGLONG IncludeComputeOnly            : 1;
        ULONGLONG IncludeDisplayOnly            : 1;
        ULONGLONG Reserved                      : 62;
    };
    D3DKMT_ALIGN64 ULONGLONG Value;
} D3DKMT_ENUMADAPTERS_FILTER;

typedef struct _D3DKMT_ENUMADAPTERS3
{
    D3DKMT_ENUMADAPTERS_FILTER      Filter;      // in: Defines the filter
    ULONG                           NumAdapters; // in/out: On input, the count of the pAdapters array buffer.  On output, the number of adapters enumerated.
    D3DKMT_PTR(D3DKMT_ADAPTERINFO*, pAdapters);  // out: Array of enumerated adapters containing NumAdapters elements
} D3DKMT_ENUMADAPTERS3;
#endif

typedef struct _D3DKMT_CLOSEADAPTER
{
    D3DKMT_HANDLE   hAdapter;   // in: adapter handle
} D3DKMT_CLOSEADAPTER;

typedef struct _D3DKMT_GETSHAREDPRIMARYHANDLE
{
    D3DKMT_HANDLE                   hAdapter;       // in: adapter handle
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;  // in: adapter's VidPN source ID
    D3DKMT_HANDLE                   hSharedPrimary; // out: global shared primary handle (if one exists currently)
} D3DKMT_GETSHAREDPRIMARYHANDLE;

typedef struct _D3DKMT_SHAREDPRIMARYLOCKNOTIFICATION
{
    LUID                            AdapterLuid;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
    RECTL                           LockRect;               // in: If zero rect then we are locking the whole primary else the lock sub-rect
} D3DKMT_SHAREDPRIMARYLOCKNOTIFICATION;

typedef struct _D3DKMT_SHAREDPRIMARYUNLOCKNOTIFICATION
{
    LUID                            AdapterLuid;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
} D3DKMT_SHAREDPRIMARYUNLOCKNOTIFICATION;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
typedef struct _D3DKMT_PINDIRECTFLIPRESOURCES
{
    D3DKMT_HANDLE               hDevice;                           // in : Indentifies the device
    UINT                        ResourceCount;                     // in : Number of resources to pin
    D3DKMT_PTR(_Field_size_(ResourceCount)  D3DKMT_HANDLE*, pResourceList);     // in : Array of resource handles to pin
} D3DKMT_PINDIRECTFLIPRESOURCES;

typedef struct _D3DKMT_UNPINDIRECTFLIPRESOURCES
{
    D3DKMT_HANDLE               hDevice;                           // in : Indentifies the device
    UINT                        ResourceCount;                     // in : Number of resources to unpin
    D3DKMT_PTR(_Field_size_(ResourceCount) D3DKMT_HANDLE*, pResourceList);     // in : Array of resource handles to unpin
} D3DKMT_UNPINDIRECTFLIPRESOURCES;
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_7)
typedef union _D3DKMT_PINRESOURCEFLAGS
{
    struct
    {
        UINT DirectFlipResources  :  1; // Used by DWM to indicate the resources are DirectFlip resources
                                        // and should be pinned in-place.
        UINT Reserved             : 31;
    };
    UINT Value;
} D3DKMT_PINRESOURCEFLAGS;

typedef struct _D3DKMT_PINRESOURCES
{
    D3DKMT_HANDLE                hDevice;                           // in : Indentifies the device
    UINT                         ResourceCount;                     // in : Number of resources to pin
    D3DKMT_PTR(_Field_size_(ResourceCount) D3DKMT_HANDLE*, pResourceList);     // in : Array of resource handles to pin
    D3DKMT_PINRESOURCEFLAGS      Flags;                             // in : Flags
    D3DKMT_HANDLE                hPagingQueue;                      // in opt : Handle to a paging queue used to synchronize the operation
    D3DKMT_ALIGN64 UINT64        PagingFence;                       // out : Fence value returned if hPagingQueue is not NULL
} D3DKMT_PINRESOURCES;

typedef struct _D3DKMT_UNPINRESOURCES
{
    D3DKMT_HANDLE                hDevice;                           // in : Indentifies the device
    UINT                         ResourceCount;                     // in : Number of resources to unpin
    D3DKMT_PTR(_Field_size_(ResourceCount) D3DKMT_HANDLE*, pResourceList);     // in : Array of resource handles to unpin
    UINT                         Reserved;
} D3DKMT_UNPINRESOURCES;
#endif

typedef enum _D3DKMT_ESCAPETYPE
{
    D3DKMT_ESCAPE_DRIVERPRIVATE                 =  0,
    D3DKMT_ESCAPE_VIDMM                         =  1,
    D3DKMT_ESCAPE_TDRDBGCTRL                    =  2,
    D3DKMT_ESCAPE_VIDSCH                        =  3,
    D3DKMT_ESCAPE_DEVICE                        =  4,
    D3DKMT_ESCAPE_DMM                           =  5,
    D3DKMT_ESCAPE_DEBUG_SNAPSHOT                =  6,
    // unused (7 was previously used to set driver update in-progress status, D3DKMT_ESCAPE_SETDRIVERUPDATESTATUS)
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
    // unused (19 was previously D3DKMT_ESCAPE_GENERIC_ADAPTER_DIAG_INFO)
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    D3DKMT_ESCAPE_MIRACAST_DISPLAY_REQUEST      = 20,
    D3DKMT_ESCAPE_HISTORY_BUFFER_STATUS         = 21,
    // 22 can be reused for future needs as it was never exposed for external purposes
    D3DKMT_ESCAPE_MIRACAST_ADAPTER_DIAG_INFO    = 23,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    D3DKMT_ESCAPE_FORCE_BDDFALLBACK_HEADLESS    = 24,
    D3DKMT_ESCAPE_REQUEST_MACHINE_CRASH         = 25,
    // unused (26 was previously D3DKMT_ESCAPE_HMD_GET_EDID_BASE_BLOCK)
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
#endif // DXGKDDI_INTERFACE_VERSION_WDDM3_0
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_6
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_4
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_3
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_2
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_1
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_0
#endif // DXGKDDI_INTERFACE_VERSION_WDDM1_3

    D3DKMT_ESCAPE_WIN32K_START                  = 1024,
    D3DKMT_ESCAPE_WIN32K_HIP_DEVICE_INFO        = 1024,
    D3DKMT_ESCAPE_WIN32K_QUERY_CD_ROTATION_BLOCK = 1025,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    D3DKMT_ESCAPE_WIN32K_DPI_INFO               = 1026, // Use hContext for the desired hdev
    D3DKMT_ESCAPE_WIN32K_PRESENTER_VIEW_INFO    = 1027,
    D3DKMT_ESCAPE_WIN32K_SYSTEM_DPI             = 1028,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    D3DKMT_ESCAPE_WIN32K_BDD_FALLBACK           = 1029,
    D3DKMT_ESCAPE_WIN32K_DDA_TEST_CTL           = 1030,
    D3DKMT_ESCAPE_WIN32K_USER_DETECTED_BLACK_SCREEN = 1031,
    // unused (1032 was previously D3DKMT_ESCAPE_WIN32K_HMD_ENUM)
    // unused (1033 was previously D3DKMT_ESCAPE_WIN32K_HMD_CONTROL)
    // unused (1034 was previously D3DKMT_ESCAPE_WIN32K_LPMDISPLAY_CONTROL)
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)
    D3DKMT_ESCAPE_WIN32K_DISPBROKER_TEST        = 1035,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)
    D3DKMT_ESCAPE_WIN32K_COLOR_PROFILE_INFO     = 1036,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_7)
    D3DKMT_ESCAPE_WIN32K_SET_DIMMED_STATE       = 1037,
    D3DKMT_ESCAPE_WIN32K_SPECIALIZED_DISPLAY_TEST = 1038,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_7
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_6
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_5
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_0
#endif // DXGKDDI_INTERFACE_VERSION_WDDM1_3
#endif // DXGKDDI_INTERFACE_VERSION_WIN8
} D3DKMT_ESCAPETYPE;

typedef struct _D3DKMT_DOD_SET_DIRTYRECT_MODE
{
    BOOL bForceFullScreenDirty;      // in: indicates if this adapter should always give full screen dirty for every Dod present
}D3DKMT_DOD_SET_DIRTYRECT_MODE;

typedef enum _D3DKMT_TDRDBGCTRLTYPE
{
    D3DKMT_TDRDBGCTRLTYPE_FORCETDR          = 0, //Simulate a TDR
    D3DKMT_TDRDBGCTRLTYPE_DISABLEBREAK      = 1, //Disable DebugBreak on timeout
    D3DKMT_TDRDBGCTRLTYPE_ENABLEBREAK       = 2, //Enable DebugBreak on timeout
    D3DKMT_TDRDBGCTRLTYPE_UNCONDITIONAL     = 3, //Disables all safety conditions (e.g. check for consecutive recoveries)
    D3DKMT_TDRDBGCTRLTYPE_VSYNCTDR          = 4, //Simulate a Vsync TDR
    D3DKMT_TDRDBGCTRLTYPE_GPUTDR            = 5, //Simulate a GPU TDR
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    D3DKMT_TDRDBGCTRLTYPE_FORCEDODTDR       = 6, //Simulate a Display Only Present TDR
    D3DKMT_TDRDBGCTRLTYPE_FORCEDODVSYNCTDR  = 7, //Simulate a Display Only Vsync TDR
    D3DKMT_TDRDBGCTRLTYPE_ENGINETDR         = 8, //Simulate an engine TDR
#endif
} D3DKMT_TDRDBGCTRLTYPE;

typedef enum _D3DKMT_VIDMMESCAPETYPE
{
    D3DKMT_VIDMMESCAPETYPE_SETFAULT                     = 0,
    D3DKMT_VIDMMESCAPETYPE_RUN_COHERENCY_TEST           = 1,
    D3DKMT_VIDMMESCAPETYPE_RUN_UNMAP_TO_DUMMY_PAGE_TEST = 2,
    D3DKMT_VIDMMESCAPETYPE_APERTURE_CORRUPTION_CHECK    = 3,
    D3DKMT_VIDMMESCAPETYPE_SUSPEND_CPU_ACCESS_TEST      = 4,
    D3DKMT_VIDMMESCAPETYPE_EVICT                        = 5,
    D3DKMT_VIDMMESCAPETYPE_EVICT_BY_NT_HANDLE           = 6,
    D3DKMT_VIDMMESCAPETYPE_GET_VAD_INFO                 = 7,
    D3DKMT_VIDMMESCAPETYPE_SET_BUDGET                   = 8,
    D3DKMT_VIDMMESCAPETYPE_SUSPEND_PROCESS              = 9,
    D3DKMT_VIDMMESCAPETYPE_RESUME_PROCESS               = 10,
    D3DKMT_VIDMMESCAPETYPE_GET_BUDGET                   = 11,
    D3DKMT_VIDMMESCAPETYPE_SET_TRIM_INTERVALS           = 12,
    D3DKMT_VIDMMESCAPETYPE_EVICT_BY_CRITERIA            = 13,
    D3DKMT_VIDMMESCAPETYPE_WAKE                         = 14,
    D3DKMT_VIDMMESCAPETYPE_DEFRAG                       = 15,
    D3DKMT_VIDMMESCAPETYPE_DELAYEXECUTION               = 16,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_7)
    D3DKMT_VIDMMESCAPETYPE_VALIDATE_INTEGRITY           = 17,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
    D3DKMT_VIDMMESCAPETYPE_SET_EVICTION_CONFIG          = 18,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_9
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_7
} D3DKMT_VIDMMESCAPETYPE;

typedef enum _D3DKMT_VIDSCHESCAPETYPE
{
    D3DKMT_VIDSCHESCAPETYPE_PREEMPTIONCONTROL    = 0, //Enable/Disable preemption
    D3DKMT_VIDSCHESCAPETYPE_SUSPENDSCHEDULER     = 1, //Suspend/Resume scheduler (obsolate)
    D3DKMT_VIDSCHESCAPETYPE_TDRCONTROL           = 2, //Tdr control
    D3DKMT_VIDSCHESCAPETYPE_SUSPENDRESUME        = 3, //Suspend/Resume scheduler
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    D3DKMT_VIDSCHESCAPETYPE_ENABLECONTEXTDELAY   = 4, //Enable/Disable context delay
#endif
    D3DKMT_VIDSCHESCAPETYPE_CONFIGURE_TDR_LIMIT  = 5, // Configure TdrLimitCount and TdrLimitTime
    D3DKMT_VIDSCHESCAPETYPE_VGPU_RESET           = 6, // Trigger VGPU reset
    D3DKMT_VIDSCHESCAPETYPE_PFN_CONTROL          = 7, // Periodic frame notification control
    D3DKMT_VIDSCHESCAPETYPE_VIRTUAL_REFRESH_RATE = 8,
} D3DKMT_VIDSCHESCAPETYPE;

typedef enum _D3DKMT_DMMESCAPETYPE
{
    D3DKMT_DMMESCAPETYPE_UNINITIALIZED                       =  0,
    D3DKMT_DMMESCAPETYPE_GET_SUMMARY_INFO                    =  1,
    D3DKMT_DMMESCAPETYPE_GET_VIDEO_PRESENT_SOURCES_INFO      =  2,
    D3DKMT_DMMESCAPETYPE_GET_VIDEO_PRESENT_TARGETS_INFO      =  3,
    D3DKMT_DMMESCAPETYPE_GET_ACTIVEVIDPN_INFO                =  4,
    D3DKMT_DMMESCAPETYPE_GET_MONITORS_INFO                   =  5,
    D3DKMT_DMMESCAPETYPE_RECENTLY_COMMITTED_VIDPNS_INFO      =  6,
    D3DKMT_DMMESCAPETYPE_RECENT_MODECHANGE_REQUESTS_INFO     =  7,
    D3DKMT_DMMESCAPETYPE_RECENTLY_RECOMMENDED_VIDPNS_INFO    =  8,
    D3DKMT_DMMESCAPETYPE_RECENT_MONITOR_PRESENCE_EVENTS_INFO =  9,
    D3DKMT_DMMESCAPETYPE_ACTIVEVIDPN_SOURCEMODESET_INFO      = 10,
    D3DKMT_DMMESCAPETYPE_ACTIVEVIDPN_COFUNCPATHMODALITY_INFO = 11,
    D3DKMT_DMMESCAPETYPE_GET_LASTCLIENTCOMMITTEDVIDPN_INFO   = 12,
    D3DKMT_DMMESCAPETYPE_GET_VERSION_INFO                    = 13,
    D3DKMT_DMMESCAPETYPE_VIDPN_MGR_DIAGNOSTICS               = 14
} D3DKMT_DMMESCAPETYPE;

typedef struct _D3DKMT_HISTORY_BUFFER_STATUS
{
    BOOLEAN Enabled;
    UINT Reserved;
} D3DKMT_HISTORY_BUFFER_STATUS;

typedef enum _D3DKMT_VAD_ESCAPE_COMMAND
{
    D3DKMT_VAD_ESCAPE_GETNUMVADS,
    D3DKMT_VAD_ESCAPE_GETVAD,
    D3DKMT_VAD_ESCAPE_GETVADRANGE,
    D3DKMT_VAD_ESCAPE_GET_PTE,
    D3DKMT_VAD_ESCAPE_GET_GPUMMU_CAPS,
    D3DKMT_VAD_ESCAPE_GET_SEGMENT_CAPS,
} D3DKMT_VAD_ESCAPE_COMMAND;

typedef struct _D3DKMT_VAD_DESC
{
    UINT                  VadIndex;           // in: 0xFFFFFFFF to use the VAD address
    D3DKMT_ALIGN64 UINT64 VadAddress;         // in
    UINT                  NumMappedRanges;    // out
    UINT                  VadType;            // out: 0 - reserved, 1 - Mapped
    D3DKMT_ALIGN64 UINT64 StartAddress;       // out
    D3DKMT_ALIGN64 UINT64 EndAddress;         // out
} D3DKMT_VAD_DESC;

typedef struct _D3DKMT_VA_RANGE_DESC
{
    D3DKMT_ALIGN64 UINT64 VadAddress;             // in
    UINT                  VaRangeIndex;           // in
    UINT                  PhysicalAdapterIndex;   // in
    D3DKMT_ALIGN64 UINT64 StartAddress;           // out
    D3DKMT_ALIGN64 UINT64 EndAddress;             // out
    D3DKMT_ALIGN64 UINT64 DriverProtection;       // out
    UINT                  OwnerType;              // out: VIDMM_VAD_OWNER_TYPE
    D3DKMT_ALIGN64 UINT64 pOwner;                 // out
    D3DKMT_ALIGN64 UINT64 OwnerOffset;            // out
    UINT                  Protection;             // out: D3DDDIGPUVIRTUALADDRESS_PROTECTION_TYPE
} D3DKMT_VA_RANGE_DESC;

typedef struct _D3DKMT_EVICTION_CRITERIA
{
    D3DKMT_ALIGN64 UINT64 MinimumSize;
    D3DKMT_ALIGN64 UINT64 MaximumSize;
    struct
    {
        union
        {
            struct
            {
                UINT Primary  :  1; // 0x00000001
                UINT Reserved : 31; // 0xFFFFFFFE
            } Flags;
            UINT Value;
        };
    };
} D3DKMT_EVICTION_CRITERIA;

typedef enum _D3DKMT_DEFRAG_ESCAPE_OPERATION
{
    D3DKMT_DEFRAG_ESCAPE_GET_FRAGMENTATION_STATS       =  0,
    D3DKMT_DEFRAG_ESCAPE_DEFRAG_UPWARD                 =  1,
    D3DKMT_DEFRAG_ESCAPE_DEFRAG_DOWNWARD               =  2,
    D3DKMT_DEFRAG_ESCAPE_DEFRAG_PASS                   =  3,
    D3DKMT_DEFRAG_ESCAPE_VERIFY_TRANSFER               =  4,
} D3DKMT_DEFRAG_ESCAPE_OPERATION;

typedef struct _D3DKMT_PAGE_TABLE_LEVEL_DESC
{
    UINT                  IndexBitCount;
    D3DKMT_ALIGN64 UINT64 IndexMask;
    D3DKMT_ALIGN64 UINT64 IndexShift;
    D3DKMT_ALIGN64 UINT64 LowerLevelsMask;
    D3DKMT_ALIGN64 UINT64 EntryCoverageInPages;
} D3DKMT_PAGE_TABLE_LEVEL_DESC;

typedef struct _DXGK_ESCAPE_GPUMMUCAPS
{
    BOOLEAN ReadOnlyMemorySupported;
    BOOLEAN NoExecuteMemorySupported;
    BOOLEAN ZeroInPteSupported;
    BOOLEAN CacheCoherentMemorySupported;
    BOOLEAN LargePageSupported;
    BOOLEAN DualPteSupported;
    BOOLEAN AllowNonAlignedLargePageAddress;
    UINT    VirtualAddressBitCount;
    UINT    PageTableLevelCount;
    D3DKMT_PAGE_TABLE_LEVEL_DESC PageTableLevelDesk[DXGK_MAX_PAGE_TABLE_LEVEL_COUNT];
} DXGK_ESCAPE_GPUMMUCAPS;

typedef struct _D3DKMT_GET_GPUMMU_CAPS
{
    UINT                    PhysicalAdapterIndex;   // In
    DXGK_ESCAPE_GPUMMUCAPS  GpuMmuCaps;             // Out
} D3DKMT_GET_GPUMMU_CAPS;

#define D3DKMT_GET_PTE_MAX 64

typedef struct _D3DKMT_GET_PTE
{
    UINT        PhysicalAdapterIndex;                               // In
    UINT        PageTableLevel;                                     // In
    UINT        PageTableIndex[DXGK_MAX_PAGE_TABLE_LEVEL_COUNT];    // In
    BOOLEAN     b64KBPte;                                           // In - Valid only when dual PTEs are supported. Out - PT is 64KB.
    UINT        NumPtes;                                            // In - Number of PTEs to fill. Out - number of filled PTEs
    DXGK_PTE    Pte[D3DKMT_GET_PTE_MAX];                            // Out
    UINT        NumValidEntries;                                    // Out
} D3DKMT_GET_PTE;

#define D3DKMT_MAX_SEGMENT_COUNT 32

typedef enum _D3DKMT_MEMORY_SEGMENT_GROUP
{
    D3DKMT_MEMORY_SEGMENT_GROUP_LOCAL = 0,
    D3DKMT_MEMORY_SEGMENT_GROUP_NON_LOCAL = 1
} D3DKMT_MEMORY_SEGMENT_GROUP;

typedef struct _D3DKMT_SEGMENT_CAPS
{
    D3DKMT_ALIGN64 UINT64 Size;
    UINT    PageSize;
    ULONG   SegmentId;
    BOOLEAN bAperture;
    BOOLEAN bReservedSysMem;
    D3DKMT_MEMORY_SEGMENT_GROUP BudgetGroup;
} D3DKMT_SEGMENT_CAPS;

typedef struct _D3DKMT_GET_SEGMENT_CAPS
{
    UINT        PhysicalAdapterIndex;                               // In
    UINT        NumSegments;                                        // Out
    D3DKMT_SEGMENT_CAPS SegmentCaps[D3DKMT_MAX_SEGMENT_COUNT];      // Out
} D3DKMT_GET_SEGMENT_CAPS;

typedef enum _D3DKMT_ESCAPE_PFN_CONTROL_COMMAND
{
    D3DKMT_ESCAPE_PFN_CONTROL_DEFAULT,
    D3DKMT_ESCAPE_PFN_CONTROL_FORCE_CPU,
    D3DKMT_ESCAPE_PFN_CONTROL_FORCE_GPU
} D3DKMT_ESCAPE_PFN_CONTROL_COMMAND;

// params for D3DKMT_VIDSCHESCAPETYPE_VIRTUAL_REFRESH_RATE
typedef enum _D3DKMT_ESCAPE_VIRTUAL_REFRESH_RATE_TYPE
{
    D3DKMT_ESCAPE_VIRTUAL_REFRESH_RATE_TYPE_SET_BASE_DESKTOP_DURATION = 0,
    D3DKMT_ESCAPE_VIRTUAL_REFRESH_RATE_TYPE_SET_VSYNC_MULTIPLIER = 1,
    D3DKMT_ESCAPE_VIRTUAL_REFRESH_RATE_TYPE_SET_PROCESS_BOOST_ELIGIBLE = 2,
} D3DKMT_ESCAPE_VIRTUAL_REFRESH_RATE_TYPE;

typedef struct _D3DKMT_ESCAPE_VIRTUAL_REFRESH_RATE
{
    D3DKMT_ESCAPE_VIRTUAL_REFRESH_RATE_TYPE Type;
    UINT     VidPnSourceId;
    BOOLEAN  ProcessBoostEligible;
    UINT     VSyncMultiplier;
    UINT     BaseDesktopDuration;
    UCHAR    Reserved[16];
} D3DKMT_ESCAPE_VIRTUAL_REFRESH_RATE;

typedef struct _D3DKMT_VIDMM_ESCAPE
{
    D3DKMT_VIDMMESCAPETYPE Type;
    union
    {
        struct
        {
            union
            {
                struct
                {
                    ULONG ProbeAndLock : 1;
                    ULONG SplitPoint : 1;
                    ULONG NoDemotion : 1;
                    ULONG SwizzlingAperture : 1;
                    ULONG PagingPathLockSubRange : 1;
                    ULONG PagingPathLockMinRange : 1;
                    ULONG ComplexLock : 1;
                    ULONG FailVARotation : 1;
                    ULONG NoWriteCombined : 1;
                    ULONG NoPrePatching : 1;
                    ULONG AlwaysRepatch : 1;
                    ULONG ExpectPreparationFailure : 1;
                    ULONG FailUserModeVAMapping : 1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
                    ULONG NeverDiscardOfferedAllocation : 1;
                    ULONG AlwaysDiscardOfferedAllocation : 1;
                    ULONG Reserved : 17;
#else
                    ULONG Reserved : 19;
#endif
                };
                ULONG Value;
            };
        } SetFault;
        struct
        {
            D3DKMT_HANDLE ResourceHandle;
            D3DKMT_HANDLE AllocationHandle;
            D3DKMT_PTR(HANDLE, hProcess);        // 0 to evict memory for the current process, otherwise it is a process handle from OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId).
        } Evict;
        struct
        {
            D3DKMT_ALIGN64 UINT64 NtHandle;           // Used by D3DKMT_VIDMMESCAPETYPE_EVICT_BY_NT_HANDLE
        } EvictByNtHandle;
        struct
        {
            union
            {
                struct
                {
                    UINT NumVads;
                } GetNumVads;
                D3DKMT_VAD_DESC GetVad;
                D3DKMT_VA_RANGE_DESC GetVadRange;
                D3DKMT_GET_GPUMMU_CAPS GetGpuMmuCaps;
                D3DKMT_GET_PTE  GetPte;
                D3DKMT_GET_SEGMENT_CAPS GetSegmentCaps;
            };
            D3DKMT_VAD_ESCAPE_COMMAND Command;      // in
            NTSTATUS    Status;                     // out
        } GetVads;
        struct
        {
            D3DKMT_ALIGN64 ULONGLONG LocalMemoryBudget;
            D3DKMT_ALIGN64 ULONGLONG SystemMemoryBudget;
        } SetBudget;
        struct
        {
            D3DKMT_PTR(HANDLE, hProcess);
            BOOL bAllowWakeOnSubmission;
        } SuspendProcess;
        struct
        {
            D3DKMT_PTR(HANDLE, hProcess);
        } ResumeProcess;
        struct
        {
            D3DKMT_ALIGN64 UINT64 NumBytesToTrim;
        } GetBudget;
        struct
        {
            ULONG MinTrimInterval; // In 100ns units
            ULONG MaxTrimInterval; // In 100ns units
            ULONG IdleTrimInterval; // In 100ns units
        } SetTrimIntervals;
        D3DKMT_EVICTION_CRITERIA EvictByCriteria;
        struct
        {
            BOOL bFlush;
        } Wake;
        struct
        {
            D3DKMT_DEFRAG_ESCAPE_OPERATION  Operation;

            UINT                            SegmentId;

            D3DKMT_ALIGN64 ULONGLONG                       TotalCommitted;
            D3DKMT_ALIGN64 ULONGLONG                       TotalFree;
            D3DKMT_ALIGN64 ULONGLONG                       LargestGapBefore;
            D3DKMT_ALIGN64 ULONGLONG                       LargestGapAfter;
        } Defrag;
        struct
        {
            D3DKMT_HANDLE hPagingQueue;
            UINT PhysicalAdapterIndex;
            ULONG Milliseconds;
            D3DKMT_ALIGN64 ULONGLONG PagingFenceValue;
        } DelayExecution;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_7)
        struct
        {
            UINT SegmentId;
        } VerifyIntegrity;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
        struct
        {
            D3DKMT_ALIGN64 LONGLONG TimerValue;
        } DelayedEvictionConfig;
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_9
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_7
    };
} D3DKMT_VIDMM_ESCAPE;

typedef struct _D3DKMT_VIDSCH_ESCAPE
{
    D3DKMT_VIDSCHESCAPETYPE Type;
    union
    {
        BOOL PreemptionControl; // enable/disable preemption
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
        BOOL EnableContextDelay; // enable/disable context delay
        struct
        {
            ULONG TdrControl;   // control tdr
            union
            {
                ULONG NodeOrdinal;  // valid if TdrControl is set to D3DKMT_TDRDBGCTRLTYPE_ENGINETDR
            };
        } TdrControl2;
#endif
        BOOL SuspendScheduler;  // suspend/resume scheduler (obsolate)
        ULONG TdrControl;       // control tdr
        ULONG SuspendTime;      // time period to suspend.
        struct
        {
            UINT Count;
            UINT Time; // In seconds
        } TdrLimit;

        D3DKMT_ESCAPE_PFN_CONTROL_COMMAND  PfnControl;   // periodic frame notification control
    };
    D3DKMT_ESCAPE_VIRTUAL_REFRESH_RATE VirtualRefreshRateControl;
} D3DKMT_VIDSCH_ESCAPE;

typedef struct _D3DKMT_TDRDBGCTRL_ESCAPE
{
    D3DKMT_TDRDBGCTRLTYPE TdrControl;   // control tdr
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    union
    {
        ULONG NodeOrdinal;  // valid if TdrControl is set to D3DKMT_TDRDBGCTRLTYPE_ENGINETDR
    };
#endif
} D3DKMT_TDRDBGCTRL_ESCAPE;

// Upper boundary on the DMM escape data size (in bytes).
enum
{
    D3DKMT_MAX_DMM_ESCAPE_DATASIZE = 100*1024
};

// NOTE: If (ProvidedBufferSize >= MinRequiredBufferSize), then MinRequiredBufferSize = size of the actual complete data set in the Data[] array.
typedef struct _D3DKMT_DMM_ESCAPE
{
    _In_  D3DKMT_DMMESCAPETYPE  Type;
    _In_  D3DKMT_ALIGN64 D3DKMT_SIZE_T ProvidedBufferSize;     // actual size of Data[] array, in bytes.
    _Out_ D3DKMT_ALIGN64 D3DKMT_SIZE_T MinRequiredBufferSize;  // minimum required size of Data[] array to contain requested data.
    _Out_writes_bytes_(ProvidedBufferSize) UCHAR  Data[1];
} D3DKMT_DMM_ESCAPE;

typedef enum _D3DKMT_BRIGHTNESS_INFO_TYPE
{
    D3DKMT_BRIGHTNESS_INFO_GET_POSSIBLE_LEVELS  = 1,
    D3DKMT_BRIGHTNESS_INFO_GET                  = 2,
    D3DKMT_BRIGHTNESS_INFO_SET                  = 3,
    D3DKMT_BRIGHTNESS_INFO_GET_CAPS             = 4,
    D3DKMT_BRIGHTNESS_INFO_SET_STATE            = 5,
    D3DKMT_BRIGHTNESS_INFO_SET_OPTIMIZATION     = 6,
    D3DKMT_BRIGHTNESS_INFO_GET_REDUCTION        = 7,
    D3DKMT_BRIGHTNESS_INFO_BEGIN_MANUAL_MODE    = 8,
    D3DKMT_BRIGHTNESS_INFO_END_MANUAL_MODE      = 9,
    D3DKMT_BRIGHTNESS_INFO_TOGGLE_LOGGING       = 10,
    D3DKMT_BRIGHTNESS_INFO_GET_NIT_RANGES       = 11,
} D3DKMT_BRIGHTNESS_INFO_TYPE;

typedef struct _D3DKMT_BRIGHTNESS_POSSIBLE_LEVELS
{
    UCHAR LevelCount;
    UCHAR BrightnessLevels[256];
} D3DKMT_BRIGHTNESS_POSSIBLE_LEVELS;

typedef struct _D3DKMT_BRIGHTNESS_INFO
{
    D3DKMT_BRIGHTNESS_INFO_TYPE    Type;
    ULONG                          ChildUid;
    union
    {
        D3DKMT_BRIGHTNESS_POSSIBLE_LEVELS   PossibleLevels;
        UCHAR                               Brightness;
        DXGK_BRIGHTNESS_CAPS                BrightnessCaps;
        DXGK_BRIGHTNESS_STATE               BrightnessState;
        DXGK_BACKLIGHT_OPTIMIZATION_LEVEL   OptimizationLevel;
        DXGK_BACKLIGHT_INFO                 ReductionInfo;
        BOOLEAN                             VerboseLogging;
        DXGK_BRIGHTNESS_GET_NIT_RANGES_OUT  NitRanges;
        DXGK_BRIGHTNESS_GET_OUT             GetBrightnessMillinits;
        DXGK_BRIGHTNESS_SET_IN              SetBrightnessMillinits;
    };
} D3DKMT_BRIGHTNESS_INFO;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
typedef struct _D3DKMT_BDDFALLBACK_CTL
{
    BOOLEAN ForceBddHeadlessNextFallback;
} D3DKMT_BDDFALLBACK_CTL;

typedef struct _D3DKMT_REQUEST_MACHINE_CRASH_ESCAPE
{
    D3DKMT_ALIGN64 D3DKMT_ULONG_PTR Param1;
    D3DKMT_ALIGN64 D3DKMT_ULONG_PTR Param2;
    D3DKMT_ALIGN64 D3DKMT_ULONG_PTR Param3;
} D3DKMT_REQUEST_MACHINE_CRASH_ESCAPE;

//
// VERIFIER OPTIONS
//
typedef enum _D3DKMT_VERIFIER_OPTION_MODE
{
    D3DKMT_VERIFIER_OPTION_QUERY,
    D3DKMT_VERIFIER_OPTION_SET
} D3DKMT_VERIFIER_OPTION_MODE;

typedef enum _D3DKMT_PROCESS_VERIFIER_OPTION_TYPE
{
    //
    // Dxgkrnl (0xxx)
    //

    //
    // VidMm (1xxx)
    //
    D3DKMT_PROCESS_VERIFIER_OPTION_VIDMM_FLAGS              = 1000,
    D3DKMT_PROCESS_VERIFIER_OPTION_VIDMM_RESTRICT_BUDGET    = 1001,

    //
    // VidSch (2xxx)
    //

} D3DKMT_PROCESS_VERIFIER_OPTION_TYPE;

typedef union _D3DKMT_PROCESS_VERIFIER_VIDMM_FLAGS
{
    struct
    {
        UINT ForceSynchronousEvict        : 1;
        UINT NeverDeferEvictions          : 1;
        UINT AlwaysFailCommitOnReclaim    : 1;
        UINT AlwaysPlaceInDemotedLocation : 1;
        UINT Reserved : 28;
    };
    UINT32 Value;
} D3DKMT_PROCESS_VERIFIER_VIDMM_FLAGS;

typedef struct _D3DKMT_PROCESS_VERIFIER_VIDMM_RESTRICT_BUDGET
{
    D3DKMT_ALIGN64 UINT64 LocalBudget;
    D3DKMT_ALIGN64 UINT64 NonLocalBudget;
} D3DKMT_PROCESS_VERIFIER_VIDMM_RESTRICT_BUDGET;

typedef union _D3DKMT_PROCESS_VERIFIER_OPTION_DATA
{
    D3DKMT_PROCESS_VERIFIER_VIDMM_FLAGS VidMmFlags;
    D3DKMT_PROCESS_VERIFIER_VIDMM_RESTRICT_BUDGET VidMmRestrictBudget;
} D3DKMT_PROCESS_VERIFIER_OPTION_DATA;

typedef struct _D3DKMT_PROCESS_VERIFIER_OPTION
{
    D3DKMT_PTR(HANDLE, hProcess);
    D3DKMT_PROCESS_VERIFIER_OPTION_TYPE Type;
    D3DKMT_VERIFIER_OPTION_MODE Mode;
    D3DKMT_PROCESS_VERIFIER_OPTION_DATA Data;
} D3DKMT_PROCESS_VERIFIER_OPTION;

typedef enum _D3DKMT_ADAPTER_VERIFIER_OPTION_TYPE
{
    //
    // Dxgkrnl (0xxx)
    //

    //
    // VidMm (1xxx)
    //
    D3DKMT_ADAPTER_VERIFIER_OPTION_VIDMM_FLAGS          = 1000,
    D3DKMT_ADAPTER_VERIFIER_OPTION_VIDMM_TRIM_INTERVAL  = 1001,

    //
    // VidSch (2xxx)
    //
} D3DKMT_ADAPTER_VERIFIER_OPTION_TYPE;

typedef union _D3DKMT_ADAPTER_VERIFIER_VIDMM_FLAGS
{
    struct
    {
        UINT AlwaysRepatch                      : 1;
        UINT FailSharedPrimary                  : 1;
        UINT FailProbeAndLock                   : 1;
        UINT AlwaysDiscardOffer                 : 1;
        UINT NeverDiscardOffer                  : 1;
        UINT ForceComplexLock                   : 1;
        UINT NeverPrepatch                      : 1;
        UINT ExpectPreparationFailure           : 1;
        UINT TakeSplitPoint                     : 1;
        UINT FailAcquireSwizzlingRange          : 1;
        UINT PagingPathLockSubrange             : 1;
        UINT PagingPathLockMinrange             : 1;
        UINT FailVaRotation                     : 1;
        UINT NoDemotion                         : 1;
        UINT FailDefragPass                     : 1;
        UINT AlwaysProcessOfferList             : 1;
        UINT AlwaysDecommitOffer                : 1;
        UINT NeverMoveDefrag                    : 1;
        UINT AlwaysRelocateDisplayableResources : 1;
        UINT AlwaysFailGrowVPRMoves             : 1;
        UINT Reserved                           : 12;
    };
    UINT32 Value;
} D3DKMT_ADAPTER_VERIFIER_VIDMM_FLAGS;

typedef struct _D3DKMT_ADAPTER_VERIFIER_VIDMM_TRIM_INTERVAL
{
    D3DKMT_ALIGN64 UINT64 MinimumTrimInterval;
    D3DKMT_ALIGN64 UINT64 MaximumTrimInterval;
    D3DKMT_ALIGN64 UINT64 IdleTrimInterval;
} D3DKMT_ADAPTER_VERIFIER_VIDMM_TRIM_INTERVAL;

typedef union _D3DKMT_ADAPTER_VERIFIER_OPTION_DATA
{
    D3DKMT_ADAPTER_VERIFIER_VIDMM_FLAGS VidMmFlags;
    D3DKMT_ADAPTER_VERIFIER_VIDMM_TRIM_INTERVAL VidMmTrimInterval;
} D3DKMT_ADAPTER_VERIFIER_OPTION_DATA;

typedef struct _D3DKMT_ADAPTER_VERIFIER_OPTION
{
    D3DKMT_ADAPTER_VERIFIER_OPTION_TYPE Type;
    D3DKMT_VERIFIER_OPTION_MODE Mode;
    D3DKMT_ADAPTER_VERIFIER_OPTION_DATA Data;
} D3DKMT_ADAPTER_VERIFIER_OPTION;

#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_0

typedef enum _D3DKMT_DEVICEESCAPE_TYPE
{
    D3DKMT_DEVICEESCAPE_VIDPNFROMALLOCATION = 0,
    D3DKMT_DEVICEESCAPE_RESTOREGAMMA        = 1,
} D3DKMT_DEVICEESCAPE_TYPE;

typedef struct _D3DKMT_DEVICE_ESCAPE
{
    D3DKMT_DEVICEESCAPE_TYPE Type;
    union
    {
        struct
        {
            D3DKMT_HANDLE                   hPrimaryAllocation; // in: Primary allocation handle
            D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;      // out: VidPnSoureId of primary allocation
        } VidPnFromAllocation;
    };
} D3DKMT_DEVICE_ESCAPE;

typedef struct _D3DKMT_DEBUG_SNAPSHOT_ESCAPE
{
    ULONG Length;   // out: Actual length of the snapshot written in Buffer
    BYTE Buffer[1]; // out: Buffer to place snapshot
} D3DKMT_DEBUG_SNAPSHOT_ESCAPE;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
#ifndef DXGK_DIAG_PROCESS_NAME_LENGTH
#define DXGK_DIAG_PROCESS_NAME_LENGTH 16
#endif

typedef enum _OUTPUTDUPL_CONTEXT_DEBUG_STATUS
{
    OUTPUTDUPL_CONTEXT_DEBUG_STATUS_INACTIVE = 0,
    OUTPUTDUPL_CONTEXT_DEBUG_STATUS_ACTIVE = 1,
    OUTPUTDUPL_CONTEXT_DEBUG_STATUS_PENDING_DESTROY = 2,
    OUTPUTDUPL_CONTEXT_DEBUG_STATUS_FORCE_UINT32 = 0xffffffff
}OUTPUTDUPL_CONTEXT_DEBUG_STATUS;

typedef struct _OUTPUTDUPL_CONTEXT_DEBUG_INFO
{
    OUTPUTDUPL_CONTEXT_DEBUG_STATUS Status;
    D3DKMT_PTR(HANDLE,              ProcessID);
    UINT32                          AccumulatedPresents;
    D3DKMT_ALIGN64 LARGE_INTEGER    LastPresentTime;
    D3DKMT_ALIGN64 LARGE_INTEGER    LastMouseTime;
    CHAR                            ProcessName[DXGK_DIAG_PROCESS_NAME_LENGTH];
} OUTPUTDUPL_CONTEXT_DEBUG_INFO;

#define GET_OUTPUT_DUPL_DEBUG_INFO_FROM_SNAPSHOT(pSnapshot, VidPnSource, OutputDuplContextIndex) \
    (pSnapshot->OutputDuplDebugInfos[(VidPnSource * pSnapshot->NumOutputDuplContexts) + OutputDuplContextIndex])

typedef struct _D3DKMT_OUTPUTDUPL_SNAPSHOT
{
    UINT Size;                          // _In_/out: Size of entire structure

    UINT SessionProcessCount;           // _Out_: Number of processes currently duplicating output in this session (max possible will be equal to NumOutputDuplContexts)
    UINT SessionActiveConnectionsCount; // _Out_: Total number of active contexts in this session, may be more than number active in 2D array because that is per adapter

    UINT NumVidPnSources;               // _Out_: Max of first array index
    UINT NumOutputDuplContexts;         // _Out_: Max of second array index

    UINT Padding;

    // This field is in reality a two dimensional array, use GET_OUTPUT_DUPL_DEBUG_INFO_FROM_SNAPSHOT macro to get a specific one
    _Field_size_bytes_(Size - sizeof(_D3DKMT_OUTPUTDUPL_SNAPSHOT)) OUTPUTDUPL_CONTEXT_DEBUG_INFO OutputDuplDebugInfos[0];
} D3DKMT_OUTPUTDUPL_SNAPSHOT;
#endif

typedef enum _D3DKMT_ACTIVATE_SPECIFIC_DIAG_TYPE
{
    D3DKMT_ACTIVATE_SPECIFIC_DIAG_TYPE_EXTRA_CCD_DATABASE_INFO = 0,
    D3DKMT_ACTIVATE_SPECIFIC_DIAG_TYPE_MODES_PRUNED            = 15,
}D3DKMT_ACTIVATE_SPECIFIC_DIAG_TYPE;

typedef struct _D3DKMT_ACTIVATE_SPECIFIC_DIAG_ESCAPE
{
    D3DKMT_ACTIVATE_SPECIFIC_DIAG_TYPE  Type;     // The escape type that needs to be (de)activated
    BOOL                                Activate; // FALSE means deactivate
} D3DKMT_ACTIVATE_SPECIFIC_DIAG_ESCAPE;

typedef struct _D3DKMT_ESCAPE
{
    D3DKMT_HANDLE       hAdapter;               // in: adapter handle
    D3DKMT_HANDLE       hDevice;                // in: device handle [Optional]
    D3DKMT_ESCAPETYPE   Type;                   // in: escape type.
    D3DDDI_ESCAPEFLAGS  Flags;                  // in: flags
    D3DKMT_PTR(VOID*,   pPrivateDriverData);    // in/out: escape data
    UINT                PrivateDriverDataSize;  // in: size of escape data
    D3DKMT_HANDLE       hContext;               // in: context handle [Optional]
} D3DKMT_ESCAPE;

//
// begin D3DKMT_QUERYSTATISTICS
//

typedef enum _D3DKMT_QUERYRESULT_PREEMPTION_ATTEMPT_RESULT
{
    D3DKMT_PreemptionAttempt                               = 0,
    D3DKMT_PreemptionAttemptSuccess                        = 1,
    D3DKMT_PreemptionAttemptMissNoCommand                  = 2,
    D3DKMT_PreemptionAttemptMissNotEnabled                 = 3,
    D3DKMT_PreemptionAttemptMissNextFence                  = 4,
    D3DKMT_PreemptionAttemptMissPagingCommand              = 5,
    D3DKMT_PreemptionAttemptMissSplittedCommand            = 6,
    D3DKMT_PreemptionAttemptMissFenceCommand               = 7,
    D3DKMT_PreemptionAttemptMissRenderPendingFlip          = 8,
    D3DKMT_PreemptionAttemptMissNotMakingProgress          = 9,
    D3DKMT_PreemptionAttemptMissLessPriority               = 10,
    D3DKMT_PreemptionAttemptMissRemainingQuantum           = 11,
    D3DKMT_PreemptionAttemptMissRemainingPreemptionQuantum = 12,
    D3DKMT_PreemptionAttemptMissAlreadyPreempting          = 13,
    D3DKMT_PreemptionAttemptMissGlobalBlock                = 14,
    D3DKMT_PreemptionAttemptMissAlreadyRunning             = 15,
    D3DKMT_PreemptionAttemptStatisticsMax                  = 16,
} D3DKMT_QUERYRESULT_PREEMPTION_ATTEMPT_RESULT;

//
// WOW will not allow enum member as array length, so define it as a constant
//
#define D3DKMT_QUERYRESULT_PREEMPTION_ATTEMPT_RESULT_MAX 16
C_ASSERT(D3DKMT_QUERYRESULT_PREEMPTION_ATTEMPT_RESULT_MAX == D3DKMT_PreemptionAttemptStatisticsMax);

//
// Command packet type
//
typedef enum _D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE {
    D3DKMT_ClientRenderBuffer       = 0, // (Dma packet) should be 0 base.
    D3DKMT_ClientPagingBuffer       = 1, // (Dma packet)
    D3DKMT_SystemPagingBuffer       = 2, // (Dma packet)
    D3DKMT_SystemPreemptionBuffer   = 3, // (Dma packet)
    D3DKMT_DmaPacketTypeMax         = 4
} D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE;

//
// WOW will not allow enum member as array length, so define it as a constant
//
#define D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_MAX 4
C_ASSERT(D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_MAX == D3DKMT_DmaPacketTypeMax);

typedef enum _D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE {
    D3DKMT_RenderCommandBuffer      = 0, // (Queue Packet) should be 0 base.
    D3DKMT_DeferredCommandBuffer    = 1, // (Queue Packet)
    D3DKMT_SystemCommandBuffer      = 2, // (Queue Packet)
    D3DKMT_MmIoFlipCommandBuffer    = 3, // (Queue Packet)
    D3DKMT_WaitCommandBuffer        = 4, // (Queue Packet)
    D3DKMT_SignalCommandBuffer      = 5, // (Queue Packet)
    D3DKMT_DeviceCommandBuffer      = 6, // (Queue Packet)
    D3DKMT_SoftwareCommandBuffer    = 7, // (Queue Packet)
    D3DKMT_QueuePacketTypeMax       = 8
} D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE;

//
// WOW will not allow enum member as array length, so define it as a constant
//
#define D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_MAX 8
C_ASSERT(D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_MAX == D3DKMT_QueuePacketTypeMax);

typedef enum _D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS
{
    D3DKMT_AllocationPriorityClassMinimum = 0,
    D3DKMT_AllocationPriorityClassLow = 1,
    D3DKMT_AllocationPriorityClassNormal = 2,
    D3DKMT_AllocationPriorityClassHigh = 3,
    D3DKMT_AllocationPriorityClassMaximum = 4,
    D3DKMT_MaxAllocationPriorityClass = 5
} D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS;

//
// WOW will not allow enum member as array length, so define it as a constant
//

#define D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS_MAX 5
C_ASSERT(D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS_MAX == D3DKMT_MaxAllocationPriorityClass);

//
// Allocation segment preference set can contain 5 preferences
//
#define D3DKMT_QUERYSTATISTICS_SEGMENT_PREFERENCE_MAX 5

typedef struct _D3DKMT_QUERYSTATISTICS_COUNTER
{
    ULONG Count;
    ULONGLONG Bytes;
} D3DKMT_QUERYSTATISTICS_COUNTER;

typedef struct _D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_INFORMATION {
    ULONG PacketSubmited;
    ULONG PacketCompleted;
    ULONG PacketPreempted;
    ULONG PacketFaulted;
} D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_INFORMATION {
    ULONG  PacketSubmited;
    ULONG  PacketCompleted;
} D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_PACKET_INFORMATION {
  D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_INFORMATION QueuePacket[D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_MAX];  //Size = D3DKMT_QueuePacketTypeMax
  D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_INFORMATION   DmaPacket[D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_MAX];    //Size = D3DKMT_DmaPacketTypeMax
} D3DKMT_QUERYSTATISTICS_PACKET_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_PREEMPTION_INFORMATION {
    ULONG PreemptionCounter[D3DKMT_QUERYRESULT_PREEMPTION_ATTEMPT_RESULT_MAX];
} D3DKMT_QUERYSTATISTICS_PREEMPTION_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION {
    D3DKMT_ALIGN64 LARGE_INTEGER                  RunningTime;          // Running time in micro-second.
    ULONG                                         ContextSwitch;
    D3DKMT_QUERYSTATISTICS_PREEMPTION_INFORMATION PreemptionStatistics;
    D3DKMT_QUERYSTATISTICS_PACKET_INFORMATION     PacketStatistics;
    D3DKMT_ALIGN64 UINT64                         Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_NODE_INFORMATION {
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION GlobalInformation; //Global statistics
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION SystemInformation; //Statistics for system thread
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
    D3DKMT_NODE_PERFDATA                            NodePerfData;
    UINT32                                          Reserved[3];
#else
    D3DKMT_ALIGN64 UINT64                           Reserved[8];
#endif
} D3DKMT_QUERYSTATISTICS_NODE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION {
    ULONG  Frame;          // both by Blt and Flip.
    ULONG  CancelledFrame; // by restart (flip only).
    ULONG  QueuedPresent;  // queued present.
    UINT   Padding;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_7)
    D3DKMT_ALIGN64 UINT64 IsVSyncEnabled;
    D3DKMT_ALIGN64 UINT64 VSyncOnTotalTimeMs;
    D3DKMT_ALIGN64 UINT64 VSyncOffKeepPhaseTotalTimeMs;
    D3DKMT_ALIGN64 UINT64 VSyncOffNoPhaseTotalTimeMs;
    D3DKMT_ALIGN64 UINT64 Reserved[4];
#else
    D3DKMT_ALIGN64 UINT64 Reserved[8];
#endif

} D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_VIDPNSOURCE_INFORMATION {
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION GlobalInformation;   //Global statistics
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION SystemInformation;   //Statistics for system thread
    D3DKMT_ALIGN64 UINT64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_VIDPNSOURCE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATSTICS_REFERENCE_DMA_BUFFER
{
    ULONG NbCall;
    ULONG NbAllocationsReferenced;
    ULONG MaxNbAllocationsReferenced;
    ULONG NbNULLReference;
    ULONG NbWriteReference;
    ULONG NbRenamedAllocationsReferenced;
    ULONG NbIterationSearchingRenamedAllocation;
    ULONG NbLockedAllocationReferenced;
    ULONG NbAllocationWithValidPrepatchingInfoReferenced;
    ULONG NbAllocationWithInvalidPrepatchingInfoReferenced;
    ULONG NbDMABufferSuccessfullyPrePatched;
    ULONG NbPrimariesReferencesOverflow;
    ULONG NbAllocationWithNonPreferredResources;
    ULONG NbAllocationInsertedInMigrationTable;
} D3DKMT_QUERYSTATSTICS_REFERENCE_DMA_BUFFER;

typedef struct _D3DKMT_QUERYSTATSTICS_RENAMING
{
    ULONG NbAllocationsRenamed;
    ULONG NbAllocationsShrinked;
    ULONG NbRenamedBuffer;
    ULONG MaxRenamingListLength;
    ULONG NbFailuresDueToRenamingLimit;
    ULONG NbFailuresDueToCreateAllocation;
    ULONG NbFailuresDueToOpenAllocation;
    ULONG NbFailuresDueToLowResource;
    ULONG NbFailuresDueToNonRetiredLimit;
} D3DKMT_QUERYSTATSTICS_RENAMING;

typedef struct _D3DKMT_QUERYSTATSTICS_PREPRATION
{
    ULONG BroadcastStall;
    ULONG NbDMAPrepared;
    ULONG NbDMAPreparedLongPath;
    ULONG ImmediateHighestPreparationPass;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocationsTrimmed;
} D3DKMT_QUERYSTATSTICS_PREPRATION;

typedef struct _D3DKMT_QUERYSTATSTICS_PAGING_FAULT
{
    D3DKMT_QUERYSTATISTICS_COUNTER Faults;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsFirstTimeAccess;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsReclaimed;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsMigration;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsIncorrectResource;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsLostContent;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsEvicted;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocationsMEM_RESET;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocationsUnresetSuccess;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocationsUnresetFail;
    ULONG AllocationsUnresetSuccessRead;
    ULONG AllocationsUnresetFailRead;

    D3DKMT_QUERYSTATISTICS_COUNTER Evictions;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToPreparation;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToLock;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToClose;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToPurge;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToSuspendCPUAccess;
} D3DKMT_QUERYSTATSTICS_PAGING_FAULT;

typedef struct _D3DKMT_QUERYSTATSTICS_PAGING_TRANSFER
{
    D3DKMT_ALIGN64 ULONGLONG BytesFilled;
    D3DKMT_ALIGN64 ULONGLONG BytesDiscarded;
    D3DKMT_ALIGN64 ULONGLONG BytesMappedIntoAperture;
    D3DKMT_ALIGN64 ULONGLONG BytesUnmappedFromAperture;
    D3DKMT_ALIGN64 ULONGLONG BytesTransferredFromMdlToMemory;
    D3DKMT_ALIGN64 ULONGLONG BytesTransferredFromMemoryToMdl;
    D3DKMT_ALIGN64 ULONGLONG BytesTransferredFromApertureToMemory;
    D3DKMT_ALIGN64 ULONGLONG BytesTransferredFromMemoryToAperture;
} D3DKMT_QUERYSTATSTICS_PAGING_TRANSFER;

typedef struct _D3DKMT_QUERYSTATSTICS_SWIZZLING_RANGE
{
    ULONG NbRangesAcquired;
    ULONG NbRangesReleased;
} D3DKMT_QUERYSTATSTICS_SWIZZLING_RANGE;

typedef struct _D3DKMT_QUERYSTATSTICS_LOCKS
{
    ULONG NbLocks;
    ULONG NbLocksWaitFlag;
    ULONG NbLocksDiscardFlag;
    ULONG NbLocksNoOverwrite;
    ULONG NbLocksNoReadSync;
    ULONG NbLocksLinearization;
    ULONG NbComplexLocks;
} D3DKMT_QUERYSTATSTICS_LOCKS;

typedef struct _D3DKMT_QUERYSTATSTICS_ALLOCATIONS
{
    D3DKMT_QUERYSTATISTICS_COUNTER Created;
    D3DKMT_QUERYSTATISTICS_COUNTER Destroyed;
    D3DKMT_QUERYSTATISTICS_COUNTER Opened;
    D3DKMT_QUERYSTATISTICS_COUNTER Closed;
    D3DKMT_QUERYSTATISTICS_COUNTER MigratedSuccess;
    D3DKMT_QUERYSTATISTICS_COUNTER MigratedFail;
    D3DKMT_QUERYSTATISTICS_COUNTER MigratedAbandoned;
} D3DKMT_QUERYSTATSTICS_ALLOCATIONS;

typedef struct _D3DKMT_QUERYSTATSTICS_TERMINATIONS
{
    //
    // We separate shared / nonshared because for nonshared we know that every alloc
    // terminated will lead cause a global alloc destroyed, but not for nonshared.
    //
    D3DKMT_QUERYSTATISTICS_COUNTER TerminatedShared;
    D3DKMT_QUERYSTATISTICS_COUNTER TerminatedNonShared;
    D3DKMT_QUERYSTATISTICS_COUNTER DestroyedShared;
    D3DKMT_QUERYSTATISTICS_COUNTER DestroyedNonShared;
} D3DKMT_QUERYSTATSTICS_TERMINATIONS;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
typedef struct _D3DKMT_QUERYSTATISTICS_ADAPTER_INFORMATION_FLAGS
{
    union
    {
        struct
        {
            UINT64 NumberOfMemoryGroups : 2;
            UINT64 SupportsDemotion     : 1;
            UINT64 Reserved             :61;
        };
        D3DKMT_ALIGN64 UINT64 Value;
    };
} D3DKMT_QUERYSTATISTICS_ADAPTER_INFORMATION_FLAGS;
#endif

typedef struct _D3DKMT_QUERYSTATISTICS_ADAPTER_INFORMATION
{
    ULONG NbSegments;
    ULONG NodeCount;
    ULONG VidPnSourceCount;

    ULONG VSyncEnabled;
    ULONG TdrDetectedCount;

    D3DKMT_ALIGN64 LONGLONG ZeroLengthDmaBuffers;
    D3DKMT_ALIGN64 ULONGLONG RestartedPeriod;

    D3DKMT_QUERYSTATSTICS_REFERENCE_DMA_BUFFER ReferenceDmaBuffer;
    D3DKMT_QUERYSTATSTICS_RENAMING Renaming;
    D3DKMT_QUERYSTATSTICS_PREPRATION Preparation;
    D3DKMT_QUERYSTATSTICS_PAGING_FAULT PagingFault;
    D3DKMT_QUERYSTATSTICS_PAGING_TRANSFER PagingTransfer;
    D3DKMT_QUERYSTATSTICS_SWIZZLING_RANGE SwizzlingRange;
    D3DKMT_QUERYSTATSTICS_LOCKS Locks;
    D3DKMT_QUERYSTATSTICS_ALLOCATIONS Allocations;
    D3DKMT_QUERYSTATSTICS_TERMINATIONS Terminations;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
    D3DKMT_QUERYSTATISTICS_ADAPTER_INFORMATION_FLAGS Flags;
    D3DKMT_ALIGN64 UINT64 Reserved[7];
#else
    D3DKMT_ALIGN64 UINT64 Reserved[8];
#endif
} D3DKMT_QUERYSTATISTICS_ADAPTER_INFORMATION;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
typedef struct _D3DKMT_QUERYSTATISTICS_PHYSICAL_ADAPTER_INFORMATION
{
    D3DKMT_ADAPTER_PERFDATA      AdapterPerfData;
    D3DKMT_ADAPTER_PERFDATACAPS  AdapterPerfDataCaps;
    D3DKMT_GPUVERSION            GpuVersion;
} D3DKMT_QUERYSTATISTICS_PHYSICAL_ADAPTER_INFORMATION;
#endif

typedef struct _D3DKMT_QUERYSTATISTICS_SYSTEM_MEMORY
{
    D3DKMT_ALIGN64 ULONGLONG BytesAllocated;
    D3DKMT_ALIGN64 ULONGLONG BytesReserved;
    ULONG SmallAllocationBlocks;
    ULONG LargeAllocationBlocks;
    D3DKMT_ALIGN64 ULONGLONG WriteCombinedBytesAllocated;
    D3DKMT_ALIGN64 ULONGLONG WriteCombinedBytesReserved;
    D3DKMT_ALIGN64 ULONGLONG CachedBytesAllocated;
    D3DKMT_ALIGN64 ULONGLONG CachedBytesReserved;
    D3DKMT_ALIGN64 ULONGLONG SectionBytesAllocated;
    D3DKMT_ALIGN64 ULONGLONG SectionBytesReserved;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
    D3DKMT_ALIGN64 ULONGLONG BytesZeroed;
#else
    D3DKMT_ALIGN64 ULONGLONG Reserved;
#endif
} D3DKMT_QUERYSTATISTICS_SYSTEM_MEMORY;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_INFORMATION
{
    ULONG NodeCount;
    ULONG VidPnSourceCount;

    D3DKMT_QUERYSTATISTICS_SYSTEM_MEMORY SystemMemory;

    D3DKMT_ALIGN64 UINT64 Reserved[7];
} D3DKMT_QUERYSTATISTICS_PROCESS_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_DMA_BUFFER
{
    D3DKMT_QUERYSTATISTICS_COUNTER Size;
    ULONG AllocationListBytes;
    ULONG PatchLocationListBytes;
} D3DKMT_QUERYSTATISTICS_DMA_BUFFER;

typedef struct _D3DKMT_QUERYSTATISTICS_COMMITMENT_DATA
{
    D3DKMT_ALIGN64 UINT64          TotalBytesEvictedFromProcess;
    D3DKMT_ALIGN64 UINT64          BytesBySegmentPreference[D3DKMT_QUERYSTATISTICS_SEGMENT_PREFERENCE_MAX];
} D3DKMT_QUERYSTATISTICS_COMMITMENT_DATA;

typedef struct _D3DKMT_QUERYSTATISTICS_POLICY
{
    D3DKMT_ALIGN64 ULONGLONG PreferApertureForRead[D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS_MAX];
    D3DKMT_ALIGN64 ULONGLONG PreferAperture[D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS_MAX];
    D3DKMT_ALIGN64 ULONGLONG MemResetOnPaging;
    D3DKMT_ALIGN64 ULONGLONG RemovePagesFromWorkingSetOnPaging;
    D3DKMT_ALIGN64 ULONGLONG MigrationEnabled;
} D3DKMT_QUERYSTATISTICS_POLICY;

// Process interference counters indicate how much this process GPU workload interferes with packets
// attempting to preempt it. 9 buckets will be exposed based on how long preemption took:
// [0] 100 microseconds <= preemption time < 250 microseconds
// [1] 250 microseconds <= preemption time < 500 microseconds
// [2] 500 microseconds <= preemption time < 1 milliseconds
// [3] 1 milliseconds   <= preemption time < 2.5 milliseconds
// [4] 2.5 milliseconds <= preemption time < 5 milliseconds
// [5] 5 milliseconds   <= preemption time < 10 milliseconds
// [6] 10 milliseconds  <= preemption time < 25 milliseconds
// [7] 25 milliseconds  <= preemption time < 50 milliseconds
// [8] 50 milliseconds  <= preemption time
//
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
#define D3DKMT_QUERYSTATISTICS_PROCESS_INTERFERENCE_BUCKET_COUNT 9

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_INTERFERENCE_COUNTERS
{
    D3DKMT_ALIGN64 UINT64 InterferenceCount[D3DKMT_QUERYSTATISTICS_PROCESS_INTERFERENCE_BUCKET_COUNT];
} D3DKMT_QUERYSTATISTICS_PROCESS_INTERFERENCE_COUNTERS;
#endif

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER_INFORMATION
{
    ULONG NbSegments;
    ULONG NodeCount;
    ULONG VidPnSourceCount;

    //
    // Virtual address space used by vidmm for this process
    //
    ULONG VirtualMemoryUsage;

    D3DKMT_QUERYSTATISTICS_DMA_BUFFER DmaBuffer;
    D3DKMT_QUERYSTATISTICS_COMMITMENT_DATA CommitmentData;
    D3DKMT_QUERYSTATISTICS_POLICY _Policy;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
    D3DKMT_QUERYSTATISTICS_PROCESS_INTERFERENCE_COUNTERS ProcessInterferenceCounters;
#else
    D3DKMT_ALIGN64 UINT64 Reserved[9];
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
    D3DKMT_CLIENTHINT       ClientHint;
#else
    UINT Reserve;
#endif
} D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_MEMORY
{
    D3DKMT_ALIGN64 ULONGLONG TotalBytesEvicted;
    ULONG AllocsCommitted;
    ULONG AllocsResident;
} D3DKMT_QUERYSTATISTICS_MEMORY;

typedef struct _D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION
{
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    D3DKMT_ALIGN64 ULONGLONG CommitLimit;
    D3DKMT_ALIGN64 ULONGLONG BytesCommitted;
    D3DKMT_ALIGN64 ULONGLONG BytesResident;
#else
    ULONG CommitLimit;
    ULONG BytesCommitted;
    ULONG BytesResident;
#endif

    D3DKMT_QUERYSTATISTICS_MEMORY Memory;

    //
    // Boolean, whether this is an aperture segment
    //
    ULONG Aperture;

    //
    // Breakdown of bytes evicted by priority class
    //
    D3DKMT_ALIGN64 ULONGLONG TotalBytesEvictedByPriority[D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS_MAX];   //Size = D3DKMT_MaxAllocationPriorityClass

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    D3DKMT_ALIGN64 UINT64 SystemMemoryEndAddress;
    struct D3DKMT_ALIGN64
    {
        UINT64 PreservedDuringStandby : 1;
        UINT64 PreservedDuringHibernate : 1;
        UINT64 PartiallyPreservedDuringHibernate : 1;
        UINT64 Reserved : 61;
    } PowerFlags;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
    struct D3DKMT_ALIGN64
    {
        UINT64 SystemMemory : 1;
        UINT64 PopulatedByReservedDDRByFirmware : 1;
        UINT64 Reserved : 62;
    } SegmentProperties;
    D3DKMT_ALIGN64 UINT64 Reserved[5];
#else
    D3DKMT_ALIGN64 UINT64 Reserved[6];
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
#else
    D3DKMT_ALIGN64 UINT64 Reserved[8];
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
} D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION;

//
// Video memory statistics.
//
typedef struct _D3DKMT_QUERYSTATISTICS_VIDEO_MEMORY
{
    ULONG AllocsCommitted;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocsResidentInP[D3DKMT_QUERYSTATISTICS_SEGMENT_PREFERENCE_MAX];
    D3DKMT_QUERYSTATISTICS_COUNTER AllocsResidentInNonPreferred;
    D3DKMT_ALIGN64 ULONGLONG TotalBytesEvictedDueToPreparation;
} D3DKMT_QUERYSTATISTICS_VIDEO_MEMORY;

//
// VidMM Policies
//
typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_POLICY
{
    D3DKMT_ALIGN64 ULONGLONG UseMRU;
} D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_POLICY;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_INFORMATION
{
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    D3DKMT_ALIGN64 ULONGLONG BytesCommitted;
    D3DKMT_ALIGN64 ULONGLONG MaximumWorkingSet;
    D3DKMT_ALIGN64 ULONGLONG MinimumWorkingSet;

    ULONG NbReferencedAllocationEvictedInPeriod;
    UINT Padding;
#else
    ULONG BytesCommitted;
    ULONG NbReferencedAllocationEvictedInPeriod;
    ULONG MaximumWorkingSet;
    ULONG MinimumWorkingSet;
#endif

    D3DKMT_QUERYSTATISTICS_VIDEO_MEMORY VideoMemory;
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_POLICY _Policy;

    D3DKMT_ALIGN64 UINT64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_INFORMATION;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_GROUP_INFORMATION
{
    D3DKMT_ALIGN64 UINT64 Budget;
    D3DKMT_ALIGN64 UINT64 Requested;
    D3DKMT_ALIGN64 UINT64 Usage;
    D3DKMT_ALIGN64 UINT64 Demoted[D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS_MAX];
} D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_GROUP_INFORMATION;
#endif

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
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_GROUP  = 9,
#endif
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
    D3DKMT_QUERYSTATISTICS_PHYSICAL_ADAPTER       = 10,
#endif
} D3DKMT_QUERYSTATISTICS_TYPE;

typedef struct _D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT
{
    ULONG SegmentId; // in: id of node to get statistics for
} D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT;

typedef struct _D3DKMT_QUERYSTATISTICS_QUERY_NODE
{
    ULONG NodeId;
} D3DKMT_QUERYSTATISTICS_QUERY_NODE;

typedef struct _D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE
{
    ULONG VidPnSourceId; // in: id of segment to get statistics for
} D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
typedef struct _D3DKMT_QUERYSTATISTICS_QUERY_PHYSICAL_ADAPTER
{
    ULONG PhysicalAdapterIndex;
} D3DKMT_QUERYSTATISTICS_QUERY_PHYSICAL_ADAPTER;
#endif

typedef union _D3DKMT_QUERYSTATISTICS_RESULT
{
    D3DKMT_QUERYSTATISTICS_ADAPTER_INFORMATION AdapterInformation;                          // out: result of D3DKMT_QUERYSTATISTICS_ADAPTER query
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
    D3DKMT_QUERYSTATISTICS_PHYSICAL_ADAPTER_INFORMATION PhysAdapterInformation;             // out: result of D3DKMT_QUERYSTATISTICS_PHYSICAL_ADAPTER query
#endif
    D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION SegmentInformation;                          // out: result of D3DKMT_QUERYSTATISTICS_SEGMENT query
    D3DKMT_QUERYSTATISTICS_NODE_INFORMATION NodeInformation;                                // out: result of D3DKMT_QUERYSTATISTICS_NODE query
    D3DKMT_QUERYSTATISTICS_VIDPNSOURCE_INFORMATION VidPnSourceInformation;                  // out: result of D3DKMT_QUERYSTATISTICS_VIDPNSOURCE query
    D3DKMT_QUERYSTATISTICS_PROCESS_INFORMATION ProcessInformation;                          // out: result of D3DKMT_QUERYSTATISTICS_PROCESS query
    D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER_INFORMATION ProcessAdapterInformation;           // out: result of D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER query
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_INFORMATION ProcessSegmentInformation;           // out: result of D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT query
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION ProcessNodeInformation;                 // out: result of D3DKMT_QUERYSTATISTICS_PROCESS_NODE query
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION ProcessVidPnSourceInformation;   // out: result of D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE query
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_GROUP_INFORMATION ProcessSegmentGroupInformation;// out: result of D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_GROUP query
#endif
} D3DKMT_QUERYSTATISTICS_RESULT;

typedef struct _D3DKMT_QUERYSTATISTICS
{
    D3DKMT_QUERYSTATISTICS_TYPE   Type;        // in: type of data requested
    LUID                          AdapterLuid; // in: adapter to get export / statistics from
    D3DKMT_PTR(HANDLE,            hProcess);   // in: process to get statistics for, if required for this query type
    D3DKMT_QUERYSTATISTICS_RESULT QueryResult; // out: requested data

    union
    {
        D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT QuerySegment;                // in: id of segment to get statistics for
        D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT QueryProcessSegment;         // in: id of segment to get statistics for
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
        D3DKMT_MEMORY_SEGMENT_GROUP QueryProcessSegmentGroup;             // in: id of segment group to get statistics for
#endif
        D3DKMT_QUERYSTATISTICS_QUERY_NODE QueryNode;                      // in: id of node to get statistics for
        D3DKMT_QUERYSTATISTICS_QUERY_NODE QueryProcessNode;               // in: id of node to get statistics for
        D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE QueryVidPnSource;        // in: id of vidpnsource to get statistics for
        D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE QueryProcessVidPnSource; // in: id of vidpnsource to get statistics for
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
        D3DKMT_QUERYSTATISTICS_QUERY_PHYSICAL_ADAPTER QueryPhysAdapter;   // in: id of physical adapter to get statistics for
#endif
    };
} D3DKMT_QUERYSTATISTICS;
#if defined(_AMD64_)
C_ASSERT(sizeof(D3DKMT_QUERYSTATISTICS) == 0x328);
#endif

//
// end D3DKMT_QUERYSTATISTICS
//


typedef struct _D3DKMT_PRESENT_STATS_DWM2
{
    ULONG                        cbSize; // in: size of struct for versioning
    UINT                         PresentCount;
    UINT                         PresentRefreshCount;
    D3DKMT_ALIGN64 LARGE_INTEGER PresentQPCTime;
    UINT                         SyncRefreshCount;
    D3DKMT_ALIGN64 LARGE_INTEGER SyncQPCTime;
    UINT                         CustomPresentDuration;
    UINT                         VirtualSyncRefreshCount;
    D3DKMT_ALIGN64 LARGE_INTEGER VirtualSyncQPCTime;
} D3DKMT_PRESENT_STATS_DWM2;


typedef enum _D3DKMT_VIDPNSOURCEOWNER_TYPE
{
     D3DKMT_VIDPNSOURCEOWNER_UNOWNED        = 0,    //Has no owner or GDI is the owner
     D3DKMT_VIDPNSOURCEOWNER_SHARED         = 1,    //Has shared owner, that is owner can yield to any exclusive owner, not available to legacy devices
     D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE      = 2,    //Has exclusive owner without shared gdi primary,
     D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVEGDI   = 3,    //Has exclusive owner with shared gdi primary and must be exclusive owner of all VidPn sources, only available to legacy devices
     D3DKMT_VIDPNSOURCEOWNER_EMULATED       = 4,    //Does not have real primary ownership, but allows the device to set gamma on its owned sources
} D3DKMT_VIDPNSOURCEOWNER_TYPE;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
typedef struct _D3DKMT_VIDPNSOURCEOWNER_FLAGS
{
    union
    {
        struct
        {
            UINT AllowOutputDuplication : 1;
            UINT DisableDWMVirtualMode  : 1;
            UINT UseNtHandles           : 1;
            UINT Reserved               : 29;
        };
        UINT Value;
    };
} D3DKMT_VIDPNSOURCEOWNER_FLAGS;
#endif

typedef struct _D3DKMT_SETVIDPNSOURCEOWNER
{
    D3DKMT_HANDLE                           hDevice;            // in: Device handle
    D3DKMT_PTR(CONST D3DKMT_VIDPNSOURCEOWNER_TYPE*, pType);     // in: OwnerType array
    D3DKMT_PTR(CONST D3DDDI_VIDEO_PRESENT_SOURCE_ID*, pVidPnSourceId); // in: VidPn source ID array
    UINT                                    VidPnSourceCount;   // in: Number of valid entries in above array
} D3DKMT_SETVIDPNSOURCEOWNER;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
typedef struct _D3DKMT_SETVIDPNSOURCEOWNER1
{
    D3DKMT_SETVIDPNSOURCEOWNER              Version0;
    D3DKMT_VIDPNSOURCEOWNER_FLAGS           Flags;
} D3DKMT_SETVIDPNSOURCEOWNER1;
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
typedef struct _D3DKMT_SETVIDPNSOURCEOWNER2
{
    D3DKMT_SETVIDPNSOURCEOWNER1             Version1;
    D3DKMT_PTR(CONST D3DKMT_PTR_TYPE*,      pVidPnSourceNtHandles); // in: VidPn source owner DispMgr NT handles
} D3DKMT_SETVIDPNSOURCEOWNER2;
#endif

typedef struct _D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP
{
    D3DKMT_HANDLE                           hAdapter;           // in: Adapter handle
    D3DDDI_VIDEO_PRESENT_SOURCE_ID          VidPnSourceId;      // in: VidPn source ID array
} D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP;

#define D3DKMT_GETPRESENTHISTORY_MAXTOKENS  2048

typedef struct _D3DKMT_GETPRESENTHISTORY
{
    D3DKMT_HANDLE                                             hAdapter;     // in: Handle to adapter
    UINT                                                      ProvidedSize; // in: Size of provided buffer
    UINT                                                      WrittenSize;  // out: Copied token size or required size for first token
    D3DKMT_PTR(_Field_size_bytes_(ProvidedSize) D3DKMT_PRESENTHISTORYTOKEN*, pTokens); // in: Pointer to buffer.
    UINT                                                      NumTokens;    // out: Number of copied token
} D3DKMT_GETPRESENTHISTORY;

typedef struct _D3DKMT_CREATEOVERLAY
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;      // in
    D3DKMT_HANDLE                   hDevice;            // in: Indentifies the device
    D3DDDI_KERNELOVERLAYINFO        OverlayInfo;        // in
    D3DKMT_HANDLE                   hOverlay;           // out: Kernel overlay handle
} D3DKMT_CREATEOVERLAY;

typedef struct _D3DKMT_UPDATEOVERLAY
{
    D3DKMT_HANDLE            hDevice;           // in: Indentifies the device
    D3DKMT_HANDLE            hOverlay;          // in: Kernel overlay handle
    D3DDDI_KERNELOVERLAYINFO OverlayInfo;       // in
} D3DKMT_UPDATEOVERLAY;

typedef struct _D3DKMT_FLIPOVERLAY
{
    D3DKMT_HANDLE        hDevice;               // in: Indentifies the device
    D3DKMT_HANDLE        hOverlay;              // in: Kernel overlay handle
    D3DKMT_HANDLE        hSource;               // in: Allocation currently displayed
    D3DKMT_PTR(VOID*,    pPrivateDriverData);   // in: Private driver data
    UINT                 PrivateDriverDataSize; // in: Size of private driver data
} D3DKMT_FLIPOVERLAY;

typedef struct _D3DKMT_GETOVERLAYSTATE
{
    D3DKMT_HANDLE        hDevice;               // in: Indentifies the device
    D3DKMT_HANDLE        hOverlay;              // in: Kernel overlay handle
    BOOLEAN              OverlayEnabled;
} D3DKMT_GETOVERLAYSTATE;

typedef struct _D3DKMT_DESTROYOVERLAY
{
    D3DKMT_HANDLE        hDevice;               // in: Indentifies the device
    D3DKMT_HANDLE        hOverlay;              // in: Kernel overlay handle
} D3DKMT_DESTROYOVERLAY;

typedef struct _D3DKMT_WAITFORVERTICALBLANKEVENT
{
    D3DKMT_HANDLE                   hAdapter;      // in: adapter handle
    D3DKMT_HANDLE                   hDevice;       // in: device handle [Optional]
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId; // in: adapter's VidPN Source ID
} D3DKMT_WAITFORVERTICALBLANKEVENT;

#define D3DKMT_MAX_WAITFORVERTICALBLANK_OBJECTS 8

typedef struct _D3DKMT_WAITFORVERTICALBLANKEVENT2
{
    D3DKMT_HANDLE                   hAdapter;      // in: adapter handle
    D3DKMT_HANDLE                   hDevice;       // in: device handle [Optional]
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId; // in: adapter's VidPN Source ID
    UINT                            NumObjects;
    D3DKMT_PTR_TYPE                 ObjectHandleArray[D3DKMT_MAX_WAITFORVERTICALBLANK_OBJECTS]; // in: Specifies the objects to wait on.
} D3DKMT_WAITFORVERTICALBLANKEVENT2;

typedef struct _D3DKMT_GETVERTICALBLANKEVENT
{
    D3DKMT_HANDLE                   hAdapter;      // in: adapter handle
    D3DKMT_HANDLE                   hDevice;       // in: device handle [Optional]
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId; // in: adapter's VidPN Source ID
    D3DKMT_PTR(D3DKMT_PTR_TYPE*,    phEvent);
} D3DKMT_GETVERTICALBLANKEVENT;

typedef struct _D3DKMT_SETSYNCREFRESHCOUNTWAITTARGET
{
    D3DKMT_HANDLE                   hAdapter;      // in: adapter handle
    D3DKMT_HANDLE                   hDevice;       // in: device handle [Optional]
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId; // in: adapter's VidPN Source ID
    UINT                            TargetSyncRefreshCount;
} D3DKMT_SETSYNCREFRESHCOUNTWAITTARGET;

typedef struct _D3DKMT_SETGAMMARAMP
{
    D3DKMT_HANDLE                   hDevice;       // in: device handle
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId; // in: adapter's VidPN Source ID
    D3DDDI_GAMMARAMP_TYPE           Type;          // in: Gamma ramp type

    union
    {
        D3DDDI_GAMMA_RAMP_RGB256x3x16* pGammaRampRgb256x3x16;
        D3DDDI_GAMMA_RAMP_DXGI_1*      pGammaRampDXGI1;
        D3DKMT_PTR_HELPER(             AlignUnionTo64)
    };
    UINT                            Size;
} D3DKMT_SETGAMMARAMP;

typedef struct _D3DKMT_ADJUSTFULLSCREENGAMMA
{
    D3DKMT_HANDLE                   hAdapter;      // in: adapter handle
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId; // in: adapter's VidPN Source ID
    D3DDDI_DXGI_RGB                 Scale;
    D3DDDI_DXGI_RGB                 Offset;
} D3DKMT_ADJUSTFULLSCREENGAMMA;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)

typedef struct _D3DKMT_SET_COLORSPACE_TRANSFORM
{
    _In_ LUID  AdapterLuid;
    _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID  VidPnTargetId;
    _In_ D3DDDI_GAMMARAMP_TYPE  Type;
    _In_ UINT  Size;
    union
    {
        _In_reads_bytes_opt_(Size) D3DKMDT_3x4_COLORSPACE_TRANSFORM*  pColorSpaceTransform;
        D3DKMT_PTR_HELPER(                                            AlignUnionTo64)
    };
} D3DKMT_SET_COLORSPACE_TRANSFORM;

#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_3

typedef struct _D3DKMT_SETVIDPNSOURCEHWPROTECTION
{
    D3DKMT_HANDLE                   hAdapter;      // in: adapter handle
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId; // in: adapter's VidPN Source ID
    BOOL                            HwProtected;   // in: HW protection status
} D3DKMT_SETVIDPNSOURCEHWPROTECTION;

typedef struct _D3DKMT_SETHWPROTECTIONTEARDOWNRECOVERY
{
    D3DKMT_HANDLE                   hAdapter;      // in: adapter handle
    BOOL                            Recovered;     // in: HW protection teardown recovery
} D3DKMT_SETHWPROTECTIONTEARDOWNRECOVERY;

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
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
} D3DKMT_DEVICEEXECUTION_STATE;

typedef struct _D3DKMT_DEVICERESET_STATE
{
    union
    {
        struct
        {
            UINT    DesktopSwitched : 1;        // 0x00000001
            UINT    Reserved        :31;        // 0xFFFFFFFE
        };
        UINT    Value;
    };
} D3DKMT_DEVICERESET_STATE;

typedef struct _D3DKMT_PRESENT_STATS
{
    UINT PresentCount;
    UINT PresentRefreshCount;
    UINT SyncRefreshCount;
    D3DKMT_ALIGN64 LARGE_INTEGER SyncQPCTime;
    D3DKMT_ALIGN64 LARGE_INTEGER SyncGPUTime;
} D3DKMT_PRESENT_STATS;

typedef struct _D3DKMT_DEVICEPRESENT_STATE
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId; // in: present source id
    D3DKMT_PRESENT_STATS           PresentStats;  // out: present stats
} D3DKMT_DEVICEPRESENT_STATE;

typedef struct _D3DKMT_PRESENT_STATS_DWM
{
    UINT PresentCount;
    UINT PresentRefreshCount;
    D3DKMT_ALIGN64 LARGE_INTEGER PresentQPCTime;
    UINT SyncRefreshCount;
    D3DKMT_ALIGN64 LARGE_INTEGER SyncQPCTime;
    UINT CustomPresentDuration;
} D3DKMT_PRESENT_STATS_DWM;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

typedef struct _D3DKMT_DEVICEPAGEFAULT_STATE
{
    D3DKMT_ALIGN64 UINT64       FaultedPrimitiveAPISequenceNumber; // when per draw fence write is enabled, identifies the draw that caused the page fault, or DXGK_PRIMITIVE_API_SEQUENCE_NUMBER_UNKNOWN if such information is not available.
    DXGK_RENDER_PIPELINE_STAGE  FaultedPipelineStage;   // render pipeline stage during which the fault was generated, or DXGK_RENDER_PIPELINE_STAGE_UNKNOWN if such information is not available.
    UINT                        FaultedBindTableEntry;  // a bind table index of a resource being accessed at the time of the fault, or DXGK_BIND_TABLE_ENTRY_UNKNOWN if such information is not available.
    DXGK_PAGE_FAULT_FLAGS       PageFaultFlags;         // flags specifying the nature of the fault
    DXGK_FAULT_ERROR_CODE       FaultErrorCode;         // Structure that contains error code describing the fault.
    D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS FaultedVirtualAddress;  // virtual address of faulting resource, or D3DGPU_NULL if such information is not available.
} D3DKMT_DEVICEPAGEFAULT_STATE;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

typedef struct _D3DKMT_DEVICEPRESENT_STATE_DWM
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId; // in: present source id
    D3DKMT_PRESENT_STATS_DWM       PresentStatsDWM; // out: present stats rev 2
} D3DKMT_DEVICEPRESENT_STATE_DWM;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)

typedef struct _D3DKMT_DEVICEPRESENT_QUEUE_STATE
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId; // in: present source id
    BOOLEAN bQueuedPresentLimitReached;           // out: whether the queued present limit has been reached
} D3DKMT_DEVICEPRESENT_QUEUE_STATE;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)

typedef enum _D3DKMT_DEVICESTATE_TYPE
{
    D3DKMT_DEVICESTATE_EXECUTION = 1,
    D3DKMT_DEVICESTATE_PRESENT   = 2,
    D3DKMT_DEVICESTATE_RESET     = 3,
    D3DKMT_DEVICESTATE_PRESENT_DWM = 4,

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

    D3DKMT_DEVICESTATE_PAGE_FAULT = 5,

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)

    D3DKMT_DEVICESTATE_PRESENT_QUEUE = 6,

#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_1
} D3DKMT_DEVICESTATE_TYPE;

typedef struct _D3DKMT_GETDEVICESTATE
{
    D3DKMT_HANDLE                   hDevice;       // in: device handle
    D3DKMT_DEVICESTATE_TYPE         StateType;     // in: device state type
    union
    {
        D3DKMT_DEVICEEXECUTION_STATE ExecutionState; // out: device state
        D3DKMT_DEVICEPRESENT_STATE   PresentState;   // in/out: present state
        D3DKMT_DEVICERESET_STATE     ResetState;     // out: reset state
        D3DKMT_DEVICEPRESENT_STATE_DWM  PresentStateDWM;  // in/out: present state

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

        D3DKMT_DEVICEPAGEFAULT_STATE PageFaultState; // out: page fault state

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)

        D3DKMT_DEVICEPRESENT_QUEUE_STATE PresentQueueState; // in/out: present queue state

#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_1
    };
} D3DKMT_GETDEVICESTATE;

typedef struct _D3DKMT_CREATEDCFROMMEMORY
{
    D3DKMT_PTR(VOID*,               pMemory);      // in: memory for DC
    D3DDDIFORMAT                    Format;        // in: Memory pixel format
    UINT                            Width;         // in: Memory Width
    UINT                            Height;        // in: Memory Height
    UINT                            Pitch;         // in: Memory pitch
    D3DKMT_PTR(HDC,                 hDeviceDc);    // in: DC describing the device
    D3DKMT_PTR(PALETTEENTRY*,       pColorTable);  // in: Palette
    D3DKMT_PTR(HDC,                 hDc);          // out: HDC
    D3DKMT_PTR(HANDLE,              hBitmap);      // out: Handle to bitmap
} D3DKMT_CREATEDCFROMMEMORY;

typedef struct _D3DKMT_DESTROYDCFROMMEMORY
{
    D3DKMT_PTR(HDC,    hDc);           // in:
    D3DKMT_PTR(HANDLE, hBitmap);       // in:
} D3DKMT_DESTROYDCFROMMEMORY;

#define D3DKMT_SETCONTEXTSCHEDULINGPRIORITY_ABSOLUTE 0x40000000

typedef struct _D3DKMT_SETCONTEXTSCHEDULINGPRIORITY
{
    D3DKMT_HANDLE                   hContext;      // in: context handle
    INT                             Priority;      // in: context priority
} D3DKMT_SETCONTEXTSCHEDULINGPRIORITY;

typedef struct _D3DKMT_SETCONTEXTINPROCESSSCHEDULINGPRIORITY
{
    D3DKMT_HANDLE                   hContext;      // in: context handle
    INT                             Priority;      // in: context priority
} D3DKMT_SETCONTEXTINPROCESSSCHEDULINGPRIORITY;

typedef struct _D3DKMT_CHANGESURFACEPOINTER
{
    D3DKMT_PTR(HDC,                 hDC);            // in: dc handle
    D3DKMT_PTR(HANDLE,              hBitmap);        // in: bitmap handle
    D3DKMT_PTR(LPVOID,              pSurfacePointer);// in: new surface pointer
    UINT                            Width;           // in: Memory Width
    UINT                            Height;          // in: Memory Height
    UINT                            Pitch;           // in: Memory pitch
} D3DKMT_CHANGESURFACEPOINTER;

typedef struct _D3DKMT_GETCONTEXTSCHEDULINGPRIORITY
{
    D3DKMT_HANDLE                   hContext;      // in: context handle
    INT                             Priority;      // out: context priority
} D3DKMT_GETCONTEXTSCHEDULINGPRIORITY;

typedef struct _D3DKMT_GETCONTEXTINPROCESSSCHEDULINGPRIORITY
{
    D3DKMT_HANDLE                   hContext;      // in: context handle
    INT                             Priority;      // out: context priority
} D3DKMT_GETCONTEXTINPROCESSSCHEDULINGPRIORITY;

typedef enum _D3DKMT_SCHEDULINGPRIORITYCLASS
{
    D3DKMT_SCHEDULINGPRIORITYCLASS_IDLE         = 0,
    D3DKMT_SCHEDULINGPRIORITYCLASS_BELOW_NORMAL = 1,
    D3DKMT_SCHEDULINGPRIORITYCLASS_NORMAL       = 2,
    D3DKMT_SCHEDULINGPRIORITYCLASS_ABOVE_NORMAL = 3,
    D3DKMT_SCHEDULINGPRIORITYCLASS_HIGH         = 4,
    D3DKMT_SCHEDULINGPRIORITYCLASS_REALTIME     = 5,
} D3DKMT_SCHEDULINGPRIORITYCLASS;

typedef struct _D3DKMT_GETSCANLINE
{
    D3DKMT_HANDLE                   hAdapter;           // in: Adapter handle
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;      // in: Adapter's VidPN Source ID
    BOOLEAN                         InVerticalBlank;    // out: Within vertical blank
    UINT                            ScanLine;           // out: Current scan line
} D3DKMT_GETSCANLINE;

typedef enum _D3DKMT_QUEUEDLIMIT_TYPE
{
    D3DKMT_SET_QUEUEDLIMIT_PRESENT     = 1,
    D3DKMT_GET_QUEUEDLIMIT_PRESENT     = 2,
} D3DKMT_QUEUEDLIMIT_TYPE;

typedef struct _D3DKMT_SETQUEUEDLIMIT
{
    D3DKMT_HANDLE                   hDevice;            // in: device handle
    D3DKMT_QUEUEDLIMIT_TYPE         Type;               // in: limit type
    union
    {
        UINT                        QueuedPresentLimit; // in (or out): queued present limit
        struct
        {
            D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;          // in: adapter's VidPN source ID
            UINT                           QueuedPendingFlipLimit; // in (or out): flip pending limit
        };
    };
} D3DKMT_SETQUEUEDLIMIT;

typedef struct _D3DKMT_POLLDISPLAYCHILDREN
{
    D3DKMT_HANDLE                   hAdapter;                 // in: Adapter handle
    UINT                            NonDestructiveOnly :  1;  // in: 0x00000001 Destructive or not
    UINT                            SynchronousPolling :  1;  // in: 0x00000002 Synchronous polling or not
    UINT                            DisableModeReset   :  1;  // in: 0x00000004 Disable DMM mode reset on monitor event
    UINT                            PollAllAdapters    :  1;  // in: 0x00000008 Poll all adapters
    UINT                            PollInterruptible  :  1;  // in: 0x00000010 Poll interruptible targets as well.
    UINT                            Reserved           : 27;  // in: 0xffffffc0
} D3DKMT_POLLDISPLAYCHILDREN;

typedef struct _D3DKMT_INVALIDATEACTIVEVIDPN
{
    D3DKMT_HANDLE                   hAdapter;               // in: Adapter handle
    D3DKMT_PTR(VOID*,               pPrivateDriverData);    // in: Private driver data
    UINT                            PrivateDriverDataSize;  // in: Size of private driver data
} D3DKMT_INVALIDATEACTIVEVIDPN;

typedef struct _D3DKMT_CHECKOCCLUSION
{
    D3DKMT_PTR(HWND,            hWindow);        // in:  Destination window handle
} D3DKMT_CHECKOCCLUSION;

typedef struct _D3DKMT_WAITFORIDLE
{
    D3DKMT_HANDLE   hDevice;        // in:  Device to wait for idle
} D3DKMT_WAITFORIDLE;

typedef struct _D3DKMT_CHECKMONITORPOWERSTATE
{
    D3DKMT_HANDLE    hAdapter;    // in: Adapter to check on
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;      // in: Adapter's VidPN Source ID
} D3DKMT_CHECKMONITORPOWERSTATE;

typedef struct _D3DKMT_SETDISPLAYPRIVATEDRIVERFORMAT
{
    D3DKMT_HANDLE                   hDevice;                         // in: Identifies the device
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;                   // in: Identifies which VidPn we are changing the private driver format attribute for
    UINT                            PrivateDriverFormatAttribute;    // In: Requested private format attribute for VidPn specified
} D3DKMT_SETDISPLAYPRIVATEDRIVERFORMAT;

typedef struct _D3DKMT_CREATEKEYEDMUTEX
{
    D3DKMT_ALIGN64 UINT64                   InitialValue;               // in:  Initial value to release to
    D3DKMT_HANDLE                           hSharedHandle;              // out:  Global handle to keyed mutex
    D3DKMT_HANDLE                           hKeyedMutex;                // out: Handle to the keyed mutex in this process
} D3DKMT_CREATEKEYEDMUTEX;

typedef struct _D3DKMT_OPENKEYEDMUTEX
{
    D3DKMT_HANDLE                           hSharedHandle;              // in:  Global handle to keyed mutex
    D3DKMT_HANDLE                           hKeyedMutex;                // out: Handle to the keyed mutex in this process
} D3DKMT_OPENKEYEDMUTEX;

typedef struct _D3DKMT_DESTROYKEYEDMUTEX
{
    D3DKMT_HANDLE                           hKeyedMutex;                // in:  Identifies the keyed mutex being destroyed.
} D3DKMT_DESTROYKEYEDMUTEX;

typedef struct _D3DKMT_ACQUIREKEYEDMUTEX
{
    D3DKMT_HANDLE                           hKeyedMutex;                // in: Handle to the keyed mutex
    D3DKMT_ALIGN64 UINT64                   Key;                        // in: Key value to Acquire
    D3DKMT_PTR(PLARGE_INTEGER,              pTimeout);                  // in: NT-style timeout value
    D3DKMT_ALIGN64 UINT64                   FenceValue;                 // out: Current fence value of the GPU sync object
} D3DKMT_ACQUIREKEYEDMUTEX;

typedef struct _D3DKMT_RELEASEKEYEDMUTEX
{
    D3DKMT_HANDLE                           hKeyedMutex;                // in: Handle to the keyed mutex
    D3DKMT_ALIGN64 UINT64                   Key;                        // in: Key value to Release to
    D3DKMT_ALIGN64 UINT64                   FenceValue;                 // in: New fence value to use for GPU sync object
} D3DKMT_RELEASEKEYEDMUTEX;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)

typedef struct _D3DKMT_CREATEKEYEDMUTEX2_FLAGS
{
    union
    {
        struct
        {
            UINT NtSecuritySharing      : 1;    // If set, the keyed mutex will be shared using DxgkShareObjects instead of D3DKMT_CREATEKEYEDMUTEX2::hSharedHandle
            UINT Reserved               : 31;
        };
        UINT Value;
    };
} D3DKMT_CREATEKEYEDMUTEX2_FLAGS;

typedef struct _D3DKMT_CREATEKEYEDMUTEX2
{
    D3DKMT_ALIGN64 UINT64                         InitialValue;           // in:  Initial value to release to
    D3DKMT_HANDLE                                 hSharedHandle;          // out: Global handle to keyed mutex, NULL if NtSecuritySharing is set.
    D3DKMT_HANDLE                                 hKeyedMutex;            // out: Handle to the keyed mutex in this process
    D3DKMT_PTR(_In_reads_bytes_opt_(PrivateRuntimeDataSize)
    VOID*,                                        pPrivateRuntimeData);   // in:  Buffer containing initial private data.
                                                                          //      If NULL then PrivateRuntimeDataSize must be 0.
    UINT                                          PrivateRuntimeDataSize; // in:  Size in bytes of pPrivateRuntimeData.
    D3DKMT_CREATEKEYEDMUTEX2_FLAGS                Flags;                  // in:  Creation flags.
} D3DKMT_CREATEKEYEDMUTEX2;

typedef struct _D3DKMT_OPENKEYEDMUTEX2
{
    D3DKMT_HANDLE                                 hSharedHandle;          // in:  Global handle to keyed mutex
    D3DKMT_HANDLE                                 hKeyedMutex;            // out: Handle to the keyed mutex in this process
    D3DKMT_PTR(_In_reads_bytes_opt_(PrivateRuntimeDataSize)
    VOID*,                                        pPrivateRuntimeData);   // in:  Buffer containing initial private data.
                                                                          //      If NULL then PrivateRuntimeDataSize must be 0.
                                                                          //      It will only be copied if the keyed mutex does not already have private data.
    UINT                                          PrivateRuntimeDataSize; // in:  Size in bytes of pPrivateRuntimeData.
} D3DKMT_OPENKEYEDMUTEX2;

typedef struct _D3DKMT_OPENKEYEDMUTEXFROMNTHANDLE
{
    D3DKMT_PTR(HANDLE,                            hNtHandle);             // in:  NT handle to keyed mutex
    D3DKMT_HANDLE                                 hKeyedMutex;            // out: Handle to the keyed mutex in this process
    D3DKMT_PTR(_In_reads_bytes_opt_(PrivateRuntimeDataSize)
    VOID*,                                        pPrivateRuntimeData);   // in:  Buffer containing initial private data.
                                                                          //      If NULL then PrivateRuntimeDataSize must be 0.
                                                                          //      It will only be copied if the keyed mutex does not already have private data.
    UINT                                          PrivateRuntimeDataSize; // in:  Size in bytes of pPrivateRuntimeData.
} D3DKMT_OPENKEYEDMUTEXFROMNTHANDLE;

typedef struct _D3DKMT_ACQUIREKEYEDMUTEX2
{
    D3DKMT_HANDLE                                       hKeyedMutex;            // in:  Handle to the keyed mutex
    D3DKMT_ALIGN64 UINT64                               Key;                    // in:  Key value to Acquire
    D3DKMT_PTR(PLARGE_INTEGER,                          pTimeout);              // in:  NT-style timeout value
    D3DKMT_ALIGN64 UINT64                               FenceValue;             // out: Current fence value of the GPU sync object
    D3DKMT_PTR(_Out_writes_bytes_all_opt_(PrivateRuntimeDataSize)
    VOID*,                                              pPrivateRuntimeData);   // out: Buffer to copy private data to.
                                                                                //      If NULL then PrivateRuntimeDataSize must be 0.
    UINT                                                PrivateRuntimeDataSize; // in:  Size in bytes of pPrivateRuntimeData.
} D3DKMT_ACQUIREKEYEDMUTEX2;

typedef struct _D3DKMT_RELEASEKEYEDMUTEX2
{
    D3DKMT_HANDLE                                 hKeyedMutex;            // in: Handle to the keyed mutex
    D3DKMT_ALIGN64 UINT64                         Key;                    // in: Key value to Release to
    D3DKMT_ALIGN64 UINT64                         FenceValue;             // in: New fence value to use for GPU sync object
    D3DKMT_PTR(_In_reads_bytes_opt_(PrivateRuntimeDataSize)
    VOID*,                                        pPrivateRuntimeData);   // in: Buffer containing new private data.
                                                                          //     If NULL then PrivateRuntimeDataSize must be 0.
    UINT                                          PrivateRuntimeDataSize; // in: Size in bytes of pPrivateRuntimeData.
} D3DKMT_RELEASEKEYEDMUTEX2;
#endif


typedef struct _D3DKMT_CONFIGURESHAREDRESOURCE
{
    D3DKMT_HANDLE   hDevice;        // in:  Device that created the resource
    D3DKMT_HANDLE   hResource;      // in: Handle for shared resource
    BOOLEAN         IsDwm;          // in: TRUE when the process is DWM
    D3DKMT_PTR(HANDLE, hProcess);   // in: Process handle for the non-DWM case
    BOOLEAN         AllowAccess;    // in: Indicates whereh the process is allowed access
} D3DKMT_CONFIGURESHAREDRESOURCE;

typedef struct _D3DKMT_CHECKSHAREDRESOURCEACCESS
{
    D3DKMT_HANDLE   hResource;      // in: Handle for the resource
    UINT            ClientPid;      // in: Client process PID
} D3DKMT_CHECKSHAREDRESOURCEACCESS;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
typedef enum _D3DKMT_OFFER_PRIORITY
{
    D3DKMT_OFFER_PRIORITY_LOW=1,    // Content is not useful
    D3DKMT_OFFER_PRIORITY_NORMAL,   // Content is useful but easy to regenerate
    D3DKMT_OFFER_PRIORITY_HIGH,     // Content is useful and difficult to regenerate
    D3DKMT_OFFER_PRIORITY_AUTO,     // Let VidMm decide offer priority based on eviction priority
} D3DKMT_OFFER_PRIORITY;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
typedef struct _D3DKMT_OFFER_FLAGS
{
    union
    {
        struct
        {
            UINT OfferImmediately :  1; // 0x00000001
            UINT AllowDecommit    :  1; // 0x00000002
            UINT Reserved         : 30; // 0xFFFFFFFC
        };
        UINT Value;
    };
} D3DKMT_OFFER_FLAGS;
#endif // DXGKDDI_INTERFACE_VERSION

typedef struct _D3DKMT_OFFERALLOCATIONS
{
    D3DKMT_HANDLE hDevice;                        // in: Device that created the allocations
    D3DKMT_PTR(D3DKMT_HANDLE*, pResources);       // in: array of D3D runtime resource handles.
    D3DKMT_PTR(CONST D3DKMT_HANDLE*, HandleList); // in: array of allocation handles. If non-NULL, pResources must be NULL.
    UINT NumAllocations;                          // in: number of items in whichever of pResources or HandleList is non-NULL.
    D3DKMT_OFFER_PRIORITY Priority;               // in: priority with which to offer the allocations
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    D3DKMT_OFFER_FLAGS Flags;                     // in: various flags for determining offer behavior
#endif // DXGKDDI_INTERFACE_VERSION
} D3DKMT_OFFERALLOCATIONS;

typedef struct _D3DKMT_RECLAIMALLOCATIONS
{
    D3DKMT_HANDLE hDevice;                        // in:  Device that created the allocations
    D3DKMT_PTR(D3DKMT_HANDLE*, pResources);       // in: array of D3D runtime resource handles.
    D3DKMT_PTR(CONST D3DKMT_HANDLE*, HandleList); // in: array of allocation handles. If non-NULL, pResources must be NULL.
    D3DKMT_PTR(BOOL*, pDiscarded);                // out: optional array of booleans specifying whether each resource or allocation was discarded.
    UINT NumAllocations;                          // in:  number of items in pDiscarded and whichever of pResources or HandleList is non-NULL.
} D3DKMT_RECLAIMALLOCATIONS;

typedef struct _D3DKMT_RECLAIMALLOCATIONS2
{
    D3DKMT_HANDLE hPagingQueue;                   // in:  Device that created the allocations
    UINT NumAllocations;                          // in:  number of items in pDiscarded and whichever of pResources or HandleList is non-NULL.
    D3DKMT_PTR(D3DKMT_HANDLE*, pResources);       // in: array of D3D runtime resource handles.
    D3DKMT_PTR(CONST D3DKMT_HANDLE*, HandleList); // in: array of allocation handles. If non-NULL, pResources must be NULL.
#if(DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1 || \
    D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1)
    union
    {
        BOOL* pDiscarded;                   // out: optional array of booleans specifying whether each resource or allocation was discarded.
        D3DDDI_RECLAIM_RESULT* pResults;    // out: array of results specifying whether each resource or allocation is OK, discarded, or has no commitment.
        D3DKMT_PTR_HELPER(AlignUnionTo64)
    };
#else
    D3DKMT_PTR(BOOL*, pDiscarded);          // out: optional array of booleans specifying whether each resource or allocation was discarded.
#endif // (DXGKDDI_INTERFACE_VERSION || D3D_UMD_INTERFACE_VERSION)
    D3DKMT_ALIGN64 UINT64 PagingFenceValue; // out: The paging fence to synchronize against before submitting work to the GPU which
                                            //      references any of the resources or allocations in the provided arrays
} D3DKMT_RECLAIMALLOCATIONS2;

typedef struct _D3DKMT_OUTPUTDUPLCREATIONFLAGS
{
    union
    {
        struct
        {
            UINT    CompositionUiCaptureOnly : 1;
            UINT    Reserved : 31;
        };
        UINT    Value;
    };
} D3DKMT_OUTPUTDUPLCREATIONFLAGS;

typedef struct _D3DKMT_OUTPUTDUPL_KEYEDMUTEX
{
    D3DKMT_PTR(HANDLE, hSharedSurfaceNt);
}D3DKMT_OUTPUTDUPL_KEYEDMUTEX;

#define OUTPUTDUPL_CREATE_MAX_KEYEDMUTXES 3
typedef struct _D3DKMT_CREATE_OUTPUTDUPL
{
    D3DKMT_HANDLE                   hAdapter;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
    UINT                            KeyedMutexCount;            // in : If zero then means is this the pre-create check
    UINT                            RequiredKeyedMutexCount;    // out: The number of keyed mutexs needed
    D3DKMT_OUTPUTDUPL_KEYEDMUTEX    KeyedMutexs[OUTPUTDUPL_CREATE_MAX_KEYEDMUTXES];
    D3DKMT_OUTPUTDUPLCREATIONFLAGS  Flags;
} D3DKMT_CREATE_OUTPUTDUPL;

typedef struct _D3DKMT_DESTROY_OUTPUTDUPL
{
    D3DKMT_HANDLE                   hAdapter;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
    BOOL                            bDestroyAllContexts;
} D3DKMT_DESTROY_OUTPUTDUPL;

typedef struct _D3DKMT_OUTPUTDUPL_POINTER_POSITION
{
    POINT   Position;
    BOOL    Visible;
} D3DKMT_OUTPUTDUPL_POINTER_POSITION;

typedef struct _D3DKMT_OUTPUTDUPL_FRAMEINFO
{
    D3DKMT_ALIGN64 LARGE_INTEGER        LastPresentTime;
    D3DKMT_ALIGN64 LARGE_INTEGER        LastMouseUpdateTime;
    UINT                                AccumulatedFrames;
    BOOL                                RectsCoalesced;
    BOOL                                ProtectedContentMaskedOut;
    D3DKMT_OUTPUTDUPL_POINTER_POSITION  PointerPosition;
    UINT                                TotalMetadataBufferSize;
    UINT                                PointerShapeBufferSize;
} D3DKMT_OUTPUTDUPL_FRAMEINFO;

typedef struct _D3DKMT_OUTPUTDUPL_GET_FRAMEINFO
{
    D3DKMT_HANDLE                   hAdapter;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
    D3DKMT_OUTPUTDUPL_FRAMEINFO     FrameInfo;
} D3DKMT_OUTPUTDUPL_GET_FRAMEINFO;

typedef enum _D3DKMT_OUTPUTDUPL_METADATATYPE
{
    D3DKMT_OUTPUTDUPL_METADATATYPE_DIRTY_RECTS = 0,
    D3DKMT_OUTPUTDUPL_METADATATYPE_MOVE_RECTS  = 1
} D3DKMT_OUTPUTDUPL_METADATATYPE;

typedef struct _D3DKMT_OUTPUTDUPL_METADATA
{
    D3DKMT_HANDLE                                                       hAdapter;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID                                      VidPnSourceId;
    D3DKMT_OUTPUTDUPL_METADATATYPE                                      Type;
    UINT                                                                BufferSizeSupplied;
    D3DKMT_PTR(_Field_size_bytes_part_(BufferSizeSupplied, BufferSizeRequired) PVOID, pBuffer);
    UINT                                                                BufferSizeRequired;
} D3DKMT_OUTPUTDUPL_METADATA;

typedef enum _D3DKMT_OUTDUPL_POINTER_SHAPE_TYPE
{
    D3DKMT_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME     = 0x00000001,
    D3DKMT_OUTDUPL_POINTER_SHAPE_TYPE_COLOR          = 0x00000002,
    D3DKMT_OUTDUPL_POINTER_SHAPE_TYPE_MASKED_COLOR   = 0x00000004
} D3DKMT_OUTDUPL_POINTER_SHAPE_TYPE;

typedef struct _D3DKMT_OUTDUPL_POINTER_SHAPE_INFO
{
    D3DKMT_OUTDUPL_POINTER_SHAPE_TYPE   Type;
    UINT                                Width;
    UINT                                Height;
    UINT                                Pitch;
    POINT                               HotSpot;
} D3DKMT_OUTDUPL_POINTER_SHAPE_INFO;

typedef struct _D3DKMT_OUTPUTDUPL_GET_POINTER_SHAPE_DATA
{
    D3DKMT_HANDLE                                                       hAdapter;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID                                      VidPnSourceId;
    UINT                                                                BufferSizeSupplied;
    D3DKMT_PTR(_Field_size_bytes_part_(BufferSizeSupplied, BufferSizeRequired) PVOID, pShapeBuffer);
    UINT                                                                BufferSizeRequired;
    D3DKMT_OUTDUPL_POINTER_SHAPE_INFO                                   ShapeInfo;
} D3DKMT_OUTPUTDUPL_GET_POINTER_SHAPE_DATA;

typedef struct _D3DKMT_OUTPUTDUPL_RELEASE_FRAME
{
    D3DKMT_HANDLE                   hAdapter;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
    UINT                            NextKeyMutexIdx;    // out : index of the next keyed mutex to use
} D3DKMT_OUTPUTDUPL_RELEASE_FRAME;


#define D3DKMT_MAX_OBJECTS_PER_HANDLE 3

#define D3DKMT_MAX_BUNDLE_OBJECTS_PER_HANDLE 16

#define D3DKMT_GDI_STYLE_HANDLE_DECORATION 0x2

typedef struct _D3DKMT_GETSHAREDRESOURCEADAPTERLUID
{
    D3DKMT_HANDLE hGlobalShare;    // in : Shared resource handle
    D3DKMT_PTR(HANDLE, hNtHandle); // in : Process's NT handle
    LUID AdapterLuid;              // out: adapter LUID
} D3DKMT_GETSHAREDRESOURCEADAPTERLUID;

typedef enum _D3DKMT_GPU_PREFERENCE_QUERY_STATE
{
    D3DKMT_GPU_PREFERENCE_STATE_UNINITIALIZED,
    D3DKMT_GPU_PREFERENCE_STATE_HIGH_PERFORMANCE,
    D3DKMT_GPU_PREFERENCE_STATE_MINIMUM_POWER,
    D3DKMT_GPU_PREFERENCE_STATE_UNSPECIFIED,
    D3DKMT_GPU_PREFERENCE_STATE_NOT_FOUND,
    D3DKMT_GPU_PREFERENCE_STATE_USER_SPECIFIED_GPU
} D3DKMT_GPU_PREFERENCE_QUERY_STATE;

typedef enum _D3DKMT_GPU_PREFERENCE_QUERY_TYPE
{
    D3DKMT_GPU_PREFERENCE_TYPE_IHV_DLIST,
    D3DKMT_GPU_PREFERENCE_TYPE_DX_DATABASE,
    D3DKMT_GPU_PREFERENCE_TYPE_USER_PREFERENCE
} D3DKMT_GPU_PREFERENCE_QUERY_TYPE;

typedef struct _D3DKMT_HYBRID_LIST
{
    D3DKMT_GPU_PREFERENCE_QUERY_STATE State;    // Gpu preference query state
    LUID AdapterLuid;                           // in,opt: Adapter luid to per-adapter DList state. Optional if QueryType == D3DKMT_GPU_PREFERENCE_TYPE_IHV_DLIST
    BOOL bUserPreferenceQuery;                  // Whether referring to user gpu preference, or per-adapter DList query
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)
    D3DKMT_GPU_PREFERENCE_QUERY_TYPE QueryType; // Replaced bUserPreferenceQuery, for referring to which D3DKMT_GPU_PREFERENCE_QUERY_TYPE
#endif
} D3DKMT_HYBRID_LIST;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
typedef struct
{
    DXGK_MIRACAST_CHUNK_INFO ChunkInfo;
    UINT                     PrivateDriverDataSize; // Size of private data
    BYTE                     PrivateDriverData[1];  // Private data buffer
} D3DKMT_MIRACAST_CHUNK_DATA;

typedef enum
{
    MiracastStopped = 0,
    MiracastStartPending = 1,
    MiracastStarted = 2,
    MiracastStopPending = 3,
} D3DKMT_MIRACAST_DISPLAY_DEVICE_STATE;

typedef enum
{
    D3DKMT_MIRACAST_DEVICE_STATUS_SUCCESS                   = 0,
    D3DKMT_MIRACAST_DEVICE_STATUS_SUCCESS_NO_MONITOR        = 1,
    D3DKMT_MIRACAST_DEVICE_STATUS_PENDING                   = 2,
    D3DKMT_MIRACAST_DEVICE_STATUS_UNKOWN_ERROR              = 0x80000001,
    D3DKMT_MIRACAST_DEVICE_STATUS_GPU_RESOURCE_IN_USE       = 0x80000002,
    D3DKMT_MIRACAST_DEVICE_STATUS_DEVICE_ERROR              = 0x80000003,
    D3DKMT_MIRACAST_DEVICE_STATUS_UNKOWN_PAIRING            = 0x80000004,
    D3DKMT_MIRACAST_DEVICE_STATUS_REMOTE_SESSION            = 0x80000005,
    D3DKMT_MIRACAST_DEVICE_STATUS_DEVICE_NOT_FOUND          = 0x80000006,
    D3DKMT_MIRACAST_DEVICE_STATUS_DEVICE_NOT_STARTED        = 0x80000007,
    D3DKMT_MIRACAST_DEVICE_STATUS_INVALID_PARAMETER         = 0x80000008,
    D3DKMT_MIRACAST_DEVICE_STATUS_INSUFFICIENT_BANDWIDTH    = 0x80000009,
    D3DKMT_MIRACAST_DEVICE_STATUS_INSUFFICIENT_MEMORY       = 0x8000000A,
    D3DKMT_MIRACAST_DEVICE_STATUS_CANCELLED                 = 0x8000000B,
} D3DKMT_MIRACAST_DEVICE_STATUS;

typedef struct _D3DKMT_MIRACAST_DISPLAY_DEVICE_STATUS
{
    //
    // Miracast display device state.
    //
    D3DKMT_MIRACAST_DISPLAY_DEVICE_STATE State;
} D3DKMT_MIRACAST_DISPLAY_DEVICE_STATUS, *PD3DKMT_MIRACAST_DISPLAY_DEVICE_STATUS;

typedef struct _D3DKMT_MIRACAST_DISPLAY_DEVICE_CAPS
{
    BOOLEAN HdcpSupported;
    ULONG DefaultControlPort;
    BOOLEAN UsesIhvSolution;
} D3DKMT_MIRACAST_DISPLAY_DEVICE_CAPS, *PD3DKMT_MIRACAST_DISPLAY_DEVICE_CAPS;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
typedef struct _D3DKMT_MIRACAST_DISPLAY_STOP_SESSIONS
{
    LUID AdapterLuid;
    D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId;
    UINT StopReason;
} D3DKMT_MIRACAST_DISPLAY_STOP_SESSIONS, *PD3DKMT_MIRACAST_DISPLAY_STOP_SESSIONS;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)


#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

typedef struct _D3DKMT_WAITFORSYNCHRONIZATIONOBJECTFROMCPU
{
    D3DKMT_HANDLE           hDevice;                        // in: Handle to the device.
    UINT                    ObjectCount;                    // in: Number of objects to wait on.

    D3DKMT_PTR(_Field_size_(ObjectCount)
    const D3DKMT_HANDLE*,   ObjectHandleArray);             // in: Handle to monitored fence synchronization objects to wait on.

    D3DKMT_PTR(_Field_size_(ObjectCount)
    const UINT64*,          FenceValueArray);               // in: Fence values to be waited on.

    D3DKMT_PTR(HANDLE,      hAsyncEvent);                   // in: Event to be signaled when the wait condition is satisfied.
                                                            // When set to NULL, the call will not return until the wait condition is satisfied.

    D3DDDI_WAITFORSYNCHRONIZATIONOBJECTFROMCPU_FLAGS Flags; // in: Flags that specify the wait mode.
} D3DKMT_WAITFORSYNCHRONIZATIONOBJECTFROMCPU;

typedef struct _D3DKMT_SIGNALSYNCHRONIZATIONOBJECTFROMCPU
{
    D3DKMT_HANDLE           hDevice;            // in: Handle to the device.
    UINT                    ObjectCount;        // in: Number of objects to signal.

    D3DKMT_PTR(_Field_size_(ObjectCount)
    const D3DKMT_HANDLE*,   ObjectHandleArray); // in: Handle to monitored fence synchronization objects to signal.

    D3DKMT_PTR(_Field_size_(ObjectCount)
    const UINT64*,          FenceValueArray);   // in: Fence values to be signaled.

    D3DDDICB_SIGNALFLAGS    Flags;              // in: Specifies signal behavior.
} D3DKMT_SIGNALSYNCHRONIZATIONOBJECTFROMCPU;

typedef struct _D3DKMT_WAITFORSYNCHRONIZATIONOBJECTFROMGPU
{
    D3DKMT_HANDLE           hContext;                   // in: Specify the context that should be waiting.
    UINT                    ObjectCount;                // in: Number of object to wait on.

    D3DKMT_PTR(_Field_size_(ObjectCount)
    const D3DKMT_HANDLE*,   ObjectHandleArray);         // in: Handles to synchronization objects to wait on.

    union
    {
        _Field_size_(ObjectCount)
        const UINT64*       MonitoredFenceValueArray;   // in: monitored fence values to be waited.

        D3DKMT_ALIGN64 UINT64 FenceValue;               // in: fence value to be waited.

        D3DKMT_ALIGN64 UINT64 Reserved[8];
    };
} D3DKMT_WAITFORSYNCHRONIZATIONOBJECTFROMGPU;

typedef struct _D3DKMT_SIGNALSYNCHRONIZATIONOBJECTFROMGPU
{
    D3DKMT_HANDLE           hContext;           // in: Identifies the context that the signal is being submitted to.
    UINT                    ObjectCount;        // in: Specifies the number of objects to signal.

    D3DKMT_PTR(_Field_size_(ObjectCount)
    const D3DKMT_HANDLE*,   ObjectHandleArray); // in: Specifies the objects to signal.

    union
    {
        _Field_size_(ObjectCount)
        const UINT64*   MonitoredFenceValueArray; // in: monitored fence values to be signaled

        D3DKMT_ALIGN64 UINT64 Reserved[8];
    };
} D3DKMT_SIGNALSYNCHRONIZATIONOBJECTFROMGPU;

typedef struct _D3DKMT_SIGNALSYNCHRONIZATIONOBJECTFROMGPU2
{
    UINT                    ObjectCount;            // in: Specifies the number of objects to signal.

    D3DKMT_PTR(_Field_size_(ObjectCount)
    const D3DKMT_HANDLE*,   ObjectHandleArray);     // in: Specifies the objects to signal.

    D3DDDICB_SIGNALFLAGS    Flags;                  // in: Specifies signal behavior.

    ULONG                   BroadcastContextCount;  // in: Specifies the number of contexts to broadcast this signal to.

    D3DKMT_PTR(_Field_size_(BroadcastContextCount)
    const D3DKMT_HANDLE*,   BroadcastContextArray); // in: Specifies context handles to broadcast to.

    union
    {
        D3DKMT_ALIGN64 UINT64 FenceValue;           // in: fence value to be signaled;

        HANDLE              CpuEventHandle;         // in: handle of a CPU event to be signaled if Flags.EnqueueCpuEvent flag is set.

        _Field_size_(ObjectCount)
        const UINT64*       MonitoredFenceValueArray; // in: monitored fence values to be signaled

        D3DKMT_ALIGN64 UINT64 Reserved[8];
    };
} D3DKMT_SIGNALSYNCHRONIZATIONOBJECTFROMGPU2;

typedef struct _D3DKMT_CREATEPAGINGQUEUE
{
    D3DKMT_HANDLE               hDevice;                        // in: Handle to the device.
    D3DDDI_PAGINGQUEUE_PRIORITY Priority;                       // in: scheduling priority relative to other paging queues on this device
    D3DKMT_HANDLE               hPagingQueue;                   // out: handle to the paging queue used to synchronize paging operations for this device.
    D3DKMT_HANDLE               hSyncObject;                    // out: handle to the monitored fence object used to synchronize paging operations for this paging queue.
    D3DKMT_PTR(VOID*,           FenceValueCPUVirtualAddress);   // out: Read-only mapping of the fence value for the CPU
    UINT                        PhysicalAdapterIndex;           // in: Physical adapter index (engine ordinal)
} D3DKMT_CREATEPAGINGQUEUE;

typedef struct _D3DKMT_EVICT
{
    D3DKMT_HANDLE               hDevice;            // in: Device that created the allocations
    UINT                        NumAllocations;     // in: number of allocation handles
    D3DKMT_PTR(CONST D3DKMT_HANDLE*, AllocationList); // in: an array of NumAllocations allocation handles
    D3DDDI_EVICT_FLAGS          Flags;              // in: eviction flags
    D3DKMT_ALIGN64 UINT64       NumBytesToTrim;     // out: This value indicates how much to trim in order to satisfy the new budget.
} D3DKMT_EVICT;

typedef struct _D3DKMT_LOCK2
{
    D3DKMT_HANDLE       hDevice;            // in: Handle to the device.
    D3DKMT_HANDLE       hAllocation;        // in: allocation to lock
    D3DDDICB_LOCK2FLAGS Flags;              // in: Bit field defined by D3DDDI_LOCK2FLAGS
    D3DKMT_PTR(PVOID,   pData);             // out: Virtual address of the locked allocation
} D3DKMT_LOCK2;

typedef struct _D3DKMT_UNLOCK2
{
    D3DKMT_HANDLE           hDevice;        // in: Handle to the device.
    D3DKMT_HANDLE           hAllocation;    // in: allocation to unlock
} D3DKMT_UNLOCK2;

typedef struct _D3DKMT_INVALIDATECACHE
{
    D3DKMT_HANDLE   hDevice;
    D3DKMT_HANDLE   hAllocation;
    D3DKMT_ALIGN64 D3DKMT_SIZE_T   Offset;
    D3DKMT_ALIGN64 D3DKMT_SIZE_T   Length;
} D3DKMT_INVALIDATECACHE;

typedef struct _D3DKMT_FREEGPUVIRTUALADDRESS
{
    D3DKMT_HANDLE           hAdapter;                               // in: Handle to an adapter.
    D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS  BaseAddress;                            // in: Start of a virtual address range in bytes
    D3DKMT_ALIGN64 D3DGPU_SIZE_T           Size;                                   // in: Size of the virtual address range in bytes
} D3DKMT_FREEGPUVIRTUALADDRESS;

typedef struct _D3DKMT_UPDATEGPUVIRTUALADDRESS
{
    D3DKMT_HANDLE                               hDevice;
    D3DKMT_HANDLE                               hContext;
    D3DKMT_HANDLE                               hFenceObject;
    UINT                                        NumOperations;
    D3DKMT_PTR(D3DDDI_UPDATEGPUVIRTUALADDRESS_OPERATION*, Operations);
    D3DKMT_ALIGN64 D3DKMT_SIZE_T                Reserved0;
    D3DKMT_ALIGN64 UINT64                       Reserved1;
    D3DKMT_ALIGN64 UINT64                       FenceValue;
    union
    {
       struct
       {
           UINT  DoNotWait  :  1;
           UINT  Reserved   : 31;
       };
       UINT Value;
    } Flags;
} D3DKMT_UPDATEGPUVIRTUALADDRESS;

typedef struct _D3DKMT_CREATECONTEXTVIRTUAL
{
    D3DKMT_HANDLE               hDevice;                        // in:
    UINT                        NodeOrdinal;                    // in:
    UINT                        EngineAffinity;                 // in:
    D3DDDI_CREATECONTEXTFLAGS   Flags;                          // in:
    D3DKMT_PTR(VOID*,           pPrivateDriverData);            // in:
    UINT                        PrivateDriverDataSize;          // in:
    D3DKMT_CLIENTHINT           ClientHint;                     // in:  Hints which client is creating the context
    D3DKMT_HANDLE               hContext;                       // out:
} D3DKMT_CREATECONTEXTVIRTUAL;

typedef struct _D3DKMT_SUBMITCOMMANDFLAGS
{
    UINT    NullRendering           :  1;  // 0x00000001
    UINT    PresentRedirected       :  1;  // 0x00000002
    UINT    NoKmdAccess             :  1;  // 0x00000004
    UINT    Reserved                : 29;  // 0xFFFFFFF8
} D3DKMT_SUBMITCOMMANDFLAGS;

typedef struct _D3DKMT_SUBMITCOMMAND
{
    D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS Commands;
    UINT                        CommandLength;
    D3DKMT_SUBMITCOMMANDFLAGS   Flags;
    D3DKMT_ALIGN64 ULONGLONG    PresentHistoryToken;                            // in: Present history token for redirected present calls
    UINT                        BroadcastContextCount;
    D3DKMT_HANDLE               BroadcastContext[D3DDDI_MAX_BROADCAST_CONTEXT];
    D3DKMT_PTR(VOID*,           pPrivateDriverData);
    UINT                        PrivateDriverDataSize;
    UINT                        NumPrimaries;
    D3DKMT_HANDLE               WrittenPrimaries[D3DDDI_MAX_WRITTEN_PRIMARIES];
    UINT                        NumHistoryBuffers;
    D3DKMT_PTR(D3DKMT_HANDLE*,  HistoryBufferArray);
} D3DKMT_SUBMITCOMMAND;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

typedef struct _D3DKMT_SUBMITCOMMANDTOHWQUEUE
{
    D3DKMT_HANDLE               hHwQueue;               // in: Context queue to submit the command to.

    D3DKMT_ALIGN64 UINT64       HwQueueProgressFenceId; // in: Hardware queue progress fence value that will be signaled once the command is finished.

    D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS CommandBuffer;// in: GPU VA of the command buffer to be executed on the GPU.
    UINT                        CommandLength;          // in: Length in bytes of the command buffer.

    UINT                        PrivateDriverDataSize;  // in: Size of private driver data in bytes.

    D3DKMT_PTR(_Field_size_bytes_(PrivateDriverDataSize)
    VOID*,                      pPrivateDriverData);    // in: Pointer to the private driver data.

    UINT                        NumPrimaries;           // in: The number of primaries written by this command buffer.

    D3DKMT_PTR(_Field_size_    (NumPrimaries)
    D3DKMT_HANDLE CONST*,       WrittenPrimaries);      // in: The array of primaries written by this command buffer.
} D3DKMT_SUBMITCOMMANDTOHWQUEUE;

typedef struct _D3DKMT_SUBMITWAITFORSYNCOBJECTSTOHWQUEUE
{
    D3DKMT_HANDLE           hHwQueue;               // in: Context queue to submit the command to.

    UINT                    ObjectCount;            // in: Number of objects to wait on.

    D3DKMT_PTR(_Field_size_(ObjectCount)
    const D3DKMT_HANDLE*,   ObjectHandleArray);     // in: Handles to monitored fence synchronization objects to wait on.

    D3DKMT_PTR(_Field_size_(ObjectCount)
    const UINT64*,          FenceValueArray);       // in: monitored fence values to be waited.
} D3DKMT_SUBMITWAITFORSYNCOBJECTSTOHWQUEUE;

typedef struct _D3DKMT_SUBMITSIGNALSYNCOBJECTSTOHWQUEUE
{
    D3DDDICB_SIGNALFLAGS    Flags;                  // in: Specifies signal behavior.

    ULONG                   BroadcastHwQueueCount;  // in: Specifies the number of hardware queues to broadcast this signal to.

    D3DKMT_PTR(_Field_size_(BroadcastHwQueueCount)
    const D3DKMT_HANDLE*,   BroadcastHwQueueArray); // in: Specifies hardware queue handles to broadcast to.

    UINT                    ObjectCount;            // in: Number of objects to signal.

    D3DKMT_PTR(_Field_size_(ObjectCount)
    const D3DKMT_HANDLE*,   ObjectHandleArray);     // in: Handles to monitored fence synchronization objects to signal.

    D3DKMT_PTR(_Field_size_(ObjectCount)
    const UINT64*,          FenceValueArray);       // in: monitored fence values to signal.
} D3DKMT_SUBMITSIGNALSYNCOBJECTSTOHWQUEUE;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

typedef struct _D3DKMT_QUERYVIDEOMEMORYINFO
{
    D3DKMT_PTR(HANDLE,          hProcess);                  // in,opt: A handle to a process. If NULL, the current process is used.
                                                            //         The process handle must be opened with PROCESS_QUERY_INFORMATION privileges
    D3DKMT_HANDLE               hAdapter;                   // in : The adapter to query for this process
    D3DKMT_MEMORY_SEGMENT_GROUP MemorySegmentGroup;         // in : The memory segment group to query.
    D3DKMT_ALIGN64 UINT64       Budget;                     // out: Total memory the application may use
    D3DKMT_ALIGN64 UINT64       CurrentUsage;               // out: Current memory usage of the device
    D3DKMT_ALIGN64 UINT64       CurrentReservation;         // out: Current reservation of the device
    D3DKMT_ALIGN64 UINT64       AvailableForReservation;    // out: Total that the device may reserve
    UINT                        PhysicalAdapterIndex;       // in : Zero based physical adapter index in the LDA configuration.
} D3DKMT_QUERYVIDEOMEMORYINFO;

typedef struct _D3DKMT_CHANGEVIDEOMMEMORYRESERVATION
{
    D3DKMT_PTR(HANDLE,          hProcess);                  // in,opt: A handle to a process. If NULL, the current process is used.
                                                            //         The process handle must be opened with PROCESS_SET_INFORMATION privileges
    D3DKMT_HANDLE               hAdapter;                   // in : The adapter to change reservation for.
    D3DKMT_MEMORY_SEGMENT_GROUP MemorySegmentGroup;         // in : The memory segment group to change reservation for.
    D3DKMT_ALIGN64 UINT64       Reservation;                // in : Desired reservation in the range between 0 and AvailableForReservation returned by QueryVideoMemoryInfo.
    UINT                        PhysicalAdapterIndex;       // in : Zero based physical adapter index in the LDA configuration.
} D3DKMT_CHANGEVIDEOMEMORYRESERVATION;

typedef struct _D3DKMT_SETSTABLEPOWERSTATE
{
    D3DKMT_HANDLE   hAdapter;   // in: The adapter to enable or disable stable power for
    BOOL            Enabled;    // in: Whether or not stable power is being requested on or off.
} D3DKMT_SETSTABLEPOWERSTATE;

// Used by Linux ioctl
typedef struct _D3DKMT_SHAREOBJECTS {
    UINT                    ObjectCount;                // in
    D3DKMT_PTR(_Field_size_(ObjectCount)
    CONST D3DKMT_HANDLE*,   ObjectHandleArray);         // in
    D3DKMT_PTR(PVOID,       pObjectAttributes);         // in
    DWORD                   DesiredAccess;              // in
    D3DKMT_PTR(HANDLE*,     pSharedNtHandle);           // out
} D3DKMT_SHAREOBJECTS;

typedef struct _D3DKMT_SHAREOBJECTWITHHOST
{
    D3DKMT_HANDLE           hDevice;                // in
    D3DKMT_HANDLE           hObject;                // in
    D3DKMT_ALIGN64 UINT64   Reserved;               // in Must be zero. Reserved for future use
    D3DKMT_ALIGN64 UINT64   hVailProcessNtHandle;   // out
} D3DKMT_SHAREOBJECTWITHHOST;

//
// This API is used to support sync_file in Android.
// A sync_file is a wrapper around the given monitored fence and the fence value.
// When a sync_file is created, a wait for sync object on CPU is issued
// and a file descriptor (FD) it returned to the app. The app can wait on the FD,
// which will be unblocked when the sync object with this fence value is signaled.
//
typedef struct _D3DKMT_CREATESYNCFILE
{
	D3DKMT_HANDLE           hDevice;                // in:  Device owner of the monitored fence.
	D3DKMT_HANDLE           hMonitoredFence;        // in:  Monitored fence object
	D3DKMT_ALIGN64 UINT64   FenceValue;             // in:  Fence value to wait for
	D3DKMT_ALIGN64 UINT64   hSyncFile;	            // out: File descriptor on Android or a NT handle on Windows (when implemented)
} D3DKMT_CREATESYNCFILE;

typedef struct _D3DKMT_TRIMNOTIFICATION
{
    D3DKMT_PTR(VOID*,                  Context);        // In: context at Register
    D3DDDI_TRIMRESIDENCYSET_FLAGS      Flags;           // In: trim flags
    D3DKMT_ALIGN64 UINT64              NumBytesToTrim;  // In: When TrimToBudget flag is set, this value indicates how much VidMm
                                                        // requests the app to trim to fit in the new budget.
} D3DKMT_TRIMNOTIFICATION;

typedef VOID (APIENTRY *PFND3DKMT_TRIMNOTIFICATIONCALLBACK)(_Inout_ D3DKMT_TRIMNOTIFICATION*);

typedef struct _D3DKMT_REGISTERTRIMNOTIFICATION
{
    LUID                               AdapterLuid;
    D3DKMT_HANDLE                      hDevice;
    PFND3DKMT_TRIMNOTIFICATIONCALLBACK Callback;
    D3DKMT_PTR(VOID*,                  Context); // In: callback context
    D3DKMT_PTR(VOID*,                  Handle);  // Out: for Unregister
} D3DKMT_REGISTERTRIMNOTIFICATION;

typedef struct _D3DKMT_UNREGISTERTRIMNOTIFICATION
{
    D3DKMT_PTR(VOID*,                              Handle);   // In: Handle returned from RegisterTrimNotification,
                                                              // or NULL to unregister all Callback instances.
    D3DKMT_PTR(PFND3DKMT_TRIMNOTIFICATIONCALLBACK, Callback); // In: When Handle is NULL, this parameter specifies that all registered instances of Callback
                                                              // should be unregistered. This unregistration method should only be used
                                                              // in DLL unload scenarios when the DLL being unloaded cannot guarantee that
                                                              // all trim callbacks are unregistered through their handles.
} D3DKMT_UNREGISTERTRIMNOTIFICATION;

typedef struct _D3DKMT_BUDGETCHANGENOTIFICATION
{
    D3DKMT_PTR(VOID*,     Context);   // In: context at Register
    D3DKMT_ALIGN64 UINT64 Budget;     // In: new budget
} D3DKMT_BUDGETCHANGENOTIFICATION;

typedef VOID (APIENTRY *PFND3DKMT_BUDGETCHANGENOTIFICATIONCALLBACK)(_In_ D3DKMT_BUDGETCHANGENOTIFICATION*);

typedef struct _D3DKMT_REGISTERBUDGETCHANGENOTIFICATION
{
    D3DKMT_HANDLE                                  hDevice;
    D3DKMT_PTR(PFND3DKMT_BUDGETCHANGENOTIFICATIONCALLBACK, Callback);
    D3DKMT_PTR(VOID*,                              Context);  // In: callback context
    D3DKMT_PTR(VOID*,                              Handle);   // Out: for Unregister
} D3DKMT_REGISTERBUDGETCHANGENOTIFICATION;

typedef struct _D3DKMT_UNREGISTERBUDGETCHANGENOTIFICATION
{
    D3DKMT_PTR(VOID*,                              Handle);   // In: from register
} D3DKMT_UNREGISTERBUDGETCHANGENOTIFICATION;

typedef struct _D3DKMT_QUERYVIDPNEXCLUSIVEOWNERSHIP
{
    D3DKMT_PTR(HANDLE,              hProcess);      // In: Process handle
    D3DKMT_PTR(HWND,                hWindow);       // In: Window handle
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;  // Out: VidPn source ID
    LUID                            AdapterLuid;    // Out: Adapter LUID
    D3DKMT_VIDPNSOURCEOWNER_TYPE    OwnerType;      // Out: Owner Type
} D3DKMT_QUERYVIDPNEXCLUSIVEOWNERSHIP;


typedef enum _D3DKMT_DEVICE_ERROR_REASON {
    D3DKMT_DEVICE_ERROR_REASON_GENERIC           = 0x80000000,
    D3DKMT_DEVICE_ERROR_REASON_DRIVER_ERROR      = 0x80000006,
} D3DKMT_DEVICE_ERROR_REASON;

typedef struct _D3DKMT_MARKDEVICEASERROR
{
    D3DKMT_HANDLE              hDevice; // in: Device handle
    D3DKMT_DEVICE_ERROR_REASON Reason;  // in: Status code
} D3DKMT_MARKDEVICEASERROR;

typedef struct _D3DKMT_FLUSHHEAPTRANSITIONS
{
    D3DKMT_HANDLE              hAdapter;
} D3DKMT_FLUSHHEAPTRANSITIONS;

typedef struct _D3DKMT_QUERYPROCESSOFFERINFO
{
    _In_ ULONG cbSize;
    D3DKMT_PTR(_In_ HANDLE, hProcess);
    _Out_ D3DKMT_ALIGN64 UINT64 DecommitUniqueness;
    _Out_ D3DKMT_ALIGN64 UINT64 DecommittableBytes;
} D3DKMT_QUERYPROCESSOFFERINFO;

typedef union _D3DKMT_TRIMPROCESSCOMMITMENT_FLAGS
{
    struct
    {
        UINT Lazy           :  1;
        UINT OnlyRepurposed :  1;
        UINT Reserved       : 30;
    };
    UINT Value;
} D3DKMT_TRIMPROCESSCOMMITMENT_FLAGS;

typedef struct _D3DKMT_TRIMPROCESSCOMMITMENT
{
    _In_ ULONG cbSize;
    D3DKMT_PTR(_In_ HANDLE, hProcess);
    _In_ D3DKMT_TRIMPROCESSCOMMITMENT_FLAGS Flags;
    _In_ D3DKMT_ALIGN64 UINT64 DecommitRequested;
    _Out_ D3DKMT_ALIGN64 UINT64 NumBytesDecommitted;
} D3DKMT_TRIMPROCESSCOMMITMENT;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

typedef struct _D3DKMT_CREATEHWCONTEXT
{
    D3DKMT_HANDLE               hDevice;                // in:  Handle to the device owning this context.
    UINT                        NodeOrdinal;            // in:  Identifier for the node targetted by this context.
    UINT                        EngineAffinity;         // in:  Engine affinity within the specified node.
    D3DDDI_CREATEHWCONTEXTFLAGS Flags;                  // in:  Context creation flags.
    UINT                        PrivateDriverDataSize;  // in:  Size of private driver data
    D3DKMT_PTR(_Inout_
    _Field_size_bytes_         (PrivateDriverDataSize)
    VOID*,                      pPrivateDriverData);    // in/out:  Private driver data
    D3DKMT_HANDLE               hHwContext;             // out: Handle of the created context.
} D3DKMT_CREATEHWCONTEXT;

typedef struct _D3DKMT_DESTROYHWCONTEXT
{
    D3DKMT_HANDLE               hHwContext;             // in:  Identifies the context being destroyed.
} D3DKMT_DESTROYHWCONTEXT;

typedef struct _D3DKMT_CREATEHWQUEUE
{
    D3DKMT_HANDLE               hHwContext;                             // in:  Handle to the hardware context the queue is associated with.
    D3DDDI_CREATEHWQUEUEFLAGS   Flags;                                  // in:  Hardware queue creation flags.
    UINT                        PrivateDriverDataSize;                  // in:  Size of private driver data
    D3DKMT_PTR(_Inout_
    _Field_size_bytes_         (PrivateDriverDataSize)
    VOID*,                      pPrivateDriverData);                    // in/out:  Private driver data
    D3DKMT_HANDLE               hHwQueue;                               // out: handle to the hardware queue object to submit work to.
    D3DKMT_HANDLE               hHwQueueProgressFence;                  // out: handle to the monitored fence object used to monitor the queue progress.
    D3DKMT_PTR(VOID*,           HwQueueProgressFenceCPUVirtualAddress); // out: Read-only mapping of the queue progress fence value for the CPU
    D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS HwQueueProgressFenceGPUVirtualAddress;  // out: Read/write mapping of the queue progress fence value for the GPU
} D3DKMT_CREATEHWQUEUE;

typedef struct _D3DKMT_DESTROYHWQUEUE
{
    D3DKMT_HANDLE               hHwQueue;   // in: handle to the hardware queue to be destroyed.
} D3DKMT_DESTROYHWQUEUE;

typedef struct _D3DKMT_GETALLOCATIONPRIORITY
{
    D3DKMT_HANDLE           hDevice;            // in: Indentifies the device
    D3DKMT_HANDLE           hResource;          // in: Specify the resource to get priority of.
    D3DKMT_PTR(CONST D3DKMT_HANDLE*, phAllocationList); // in: pointer to an array allocation to get priorities of.
    UINT                    AllocationCount;    // in: Number of allocations in phAllocationList
    D3DKMT_PTR(UINT*,       pPriorities);       // out: Priority for each of the allocation in the array.
} D3DKMT_GETALLOCATIONPRIORITY;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)



#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
typedef union _D3DKMT_SETFSEBLOCKFLAGS
{
    struct
    {
        UINT Block : 1;
        UINT Reserved : 31;
    };
    UINT Value;
} D3DKMT_SETFSEBLOCKFLAGS;

typedef struct _D3DKMT_SETFSEBLOCK
{
    LUID                            AdapterLuid;
    D3DKMT_HANDLE                   hAdapter;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
    D3DKMT_SETFSEBLOCKFLAGS         Flags;
} D3DKMT_SETFSEBLOCK;

typedef union _D3DKMT_QUERYFSEFLAGS
{
    struct
    {
        UINT Blocked : 1;
        UINT Reserved : 31;
    };
    UINT Value;
} D3DKMT_QUERYFSEBLOCKFLAGS;

typedef struct _D3DKMT_QUERYFSEBLOCK
{
    LUID                            AdapterLuid;
    D3DKMT_HANDLE                   hAdapter;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
    D3DKMT_QUERYFSEBLOCKFLAGS       Flags;
} D3DKMT_QUERYFSEBLOCK;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)

typedef struct _D3DKMT_CREATEPROTECTEDSESSION
{
    D3DKMT_HANDLE                                           hDevice;                // in: device handle
    D3DKMT_HANDLE                                           hSyncObject;            // in: monitored fence handle associated to this session (kernel handle)
    D3DKMT_PTR(_Field_size_bytes_(PrivateDriverDataSize)
    CONST VOID*,                                            pPrivateDriverData);    // in: Private driver data
    UINT                                                    PrivateDriverDataSize;  // in: Size of private runtime data
    D3DKMT_PTR(_Field_size_bytes_(PrivateRuntimeDataSize)
    CONST VOID*,                                            pPrivateRuntimeData);   // in: Private runtime data
    UINT                                                    PrivateRuntimeDataSize; // in: Size of private runtime data

    D3DKMT_HANDLE                                           hHandle;                // out: protected session handle (kernel handle)

} D3DKMT_CREATEPROTECTEDSESSION;

typedef struct _D3DKMT_DESTROYPROTECTEDSESSION
{
    D3DKMT_HANDLE                                           hHandle; // in: protected session handle (kernel handle)

} D3DKMT_DESTROYPROTECTEDSESSION;

typedef enum _D3DKMT_PROTECTED_SESSION_STATUS
{
    D3DKMT_PROTECTED_SESSION_STATUS_OK         = 0,
    D3DKMT_PROTECTED_SESSION_STATUS_INVALID    = 1,
} D3DKMT_PROTECTED_SESSION_STATUS;

typedef struct _D3DKMT_QUERYPROTECTEDSESSIONSTATUS
{
    D3DKMT_HANDLE                                           hHandle; // in: protected session handle (kernel handle)
    D3DKMT_PROTECTED_SESSION_STATUS                         Status;  // out: protected session status

} D3DKMT_QUERYPROTECTEDSESSIONSTATUS;

typedef struct _D3DKMT_QUERYPROTECTEDSESSIONINFOFROMNTHANDLE
{
    D3DKMT_PTR(HANDLE,                                      hNtHandle);             // in: protected session handle (NT handle)
    D3DKMT_PTR(_Field_size_bytes_(PrivateDriverDataSize)
    CONST VOID*,                                            pPrivateDriverData);    // in: Private driver data
    UINT                                                    PrivateDriverDataSize;  // in/out: Size of private runtime data
    D3DKMT_PTR(_Field_size_bytes_(PrivateRuntimeDataSize)
    CONST VOID*,                                            pPrivateRuntimeData);   // in: Private runtime data
    UINT                                                    PrivateRuntimeDataSize; // in/out: Size of private runtime data

} D3DKMT_QUERYPROTECTEDSESSIONINFOFROMNTHANDLE;

typedef struct _D3DKMT_OPENPROTECTEDSESSIONFROMNTHANDLE
{
    D3DKMT_PTR(HANDLE,                                      hNtHandle);// in: protected session handle (NT handle)
    D3DKMT_HANDLE                                           hHandle;   // out: protected session handle (kernel handle)

} D3DKMT_OPENPROTECTEDSESSIONFROMNTHANDLE;


typedef struct _D3DKMT_GETPROCESSDEVICEREMOVALSUPPORT
{
    D3DKMT_PTR(HANDLE, hProcess);    // in: Process handle
    LUID    AdapterLuid; // in: Luid of Adapter that is potentially being detached
    BOOLEAN Support;     // out: Whether or not the process using the adapter can recover from graphics device removal

} D3DKMT_GETPROCESSDEVICEREMOVALSUPPORT;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)


// All tracked workload functionality is accessible just by the D3D11 and D3D12 runtimes
typedef enum _D3DKMT_TRACKEDWORKLOADPOLICY
{
       D3DKMT_TRACKEDWORKLOADPOLICY_NORMAL           = 0,
       D3DKMT_TRACKEDWORKLOADPOLICY_ENERGY_EFFICIENT = 1,
       D3DKMT_TRACKEDWORKLOADPOLICY_HIGH_SPEED       = 2
} D3DKMT_TRACKEDWORKLOADPOLICY;

typedef enum _D3DKMT_TRACKEDWORKLOADDEADLINETYPE
{
       D3DKMT_TRACKEDWORKLOADDEADLINETYPE_ABSOLUTE = 0,
       D3DKMT_TRACKEDWORKLOADDEADLINETYPE_VBLANK = 1,
} D3DKMT_TRACKEDWORKLOADDEADLINETYPE;

typedef struct _D3DKMT_TRACKEDWORKLOADDEADLINE {
    union {
        D3DKMT_ALIGN64 UINT64 VBlankOffsetHundredsNS;
        D3DKMT_ALIGN64 UINT64 AbsoluteQPC;
    };
} D3DKMT_TRACKEDWORKLOADDEADLINE;

typedef struct _D3DKMT_TRACKEDWORKLOADFLAGS
{
    union
    {
        struct
        {
            UINT Periodic    : 1;                   // 0x00000001 - workload instances occur at a periodic rate
            UINT SimilarLoad : 1;                   // 0x00000002 - workload instances have a similar load
            UINT Reserved    : 30;
        };
        UINT Value;
    };
} D3DKMT_TRACKEDWORKLOADFLAGS;

#define D3DKMT_MAX_TRACKED_WORKLOAD_INSTANCE_PAIRS 32

typedef struct _D3DKMT_CREATETRACKEDWORKLOAD
{
    ULONG                              cbSize;                   // in: size of structure for versioning
    ULONG                              ContextCount;             // in: Specifies the number of contexts to create the workload
    D3DKMT_PTR(_Field_size_(ContextCount)
    const D3DKMT_HANDLE*,              ContextArray);            // in: Specifies context handles in which to create the workload
    D3DKMT_TRACKEDWORKLOADDEADLINETYPE DeadlineType;             // in: Specifies the deadline type of the tracked workload
    UINT32                             VidPnTargetId;            // in: Specifies the target ID. Needed for VBLANK DEADLINETYPE
    D3DKMT_TRACKEDWORKLOADFLAGS        Flags;                    // in: Flags to create the workload with
    D3DKMT_TRACKEDWORKLOADPOLICY       Policy;                   // in: Which policy to use
    UINT                               MaxInstances;             // in: maximum number of instances this workload can have
    UINT                               MaxInstancePairs;         // in: maximum number of instance pairs this workload can have (includes suspend/resume)
    D3DKMT_HANDLE                      hResourceQueryTimestamps; // in: buffer which will contain the resolved query timestamps for the tracked workloads
    D3DKMT_HANDLE                      hTrackedWorkload;         // out: the tracked workload handle
} D3DKMT_CREATETRACKEDWORKLOAD;

typedef struct _D3DKMT_DESTROYTRACKEDWORKLOAD
{
    ULONG                        cbSize;                   // in: size of structure for versioning
    D3DKMT_HANDLE                hTrackedWorkload;         // in: tracked workload handle
} D3DKMT_DESTROYTRACKEDWORKLOAD;

typedef struct _D3DKMT_UPDATETRACKEDWORKLOAD
{
    ULONG                           cbSize;                   // in: size of structure for versioning
    D3DKMT_HANDLE                   hTrackedWorkload;         // in: tracked workload handle
    D3DKMT_TRACKEDWORKLOADDEADLINE  FinishDeadline;           // in: specifies the deadline by which this workload should be finished
    UINT                            BeginTrackedWorkloadIndex;// in: slot for the timestamp for the start of this workload pair (index in buffer pointed to by hResourceQueryTimestamps)
    UINT                            EndTrackedWorkloadIndex;  // in: slot for the timestamp for the end of this workload pair (index in buffer pointed to by hResourceQueryTimestamps)
    BOOL                            Resume;                   // in: TRUE if the start of this workload pair is a Resume instead of a Begin
    BOOL                            Suspend;                  // in: TRUE if the end of this workload pair is a Suspend instead of an End
    D3DKMT_ALIGN64 UINT64           PairID;                   // in: identifier for the Begin/End tracked workload pair (should include any suspend/resume in the pair)
    D3DKMT_ALIGN64 UINT64           FenceSubmissionValue;     // in: fence value for the submission of this workload
    D3DKMT_ALIGN64 UINT64           FenceCompletedValue;      // in: fence value for the completed workloads
    D3DKMT_ALIGN64 UINT64           GPUTimestampFrequency;    // in: GPU timestamp frequency for resolving query timestamps
    D3DKMT_ALIGN64 UINT64           GPUCalibrationTimestamp;  // in: value of the GPU calibration timestamp counter
    D3DKMT_ALIGN64 UINT64           CPUCalibrationTimestamp;  // in: value of the CPU calibration timestamp counter
    D3DKMT_ALIGN64 UINT64           TimestampArray[D3DKMT_MAX_TRACKED_WORKLOAD_INSTANCE_PAIRS * 2];           // in: specifies the already read timestamp data (D3D11 only)
    BOOL                            TimestampArrayProcessed;  // out: TRUE if the timestamp array entries were processed (D3D11 only)
} D3DKMT_UPDATETRACKEDWORKLOAD;

typedef struct _D3DKMT_GETAVAILABLETRACKEDWORKLOADINDEX
{
    ULONG                        cbSize;                        // in: size of structure for versioning
    D3DKMT_HANDLE                hTrackedWorkload;              // in: tracked workload handle
    D3DKMT_ALIGN64 UINT64        FenceCompletedValue;           // in: fence value for the completed workloads
    D3DKMT_ALIGN64 UINT64        TimestampArray[D3DKMT_MAX_TRACKED_WORKLOAD_INSTANCE_PAIRS * 2];           // in: specifies the already read timestamp data (D3D11 only)
    UINT                         AvailableTrackedWorkloadIndex; // out: first available tracked workload slot
    BOOL                         TimestampArrayProcessed;       // out: TRUE if the timestamp array entries were processed (D3D11 only)
} D3DKMT_GETAVAILABLETRACKEDWORKLOADINDEX;

typedef struct _D3DKMT_TRACKEDWORKLOADSTATEFLAGS
{
    union
    {
        struct
        {
            UINT Saturated : 1;                      // 0x00000001 - in the current state of execution, tracked workload cannot meet its deadline.
            UINT NotEnoughSamples: 1;                // 0x00000002 - we don't have enough samples to produce stats yet
            UINT Reserved  : 30;
        };
        UINT Value;
    };
} D3DKMT_TRACKEDWORKLOADSTATEFLAGS;

typedef struct _D3DKMT_TRACKEDWORKLOAD_STATISTICS
{
    D3DKMT_ALIGN64 INT64 Mean;
    D3DKMT_ALIGN64 INT64 Minimum;
    D3DKMT_ALIGN64 INT64 Maximum;
    D3DKMT_ALIGN64 INT64 Variance;
    D3DKMT_ALIGN64 UINT64 Count;
} D3DKMT_TRACKEDWORKLOAD_STATISTICS;

typedef struct _D3DKMT_GETTRACKEDWORKLOADSTATISTICS
{
    ULONG                              cbSize;                       // in: size of structure for versioning
    D3DKMT_HANDLE                      hTrackedWorkload;             // in: tracked workload handle
    D3DKMT_ALIGN64 UINT64              FenceCompletedValue;          // in: fence value for the completed workloads
    D3DKMT_ALIGN64 UINT64              TimestampArray[D3DKMT_MAX_TRACKED_WORKLOAD_INSTANCE_PAIRS * 2];  // in: specifies the already read timestamp data (D3D11 only)
    BOOL                               TimestampArrayProcessed;      // out: TRUE if the timestamp array entries were processed (D3D11 only)
    D3DKMT_TRACKEDWORKLOAD_STATISTICS  DeadlineOffsetHundredsNS;     // out: statistics for the offset of the deadline achieved of the tracked workload in hundreds of nanosecs
    D3DKMT_ALIGN64 UINT64              MissedDeadlines;              // out: count of missed deadlines
    D3DKMT_TRACKEDWORKLOADSTATEFLAGS   Flags;                        // out: current state flags
} D3DKMT_GETTRACKEDWORKLOADSTATISTICS;

typedef struct _D3DKMT_RESETTRACKEDWORKLOADSTATISTICS
{
    ULONG                            cbSize;              // in: size of structure for versioning
    D3DKMT_HANDLE                    hTrackedWorkload;    // in: tracked workload handle
} D3DKMT_RESETTRACKEDWORKLOADSTATISTICS;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_7)


#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_7


typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CREATEALLOCATION)(_Inout_ D3DKMT_CREATEALLOCATION*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CREATEALLOCATION2)(_Inout_ D3DKMT_CREATEALLOCATION*); // _ADVSCH_
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_QUERYRESOURCEINFO)(_Inout_ D3DKMT_QUERYRESOURCEINFO*);
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_QUERYRESOURCEINFOFROMNTHANDLE)(_Inout_ D3DKMT_QUERYRESOURCEINFOFROMNTHANDLE*);

typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SHAREOBJECTS)(
    _In_range_(1, D3DKMT_MAX_OBJECTS_PER_HANDLE) UINT   cObjects,
    _In_reads_(cObjects) CONST D3DKMT_HANDLE *          hObjects,
    _In_ POBJECT_ATTRIBUTES                             pObjectAttributes,
    _In_ DWORD                                          dwDesiredAccess,
    _Out_ HANDLE *                                      phSharedNtHandle
    );
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OPENNTHANDLEFROMNAME)(_Inout_ D3DKMT_OPENNTHANDLEFROMNAME*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OPENRESOURCEFROMNTHANDLE)(_Inout_ D3DKMT_OPENRESOURCEFROMNTHANDLE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OPENSYNCOBJECTFROMNTHANDLE)(_Inout_ D3DKMT_OPENSYNCOBJECTFROMNTHANDLE*);
#endif
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OPENRESOURCE)(_Inout_ D3DKMT_OPENRESOURCE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OPENRESOURCE2)(_Inout_ D3DKMT_OPENRESOURCE*); // _ADVSCH_
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_DESTROYALLOCATION)(_In_ CONST D3DKMT_DESTROYALLOCATION*);

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_DESTROYALLOCATION2)(_In_ CONST D3DKMT_DESTROYALLOCATION2*);
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_0

typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SETALLOCATIONPRIORITY)(_In_ CONST D3DKMT_SETALLOCATIONPRIORITY*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_QUERYALLOCATIONRESIDENCY)(_In_ CONST D3DKMT_QUERYALLOCATIONRESIDENCY*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CREATEDEVICE)(_Inout_ D3DKMT_CREATEDEVICE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_DESTROYDEVICE)(_In_ CONST D3DKMT_DESTROYDEVICE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CREATECONTEXT)(_Inout_ D3DKMT_CREATECONTEXT*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_DESTROYCONTEXT)(_In_ CONST D3DKMT_DESTROYCONTEXT*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CREATESYNCHRONIZATIONOBJECT)(_Inout_ D3DKMT_CREATESYNCHRONIZATIONOBJECT*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CREATESYNCHRONIZATIONOBJECT2)(_Inout_ D3DKMT_CREATESYNCHRONIZATIONOBJECT2*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OPENSYNCHRONIZATIONOBJECT)(_Inout_ D3DKMT_OPENSYNCHRONIZATIONOBJECT*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_DESTROYSYNCHRONIZATIONOBJECT)(_In_ CONST D3DKMT_DESTROYSYNCHRONIZATIONOBJECT*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_WAITFORSYNCHRONIZATIONOBJECT)(_In_ CONST D3DKMT_WAITFORSYNCHRONIZATIONOBJECT*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_WAITFORSYNCHRONIZATIONOBJECT2)(_In_ CONST D3DKMT_WAITFORSYNCHRONIZATIONOBJECT2*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SIGNALSYNCHRONIZATIONOBJECT)(_In_ CONST D3DKMT_SIGNALSYNCHRONIZATIONOBJECT*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SIGNALSYNCHRONIZATIONOBJECT2)(_In_ CONST D3DKMT_SIGNALSYNCHRONIZATIONOBJECT2*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_LOCK)(_Inout_ D3DKMT_LOCK*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_UNLOCK)(_In_ CONST D3DKMT_UNLOCK*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_GETDISPLAYMODELIST)(_Inout_ D3DKMT_GETDISPLAYMODELIST*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SETDISPLAYMODE)(_Inout_ CONST D3DKMT_SETDISPLAYMODE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_GETMULTISAMPLEMETHODLIST)(_Inout_ D3DKMT_GETMULTISAMPLEMETHODLIST*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_PRESENT)(_Inout_ D3DKMT_PRESENT*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_RENDER)(_Inout_ D3DKMT_RENDER*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_GETRUNTIMEDATA)(_Inout_ CONST D3DKMT_GETRUNTIMEDATA*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_QUERYADAPTERINFO)(_Inout_ CONST D3DKMT_QUERYADAPTERINFO*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OPENADAPTERFROMHDC)(_Inout_ D3DKMT_OPENADAPTERFROMHDC*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OPENADAPTERFROMGDIDISPLAYNAME)(_Inout_ D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OPENADAPTERFROMDEVICENAME)(_Inout_ D3DKMT_OPENADAPTERFROMDEVICENAME*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CLOSEADAPTER)(_In_ CONST D3DKMT_CLOSEADAPTER*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_GETSHAREDPRIMARYHANDLE)(_Inout_ D3DKMT_GETSHAREDPRIMARYHANDLE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_ESCAPE)(_In_ CONST D3DKMT_ESCAPE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_QUERYSTATISTICS)(_In_ CONST D3DKMT_QUERYSTATISTICS*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SETVIDPNSOURCEOWNER)(_In_ CONST D3DKMT_SETVIDPNSOURCEOWNER*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_GETPRESENTHISTORY)(_Inout_ D3DKMT_GETPRESENTHISTORY*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CREATEOVERLAY)(_Inout_ D3DKMT_CREATEOVERLAY*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_UPDATEOVERLAY)(_In_ CONST D3DKMT_UPDATEOVERLAY*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_FLIPOVERLAY)(_In_ CONST D3DKMT_FLIPOVERLAY*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_DESTROYOVERLAY)(_In_ CONST D3DKMT_DESTROYOVERLAY*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_WAITFORVERTICALBLANKEVENT)(_In_ CONST D3DKMT_WAITFORVERTICALBLANKEVENT*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SETGAMMARAMP)(_In_ CONST D3DKMT_SETGAMMARAMP*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_GETDEVICESTATE)(_Inout_ D3DKMT_GETDEVICESTATE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CREATEDCFROMMEMORY)(_Inout_ D3DKMT_CREATEDCFROMMEMORY*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_DESTROYDCFROMMEMORY)(_In_ CONST D3DKMT_DESTROYDCFROMMEMORY*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SETCONTEXTSCHEDULINGPRIORITY)(_In_ CONST D3DKMT_SETCONTEXTSCHEDULINGPRIORITY*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_GETCONTEXTSCHEDULINGPRIORITY)(_Inout_ D3DKMT_GETCONTEXTSCHEDULINGPRIORITY*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SETPROCESSSCHEDULINGPRIORITYCLASS)(_In_ HANDLE, _In_ D3DKMT_SCHEDULINGPRIORITYCLASS);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_GETPROCESSSCHEDULINGPRIORITYCLASS)(_In_ HANDLE, _Out_ D3DKMT_SCHEDULINGPRIORITYCLASS*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_RELEASEPROCESSVIDPNSOURCEOWNERS)(_In_ HANDLE);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_GETSCANLINE)(_Inout_ D3DKMT_GETSCANLINE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CHANGESURFACEPOINTER)(_In_ CONST D3DKMT_CHANGESURFACEPOINTER*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SETQUEUEDLIMIT)(_In_ CONST D3DKMT_SETQUEUEDLIMIT*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_POLLDISPLAYCHILDREN)(_In_ CONST D3DKMT_POLLDISPLAYCHILDREN*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_INVALIDATEACTIVEVIDPN)(_In_ CONST D3DKMT_INVALIDATEACTIVEVIDPN*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CHECKOCCLUSION)(_In_ CONST D3DKMT_CHECKOCCLUSION*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_WAITFORIDLE)(_In_ CONST D3DKMT_WAITFORIDLE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CHECKMONITORPOWERSTATE)(_In_ CONST D3DKMT_CHECKMONITORPOWERSTATE*);
typedef _Check_return_ BOOLEAN  (APIENTRY *PFND3DKMT_CHECKEXCLUSIVEOWNERSHIP)();
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP)(_In_ CONST D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SETDISPLAYPRIVATEDRIVERFORMAT)(_In_ CONST D3DKMT_SETDISPLAYPRIVATEDRIVERFORMAT*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SHAREDPRIMARYLOCKNOTIFICATION)(_In_ CONST D3DKMT_SHAREDPRIMARYLOCKNOTIFICATION*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SHAREDPRIMARYUNLOCKNOTIFICATION)(_In_ CONST D3DKMT_SHAREDPRIMARYUNLOCKNOTIFICATION*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CREATEKEYEDMUTEX)(_Inout_ D3DKMT_CREATEKEYEDMUTEX*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OPENKEYEDMUTEX)(_Inout_ D3DKMT_OPENKEYEDMUTEX*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_DESTROYKEYEDMUTEX)(_In_ CONST D3DKMT_DESTROYKEYEDMUTEX*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_ACQUIREKEYEDMUTEX)(_Inout_ D3DKMT_ACQUIREKEYEDMUTEX*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_RELEASEKEYEDMUTEX)(_Inout_ D3DKMT_RELEASEKEYEDMUTEX*);
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CREATEKEYEDMUTEX2)(_Inout_ D3DKMT_CREATEKEYEDMUTEX2*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OPENKEYEDMUTEX2)(_Inout_ D3DKMT_OPENKEYEDMUTEX2*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_ACQUIREKEYEDMUTEX2)(_Inout_ D3DKMT_ACQUIREKEYEDMUTEX2*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_RELEASEKEYEDMUTEX2)(_Inout_ D3DKMT_RELEASEKEYEDMUTEX2*);
#endif
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CONFIGURESHAREDRESOURCE)(_In_ CONST D3DKMT_CONFIGURESHAREDRESOURCE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_GETOVERLAYSTATE)(_Inout_ D3DKMT_GETOVERLAYSTATE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CHECKSHAREDRESOURCEACCESS)(_In_ CONST D3DKMT_CHECKSHAREDRESOURCEACCESS*);
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OFFERALLOCATIONS)(_In_ CONST D3DKMT_OFFERALLOCATIONS*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_RECLAIMALLOCATIONS)(_Inout_ CONST D3DKMT_RECLAIMALLOCATIONS*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CREATEOUTPUTDUPL)(_In_ CONST D3DKMT_CREATE_OUTPUTDUPL*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_DESTROYOUTPUTDUPL)(_In_ CONST D3DKMT_DESTROY_OUTPUTDUPL*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OUTPUTDUPLGETFRAMEINFO)(_Inout_ D3DKMT_OUTPUTDUPL_GET_FRAMEINFO*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OUTPUTDUPLGETMETADATA)(_Inout_ D3DKMT_OUTPUTDUPL_METADATA*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OUTPUTDUPLGETPOINTERSHAPEDATA)(_Inout_ D3DKMT_OUTPUTDUPL_GET_POINTER_SHAPE_DATA*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OUTPUTDUPLRELEASEFRAME)(_In_ D3DKMT_OUTPUTDUPL_RELEASE_FRAME*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OUTPUTDUPLPRESENT)(_In_ CONST D3DKMT_OUTPUTDUPLPRESENT*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_ENUMADAPTERS)(_Inout_ CONST D3DKMT_ENUMADAPTERS*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_ENUMADAPTERS2)(_Inout_ CONST D3DKMT_ENUMADAPTERS2*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OPENADAPTERFROMLUID)(_Inout_ D3DKMT_OPENADAPTERFROMLUID*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_QUERYREMOTEVIDPNSOURCEFROMGDIDISPLAYNAME)(_Inout_ D3DKMT_QUERYREMOTEVIDPNSOURCEFROMGDIDISPLAYNAME*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SETVIDPNSOURCEOWNER1)(_In_ CONST D3DKMT_SETVIDPNSOURCEOWNER1*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_PINDIRECTFLIPRESOURCES)(_In_ CONST D3DKMT_PINDIRECTFLIPRESOURCES*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_UNPINDIRECTFLIPRESOURCES)(_In_ CONST D3DKMT_UNPINDIRECTFLIPRESOURCES*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_WAITFORVERTICALBLANKEVENT2)(_In_ CONST D3DKMT_WAITFORVERTICALBLANKEVENT2*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_GETDWMVERTICALBLANKEVENT)(_In_ CONST D3DKMT_GETVERTICALBLANKEVENT*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SETSYNCREFRESHCOUNTWAITTARGET)(_In_ CONST D3DKMT_SETSYNCREFRESHCOUNTWAITTARGET*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SETCONTEXTINPROCESSSCHEDULINGPRIORITY)(_In_ CONST D3DKMT_SETCONTEXTINPROCESSSCHEDULINGPRIORITY*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_GETCONTEXTINPROCESSSCHEDULINGPRIORITY)(_Inout_ D3DKMT_GETCONTEXTINPROCESSSCHEDULINGPRIORITY*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_PRESENTMULTIPLANEOVERLAY)(_In_ CONST D3DKMT_PRESENT_MULTIPLANE_OVERLAY*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_GETSHAREDRESOURCEADAPTERLUID)(_Inout_ D3DKMT_GETSHAREDRESOURCEADAPTERLUID*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SETSTEREOENABLED)(_In_ BOOL);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_QUERYHYBRIDLISTVALUE)(_Inout_ D3DKMT_HYBRID_LIST*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SETHYBRIDLISTVVALUE)(_Inout_ D3DKMT_HYBRID_LIST*);
#endif
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CHECKMULTIPLANEOVERLAYSUPPORT)(_Inout_ D3DKMT_CHECKMULTIPLANEOVERLAYSUPPORT*);
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_MAKERESIDENT)(_Inout_ D3DDDI_MAKERESIDENT*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_EVICT)(_Inout_ D3DKMT_EVICT*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_WAITFORSYNCHRONIZATIONOBJECTFROMCPU)(_In_ CONST D3DKMT_WAITFORSYNCHRONIZATIONOBJECTFROMCPU*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SIGNALSYNCHRONIZATIONOBJECTFROMCPU)(_In_ CONST D3DKMT_SIGNALSYNCHRONIZATIONOBJECTFROMCPU*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_WAITFORSYNCHRONIZATIONOBJECTFROMGPU)(_In_ CONST D3DKMT_WAITFORSYNCHRONIZATIONOBJECTFROMGPU*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SIGNALSYNCHRONIZATIONOBJECTFROMGPU)(_In_ CONST D3DKMT_SIGNALSYNCHRONIZATIONOBJECTFROMGPU*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SIGNALSYNCHRONIZATIONOBJECTFROMGPU2)(_In_ CONST D3DKMT_SIGNALSYNCHRONIZATIONOBJECTFROMGPU2*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CREATEPAGINGQUEUE)(_Inout_ D3DKMT_CREATEPAGINGQUEUE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_DESTROYPAGINGQUEUE)(_Inout_ D3DDDI_DESTROYPAGINGQUEUE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_LOCK2)(_Inout_ D3DKMT_LOCK2*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_UNLOCK2)(_In_ CONST D3DKMT_UNLOCK2*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_INVALIDATECACHE)(_In_ const D3DKMT_INVALIDATECACHE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_RESERVEGPUVIRTUALADDRESS)(_Inout_ D3DDDI_RESERVEGPUVIRTUALADDRESS*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_MAPGPUVIRTUALADDRESS)(_Inout_ D3DDDI_MAPGPUVIRTUALADDRESS*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_FREEGPUVIRTUALADDRESS)(_In_ CONST D3DKMT_FREEGPUVIRTUALADDRESS*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_UPDATEGPUVIRTUALADDRESS)(_In_ CONST D3DKMT_UPDATEGPUVIRTUALADDRESS*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_GETRESOURCEPRESENTPRIVATEDRIVERDATA)(_Inout_ D3DDDI_GETRESOURCEPRESENTPRIVATEDRIVERDATA*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CREATECONTEXTVIRTUAL)(_Inout_ D3DKMT_CREATECONTEXTVIRTUAL*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SUBMITCOMMAND)(_In_ CONST D3DKMT_SUBMITCOMMAND*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OPENSYNCOBJECTFROMNTHANDLE2)(_Inout_ D3DKMT_OPENSYNCOBJECTFROMNTHANDLE2*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OPENSYNCOBJECTNTHANDLEFROMNAME)(_Inout_ D3DKMT_OPENSYNCOBJECTNTHANDLEFROMNAME*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_QUERYVIDEOMEMORYINFO)(_Inout_ D3DKMT_QUERYVIDEOMEMORYINFO*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CHANGEVIDEOMEMORYRESERVATION)(_In_ CONST D3DKMT_CHANGEVIDEOMEMORYRESERVATION*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_REGISTERTRIMNOTIFICATION)(_Inout_ D3DKMT_REGISTERTRIMNOTIFICATION*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_UNREGISTERTRIMNOTIFICATION)(_Inout_ D3DKMT_UNREGISTERTRIMNOTIFICATION*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_REGISTERBUDGETCHANGENOTIFICATION)(_Inout_ D3DKMT_REGISTERBUDGETCHANGENOTIFICATION*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_UNREGISTERBUDGETCHANGENOTIFICATION)(_Inout_ D3DKMT_UNREGISTERBUDGETCHANGENOTIFICATION*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CHECKMULTIPLANEOVERLAYSUPPORT2)(_Inout_ D3DKMT_CHECKMULTIPLANEOVERLAYSUPPORT2*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_PRESENTMULTIPLANEOVERLAY2)(_In_ CONST D3DKMT_PRESENT_MULTIPLANE_OVERLAY2*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_RECLAIMALLOCATIONS2)(_Inout_ D3DKMT_RECLAIMALLOCATIONS2*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SETSTABLEPOWERSTATE)(_In_ CONST D3DKMT_SETSTABLEPOWERSTATE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_QUERYCLOCKCALIBRATION)(_Inout_ D3DKMT_QUERYCLOCKCALIBRATION*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_QUERYVIDPNEXCLUSIVEOWNERSHIP)(_Inout_ D3DKMT_QUERYVIDPNEXCLUSIVEOWNERSHIP*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_ADJUSTFULLSCREENGAMMA)(_In_ D3DKMT_ADJUSTFULLSCREENGAMMA*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SETVIDPNSOURCEHWPROTECTION)(_In_ D3DKMT_SETVIDPNSOURCEHWPROTECTION*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_MARKDEVICEASERROR)(_In_ D3DKMT_MARKDEVICEASERROR*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_FLUSHHEAPTRANSITIONS)(_In_ D3DKMT_FLUSHHEAPTRANSITIONS*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SETHWPROTECTIONTEARDOWNRECOVERY)(_In_ D3DKMT_SETHWPROTECTIONTEARDOWNRECOVERY*);
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_QUERYPROCESSOFFERINFO)(_Inout_ D3DKMT_QUERYPROCESSOFFERINFO*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_TRIMPROCESSCOMMITMENT)(_Inout_ D3DKMT_TRIMPROCESSCOMMITMENT*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_UPDATEALLOCATIONPROPERTY)(_Inout_ D3DDDI_UPDATEALLOCPROPERTY*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CHECKMULTIPLANEOVERLAYSUPPORT3)(_Inout_ D3DKMT_CHECKMULTIPLANEOVERLAYSUPPORT3*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_PRESENTMULTIPLANEOVERLAY3)(_In_ CONST D3DKMT_PRESENT_MULTIPLANE_OVERLAY3*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SETFSEBLOCK)(_In_ CONST D3DKMT_SETFSEBLOCK*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_QUERYFSEBLOCK)(_Inout_ D3DKMT_QUERYFSEBLOCK*);
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_GETALLOCATIONPRIORITY)(_In_ CONST D3DKMT_GETALLOCATIONPRIORITY*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_GETMULTIPLANEOVERLAYCAPS)(_Inout_ D3DKMT_GET_MULTIPLANE_OVERLAY_CAPS*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_GETPOSTCOMPOSITIONCAPS)(_Inout_ D3DKMT_GET_POST_COMPOSITION_CAPS*);
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SETVIDPNSOURCEOWNER2)(_In_ CONST D3DKMT_SETVIDPNSOURCEOWNER2*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_GETPROCESSDEVICEREMOVALSUPPORT)(_Inout_ D3DKMT_GETPROCESSDEVICEREMOVALSUPPORT*);

typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CREATEPROTECTEDSESSION)(_Inout_ D3DKMT_CREATEPROTECTEDSESSION*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_DESTROYPROTECTEDSESSION)(_Inout_ D3DKMT_DESTROYPROTECTEDSESSION*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_QUERYPROTECTEDSESSIONSTATUS)(_Inout_ D3DKMT_QUERYPROTECTEDSESSIONSTATUS*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_QUERYPROTECTEDSESSIONINFOFROMNTHANDLE)(_Inout_ D3DKMT_QUERYPROTECTEDSESSIONINFOFROMNTHANDLE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OPENPROTECTEDSESSIONFROMNTHANDLE)(_Inout_ D3DKMT_OPENPROTECTEDSESSIONFROMNTHANDLE*);


typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OPENKEYEDMUTEXFROMNTHANDLE)(_Inout_ D3DKMT_OPENKEYEDMUTEXFROMNTHANDLE*);

#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CREATEHWQUEUE)(_Inout_ D3DKMT_CREATEHWQUEUE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_DESTROYHWQUEUE)(_In_ CONST D3DKMT_DESTROYHWQUEUE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SUBMITCOMMANDTOHWQUEUE)(_In_ CONST D3DKMT_SUBMITCOMMANDTOHWQUEUE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SUBMITWAITFORSYNCOBJECTSTOHWQUEUE)(_In_ CONST D3DKMT_SUBMITWAITFORSYNCOBJECTSTOHWQUEUE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SUBMITSIGNALSYNCOBJECTSTOHWQUEUE)(_In_ CONST D3DKMT_SUBMITSIGNALSYNCOBJECTSTOHWQUEUE*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SUBMITPRESENTBLTTOHWQUEUE)(_In_ CONST D3DKMT_SUBMITPRESENTBLTTOHWQUEUE*);
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_SUBMITPRESENTTOHWQUEUE)(_Inout_ D3DKMT_SUBMITPRESENTTOHWQUEUE*);
#endif  // DXGKDDI_INTERFACE_VERSION_WDDM2_5

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)

typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OUTPUTDUPLPRESENTTOHWQUEUE)(_In_ CONST D3DKMT_OUTPUTDUPLPRESENTTOHWQUEUE*);


#endif  // DXGKDDI_INTERFACE_VERSION_WDDM2_6

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_7)

typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_ENUMADAPTERS3)(_Inout_ D3DKMT_ENUMADAPTERS3*);

typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_PINRESOURCES)(_Inout_ D3DKMT_PINRESOURCES*);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_UNPINRESOURCES)(_In_ CONST D3DKMT_UNPINRESOURCES*);
#ifdef _WIN32
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_DISPLAYPORT_OPERATION)(_Inout_ D3DKMT_DISPLAYPORT_OPERATION_HEADER*);
#endif // _WIN32

#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)


#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)

typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CANCELPRESENTS)(_In_ D3DKMT_CANCEL_PRESENTS*);

#endif

#if !defined(__REACTOS__) || (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTShareObjectWithHost(_Inout_ D3DKMT_SHAREOBJECTWITHHOST*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCreateSyncFile(_Inout_ D3DKMT_CREATESYNCFILE*);

// Used in WSL to close the internal file descriptor to /dev/dxg
EXTERN_C VOID APIENTRY D3DKMTCloseDxCoreDevice();
#endif

#if !defined(D3DKMDT_SPECIAL_MULTIPLATFORM_TOOL)

#ifdef __cplusplus
extern "C"
{
#endif

EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCreateAllocation(_Inout_ D3DKMT_CREATEALLOCATION*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCreateAllocation2(_Inout_ D3DKMT_CREATEALLOCATION*); // _ADVSCH_
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTQueryResourceInfo(_Inout_ D3DKMT_QUERYRESOURCEINFO*);
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTQueryResourceInfoFromNtHandle(_Inout_ D3DKMT_QUERYRESOURCEINFOFROMNTHANDLE*);

EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTShareObjects(
    _In_range_(1, D3DKMT_MAX_OBJECTS_PER_HANDLE) UINT   cObjects,
    _In_reads_(cObjects) CONST D3DKMT_HANDLE *          hObjects,
    _In_ POBJECT_ATTRIBUTES                             pObjectAttributes,
    _In_ DWORD                                          dwDesiredAccess,
    _Out_ HANDLE *                                      phSharedNtHandle
    );

EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOpenNtHandleFromName(_Inout_ D3DKMT_OPENNTHANDLEFROMNAME*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOpenResourceFromNtHandle(_Inout_ D3DKMT_OPENRESOURCEFROMNTHANDLE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOpenSyncObjectFromNtHandle(_Inout_ D3DKMT_OPENSYNCOBJECTFROMNTHANDLE*);
#endif
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOpenResource(_Inout_ D3DKMT_OPENRESOURCE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOpenResource2(_Inout_ D3DKMT_OPENRESOURCE*); // _ADVSCH_
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTDestroyAllocation(_In_ CONST D3DKMT_DESTROYALLOCATION*);

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTDestroyAllocation2(_In_ CONST D3DKMT_DESTROYALLOCATION2*);
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_0

EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSetAllocationPriority(_In_ CONST D3DKMT_SETALLOCATIONPRIORITY*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTQueryAllocationResidency(_In_ CONST D3DKMT_QUERYALLOCATIONRESIDENCY*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCreateDevice(_Inout_ D3DKMT_CREATEDEVICE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTDestroyDevice(_In_ CONST D3DKMT_DESTROYDEVICE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCreateContext(_Inout_ D3DKMT_CREATECONTEXT*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTDestroyContext(_In_ CONST D3DKMT_DESTROYCONTEXT*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCreateSynchronizationObject(_Inout_ D3DKMT_CREATESYNCHRONIZATIONOBJECT*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCreateSynchronizationObject2(_Inout_ D3DKMT_CREATESYNCHRONIZATIONOBJECT2*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOpenSynchronizationObject(_Inout_ D3DKMT_OPENSYNCHRONIZATIONOBJECT*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTDestroySynchronizationObject(_In_ CONST D3DKMT_DESTROYSYNCHRONIZATIONOBJECT*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTWaitForSynchronizationObject(_In_ CONST D3DKMT_WAITFORSYNCHRONIZATIONOBJECT*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTWaitForSynchronizationObject2(_In_ CONST D3DKMT_WAITFORSYNCHRONIZATIONOBJECT2*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSignalSynchronizationObject(_In_ CONST D3DKMT_SIGNALSYNCHRONIZATIONOBJECT*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSignalSynchronizationObject2(_In_ CONST D3DKMT_SIGNALSYNCHRONIZATIONOBJECT2*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTLock(_Inout_ D3DKMT_LOCK*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTUnlock(_In_ CONST D3DKMT_UNLOCK*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTGetDisplayModeList(_Inout_ D3DKMT_GETDISPLAYMODELIST*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSetDisplayMode(_Inout_ CONST D3DKMT_SETDISPLAYMODE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTGetMultisampleMethodList(_Inout_ D3DKMT_GETMULTISAMPLEMETHODLIST*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTPresent(_Inout_ D3DKMT_PRESENT*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTRender(_Inout_ D3DKMT_RENDER*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTGetRuntimeData(_Inout_ CONST D3DKMT_GETRUNTIMEDATA*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTQueryAdapterInfo(_Inout_ CONST D3DKMT_QUERYADAPTERINFO*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOpenAdapterFromHdc(_Inout_ D3DKMT_OPENADAPTERFROMHDC*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOpenAdapterFromGdiDisplayName(_Inout_ D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOpenAdapterFromDeviceName(_Inout_ D3DKMT_OPENADAPTERFROMDEVICENAME*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCloseAdapter(_In_ CONST D3DKMT_CLOSEADAPTER*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTGetSharedPrimaryHandle(_Inout_ D3DKMT_GETSHAREDPRIMARYHANDLE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTEscape(_In_ CONST D3DKMT_ESCAPE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTQueryStatistics(_In_ CONST D3DKMT_QUERYSTATISTICS*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSetVidPnSourceOwner(_In_ CONST D3DKMT_SETVIDPNSOURCEOWNER*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTGetPresentHistory(_Inout_ D3DKMT_GETPRESENTHISTORY*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTGetPresentQueueEvent(_In_ D3DKMT_HANDLE hAdapter, _Inout_ HANDLE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCreateOverlay(_Inout_ D3DKMT_CREATEOVERLAY*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTUpdateOverlay(_In_ CONST D3DKMT_UPDATEOVERLAY*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTFlipOverlay(_In_ CONST D3DKMT_FLIPOVERLAY*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTDestroyOverlay(_In_ CONST D3DKMT_DESTROYOVERLAY*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTWaitForVerticalBlankEvent(_In_ CONST D3DKMT_WAITFORVERTICALBLANKEVENT*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSetGammaRamp(_In_ CONST D3DKMT_SETGAMMARAMP*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTGetDeviceState(_Inout_ D3DKMT_GETDEVICESTATE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCreateDCFromMemory(_Inout_ D3DKMT_CREATEDCFROMMEMORY*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTDestroyDCFromMemory(_In_ CONST D3DKMT_DESTROYDCFROMMEMORY*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSetContextSchedulingPriority(_In_ CONST D3DKMT_SETCONTEXTSCHEDULINGPRIORITY*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTGetContextSchedulingPriority(_Inout_ D3DKMT_GETCONTEXTSCHEDULINGPRIORITY*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSetProcessSchedulingPriorityClass(_In_ HANDLE, _In_ D3DKMT_SCHEDULINGPRIORITYCLASS);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTGetProcessSchedulingPriorityClass(_In_ HANDLE, _Out_ D3DKMT_SCHEDULINGPRIORITYCLASS*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTReleaseProcessVidPnSourceOwners(_In_ HANDLE);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTGetScanLine(_Inout_ D3DKMT_GETSCANLINE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTChangeSurfacePointer(_In_ CONST D3DKMT_CHANGESURFACEPOINTER*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSetQueuedLimit(_In_ CONST D3DKMT_SETQUEUEDLIMIT*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTPollDisplayChildren(_In_ CONST D3DKMT_POLLDISPLAYCHILDREN*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTInvalidateActiveVidPn(_In_ CONST D3DKMT_INVALIDATEACTIVEVIDPN*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCheckOcclusion(_In_ CONST D3DKMT_CHECKOCCLUSION*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTWaitForIdle(IN CONST D3DKMT_WAITFORIDLE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCheckMonitorPowerState(_In_ CONST D3DKMT_CHECKMONITORPOWERSTATE*);
EXTERN_C _Check_return_ BOOLEAN  APIENTRY D3DKMTCheckExclusiveOwnership(VOID);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCheckVidPnExclusiveOwnership(_In_ CONST D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSetDisplayPrivateDriverFormat(_In_ CONST D3DKMT_SETDISPLAYPRIVATEDRIVERFORMAT*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSharedPrimaryLockNotification(_In_ CONST D3DKMT_SHAREDPRIMARYLOCKNOTIFICATION*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSharedPrimaryUnLockNotification(_In_ CONST D3DKMT_SHAREDPRIMARYUNLOCKNOTIFICATION*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCreateKeyedMutex(_Inout_ D3DKMT_CREATEKEYEDMUTEX*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOpenKeyedMutex(_Inout_ D3DKMT_OPENKEYEDMUTEX*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTDestroyKeyedMutex(_In_ CONST D3DKMT_DESTROYKEYEDMUTEX*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTAcquireKeyedMutex(_Inout_ D3DKMT_ACQUIREKEYEDMUTEX*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTReleaseKeyedMutex(_Inout_ D3DKMT_RELEASEKEYEDMUTEX*);
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCreateKeyedMutex2(_Inout_ D3DKMT_CREATEKEYEDMUTEX2*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOpenKeyedMutex2(_Inout_ D3DKMT_OPENKEYEDMUTEX2*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTAcquireKeyedMutex2(_Inout_ D3DKMT_ACQUIREKEYEDMUTEX2*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTReleaseKeyedMutex2(_Inout_ D3DKMT_RELEASEKEYEDMUTEX2*);
#endif
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTConfigureSharedResource(_In_ CONST D3DKMT_CONFIGURESHAREDRESOURCE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTGetOverlayState(_Inout_ D3DKMT_GETOVERLAYSTATE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCheckSharedResourceAccess(_In_ CONST D3DKMT_CHECKSHAREDRESOURCEACCESS*);
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOfferAllocations(_In_ CONST D3DKMT_OFFERALLOCATIONS*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTReclaimAllocations(_Inout_ CONST D3DKMT_RECLAIMALLOCATIONS*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCreateOutputDupl(_In_ CONST D3DKMT_CREATE_OUTPUTDUPL*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTDestroyOutputDupl(_In_ CONST D3DKMT_DESTROY_OUTPUTDUPL*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOutputDuplGetFrameInfo(_Inout_ D3DKMT_OUTPUTDUPL_GET_FRAMEINFO*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOutputDuplGetMetaData(_Inout_ D3DKMT_OUTPUTDUPL_METADATA*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOutputDuplGetPointerShapeData(_Inout_ D3DKMT_OUTPUTDUPL_GET_POINTER_SHAPE_DATA*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOutputDuplReleaseFrame(_Inout_ D3DKMT_OUTPUTDUPL_RELEASE_FRAME*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOutputDuplPresent(_In_ CONST D3DKMT_OUTPUTDUPLPRESENT*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTEnumAdapters(_Inout_ CONST D3DKMT_ENUMADAPTERS*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTEnumAdapters2(_Inout_ CONST D3DKMT_ENUMADAPTERS2*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOpenAdapterFromLuid(_Inout_ CONST D3DKMT_OPENADAPTERFROMLUID*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTQueryRemoteVidPnSourceFromGdiDisplayName(_Inout_ D3DKMT_QUERYREMOTEVIDPNSOURCEFROMGDIDISPLAYNAME*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSetVidPnSourceOwner1(_In_ CONST D3DKMT_SETVIDPNSOURCEOWNER1*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTWaitForVerticalBlankEvent2(_In_ CONST D3DKMT_WAITFORVERTICALBLANKEVENT2*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSetSyncRefreshCountWaitTarget(_In_ CONST D3DKMT_SETSYNCREFRESHCOUNTWAITTARGET*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTGetDWMVerticalBlankEvent(_In_ CONST D3DKMT_GETVERTICALBLANKEVENT*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTPresentMultiPlaneOverlay(_In_ CONST D3DKMT_PRESENT_MULTIPLANE_OVERLAY*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTGetSharedResourceAdapterLuid(_Inout_ D3DKMT_GETSHAREDRESOURCEADAPTERLUID*);
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)

EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCheckMultiPlaneOverlaySupport(_Inout_ D3DKMT_CHECKMULTIPLANEOVERLAYSUPPORT*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSetContextInProcessSchedulingPriority(_In_ CONST D3DKMT_SETCONTEXTINPROCESSSCHEDULINGPRIORITY*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTGetContextInProcessSchedulingPriority(_Inout_ D3DKMT_GETCONTEXTINPROCESSSCHEDULINGPRIORITY*);
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTMakeResident(_Inout_ D3DDDI_MAKERESIDENT*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTEvict(_Inout_ D3DKMT_EVICT*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTWaitForSynchronizationObjectFromCpu(_In_ CONST D3DKMT_WAITFORSYNCHRONIZATIONOBJECTFROMCPU*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSignalSynchronizationObjectFromCpu(_In_ CONST D3DKMT_SIGNALSYNCHRONIZATIONOBJECTFROMCPU*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTWaitForSynchronizationObjectFromGpu(_In_ CONST D3DKMT_WAITFORSYNCHRONIZATIONOBJECTFROMGPU*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSignalSynchronizationObjectFromGpu(_In_ CONST D3DKMT_SIGNALSYNCHRONIZATIONOBJECTFROMGPU*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSignalSynchronizationObjectFromGpu2(_In_ CONST D3DKMT_SIGNALSYNCHRONIZATIONOBJECTFROMGPU2*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCreatePagingQueue(_Inout_ D3DKMT_CREATEPAGINGQUEUE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTDestroyPagingQueue(_Inout_ D3DDDI_DESTROYPAGINGQUEUE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTLock2(_Inout_ D3DKMT_LOCK2*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTUnlock2(_In_ CONST D3DKMT_UNLOCK2*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTInvalidateCache(_In_ CONST D3DKMT_INVALIDATECACHE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTMapGpuVirtualAddress(_Inout_ D3DDDI_MAPGPUVIRTUALADDRESS*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTReserveGpuVirtualAddress(_Inout_ D3DDDI_RESERVEGPUVIRTUALADDRESS*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTFreeGpuVirtualAddress(_In_ CONST D3DKMT_FREEGPUVIRTUALADDRESS*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTUpdateGpuVirtualAddress(_In_ CONST D3DKMT_UPDATEGPUVIRTUALADDRESS*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTGetResourcePresentPrivateDriverData(_Inout_ D3DDDI_GETRESOURCEPRESENTPRIVATEDRIVERDATA*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCreateContextVirtual(_In_ D3DKMT_CREATECONTEXTVIRTUAL*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSubmitCommand(_In_ CONST D3DKMT_SUBMITCOMMAND*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOpenSyncObjectFromNtHandle2(_Inout_ D3DKMT_OPENSYNCOBJECTFROMNTHANDLE2*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOpenSyncObjectNtHandleFromName(_Inout_ D3DKMT_OPENSYNCOBJECTNTHANDLEFROMNAME*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTQueryVideoMemoryInfo(_Inout_ D3DKMT_QUERYVIDEOMEMORYINFO*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTChangeVideoMemoryReservation(_In_ CONST D3DKMT_CHANGEVIDEOMEMORYRESERVATION*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTRegisterTrimNotification(_Inout_ D3DKMT_REGISTERTRIMNOTIFICATION*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTUnregisterTrimNotification(_Inout_ D3DKMT_UNREGISTERTRIMNOTIFICATION*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCheckMultiPlaneOverlaySupport2(_Inout_ D3DKMT_CHECKMULTIPLANEOVERLAYSUPPORT2*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTPresentMultiPlaneOverlay2(_In_ CONST D3DKMT_PRESENT_MULTIPLANE_OVERLAY2*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTReclaimAllocations2(_Inout_ D3DKMT_RECLAIMALLOCATIONS2*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSetStablePowerState(_In_ CONST D3DKMT_SETSTABLEPOWERSTATE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTQueryClockCalibration(_Inout_ D3DKMT_QUERYCLOCKCALIBRATION*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTQueryVidPnExclusiveOwnership(_Inout_ D3DKMT_QUERYVIDPNEXCLUSIVEOWNERSHIP*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTAdjustFullscreenGamma(_In_ D3DKMT_ADJUSTFULLSCREENGAMMA*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSetVidPnSourceHwProtection(_In_ D3DKMT_SETVIDPNSOURCEHWPROTECTION*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTMarkDeviceAsError(_In_ D3DKMT_MARKDEVICEASERROR*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTFlushHeapTransitions(_In_ D3DKMT_FLUSHHEAPTRANSITIONS*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSetHwProtectionTeardownRecovery(_In_ D3DKMT_SETHWPROTECTIONTEARDOWNRECOVERY*);
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTQueryProcessOfferInfo(_Inout_ D3DKMT_QUERYPROCESSOFFERINFO*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTTrimProcessCommitment(_Inout_ D3DKMT_TRIMPROCESSCOMMITMENT*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTUpdateAllocationProperty(_Inout_ D3DDDI_UPDATEALLOCPROPERTY*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCheckMultiPlaneOverlaySupport3(_Inout_ D3DKMT_CHECKMULTIPLANEOVERLAYSUPPORT3*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTPresentMultiPlaneOverlay3(_In_ CONST D3DKMT_PRESENT_MULTIPLANE_OVERLAY3*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSetFSEBlock(_In_ CONST D3DKMT_SETFSEBLOCK*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTQueryFSEBlock(_Inout_ D3DKMT_QUERYFSEBLOCK*);
#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCreateHwContext(_Inout_ D3DKMT_CREATEHWCONTEXT*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTDestroyHwContext(_In_ CONST D3DKMT_DESTROYHWCONTEXT*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCreateHwQueue(_Inout_ D3DKMT_CREATEHWQUEUE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTDestroyHwQueue(_In_ CONST D3DKMT_DESTROYHWQUEUE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSubmitCommandToHwQueue(_In_ CONST D3DKMT_SUBMITCOMMANDTOHWQUEUE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSubmitWaitForSyncObjectsToHwQueue(_In_ CONST D3DKMT_SUBMITWAITFORSYNCOBJECTSTOHWQUEUE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSubmitSignalSyncObjectsToHwQueue(_In_ CONST D3DKMT_SUBMITSIGNALSYNCOBJECTSTOHWQUEUE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTGetAllocationPriority(_In_ CONST D3DKMT_GETALLOCATIONPRIORITY*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTGetMultiPlaneOverlayCaps(_Inout_ D3DKMT_GET_MULTIPLANE_OVERLAY_CAPS*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTGetPostCompositionCaps(_Inout_ D3DKMT_GET_POST_COMPOSITION_CAPS*);

#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTPresentRedirected(_In_ D3DKMT_PRESENT_REDIRECTED*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSetVidPnSourceOwner2(_In_ CONST D3DKMT_SETVIDPNSOURCEOWNER2*);


EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSetMonitorColorSpaceTransform(_In_ D3DKMT_SET_COLORSPACE_TRANSFORM*);

EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCreateProtectedSession(_Inout_ D3DKMT_CREATEPROTECTEDSESSION*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTDestroyProtectedSession(_Inout_ D3DKMT_DESTROYPROTECTEDSESSION*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTQueryProtectedSessionStatus(_Inout_ D3DKMT_QUERYPROTECTEDSESSIONSTATUS*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTQueryProtectedSessionInfoFromNtHandle(_Inout_ D3DKMT_QUERYPROTECTEDSESSIONINFOFROMNTHANDLE*);
EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOpenProtectedSessionFromNtHandle(_Inout_ D3DKMT_OPENPROTECTEDSESSIONFROMNTHANDLE*);

EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTGetProcessDeviceRemovalSupport(_Inout_ D3DKMT_GETPROCESSDEVICEREMOVALSUPPORT*);

EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOpenKeyedMutexFromNtHandle(_Inout_ D3DKMT_OPENKEYEDMUTEXFROMNTHANDLE*);

#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)


EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSubmitPresentBltToHwQueue(_In_ CONST D3DKMT_SUBMITPRESENTBLTTOHWQUEUE*);

#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)

EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTSubmitPresentToHwQueue(_Inout_ D3DKMT_SUBMITPRESENTTOHWQUEUE*);

#endif


#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)

EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTOutputDuplPresentToHwQueue(_In_ CONST D3DKMT_OUTPUTDUPLPRESENTTOHWQUEUE*);


#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_7)


EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTEnumAdapters3(_Inout_ D3DKMT_ENUMADAPTERS3*);


#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)


#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)

EXTERN_C _Check_return_ NTSTATUS APIENTRY D3DKMTCancelPresents(_In_ D3DKMT_CANCEL_PRESENTS*);


#endif

//
// Interface used for shared power component management
// {ea5c6870-e93c-4588-bef1-fec42fc9429a}
//

DEFINE_GUID(GUID_DEVINTERFACE_GRAPHICSPOWER, 0xea5c6870, 0xe93c, 0x4588, 0xbe, 0xf1, 0xfe, 0xc4, 0x2f, 0xc9, 0x42, 0x9a);

#define IOCTL_INTERNAL_GRAPHICSPOWER_REGISTER \
    CTL_CODE(FILE_DEVICE_VIDEO, 0xa01, METHOD_NEITHER, FILE_ANY_ACCESS)

#define DXGK_GRAPHICSPOWER_VERSION_1_0 0x1000
#define DXGK_GRAPHICSPOWER_VERSION_1_1 0x1001
#define DXGK_GRAPHICSPOWER_VERSION_1_2 0x1002
#define DXGK_GRAPHICSPOWER_VERSION DXGK_GRAPHICSPOWER_VERSION_1_2

typedef
    _IRQL_requires_max_(PASSIVE_LEVEL)
VOID
(*PDXGK_POWER_NOTIFICATION)(
    PVOID GraphicsDeviceHandle,
    DEVICE_POWER_STATE NewGrfxPowerState,
    BOOLEAN PreNotification,
    PVOID PrivateHandle
    );

typedef
    _IRQL_requires_max_(PASSIVE_LEVEL)
VOID
(*PDXGK_REMOVAL_NOTIFICATION)(
    PVOID GraphicsDeviceHandle,
    PVOID PrivateHandle
    );

typedef
    _IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(*PDXGK_FSTATE_NOTIFICATION)(
    PVOID GraphicsDeviceHandle,
    ULONG ComponentIndex,
    UINT NewFState,
    BOOLEAN PreNotification,
    PVOID PrivateHandle
    );

typedef
    _IRQL_requires_(DISPATCH_LEVEL)
VOID
(*PDXGK_INITIAL_COMPONENT_STATE) (
    PVOID GraphicsDeviceHandle,
    PVOID PrivateHandle,
    ULONG ComponentIndex,
    BOOLEAN IsBlockingType,
    UINT InitialFState,
    GUID ComponentGuid,
    UINT PowerComponentMappingFlag
);


typedef struct _DXGK_GRAPHICSPOWER_REGISTER_INPUT_V_1_2 {
    ULONG Version;
    PVOID PrivateHandle;
    PDXGK_POWER_NOTIFICATION PowerNotificationCb;
    PDXGK_REMOVAL_NOTIFICATION RemovalNotificationCb;
    PDXGK_FSTATE_NOTIFICATION FStateNotificationCb;
    PDXGK_INITIAL_COMPONENT_STATE InitialComponentStateCb;
} DXGK_GRAPHICSPOWER_REGISTER_INPUT_V_1_2, *PDXGK_GRAPHICSPOWER_REGISTER_INPUT_V_1_2;

typedef DXGK_GRAPHICSPOWER_REGISTER_INPUT_V_1_2 DXGK_GRAPHICSPOWER_REGISTER_INPUT;
typedef DXGK_GRAPHICSPOWER_REGISTER_INPUT *PDXGK_GRAPHICSPOWER_REGISTER_INPUT;

typedef
    _Check_return_
    _IRQL_requires_max_(APC_LEVEL)
NTSTATUS
(*PDXGK_SET_SHARED_POWER_COMPONENT_STATE)(
    PVOID DeviceHandle,
    PVOID PrivateHandle,
    ULONG ComponentIndex,
    BOOLEAN Active
    );

typedef
    _Check_return_
    _IRQL_requires_max_(APC_LEVEL)
NTSTATUS
(*PDXGK_GRAPHICSPOWER_UNREGISTER)(
    PVOID DeviceHandle,
    PVOID PrivateHandle
    );

typedef struct _DXGK_GRAPHICSPOWER_REGISTER_OUTPUT
{
    PVOID DeviceHandle;
    DEVICE_POWER_STATE InitialGrfxPowerState;
    PDXGK_SET_SHARED_POWER_COMPONENT_STATE SetSharedPowerComponentStateCb;
    PDXGK_GRAPHICSPOWER_UNREGISTER UnregisterCb;
} DXGK_GRAPHICSPOWER_REGISTER_OUTPUT, *PDXGK_GRAPHICSPOWER_REGISTER_OUTPUT;

typedef enum _DXGKMT_POWER_SHARED_TYPE
{
    DXGKMT_POWER_SHARED_TYPE_AUDIO   = 0,
} DXGKMT_POWER_SHARED_TYPE;

#ifdef __cplusplus
}
#endif

#endif // !defined(D3DKMDT_SPECIAL_MULTIPLATFORM_TOOL)

#endif // (NTDDI_VERSION >= NTDDI_LONGHORN) || defined(D3DKMDT_SPECIAL_MULTIPLATFORM_TOOL)


#pragma warning(pop)

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#endif /* _D3DKMTHK_H_ */

