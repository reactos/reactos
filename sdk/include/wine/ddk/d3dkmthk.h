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
#include <winternl.h>

typedef enum _D3DKMT_VIDPNSOURCEOWNER_TYPE
{
    D3DKMT_VIDPNSOURCEOWNER_UNOWNED = 0,
    D3DKMT_VIDPNSOURCEOWNER_SHARED = 1,
    D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE = 2,
    D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVEGDI = 3,
    D3DKMT_VIDPNSOURCEOWNER_EMULATED = 4
} D3DKMT_VIDPNSOURCEOWNER_TYPE;

typedef enum _D3DKMT_MEMORY_SEGMENT_GROUP
{
    D3DKMT_MEMORY_SEGMENT_GROUP_LOCAL = 0,
    D3DKMT_MEMORY_SEGMENT_GROUP_NON_LOCAL = 1
} D3DKMT_MEMORY_SEGMENT_GROUP;

typedef struct _D3DKMT_CREATEDEVICEFLAGS
{
    UINT LegacyMode : 1;
    UINT RequestVSync : 1;
    UINT DisableGpuTimeout : 1;
    UINT Reserved : 29;
} D3DKMT_CREATEDEVICEFLAGS;

typedef struct _D3DDDI_ALLOCATIONLIST
{
    D3DKMT_HANDLE hAllocation;
    union
    {
        struct
        {
            UINT WriteOperation : 1;
            UINT DoNotRetireInstance : 1;
            UINT OfferPriority : 3;
            UINT Reserved : 27;
        } DUMMYSTRUCTNAME;
        UINT Value;
    } DUMMYUNIONNAME;
} D3DDDI_ALLOCATIONLIST;

typedef struct _D3DDDI_PATCHLOCATIONLIST
{
    UINT AllocationIndex;
    union
    {
        struct
        {
            UINT SlotId : 24;
            UINT Reserved : 8;
        } DUMMYSTRUCTNAME;
        UINT Value;
    } DUMMYUNIONNAME;
    UINT DriverId;
    UINT AllocationOffset;
    UINT PatchOffset;
    UINT SplitOffset;
} D3DDDI_PATCHLOCATIONLIST;

typedef struct _D3DKMT_DESTROYDEVICE
{
    D3DKMT_HANDLE hDevice;
} D3DKMT_DESTROYDEVICE;

typedef struct _D3DKMT_CHECKOCCLUSION
{
    HWND hWnd;
} D3DKMT_CHECKOCCLUSION;

typedef struct _D3DKMT_CREATEDEVICE
{
    union
    {
        D3DKMT_HANDLE hAdapter;
        VOID *pAdapter;
    } DUMMYUNIONNAME;
    D3DKMT_CREATEDEVICEFLAGS Flags;
    D3DKMT_HANDLE hDevice;
    VOID *pCommandBuffer;
    UINT CommandBufferSize;
    D3DDDI_ALLOCATIONLIST *pAllocationList;
    UINT AllocationListSize;
    D3DDDI_PATCHLOCATIONLIST *pPatchLocationList;
    UINT PatchLocationListSize;
} D3DKMT_CREATEDEVICE;

typedef struct _D3DKMT_OPENADAPTERFROMHDC
{
    HDC hDc;
    D3DKMT_HANDLE hAdapter;
    LUID AdapterLuid;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
} D3DKMT_OPENADAPTERFROMHDC;

typedef struct _D3DKMT_OPENADAPTERFROMDEVICENAME
{
    const WCHAR *pDeviceName;
    D3DKMT_HANDLE hAdapter;
    LUID AdapterLuid;
} D3DKMT_OPENADAPTERFROMDEVICENAME;

typedef struct _D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME
{
    WCHAR DeviceName[32];
    D3DKMT_HANDLE hAdapter;
    LUID AdapterLuid;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
} D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME;

typedef struct _D3DKMT_OPENADAPTERFROMLUID
{
    LUID AdapterLuid;
    D3DKMT_HANDLE hAdapter;
} D3DKMT_OPENADAPTERFROMLUID;

typedef struct _D3DKMT_SETVIDPNSOURCEOWNER
{
    D3DKMT_HANDLE hDevice;
    const D3DKMT_VIDPNSOURCEOWNER_TYPE *pType;
    const D3DDDI_VIDEO_PRESENT_SOURCE_ID *pVidPnSourceId;
    UINT VidPnSourceCount;
} D3DKMT_SETVIDPNSOURCEOWNER;

typedef struct _D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP
{
    D3DKMT_HANDLE hAdapter;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
} D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP;

#define D3DKMT_QUERYSTATISTICS_SEGMENT_PREFERENCE_MAX 5

typedef struct _D3DKMT_CLOSEADAPTER
{
    D3DKMT_HANDLE hAdapter;
} D3DKMT_CLOSEADAPTER;

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

typedef enum _KMTQUERYADAPTERINFOTYPE
{
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
    KMTQAITYPE_DRIVERVERSION = 13,
    KMTQAITYPE_ADAPTERTYPE = 15,
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
    KMTQAITYPE_PARAVIRTUALIZATION_RENDER,
    KMTQAITYPE_SERVICENAME,
    KMTQAITYPE_WDDM_2_7_CAPS,
    KMTQAITYPE_DISPLAY_UMDRIVERNAME = 71,
    KMTQAITYPE_TRACKEDWORKLOAD_SUPPORT,
    KMTQAITYPE_HYBRID_DLIST_DLL_SUPPORT,
    KMTQAITYPE_DISPLAY_CAPS,
    KMTQAITYPE_WDDM_2_9_CAPS,
    KMTQAITYPE_CROSSADAPTERRESOURCE_SUPPORT,
    KMTQAITYPE_WDDM_3_0_CAPS,
    KMTQAITYPE_WSAUMDIMAGENAME,
    KMTQAITYPE_VGPUINTERFACEID,
    KMTQAITYPE_WDDM_3_1_CAPS
} KMTQUERYADAPTERINFOTYPE;

typedef struct _D3DKMT_QUERYADAPTERINFO
{
    D3DKMT_HANDLE           hAdapter;
    KMTQUERYADAPTERINFOTYPE Type;
    VOID                    *pPrivateDriverData;
    UINT                    PrivateDriverDataSize;
} D3DKMT_QUERYADAPTERINFO;

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
    D3DKMT_PreemptionAttemptStatisticsMax
} D3DKMT_QUERYRESULT_PREEMPTION_ATTEMPT_RESULT;

typedef enum _D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS
{
    D3DKMT_AllocationPriorityClassMinimum,
    D3DKMT_AllocationPriorityClassLow,
    D3DKMT_AllocationPriorityClassNormal,
    D3DKMT_AllocationPriorityClassHigh,
    D3DKMT_AllocationPriorityClassMaximum,
    D3DKMT_MaxAllocationPriorityClass
} D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS;

typedef enum _D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE
{
    D3DKMT_RenderCommandBuffer,
    D3DKMT_DeferredCommandBuffer,
    D3DKMT_SystemCommandBuffer,
    D3DKMT_MmIoFlipCommandBuffer,
    D3DKMT_WaitCommandBuffer,
    D3DKMT_SignalCommandBuffer,
    D3DKMT_DeviceCommandBuffer,
    D3DKMT_SoftwareCommandBuffer,
    D3DKMT_QueuePacketTypeMax
} D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE;

typedef enum _D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE
{
    D3DKMT_ClientRenderBuffer,
    D3DKMT_ClientPagingBuffer,
    D3DKMT_SystemPagingBuffer,
    D3DKMT_SystemPreemptionBuffer,
    D3DKMT_DmaPacketTypeMax
} D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE;

typedef struct _D3DKMT_QUERYSTATSTICS_PAGING_TRANSFER
{
    ULONGLONG BytesFilled;
    ULONGLONG BytesDiscarded;
    ULONGLONG BytesMappedIntoAperture;
    ULONGLONG BytesUnmappedFromAperture;
    ULONGLONG BytesTransferredFromMdlToMemory;
    ULONGLONG BytesTransferredFromMemoryToMdl;
    ULONGLONG BytesTransferredFromApertureToMemory;
    ULONGLONG BytesTransferredFromMemoryToAperture;
} D3DKMT_QUERYSTATSTICS_PAGING_TRANSFER;

typedef struct _D3DKMT_QUERYSTATISTICS_COMMITMENT_DATA
{
    ULONG64 TotalBytesEvictedFromProcess;
    ULONG64 BytesBySegmentPreference[D3DKMT_QUERYSTATISTICS_SEGMENT_PREFERENCE_MAX];
} D3DKMT_QUERYSTATISTICS_COMMITMENT_DATA;

typedef struct _D3DKMT_QUERYSTATSTICS_SWIZZLING_RANGE
{
    ULONG NbRangesAcquired;
    ULONG NbRangesReleased;
} D3DKMT_QUERYSTATSTICS_SWIZZLING_RANGE;

typedef struct _D3DKMT_QUERYSTATISTICS_COUNTER
{
    ULONG     Count;
    ULONGLONG Bytes;
} D3DKMT_QUERYSTATISTICS_COUNTER;

typedef struct _D3DKMT_QUERYSTATISTICS_DMA_BUFFER
{
    D3DKMT_QUERYSTATISTICS_COUNTER Size;
    ULONG AllocationListBytes;
    ULONG PatchLocationListBytes;
} D3DKMT_QUERYSTATISTICS_DMA_BUFFER;

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
    D3DKMT_QUERYSTATISTICS_COUNTER TerminatedShared;
    D3DKMT_QUERYSTATISTICS_COUNTER TerminatedNonShared;
    D3DKMT_QUERYSTATISTICS_COUNTER DestroyedShared;
    D3DKMT_QUERYSTATISTICS_COUNTER DestroyedNonShared;
} D3DKMT_QUERYSTATSTICS_TERMINATIONS;

typedef struct _D3DKMT_QUERYSTATISTICS_QUERY_NODE
{
    ULONG NodeId;
} D3DKMT_QUERYSTATISTICS_QUERY_NODE;

typedef struct _D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT
{
    ULONG SegmentId;
} D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT;

typedef struct _D3DKMT_QUERYSTATISTICS_PREEMPTION_INFORMATION
{
    ULONG PreemptionCounter[D3DKMT_PreemptionAttemptStatisticsMax];
} D3DKMT_QUERYSTATISTICS_PREEMPTION_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE
{
    ULONG VidPnSourceId;
} D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE;

typedef struct _D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_INFORMATION
{
    ULONG PacketSubmited;
    ULONG PacketCompleted;
} D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_INFORMATION
{
    ULONG PacketSubmited;
    ULONG PacketCompleted;
    ULONG PacketPreempted;
    ULONG PacketFaulted;
} D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_SYSTEM_MEMORY
{
    ULONGLONG BytesAllocated;
    ULONGLONG BytesReserved;
    ULONG SmallAllocationBlocks;
    ULONG LargeAllocationBlocks;
    ULONGLONG WriteCombinedBytesAllocated;
    ULONGLONG WriteCombinedBytesReserved;
    ULONGLONG CachedBytesAllocated;
    ULONGLONG CachedBytesReserved;
    ULONGLONG SectionBytesAllocated;
    ULONGLONG SectionBytesReserved;
} D3DKMT_QUERYSTATISTICS_SYSTEM_MEMORY;

typedef enum _D3DKMT_QUERYSTATISTICS_TYPE
{
    D3DKMT_QUERYSTATISTICS_ADAPTER,
    D3DKMT_QUERYSTATISTICS_PROCESS,
    D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER,
    D3DKMT_QUERYSTATISTICS_SEGMENT,
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT,
    D3DKMT_QUERYSTATISTICS_NODE,
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE,
    D3DKMT_QUERYSTATISTICS_VIDPNSOURCE,
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE
} D3DKMT_QUERYSTATISTICS_TYPE;

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

typedef struct _D3DKMT_QUERYSTATISTICS_MEMORY
{
    ULONGLONG TotalBytesEvicted;
    ULONG     AllocsCommitted;
    ULONG     AllocsResident;
} D3DKMT_QUERYSTATISTICS_MEMORY;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION
{
    ULONG   Frame;
    ULONG   CancelledFrame;
    ULONG   QueuedPresent;
    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_PACKET_INFORMATION
{
    D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_INFORMATION QueuePacket[D3DKMT_QueuePacketTypeMax];
    D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_INFORMATION   DmaPacket[D3DKMT_DmaPacketTypeMax];
} D3DKMT_QUERYSTATISTICS_PACKET_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION
{
    LARGE_INTEGER RunningTime;
    ULONG         ContextSwitch;
    D3DKMT_QUERYSTATISTICS_PREEMPTION_INFORMATION PreemptionStatistics;
    D3DKMT_QUERYSTATISTICS_PACKET_INFORMATION     PacketStatistics;
    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_NODE_INFORMATION
{
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION GlobalInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION SystemInformation;
    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_NODE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_VIDPNSOURCE_INFORMATION
{
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION GlobalInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION SystemInformation;
    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_VIDPNSOURCE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_ADAPTER_INFORMATION
{
    ULONG NbSegments;
    ULONG NodeCount;
    ULONG VidPnSourceCount;
    ULONG VSyncEnabled;
    ULONG TdrDetectedCount;
    LONGLONG ZeroLengthDmaBuffers;
    ULONGLONG RestartedPeriod;
    D3DKMT_QUERYSTATSTICS_REFERENCE_DMA_BUFFER ReferenceDmaBuffer;
    D3DKMT_QUERYSTATSTICS_RENAMING Renaming;
    D3DKMT_QUERYSTATSTICS_PREPRATION Preparation;
    D3DKMT_QUERYSTATSTICS_PAGING_FAULT PagingFault;
    D3DKMT_QUERYSTATSTICS_PAGING_TRANSFER PagingTransfer;
    D3DKMT_QUERYSTATSTICS_SWIZZLING_RANGE SwizzlingRange;
    D3DKMT_QUERYSTATSTICS_LOCKS Locks;
    D3DKMT_QUERYSTATSTICS_ALLOCATIONS Allocations;
    D3DKMT_QUERYSTATSTICS_TERMINATIONS Terminations;
    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_ADAPTER_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_POLICY
{
    ULONGLONG PreferApertureForRead[D3DKMT_MaxAllocationPriorityClass];
    ULONGLONG PreferAperture[D3DKMT_MaxAllocationPriorityClass];
    ULONGLONG MemResetOnPaging;
    ULONGLONG RemovePagesFromWorkingSetOnPaging;
    ULONGLONG MigrationEnabled;
} D3DKMT_QUERYSTATISTICS_POLICY;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER_INFORMATION
{
    ULONG NbSegments;
    ULONG NodeCount;
    ULONG VidPnSourceCount;
    ULONG VirtualMemoryUsage;
    D3DKMT_QUERYSTATISTICS_DMA_BUFFER DmaBuffer;
    D3DKMT_QUERYSTATISTICS_COMMITMENT_DATA CommitmentData;
    D3DKMT_QUERYSTATISTICS_POLICY _Policy;
    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_VIDEO_MEMORY
{
    ULONG AllocsCommitted;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocsResidentInP[D3DKMT_QUERYSTATISTICS_SEGMENT_PREFERENCE_MAX];
    D3DKMT_QUERYSTATISTICS_COUNTER AllocsResidentInNonPreferred;
    ULONGLONG TotalBytesEvictedDueToPreparation;
} D3DKMT_QUERYSTATISTICS_VIDEO_MEMORY;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_POLICY
{
    ULONGLONG UseMRU;
} D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_POLICY;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_INFORMATION
{
    ULONGLONG BytesCommitted;
    ULONGLONG MaximumWorkingSet;
    ULONGLONG MinimumWorkingSet;

    ULONG NbReferencedAllocationEvictedInPeriod;

    D3DKMT_QUERYSTATISTICS_VIDEO_MEMORY VideoMemory;
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_POLICY _Policy;

    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_INFORMATION
{
    ULONG NodeCount;
    ULONG VidPnSourceCount;
    D3DKMT_QUERYSTATISTICS_SYSTEM_MEMORY SystemMemory;
    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION
{
    ULONGLONG CommitLimit;
    ULONGLONG BytesCommitted;
    ULONGLONG BytesResident;
    D3DKMT_QUERYSTATISTICS_MEMORY Memory;
    ULONG Aperture;
    ULONGLONG TotalBytesEvictedByPriority[D3DKMT_MaxAllocationPriorityClass];
    ULONG64 SystemMemoryEndAddress;
    struct
    {
        ULONG64 PreservedDuringStandby            :  1;
        ULONG64 PreservedDuringHibernate          :  1;
        ULONG64 PartiallyPreservedDuringHibernate :  1;
        ULONG64 Reserved                          : 61;
    } PowerFlags;
    ULONG64 Reserved[6];
} D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1
{
    ULONG CommitLimit;
    ULONG BytesCommitted;
    ULONG BytesResident;
    D3DKMT_QUERYSTATISTICS_MEMORY Memory;
    ULONG Aperture;
    ULONGLONG TotalBytesEvictedByPriority[D3DKMT_MaxAllocationPriorityClass];
    ULONG64 SystemMemoryEndAddress;
    struct
    {
       ULONG64 PreservedDuringStandby            :  1;
       ULONG64 PreservedDuringHibernate          :  1;
       ULONG64 PartiallyPreservedDuringHibernate :  1;
       ULONG64 Reserved                          : 61;
    } PowerFlags;

    ULONG64 Reserved[7];
} D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1;

typedef union _D3DKMT_QUERYSTATISTICS_RESULT
{
    D3DKMT_QUERYSTATISTICS_ADAPTER_INFORMATION             AdapterInformation;
    D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1          SegmentInformationV1;
    D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION             SegmentInformation;
    D3DKMT_QUERYSTATISTICS_NODE_INFORMATION                NodeInformation;
    D3DKMT_QUERYSTATISTICS_VIDPNSOURCE_INFORMATION         VidPnSourceInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_INFORMATION             ProcessInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER_INFORMATION     ProcessAdapterInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_INFORMATION     ProcessSegmentInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION        ProcessNodeInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION ProcessVidPnSourceInformation;
} D3DKMT_QUERYSTATISTICS_RESULT;

typedef struct _D3DKMT_QUERYSTATISTICS
{
    D3DKMT_QUERYSTATISTICS_TYPE Type;
    LUID AdapterLuid;
    HANDLE hProcess;
    D3DKMT_QUERYSTATISTICS_RESULT QueryResult;

    union
    {
        D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT     QuerySegment;
        D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT     QueryProcessSegment;
        D3DKMT_QUERYSTATISTICS_QUERY_NODE        QueryNode;
        D3DKMT_QUERYSTATISTICS_QUERY_NODE        QueryProcessNode;
        D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE QueryVidPnSource;
        D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE QueryProcessVidPnSource;
    } DUMMYUNIONNAME;
} D3DKMT_QUERYSTATISTICS;

typedef struct _D3DKMT_QUERYVIDEOMEMORYINFO
{
    HANDLE                      hProcess;
    D3DKMT_HANDLE               hAdapter;
    D3DKMT_MEMORY_SEGMENT_GROUP MemorySegmentGroup;
    UINT64                      Budget;
    UINT64                      CurrentUsage;
    UINT64                      CurrentReservation;
    UINT64                      AvailableForReservation;
    UINT                        PhysicalAdapterIndex;
} D3DKMT_QUERYVIDEOMEMORYINFO;

typedef enum _D3DKMT_QUEUEDLIMIT_TYPE
{
    D3DKMT_SET_QUEUEDLIMIT_PRESENT = 1,
    D3DKMT_GET_QUEUEDLIMIT_PRESENT
} D3DKMT_QUEUEDLIMIT_TYPE;

typedef struct _D3DKMT_SETQUEUEDLIMIT
{
    D3DKMT_HANDLE hDevice;
    D3DKMT_QUEUEDLIMIT_TYPE Type;
    union
    {
        UINT QueuedPresentLimit;
        struct
        {
            D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
            UINT QueuedPendingFlipLimit;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} D3DKMT_SETQUEUEDLIMIT;

typedef enum _D3DKMT_ESCAPETYPE
{
    D3DKMT_ESCAPE_DRIVERPRIVATE,
    D3DKMT_ESCAPE_VIDMM,
    D3DKMT_ESCAPE_TDRDBGCTRL,
    D3DKMT_ESCAPE_VIDSCH,
    D3DKMT_ESCAPE_DEVICE,
    D3DKMT_ESCAPE_DMM,
    D3DKMT_ESCAPE_DEBUG_SNAPSHOT,
    D3DKMT_ESCAPE_SETDRIVERUPDATESTATUS,
    D3DKMT_ESCAPE_DRT_TEST,
    D3DKMT_ESCAPE_DIAGNOSTICS,
} D3DKMT_ESCAPETYPE;

typedef struct _D3DKMT_ESCAPE
{
    D3DKMT_HANDLE      hAdapter;
    D3DKMT_HANDLE      hDevice;
    D3DKMT_ESCAPETYPE  Type;
    D3DDDI_ESCAPEFLAGS Flags;
    void              *pPrivateDriverData;
    UINT               PrivateDriverDataSize;
    D3DKMT_HANDLE      hContext;
} D3DKMT_ESCAPE;

typedef struct _D3DKMT_ADAPTERINFO
{
  D3DKMT_HANDLE hAdapter;
  LUID          AdapterLuid;
  ULONG         NumOfSources;
  BOOL          bPrecisePresentRegionsPreferred;
} D3DKMT_ADAPTERINFO;

#define MAX_ENUM_ADAPTERS 16

typedef struct _D3DKMT_ENUMADAPTERS
{
    ULONG              NumAdapters;
    D3DKMT_ADAPTERINFO Adapters[MAX_ENUM_ADAPTERS];
} D3DKMT_ENUMADAPTERS;

typedef struct _D3DKMT_ENUMADAPTERS2
{
  ULONG               NumAdapters;
  D3DKMT_ADAPTERINFO *pAdapters;
} D3DKMT_ENUMADAPTERS2;

typedef struct _D3DKMT_CREATEKEYEDMUTEX
{
    UINT64 InitialValue;
    D3DKMT_HANDLE hSharedHandle;
    D3DKMT_HANDLE hKeyedMutex;
} D3DKMT_CREATEKEYEDMUTEX;

typedef struct _D3DDDICB_SIGNALFLAGS
{
    union
    {
        struct
        {
            UINT SignalAtSubmission : 1;
            UINT EnqueueCpuEvent : 1;
            UINT AllowFenceRewind : 1;
            UINT Reserved : 28;
            UINT DXGK_SIGNAL_FLAG_INTERNAL0 : 1;
        };
        UINT Value;
    };
} D3DDDICB_SIGNALFLAGS;

typedef struct _D3DKMT_CREATEKEYEDMUTEX2_FLAGS
{
    union
    {
        struct
        {
            UINT NtSecuritySharing : 1;
            UINT Reserved : 31;
        };
        UINT Value;
    };
} D3DKMT_CREATEKEYEDMUTEX2_FLAGS;

typedef struct _D3DKMT_CREATEKEYEDMUTEX2
{
    UINT64 InitialValue;
    D3DKMT_HANDLE hSharedHandle;
    D3DKMT_HANDLE hKeyedMutex;
    void *pPrivateRuntimeData;
    UINT PrivateRuntimeDataSize;
    D3DKMT_CREATEKEYEDMUTEX2_FLAGS Flags;
} D3DKMT_CREATEKEYEDMUTEX2;

typedef struct _D3DKMT_DESTROYKEYEDMUTEX
{
    D3DKMT_HANDLE hKeyedMutex;
} D3DKMT_DESTROYKEYEDMUTEX;

typedef struct _D3DKMT_OPENKEYEDMUTEX
{
    D3DKMT_HANDLE hSharedHandle;
    D3DKMT_HANDLE hKeyedMutex;
} D3DKMT_OPENKEYEDMUTEX;

typedef struct _D3DKMT_OPENKEYEDMUTEX2
{
    D3DKMT_HANDLE hSharedHandle;
    D3DKMT_HANDLE hKeyedMutex;
    void *pPrivateRuntimeData;
    UINT PrivateRuntimeDataSize;
} D3DKMT_OPENKEYEDMUTEX2;

typedef struct _D3DKMT_OPENKEYEDMUTEXFROMNTHANDLE
{
    HANDLE hNtHandle;
    D3DKMT_HANDLE hKeyedMutex;
    void *pPrivateRuntimeData;
    UINT PrivateRuntimeDataSize;
} D3DKMT_OPENKEYEDMUTEXFROMNTHANDLE;

typedef ULONGLONG D3DGPU_VIRTUAL_ADDRESS;

#ifndef D3DDDI_SYNCHRONIZATIONOBJECT_FLAGS_EXT
#define D3DDDI_SYNCHRONIZATIONOBJECT_FLAGS_EXT
#define D3DDDI_SYNCHRONIZATIONOBJECT_FLAGS_RESERVED0 Reserved0
#endif

typedef struct _D3DDDI_SYNCHRONIZATIONOBJECT_FLAGS
{
    union
    {
        struct
        {
            UINT Shared : 1;
            UINT NtSecuritySharing : 1;
            UINT CrossAdapter : 1;
            UINT TopOfPipeline : 1;
            UINT NoSignal : 1;
            UINT NoWait : 1;
            UINT NoSignalMaxValueOnTdr : 1;
            UINT NoGPUAccess : 1;
            UINT Reserved : 23;
            UINT D3DDDI_SYNCHRONIZATIONOBJECT_FLAGS_RESERVED0 : 1;
        };
        UINT Value;
    };
} D3DDDI_SYNCHRONIZATIONOBJECT_FLAGS;

typedef UINT D3DDDI_VIDEO_PRESENT_TARGET_ID;

typedef enum _D3DDDI_SYNCHRONIZATIONOBJECT_TYPE
{
    D3DDDI_SYNCHRONIZATION_MUTEX = 1,
    D3DDDI_SEMAPHORE = 2,
    D3DDDI_FENCE = 3,
    D3DDDI_CPU_NOTIFICATION = 4,
    D3DDDI_MONITORED_FENCE = 5,
    D3DDDI_PERIODIC_MONITORED_FENCE = 6,
    D3DDDI_SYNCHRONIZATION_TYPE_LIMIT
} D3DDDI_SYNCHRONIZATIONOBJECT_TYPE;

typedef struct _D3DDDI_SYNCHRONIZATIONOBJECTINFO
{
    D3DDDI_SYNCHRONIZATIONOBJECT_TYPE Type;
    union
    {
        struct
        {
            BOOL InitialState;
        } SynchronizationMutex;
        struct
        {
            UINT MaxCount;
            UINT InitialCount;
        } Semaphore;
        struct
        {
            UINT Reserved[16];
        } Reserved;
    };
} D3DDDI_SYNCHRONIZATIONOBJECTINFO;

typedef struct _D3DKMT_CREATESYNCHRONIZATIONOBJECT
{
    D3DKMT_HANDLE hDevice;
    D3DDDI_SYNCHRONIZATIONOBJECTINFO Info;
    D3DKMT_HANDLE hSyncObject;
} D3DKMT_CREATESYNCHRONIZATIONOBJECT;

typedef struct _D3DDDI_SYNCHRONIZATIONOBJECTINFO2
{
    D3DDDI_SYNCHRONIZATIONOBJECT_TYPE Type;
    D3DDDI_SYNCHRONIZATIONOBJECT_FLAGS Flags;
    union
    {
        struct
        {
            BOOL InitialState;
        } SynchronizationMutex;
        struct
        {
            UINT MaxCount;
            UINT InitialCount;
        } Semaphore;
        struct
        {
            UINT64 FenceValue;
        } Fence;
        struct
        {
            HANDLE Event;
        } CPUNotification;
        struct
        {
            UINT64 InitialFenceValue;
            void *FenceValueCPUVirtualAddress;
            D3DGPU_VIRTUAL_ADDRESS FenceValueGPUVirtualAddress;
            UINT EngineAffinity;
        } MonitoredFence;
        struct
        {
            D3DKMT_HANDLE hAdapter;
            D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId;
            UINT64 Time;
            void *FenceValueCPUVirtualAddress;
            D3DGPU_VIRTUAL_ADDRESS FenceValueGPUVirtualAddress;
            UINT EngineAffinity;
        } PeriodicMonitoredFence;
        struct
        {
            UINT64 Reserved[8];
        } Reserved;
    };
    D3DKMT_HANDLE SharedHandle;
} D3DDDI_SYNCHRONIZATIONOBJECTINFO2;

typedef struct _D3DKMT_CREATESYNCHRONIZATIONOBJECT2
{
    D3DKMT_HANDLE hDevice;
    D3DDDI_SYNCHRONIZATIONOBJECTINFO2 Info;
    D3DKMT_HANDLE hSyncObject;
} D3DKMT_CREATESYNCHRONIZATIONOBJECT2;

typedef struct _D3DKMT_OPENSYNCOBJECTFROMNTHANDLE
{
    HANDLE hNtHandle;
    D3DKMT_HANDLE hSyncObject;
} D3DKMT_OPENSYNCOBJECTFROMNTHANDLE;

typedef struct _D3DKMT_OPENSYNCOBJECTFROMNTHANDLE2
{
    HANDLE hNtHandle;
    D3DKMT_HANDLE hDevice;
    D3DDDI_SYNCHRONIZATIONOBJECT_FLAGS Flags;
    D3DKMT_HANDLE hSyncObject;
    union
    {

        struct
        {
            void *FenceValueCPUVirtualAddress;
            D3DGPU_VIRTUAL_ADDRESS FenceValueGPUVirtualAddress;
            UINT EngineAffinity;
        } MonitoredFence;
        UINT64 Reserved[8];
    };
} D3DKMT_OPENSYNCOBJECTFROMNTHANDLE2;

typedef struct _D3DKMT_OPENSYNCHRONIZATIONOBJECT
{
    D3DKMT_HANDLE hSharedHandle;
    D3DKMT_HANDLE hSyncObject;
    UINT64 Reserved[8];
} D3DKMT_OPENSYNCHRONIZATIONOBJECT;

typedef struct _D3DKMT_OPENSYNCOBJECTNTHANDLEFROMNAME
{
    DWORD dwDesiredAccess;
    OBJECT_ATTRIBUTES *pObjAttrib;
    HANDLE hNtHandle;
} D3DKMT_OPENSYNCOBJECTNTHANDLEFROMNAME;

typedef struct _D3DKMT_DESTROYSYNCHRONIZATIONOBJECT
{
    D3DKMT_HANDLE hSyncObject;
} D3DKMT_DESTROYSYNCHRONIZATIONOBJECT;

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

typedef enum _D3DKMT_STANDARDALLOCATIONTYPE
{
    D3DKMT_STANDARDALLOCATIONTYPE_EXISTINGHEAP = 1,
} D3DKMT_STANDARDALLOCATIONTYPE;

typedef struct _D3DKMT_STANDARDALLOCATION_EXISTINGHEAP
{
    SIZE_T Size;
} D3DKMT_STANDARDALLOCATION_EXISTINGHEAP;

typedef struct _D3DKMT_CREATESTANDARDALLOCATION
{
    D3DKMT_STANDARDALLOCATIONTYPE Type;
    union
    {
        D3DKMT_STANDARDALLOCATION_EXISTINGHEAP ExistingHeapData;
    };
    D3DKMT_CREATESTANDARDALLOCATIONFLAGS Flags;
} D3DKMT_CREATESTANDARDALLOCATION;

typedef struct _D3DDDI_ALLOCATIONINFO
{
    D3DKMT_HANDLE hAllocation;
    const void *pSystemMem;
    void *pPrivateDriverData;
    UINT PrivateDriverDataSize;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    union
    {
        struct
        {
            UINT Primary : 1;
            UINT Stereo : 1;
            UINT Reserved : 30;
        };
        UINT Value;
    } Flags;
} D3DDDI_ALLOCATIONINFO;

typedef struct _D3DDDI_ALLOCATIONINFO2
{
    D3DKMT_HANDLE hAllocation;
    union
    {
        HANDLE hSection;
        const void *pSystemMem;
    };
    VOID *pPrivateDriverData;
    UINT PrivateDriverDataSize;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    union
    {
        struct
        {
            UINT Primary : 1;
            UINT Stereo : 1;
            UINT OverridePriority : 1;
            UINT Reserved : 29;
        };
        UINT Value;
    } Flags;
    D3DGPU_VIRTUAL_ADDRESS GpuVirtualAddress;
    union
    {
        UINT Priority;
        ULONG_PTR Unused;
    };
    ULONG_PTR Reserved[5];
} D3DDDI_ALLOCATIONINFO2;

typedef struct _D3DKMT_CREATEALLOCATIONFLAGS
{
    UINT CreateResource : 1;
    UINT CreateShared : 1;
    UINT NonSecure : 1;
    UINT CreateProtected : 1;
    UINT RestrictSharedAccess : 1;
    UINT ExistingSysMem : 1;
    UINT NtSecuritySharing : 1;
    UINT ReadOnly : 1;
    UINT CreateWriteCombined : 1;
    UINT CreateCached : 1;
    UINT SwapChainBackBuffer : 1;
    UINT CrossAdapter : 1;
    UINT OpenCrossAdapter : 1;
    UINT PartialSharedCreation : 1;
    UINT Zeroed : 1;
    UINT WriteWatch : 1;
    UINT StandardAllocation : 1;
    UINT ExistingSection : 1;
    UINT AllowNotZeroed : 1;
    UINT PhysicallyContiguous : 1;
    UINT Reserved : 12;
} D3DKMT_CREATEALLOCATIONFLAGS;

typedef struct _D3DKMT_CREATEALLOCATION
{
    D3DKMT_HANDLE hDevice;
    D3DKMT_HANDLE hResource;
    D3DKMT_HANDLE hGlobalShare;
    const void *pPrivateRuntimeData;
    UINT PrivateRuntimeDataSize;
    union
    {
        D3DKMT_CREATESTANDARDALLOCATION *pStandardAllocation;
        const void *pPrivateDriverData;
    };
    UINT PrivateDriverDataSize;
    UINT NumAllocations;
    union
    {
        D3DDDI_ALLOCATIONINFO *pAllocationInfo;
        D3DDDI_ALLOCATIONINFO2 *pAllocationInfo2;
    };
    D3DKMT_CREATEALLOCATIONFLAGS Flags;
    HANDLE hPrivateRuntimeResourceHandle;
} D3DKMT_CREATEALLOCATION;

typedef struct _D3DKMT_DESTROYALLOCATION
{
    D3DKMT_HANDLE hDevice;
    D3DKMT_HANDLE hResource;
    const D3DKMT_HANDLE *phAllocationList;
    UINT AllocationCount;
} D3DKMT_DESTROYALLOCATION;

typedef struct _D3DDDICB_DESTROYALLOCATION2FLAGS
{
    union
    {
        struct
        {
            UINT AssumeNotInUse : 1;
            UINT SynchronousDestroy : 1;
            UINT Reserved : 29;
            UINT SystemUseOnly : 1;
        };
        UINT Value;
    };
} D3DDDICB_DESTROYALLOCATION2FLAGS;

typedef struct _D3DKMT_DESTROYALLOCATION2
{
    D3DKMT_HANDLE hDevice;
    D3DKMT_HANDLE hResource;
    const D3DKMT_HANDLE *phAllocationList;
    UINT AllocationCount;
    D3DDDICB_DESTROYALLOCATION2FLAGS Flags;
} D3DKMT_DESTROYALLOCATION2;

typedef struct _D3DDDI_OPENALLOCATIONINFO
{
    D3DKMT_HANDLE hAllocation;
    const void *pPrivateDriverData;
    UINT PrivateDriverDataSize;
} D3DDDI_OPENALLOCATIONINFO;

typedef struct _D3DDDI_OPENALLOCATIONINFO2
{
    D3DKMT_HANDLE hAllocation;
    const void *pPrivateDriverData;
    UINT PrivateDriverDataSize;
    D3DGPU_VIRTUAL_ADDRESS GpuVirtualAddress;
    ULONG_PTR Reserved[6];
} D3DDDI_OPENALLOCATIONINFO2;

typedef struct _D3DKMT_OPENRESOURCE
{
    D3DKMT_HANDLE hDevice;
    D3DKMT_HANDLE hGlobalShare;
    UINT NumAllocations;
    union
    {
        D3DDDI_OPENALLOCATIONINFO *pOpenAllocationInfo;
        D3DDDI_OPENALLOCATIONINFO2 *pOpenAllocationInfo2;
    };
    void *pPrivateRuntimeData;
    UINT PrivateRuntimeDataSize;
    void *pResourcePrivateDriverData;
    UINT ResourcePrivateDriverDataSize;
    void *pTotalPrivateDriverDataBuffer;
    UINT TotalPrivateDriverDataBufferSize;
    D3DKMT_HANDLE hResource;
} D3DKMT_OPENRESOURCE;

typedef struct _D3DKMT_OPENRESOURCEFROMNTHANDLE
{
    D3DKMT_HANDLE hDevice;
    HANDLE hNtHandle;
    UINT NumAllocations;
    D3DDDI_OPENALLOCATIONINFO2 *pOpenAllocationInfo2;
    UINT PrivateRuntimeDataSize;
    void *pPrivateRuntimeData;
    UINT ResourcePrivateDriverDataSize;
    void *pResourcePrivateDriverData;
    UINT TotalPrivateDriverDataBufferSize;
    void *pTotalPrivateDriverDataBuffer;
    D3DKMT_HANDLE hResource;
    D3DKMT_HANDLE hKeyedMutex;
    void *pKeyedMutexPrivateRuntimeData;
    UINT KeyedMutexPrivateRuntimeDataSize;
    D3DKMT_HANDLE hSyncObject;
} D3DKMT_OPENRESOURCEFROMNTHANDLE;

typedef struct _D3DKMT_QUERYRESOURCEINFO
{
    D3DKMT_HANDLE hDevice;
    D3DKMT_HANDLE hGlobalShare;
    void *pPrivateRuntimeData;
    UINT PrivateRuntimeDataSize;
    UINT TotalPrivateDriverDataSize;
    UINT ResourcePrivateDriverDataSize;
    UINT NumAllocations;
} D3DKMT_QUERYRESOURCEINFO;

typedef struct _D3DKMT_QUERYRESOURCEINFOFROMNTHANDLE
{
    D3DKMT_HANDLE hDevice;
    HANDLE hNtHandle;
    void *pPrivateRuntimeData;
    UINT PrivateRuntimeDataSize;
    UINT TotalPrivateDriverDataSize;
    UINT ResourcePrivateDriverDataSize;
    UINT NumAllocations;
} D3DKMT_QUERYRESOURCEINFOFROMNTHANDLE;

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

NTSTATUS WINAPI D3DKMTCheckVidPnExclusiveOwnership(const D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP *desc);
NTSTATUS WINAPI D3DKMTCloseAdapter(const D3DKMT_CLOSEADAPTER *desc);
NTSTATUS WINAPI D3DKMTCreateAllocation( D3DKMT_CREATEALLOCATION *params );
NTSTATUS WINAPI D3DKMTCreateAllocation2( D3DKMT_CREATEALLOCATION *params );
NTSTATUS WINAPI D3DKMTCreateDCFromMemory(D3DKMT_CREATEDCFROMMEMORY *desc);
NTSTATUS WINAPI D3DKMTCreateDevice(D3DKMT_CREATEDEVICE *desc);
NTSTATUS WINAPI D3DKMTCreateKeyedMutex( D3DKMT_CREATEKEYEDMUTEX *params );
NTSTATUS WINAPI D3DKMTCreateKeyedMutex2( D3DKMT_CREATEKEYEDMUTEX2 *params );
NTSTATUS WINAPI D3DKMTCreateSynchronizationObject( D3DKMT_CREATESYNCHRONIZATIONOBJECT *params );
NTSTATUS WINAPI D3DKMTCreateSynchronizationObject2( D3DKMT_CREATESYNCHRONIZATIONOBJECT2 *params );
NTSTATUS WINAPI D3DKMTDestroyAllocation( const D3DKMT_DESTROYALLOCATION *params );
NTSTATUS WINAPI D3DKMTDestroyAllocation2( const D3DKMT_DESTROYALLOCATION2 *params );
NTSTATUS WINAPI D3DKMTDestroyDCFromMemory(const D3DKMT_DESTROYDCFROMMEMORY *desc);
NTSTATUS WINAPI D3DKMTDestroyDevice(const D3DKMT_DESTROYDEVICE *desc);
NTSTATUS WINAPI D3DKMTDestroyKeyedMutex( const D3DKMT_DESTROYKEYEDMUTEX *params );
NTSTATUS WINAPI D3DKMTDestroySynchronizationObject( const D3DKMT_DESTROYSYNCHRONIZATIONOBJECT *params );
NTSTATUS WINAPI D3DKMTEnumAdapters2(D3DKMT_ENUMADAPTERS2 *desc);
NTSTATUS WINAPI D3DKMTEscape( const D3DKMT_ESCAPE *desc );
NTSTATUS WINAPI D3DKMTOpenAdapterFromGdiDisplayName(D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME *desc);
NTSTATUS WINAPI D3DKMTOpenAdapterFromHdc( D3DKMT_OPENADAPTERFROMHDC *desc );
NTSTATUS WINAPI D3DKMTOpenAdapterFromLuid( D3DKMT_OPENADAPTERFROMLUID * desc );
NTSTATUS WINAPI D3DKMTOpenKeyedMutex( D3DKMT_OPENKEYEDMUTEX *params );
NTSTATUS WINAPI D3DKMTOpenKeyedMutex2( D3DKMT_OPENKEYEDMUTEX2 *params );
NTSTATUS WINAPI D3DKMTOpenKeyedMutexFromNtHandle( D3DKMT_OPENKEYEDMUTEXFROMNTHANDLE *params );
NTSTATUS WINAPI D3DKMTOpenResource( D3DKMT_OPENRESOURCE *params );
NTSTATUS WINAPI D3DKMTOpenResource2( D3DKMT_OPENRESOURCE *params );
NTSTATUS WINAPI D3DKMTOpenResourceFromNtHandle( D3DKMT_OPENRESOURCEFROMNTHANDLE *params );
NTSTATUS WINAPI D3DKMTOpenSynchronizationObject( D3DKMT_OPENSYNCHRONIZATIONOBJECT *params );
NTSTATUS WINAPI D3DKMTOpenSyncObjectFromNtHandle( D3DKMT_OPENSYNCOBJECTFROMNTHANDLE *params );
NTSTATUS WINAPI D3DKMTOpenSyncObjectFromNtHandle2( D3DKMT_OPENSYNCOBJECTFROMNTHANDLE2 *params );
NTSTATUS WINAPI D3DKMTOpenSyncObjectNtHandleFromName( D3DKMT_OPENSYNCOBJECTNTHANDLEFROMNAME *params );
NTSTATUS WINAPI D3DKMTQueryAdapterInfo(D3DKMT_QUERYADAPTERINFO *desc);
NTSTATUS WINAPI D3DKMTQueryResourceInfo( D3DKMT_QUERYRESOURCEINFO *params );
NTSTATUS WINAPI D3DKMTQueryResourceInfoFromNtHandle( D3DKMT_QUERYRESOURCEINFOFROMNTHANDLE *params );
NTSTATUS WINAPI D3DKMTQueryStatistics(D3DKMT_QUERYSTATISTICS *stats);
NTSTATUS WINAPI D3DKMTQueryVideoMemoryInfo(D3DKMT_QUERYVIDEOMEMORYINFO *desc);
NTSTATUS WINAPI D3DKMTSetQueuedLimit(D3DKMT_SETQUEUEDLIMIT *desc);
NTSTATUS WINAPI D3DKMTSetVidPnSourceOwner(const D3DKMT_SETVIDPNSOURCEOWNER *desc);
NTSTATUS WINAPI D3DKMTShareObjects( UINT count, const D3DKMT_HANDLE *handles, OBJECT_ATTRIBUTES *attr, UINT access, HANDLE *handle );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WINE_D3DKMTHK_H */
