/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    haltypes.h

Abstract:

    Type definitions for the HAL.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _HALTYPES_H
#define _HALTYPES_H

//
// Dependencies
//
#include <umtypes.h>

#ifndef NTOS_MODE_USER

typedef struct _LOADER_PARAMETER_BLOCK *PLOADER_PARAMETER_BLOCK;
typedef struct _KPRCB *PKPRCB;

typedef struct _KTB_FLUSH_VA
{
    union {
        struct {
            ULONG_PTR NumberOfEntries : 11;
            ULONG_PTR PageSize : 1;
        };
        PVOID Va;
        ULONG_PTR VaLong;
    } u1;
} KTB_FLUSH_VA, *PKTB_FLUSH_VA;

typedef struct _INTERRUPT_REMAPPING_INFO
{
    ULONG IrtIndex : 30;
    ULONG FlagHalInternal : 1;
    ULONG FlagTranslated : 1;
    union {
        ULARGE_INTEGER RemappedFormat;
        struct {
            ULONG   MessageAddressLow;
            USHORT  MessageData;
            USHORT  Reserved;
        } Msi;
    } u;
} INTERRUPT_REMAPPING_INFO, *PINTERRUPT_REMAPPING_INFO;

typedef enum {
    InterruptTypeControllerInput,
    InterruptTypeXapicMessage,
    InterruptTypeHypertransport,
    InterruptTypeMessageRequest
} INTERRUPT_CONNECTION_TYPE;

typedef struct _INTERRUPT_HT_INTR_INFO
{
    union {
        struct {
            ULONG Mask: 1 ;
            ULONG Polarity : 1;
            ULONG MessageType : 3;
            ULONG RequestEOI : 1;
            ULONG DestinationMode : 1;
            ULONG MessageType3 : 1;
            ULONG Destination : 8;
            ULONG Vector : 8;
            ULONG ExtendedAddress : 8;
        } bits;
        ULONG AsULONG;
    } LowPart;
    union {
        struct {
            ULONG ExtendedDestination : 24;
            ULONG Reserved : 6;
            ULONG PassPW : 1;
            ULONG WaitingForEOI : 1;
        } bits;
        ULONG AsULONG;
    } HighPart;
} INTERRUPT_HT_INTR_INFO, *PINTERRUPT_HT_INTR_INFO;

typedef struct _INTERRUPT_VECTOR_DATA
{
    INTERRUPT_CONNECTION_TYPE Type;
    ULONG Vector;
    KIRQL Irql;
    KINTERRUPT_POLARITY Polarity;
    KINTERRUPT_MODE Mode;
    GROUP_AFFINITY TargetProcessors;
    INTERRUPT_REMAPPING_INFO IntRemapInfo;
#if (NTDDI_VERSION >= NTDDI_WIN10)
    struct {
        ULONG Gsiv;
        ULONG WakeInterrupt : 1;
        ULONG ReservedFlags : 31;
    } ControllerInput;
    union {
#else
    union {
        struct {
            ULONG Gsiv;
            ULONG WakeInterrupt : 1;
            ULONG ReservedFlags : 31;
        } ControllerInput;
#endif
        struct {
            PHYSICAL_ADDRESS Address;
            ULONG DataPayload;
        } XapicMessage;
        struct {
            INTERRUPT_HT_INTR_INFO IntrInfo;
        } Hypertransport;
        struct {
            PHYSICAL_ADDRESS Address;
            ULONG DataPayload;
        } GenericMessage;
        struct {
            HAL_APIC_DESTINATION_MODE DestinationMode;
        } MessageRequest;
    };
} INTERRUPT_VECTOR_DATA, *PINTERRUPT_VECTOR_DATA;

typedef struct _INTERRUPT_CONNECTION_DATA
{
    ULONG Count;
#if (NTDDI_VERSION < NTDDI_WIN10)
    GROUP_AFFINITY OriginalAffinity;
    LIST_ENTRY SteeringListEntry;
    VOID* SteeringListRoot;
    ULONGLONG IsrTime;
    ULONGLONG DpcTime;
    ULONG IsrLoad;
    ULONG DpcLoad;
    UCHAR IsPrimaryInterrupt;
    PKINTERRUPT *InterruptObjectArray;
    ULONG InterruptObjectCount;
#endif
    INTERRUPT_VECTOR_DATA Vectors[1];
} INTERRUPT_CONNECTION_DATA, *PINTERRUPT_CONNECTION_DATA;

//
// HalShutdownSystem Types
//
typedef enum _FIRMWARE_REENTRY
{
    HalHaltRoutine,
    HalPowerDownRoutine,
    HalRestartRoutine,
    HalRebootRoutine,
    HalInteractiveModeRoutine,
    HalMaximumRoutine
} FIRMWARE_REENTRY, *PFIRMWARE_REENTRY;

//
// HAL Private function Types
//
typedef
PBUS_HANDLER
(FASTCALL *pHalHandlerForConfigSpace)(
    _In_ BUS_DATA_TYPE ConfigSpace,
    _In_ ULONG BusNumber
);

typedef
NTSTATUS
(NTAPI *PINSTALL_BUS_HANDLER)(
    _In_ PBUS_HANDLER Bus
);

typedef
NTSTATUS
(NTAPI *pHalRegisterBusHandler)(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ BUS_DATA_TYPE ConfigSpace,
    _In_ ULONG BusNumber,
    _In_ INTERFACE_TYPE ParentInterfaceType,
    _In_ ULONG ParentBusNumber,
    _In_ ULONG ContextSize,
    _In_ PINSTALL_BUS_HANDLER InstallCallback,
    _Out_ PBUS_HANDLER *BusHandler
);

typedef
VOID
(NTAPI *pHalSetWakeEnable)(
    _In_ BOOLEAN Enable
);

#if (NTDDI_VERSION >= NTDDI_WIN8)
typedef
VOID
(NTAPI *pHalSetWakeAlarm)(
    _In_ ULONGLONG AlartTime,
    _In_ ULONGLONG DcWakeTime
);
#else
typedef
VOID
(NTAPI *pHalSetWakeAlarm)(
    _In_ ULONGLONG AlartTime,
    _In_ PTIME_FIELDS TimeFields
);
#endif

typedef
VOID
(NTAPI *pHalLocateHiberRanges)(
    _In_ PVOID MemoryMap
);

typedef
NTSTATUS
(NTAPI *pHalAllocateMapRegisters)(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ ULONG NumberOfMapRegisters,
    _In_ ULONG BaseAddressCount,
    _Out_ PMAP_REGISTER_ENTRY MapRegisterArray
);

typedef
ULONG
(NTAPI *pHalGetInterruptVector)(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber,
    _In_ ULONG BusInterruptLevel,
    _In_ ULONG BusInterruptVector,
    _Out_ PKIRQL Irql,
    _Out_ PKAFFINITY Affinity
);

#if (NTDDI_VERSION >= NTDDI_WIN7)
typedef
NTSTATUS
(NTAPI *pHalGetVectorInput)(
    _In_ ULONG Vector,
    _In_ PGROUP_AFFINITY Affinity,
    _Out_ PULONG Input,
    _Out_ PKINTERRUPT_POLARITY Polarity,
    _Out_ PINTERRUPT_REMAPPING_INFO IntRemapInfo
);
#else
typedef
NTSTATUS
(NTAPI *pHalGetVectorInput)(
    _In_ ULONG Vector,
    _In_ KAFFINITY Affinity,
    _Out_ PULONG Input,
    _Out_ PKINTERRUPT_POLARITY Polarity
);
#endif

typedef
NTSTATUS
(NTAPI *pHalLoadMicrocode)(
    _In_ PVOID ImageHandle
);

typedef
NTSTATUS
(NTAPI *pHalUnloadMicrocode)(
    VOID
);

typedef
NTSTATUS
(NTAPI *pHalPostMicrocodeUpdate)(
    VOID
);

#if (NTDDI_VERSION >= NTDDI_WIN7)
typedef
NTSTATUS
(NTAPI *pHalAllocateMessageTarget)(
    _In_ PDEVICE_OBJECT Owner,
    _In_ PGROUP_AFFINITY ProcessorSet,
    _In_ ULONG NumberOfIdtEntries,
    _In_ KINTERRUPT_MODE Mode,
    _In_ BOOLEAN ShareVector,
    _Out_ PULONG Vector,
    _Out_ PKIRQL Irql,
    _Out_ PULONG IdtEntry
);

typedef
VOID
(NTAPI *pHalFreeMessageTarget)(
    _In_ PDEVICE_OBJECT Owner,
    _In_ ULONG Vector,
    _In_ PGROUP_AFFINITY ProcessorSet
);
#else
typedef
NTSTATUS
(NTAPI *pHalAllocateMessageTarget)(
    _In_ PDEVICE_OBJECT Owner,
    _In_ KAFFINITY ProcessorSet,
    _In_ ULONG NumberOfIdtEntries,
    _In_ KINTERRUPT_MODE Mode,
    _In_ BOOLEAN ShareVector,
    _Out_ PULONG Vector,
    _Out_ PKIRQL Irql,
    _Out_ PULONG IdtEntry
);

typedef
VOID
(NTAPI *pHalFreeMessageTarget)(
    _In_ PDEVICE_OBJECT Owner,
    _In_ ULONG Vector,
    _In_ KAFFINITY ProcessorSet
);
#endif

typedef struct _PNP_REPLACE_PROCESSOR_LIST *PPNP_REPLACE_PROCESSOR_LIST;

typedef struct _HAL_DP_REPLACE_PARAMETERS
{
    ULONG Flags;
    PPNP_REPLACE_PROCESSOR_LIST TargetProcessors;
    PPNP_REPLACE_PROCESSOR_LIST SpareProcessors;
} HAL_DP_REPLACE_PARAMETERS, *PHAL_DP_REPLACE_PARAMETERS;

typedef
NTSTATUS
(NTAPI *pHalDpReplaceBegin)(
    _In_ PHAL_DP_REPLACE_PARAMETERS Parameters,
    _Outptr_ PVOID *ReplaceContext
);

typedef
VOID
(NTAPI *pHalDpReplaceTarget)(
    _In_ PVOID ReplaceContext
);

typedef
NTSTATUS
(NTAPI *pHalDpReplaceControl)(
    _In_ ULONG Phase,
    _In_ PVOID ReplaceContext
);

typedef
VOID
(NTAPI *pHalDpReplaceEnd)(
    _In_ PVOID ReplaceContext
);

typedef
NTSTATUS
(NTAPI *pHalDpMaskLevelTriggeredInterrupts)(
    VOID
);

typedef
NTSTATUS
(NTAPI *pHalDpUnmaskLevelTriggeredInterrupts)(
    VOID
);

typedef
NTSTATUS
(NTAPI *pHalDpGetInterruptReplayState)(
    _In_ PVOID ReplaceContext,
    _Outptr_ PVOID *Buffer
);

typedef
NTSTATUS
(NTAPI *pHalDpReplayInterrupts)(
    _In_ PVOID InterruptState
);

typedef
VOID
(NTAPI *pHalPrepareForBugcheck)(
    _In_ ULONG Flags
);

#if (NTDDI_VERSION >= NTDDI_WIN8)
typedef
BOOLEAN
(NTAPI *pHalQueryWakeTime)(
    _Out_ PULONGLONG WakeTime,
    _Out_opt_ PULONGLONG TscOffset
);
#else
typedef
BOOLEAN
(NTAPI *pHalQueryWakeTime)(
    _Out_ PULONGLONG WakeTime
);
#endif

typedef
BOOLEAN
(NTAPI *pHalQueryIoPortAccessSupported)(
    VOID
);

typedef
VOID
(NTAPI *pHalReportIdleStateUsage)(
    _In_ UCHAR DeepestHardwareIdleState,
    _In_ PKAFFINITY_EX TargetSet
);

typedef
VOID
(NTAPI *pHalTscSynchronization)(
    _In_ BOOLEAN ForcedSynchronization,
    _In_opt_ PULONG TargetProcessor
);

typedef struct _WHEA_ERROR_RECORD_SECTION_DESCRIPTOR
    *PWHEA_ERROR_RECORD_SECTION_DESCRIPTOR;

typedef struct _WHEA_PROCESSOR_GENERIC_ERROR_SECTION
    *PWHEA_PROCESSOR_GENERIC_ERROR_SECTION;

typedef
NTSTATUS
(NTAPI *pHalWheaInitProcessorGenericSection)(
    _Out_ PWHEA_ERROR_RECORD_SECTION_DESCRIPTOR Descriptor,
    _Out_ PWHEA_PROCESSOR_GENERIC_ERROR_SECTION Section
);

#if (NTDDI_VERSION >= NTDDI_WIN8)
typedef
VOID
(NTAPI *pHalStopLegacyUsbInterrupts)(
    _In_ SYSTEM_POWER_STATE LastSystemState
);
#else
typedef
VOID
(NTAPI *pHalStopLegacyUsbInterrupts)(
    VOID
);
#endif

typedef
NTSTATUS
(NTAPI *pHalReadWheaPhysicalMemory)(
    _In_ PHYSICAL_ADDRESS PhysicalAddress,
    _In_ ULONG Length,
    _Out_writes_bytes_(Length) PVOID Data
);

typedef
NTSTATUS
(NTAPI *pHalWriteWheaPhysicalMemory)(
    _In_ PHYSICAL_ADDRESS PhysicalAddress,
    _In_ ULONG Length,
    _In_reads_bytes_(Length) PVOID Data
);

typedef ULONG HAL_HV_LOGICAL_PROCESSOR_INDEX, *PHAL_HV_LOGICAL_PROCESSOR_INDEX;

typedef
VOID
(NTAPI *PHAL_ENLIGHTENMENT_EOI)(
    VOID
);

typedef
VOID
(NTAPI *PHAL_ENLIGHTENMENT_WRITE_ICR)(
    _In_ ULONG Target,
    _In_ ULONG Command
);

typedef
VOID
(NTAPI *PHAL_LONG_SPIN_WAIT)(
    _In_ ULONG SpinCount
);

typedef
ULONG64
(NTAPI *PHAL_GET_REFERENCE_TIME)(
    VOID
);

typedef
NTSTATUS
(NTAPI *PHAL_SET_SYSTEM_SLEEP_PROPERTY)(
    _In_ UINT32 SleepState,
    _In_ UINT8 Pm1a_SLP_TYP,
    _In_ UINT8 Pm1b_SLP_TYP
);

typedef
NTSTATUS
(NTAPI *PHAL_SET_SYSTEM_MACHINE_CHECK_PROPERTY)(
    _In_ PVOID MachineCheckPropertyInfo
);

typedef
NTSTATUS
(NTAPI *PHAL_ENTER_SLEEP_STATE)(
    _In_ UINT32 SleepState
);

typedef
NTSTATUS
(NTAPI *PHAL_NOTIFY_DEBUG_DEVICE_AVAILABLE)(
    VOID
);

typedef
NTSTATUS
(NTAPI *PHAL_MAP_DEVICE_INTERRUPT)(
    _In_ ULONGLONG DeviceId,
    _In_ PVOID InterruptDescriptor,
    _In_opt_ PGROUP_AFFINITY TargetProcessors,
    _Out_ PVOID InterruptEntry
);

typedef
NTSTATUS
(NTAPI *PHAL_UNMAP_DEVICE_INTERRUPT)(
    _In_ ULONGLONG DeviceId,
    _In_ PVOID InterruptEntry
);

typedef
NTSTATUS
(NTAPI *PHAL_RETARGET_DEVICE_INTERRUPT)(
    _In_ ULONGLONG DeviceId,
    _In_ PVOID InterruptEntry,
    _In_ PVOID InterruptTarget,
    _In_ PGROUP_AFFINITY TargetProcessors,
    _Out_opt_ PVOID NewInterruptEntry
);

typedef
NTSTATUS
(NTAPI *PHAL_SET_HPET_CONFIG)(
    _In_ PHYSICAL_ADDRESS BaseAddress,
    _In_ ULONG TimerIndex,
    _In_ UINT64 DeviceId,
    _In_ UCHAR TimerInterruptPin,
    _Out_ PVOID InterruptEntry
);

typedef
NTSTATUS
(NTAPI *PHAL_NOTIFY_HPET_ENABLED)(
    VOID
);

typedef
NTSTATUS
(NTAPI *PHAL_QUERY_ASSOCIATED_PROCESSORS)(
    _In_ ULONG VpIndex,
    _Inout_ PULONG Count,
    _Out_writes_to_opt_(*Count, *Count) PHAL_HV_LOGICAL_PROCESSOR_INDEX PpIndices
);

typedef
NTSTATUS
(NTAPI *PHAL_LP_READ_MULTIPLE_MSR)(
    _In_ HAL_HV_LOGICAL_PROCESSOR_INDEX HvLpIndex,
    _In_ ULONG Count,
    _In_reads_(Count) PULONG MsrIndices,
    _Out_writes_(Count) PULONG64 MsrValues
);

typedef
NTSTATUS
(NTAPI *PHAL_LP_WRITE_MULTIPLE_MSR)(
    _In_ HAL_HV_LOGICAL_PROCESSOR_INDEX HvLpIndex,
    _In_ ULONG Count,
    _In_reads_(Count) PULONG MsrIndices,
    _In_reads_(Count) PULONG64 MsrValues
);

typedef
NTSTATUS
(NTAPI *PHAL_LP_READ_CPUID)(
    _In_ HAL_HV_LOGICAL_PROCESSOR_INDEX HvLpIndex,
    _In_ ULONG InEax,
    _Out_ PULONG OutEax,
    _Out_ PULONG OutEbx,
    _Out_ PULONG OutEcx,
    _Out_ PULONG OutEdx
);

typedef
NTSTATUS
(NTAPI *PHAL_LP_WRITEBACK_INVALIDATE)(
    _In_ HAL_HV_LOGICAL_PROCESSOR_INDEX HvLpIndex
);

typedef
NTSTATUS
(NTAPI *PHAL_LP_GET_MACHINE_CHECK_CONTEXT)(
    _In_ HAL_HV_LOGICAL_PROCESSOR_INDEX HvLpIndex,
    _Out_ UINT32 *Source,
    _Out_ UINT64 *PartitionId,
    _Out_ UINT32 *VpIndex
);

typedef
NTSTATUS
(NTAPI *PHAL_SUSPEND_PARTITION)(
    _In_ UINT64 PartitionId
);

typedef
NTSTATUS
(NTAPI *PHAL_RESUME_PARTITION)(
    _In_ UINT64 PartitionId
);

typedef struct _WHEA_RECOVERY_CONTEXT
    *PWHEA_RECOVERY_CONTEXT;

typedef
NTSTATUS
(NTAPI *PHAL_WHEA_ERROR_NOTIFICATION)(
    _In_ PWHEA_RECOVERY_CONTEXT RecoveryContext,
    _In_ BOOLEAN PlatformDirected,
    _In_ BOOLEAN Poisoned
);

typedef
ULONG
(NTAPI *PHAL_GET_PROCESSOR_INDEX_FROM_VP_INDEX)(
    _In_ ULONG VpIndex
);

typedef
NTSTATUS
(NTAPI *PHAL_SYNTHETIC_CLUSTER_IPI)(
    _In_ PKAFFINITY_EX Affinity,
    _In_ ULONG Vector
);

typedef
BOOLEAN
(NTAPI *PHAL_VP_START_ENABLED)(
    VOID
);

typedef
NTSTATUS
(NTAPI *PHAL_START_VIRTUAL_PROCESSOR)(
    _In_ ULONG VpIndex,
    _In_ PVOID Context
);

typedef
NTSTATUS
(NTAPI *PHAL_GET_VP_INDEX_FROM_APIC_ID)(
    _In_ ULONG ApicId,
    _Out_ PULONG VpIndex
);

typedef struct _HAL_HV_SVM_SYSTEM_CAPABILITIES
{
    struct {
        ULONG SvmSupported : 1;
        ULONG GpaAlwaysValid : 1;
    };
    ULONG MaxPasidSpaceCount;
    ULONG MaxPasidSpacePasidCount;
    ULONG MaxPrqSize;
    ULONG IommuCount;
    ULONG MinIommuPasidCount;
} HAL_HV_SVM_SYSTEM_CAPABILITIES, *PHAL_HV_SVM_SYSTEM_CAPABILITIES;

typedef
VOID
(NTAPI *PHAL_SVM_GET_SYSTEM_CAPABILITIES)(
    _Out_ PHAL_HV_SVM_SYSTEM_CAPABILITIES Capabilities
);

typedef
SIZE_T
(NTAPI *PHAL_IUM_EFI_RUNTIME_SERVICE)(
    _In_ ULONG Service,
    _Inout_updates_bytes_(Size) PVOID Data,
    _In_ ULONGLONG Size,
    _Inout_opt_ ULONGLONG Parameters[]
);

typedef struct _HAL_HV_SVM_DEVICE_CAPABILITIES
{
    struct {
        ULONG SvmSupported : 1;
        ULONG PciExecute : 1;
        ULONG NoExecute : 1;
        ULONG Reserved : 28;
        ULONG OverflowPossible : 1;
    };
    ULONG PasidCount;
    ULONG IommuIndex;
} HAL_HV_SVM_DEVICE_CAPABILITIES, *PHAL_HV_SVM_DEVICE_CAPABILITIES;

typedef
NTSTATUS
(NTAPI *PHAL_SVM_GET_DEVICE_CAPABILITIES)(
    _In_ ULONG DeviceId,
    _Out_ PHAL_HV_SVM_DEVICE_CAPABILITIES Capabilities
);

typedef
NTSTATUS
(NTAPI *PHAL_SVM_CREATE_PASID_SPACE)(
    _In_ ULONG PasidSpaceId,
    _In_ ULONG PasidCount
);

typedef
NTSTATUS
(NTAPI *PHAL_SVM_SET_PASID_ADDRESS_SPACE)(
    _In_ ULONG PasidSpaceId,
    _In_ ULONG Pasid,
    _In_ ULONGLONG AddressSpace
);

typedef
VOID
(NTAPI *PHAL_SVM_FLUSH_PASID)(
    _In_ ULONG PasidSpaceId,
    _In_ ULONG Pasid,
    _In_ ULONG Number,
    _In_reads_opt_(Number) KTB_FLUSH_VA Virtual[]
);

typedef
NTSTATUS
(NTAPI *PHAL_SVM_ATTACH_PASID_SPACE)(
    _In_ ULONG DeviceId,
    _In_ ULONG PasidSpaceId,
    _In_ ULONG PrqId,
    _In_ ULONG PciCapabilities
);

typedef
NTSTATUS
(NTAPI *PHAL_SVM_DETACH_PASID_SPACE)(
    _In_ ULONG DeviceId
);

typedef
NTSTATUS
(NTAPI *PHAL_SVM_ENABLE_PASID)(
    _In_ ULONG DeviceId,
    _In_ ULONG Pasid
);

typedef
NTSTATUS
(NTAPI *PHAL_SVM_DISABLE_PASID)(
    _In_ ULONG DeviceId,
    _In_ ULONG Pasid
);

typedef
NTSTATUS
(NTAPI *PHAL_SVM_ACKNOWLEDGE_PAGE_REQUEST)(
    _In_ ULONG Count,
    _In_ PVOID PageRequestList,
    _Out_opt_ PULONG Processed
);

typedef
NTSTATUS
(NTAPI *PHAL_SVM_CREATE_PR_QUEUE)(
    _In_ ULONG QueueId,
    _In_ ULONG Size,
    _In_ PHYSICAL_ADDRESS BaseAddress,
    _In_ ULONG InterruptVector,
    _In_ ULONG InterruptProcessorIndex
);

typedef
NTSTATUS
(NTAPI *PHAL_SVM_DELETE_PR_QUEUE)(
    _In_ ULONG QueueId
);

typedef
NTSTATUS
(NTAPI *PHAL_SVM_CLEAR_PRQ_STALLED)(
    _In_ ULONG QueueId
);

typedef
NTSTATUS
(NTAPI *PHAL_SVM_SET_DEVICE_ENABLED)(
    _In_ ULONG DeviceId,
    _In_ BOOLEAN Enabled
);

#if !defined(_ARM64_) && !defined(_ARM_)
typedef struct _HAL_INTEL_ENLIGHTENMENT_INFORMATION
{
    ULONG Enlightenments;
    LOGICAL HypervisorConnected;
    PHAL_ENLIGHTENMENT_EOI EndOfInterrupt;
    PHAL_ENLIGHTENMENT_WRITE_ICR ApicWriteIcr;
    ULONG Reserved0;
    ULONG SpinCountMask;
    PHAL_LONG_SPIN_WAIT LongSpinWait;
    PHAL_GET_REFERENCE_TIME GetReferenceTime;
    PHAL_SET_SYSTEM_SLEEP_PROPERTY SetSystemSleepProperty;
    PHAL_ENTER_SLEEP_STATE EnterSleepState;
    PHAL_NOTIFY_DEBUG_DEVICE_AVAILABLE NotifyDebugDeviceAvailable;
    PHAL_MAP_DEVICE_INTERRUPT MapDeviceInterrupt;
    PHAL_UNMAP_DEVICE_INTERRUPT UnmapDeviceInterrupt;
    PHAL_RETARGET_DEVICE_INTERRUPT RetargetDeviceInterrupt;
    PHAL_SET_HPET_CONFIG SetHpetConfig;
    PHAL_NOTIFY_HPET_ENABLED NotifyHpetEnabled;
    PHAL_QUERY_ASSOCIATED_PROCESSORS QueryAssociatedProcessors;
    PHAL_LP_READ_MULTIPLE_MSR ReadMultipleMsr;
    PHAL_LP_WRITE_MULTIPLE_MSR WriteMultipleMsr;
    PHAL_LP_READ_CPUID ReadCpuid;
    PHAL_LP_WRITEBACK_INVALIDATE LpWritebackInvalidate;
    PHAL_LP_GET_MACHINE_CHECK_CONTEXT GetMachineCheckContext;
    PHAL_SUSPEND_PARTITION SuspendPartition;
    PHAL_RESUME_PARTITION ResumePartition;
    PHAL_SET_SYSTEM_MACHINE_CHECK_PROPERTY SetSystemMachineCheckProperty;
    PHAL_WHEA_ERROR_NOTIFICATION WheaErrorNotification;
    PHAL_GET_PROCESSOR_INDEX_FROM_VP_INDEX GetProcessorIndexFromVpIndex;
    PHAL_SYNTHETIC_CLUSTER_IPI SyntheticClusterIpi;
    PHAL_VP_START_ENABLED VpStartEnabled;
    PHAL_START_VIRTUAL_PROCESSOR StartVirtualProcessor;
    PHAL_GET_VP_INDEX_FROM_APIC_ID GetVpIndexFromApicId;
    PHAL_IUM_EFI_RUNTIME_SERVICE IumEfiRuntimeService;
    PHAL_SVM_GET_SYSTEM_CAPABILITIES SvmGetSystemCapabilities;
    PHAL_SVM_GET_DEVICE_CAPABILITIES SvmGetDeviceCapabilities;
    PHAL_SVM_CREATE_PASID_SPACE SvmCreatePasidSpace;
    PHAL_SVM_SET_PASID_ADDRESS_SPACE SvmSetPasidAddressSpace;
    PHAL_SVM_FLUSH_PASID SvmFlushPasid;
    PHAL_SVM_ATTACH_PASID_SPACE SvmAttachPasidSpace;
    PHAL_SVM_DETACH_PASID_SPACE SvmDetachPasidSpace;
    PHAL_SVM_ENABLE_PASID SvmEnablePasid;
    PHAL_SVM_DISABLE_PASID SvmDisablePasid;
    PHAL_SVM_ACKNOWLEDGE_PAGE_REQUEST SvmAcknowledgePageRequest;
    PHAL_SVM_CREATE_PR_QUEUE SvmCreatePrQueue;
    PHAL_SVM_DELETE_PR_QUEUE SvmDeletePrQueue;
    PHAL_SVM_CLEAR_PRQ_STALLED SvmClearPrqStalled;
    PHAL_SVM_SET_DEVICE_ENABLED SvmSetDeviceEnabled;
} HAL_INTEL_ENLIGHTENMENT_INFORMATION, *PHAL_INTEL_ENLIGHTENMENT_INFORMATION;
#else
typedef struct _HAL_ARM_ENLIGHTENMENT_INFORMATION
{
    ULONG Enlightenments;
    LOGICAL HypervisorConnected;
    ULONG Reserved0;
    ULONG SpinCountMask;
    PHAL_LONG_SPIN_WAIT LongSpinWait;
    PHAL_GET_REFERENCE_TIME GetReferenceTime;
    PHAL_SET_SYSTEM_SLEEP_PROPERTY SetSystemSleepProperty;
    PHAL_ENTER_SLEEP_STATE EnterSleepState;
    PHAL_NOTIFY_DEBUG_DEVICE_AVAILABLE NotifyDebugDeviceAvailable;
    PHAL_MAP_DEVICE_INTERRUPT MapDeviceInterrupt;
    PHAL_UNMAP_DEVICE_INTERRUPT UnmapDeviceInterrupt;
    PHAL_RETARGET_DEVICE_INTERRUPT RetargetDeviceInterrupt;
    PHAL_QUERY_ASSOCIATED_PROCESSORS QueryAssociatedProcessors;
    PHAL_LP_GET_MACHINE_CHECK_CONTEXT GetMachineCheckContext;
    PHAL_SUSPEND_PARTITION SuspendPartition;
    PHAL_RESUME_PARTITION ResumePartition;
    PHAL_SET_SYSTEM_MACHINE_CHECK_PROPERTY SetSystemMachineCheckProperty;
    PHAL_WHEA_ERROR_NOTIFICATION WheaErrorNotification;
    PHAL_GET_PROCESSOR_INDEX_FROM_VP_INDEX GetProcessorIndexFromVpIndex;
    PHAL_SYNTHETIC_CLUSTER_IPI SyntheticClusterIpi;
} HAL_ARM_ENLIGHTENMENT_INFORMATION, *PHAL_ARM_ENLIGHTENMENT_INFORMATION;
#endif

#if !defined(_ARM64_) && !defined(_ARM_)
#define _HAL_ENLIGHTENMENT_INFORMATION _HAL_INTEL_ENLIGHTENMENT_INFORMATION
#define  HAL_ENLIGHTENMENT_INFORMATION  HAL_INTEL_ENLIGHTENMENT_INFORMATION
#define PHAL_ENLIGHTENMENT_INFORMATION PHAL_INTEL_ENLIGHTENMENT_INFORMATION
#else
#define _HAL_ENLIGHTENMENT_INFORMATION _HAL_ARM_ENLIGHTENMENT_INFORMATION
#define  HAL_ENLIGHTENMENT_INFORMATION  HAL_ARM_ENLIGHTENMENT_INFORMATION
#define PHAL_ENLIGHTENMENT_INFORMATION PHAL_ARM_ENLIGHTENMENT_INFORMATION
#endif

typedef struct _HAL_ENLIGHTENMENT_INFORMATION
    HAL_ENLIGHTENMENT_INFORMATION, *PHAL_ENLIGHTENMENT_INFORMATION;

typedef
VOID
(NTAPI *pHalGetEnlightenmentInformation)(
    _Out_ PHAL_ENLIGHTENMENT_INFORMATION EnlightenmentInformation
);

typedef
PVOID
(NTAPI *pHalAllocateEarlyPages)(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_ ULONG PageCount,
    _Out_ PULONG64 PhysicalAddress,
    _In_ ULONG Protection
);

typedef
PVOID
(NTAPI *pHalMapEarlyPages)(
    _In_ ULONG64 PhysicalAddress,
    _In_ ULONG PageCount,
    _In_ ULONG Protection
);

#if (NTDDI_VERSION >= NTDDI_WINBLUE)
typedef
NTSTATUS
(NTAPI *pHalPrepareProcessorForIdle)(
    _In_ ULONG Flags
);

typedef
VOID
(NTAPI *pHalResumeProcessorFromIdle)(
    VOID
);

typedef
VOID
(NTAPI *pHalNotifyProcessorFreeze)(
    _In_ BOOLEAN Freezing,
    _In_ BOOLEAN ThawingToSpinLoop
);
#else
typedef
NTSTATUS
(NTAPI *pHalPrepareProcessorForIdle)(
    ULONG Unknown1,
    ULONG Unknown2,
    ULONG Unknown3
);

typedef
VOID
(NTAPI *pHalResumeProcessorFromIdle)(
    PULONG Unknown
);

typedef
VOID
(NTAPI *pHalNotifyProcessorFreeze)(
    BOOLEAN Unknown
);
#endif

typedef
VOID
(NTAPI *PHAL_LOG_ROUTINE)(
    _In_ ULONG EventId,
    _In_ PVOID Buffer,
    _In_ ULONG Size
);

typedef struct _HAL_LOG_REGISTER_CONTEXT
{
    PHAL_LOG_ROUTINE LogRoutine;
    ULONG Flag;
} HAL_LOG_REGISTER_CONTEXT, *PHAL_LOG_REGISTER_CONTEXT;

typedef
VOID
(NTAPI *pHalRegisterLogRoutine)(
    _In_ PHAL_LOG_REGISTER_CONTEXT Context
);

typedef enum _HAL_CLOCK_TIMER_MODE
{
    HalClockTimerModePeriodic,
    HalClockTimerModeOneShot,
    HalClockTimerModeMax
} HAL_CLOCK_TIMER_MODE, *PHAL_CLOCK_TIMER_MODE;

typedef struct _HAL_CLOCK_TIMER_CONFIGURATION
{
    union {
        BOOLEAN Flags;
        struct {
            BOOLEAN AlwaysOnTimer: 1;
            BOOLEAN HighLatency: 1;
            BOOLEAN PerCpuTimer: 1;
            BOOLEAN DynamicTickSupported: 1;
        };
    };

    ULONG KnownType;
    ULONG Capabilities;
    ULONG64 MaxIncrement;
    ULONG MinIncrement;
} HAL_CLOCK_TIMER_CONFIGURATION, *PHAL_CLOCK_TIMER_CONFIGURATION;

typedef
ULONG
(NTAPI *pHalGetClockOwner)(
    VOID
);

#if (NTDDI_VERSION >= NTDDI_WINBLUE)
typedef
VOID
(NTAPI *pHalGetClockConfiguration)(
    _Out_ PHAL_CLOCK_TIMER_CONFIGURATION Configuration
);
#else
typedef
VOID
(NTAPI *pHalGetClockConfiguration)(
    PULONG Unknown1,
    PULONG Unknown2,
    PUCHAR Unknown3
);
#endif

typedef
VOID
(NTAPI *pHalClockTimerActivate)(
    _In_ BOOLEAN ClockOwner
);

typedef
VOID
(NTAPI *pHalClockTimerInitialize)(
    VOID
);

typedef
VOID
(NTAPI *pHalClockTimerStop)(
    VOID
);

typedef
NTSTATUS
(NTAPI *pHalClockTimerArm)(
    _In_ HAL_CLOCK_TIMER_MODE Mode,
    _In_ ULONG64 RequestedInteval,
    _Out_ PULONG64 ActualInterval
);

typedef
BOOLEAN
(NTAPI *pHalTimerOnlyClockInterruptPending)(
    VOID
);

struct _HAL_IOMMU_DISPATCH;

typedef
VOID
(NTAPI *pHalIommuRegisterDispatchTable)(
    _Inout_ struct _HAL_IOMMU_DISPATCH *DispatchTable
);

typedef
ULONG
(NTAPI *pHalVectorToIDTEntryEx)(
    _In_ ULONG Vector
);

typedef
NTSTATUS
(NTAPI *pHalSecondaryInterruptQueryPrimaryInformation)(
    _In_ PINTERRUPT_VECTOR_DATA VectorData,
    _Out_ PULONG PrimaryGsiv
);

typedef
BOOLEAN
(NTAPI *pHalIsInterruptTypeSecondary)(
    _In_ ULONG Type,
    _In_ ULONG InputGsiv
);

typedef
NTSTATUS
(NTAPI *pHalMaskInterrupt)(
    _In_ ULONG InputGsiv,
    _In_ ULONG Flags
);

typedef
NTSTATUS
(NTAPI *pHalUnmaskInterrupt)(
    _In_ ULONG InputGsiv,
    _In_ ULONG Flags
);

typedef
NTSTATUS
(NTAPI *pHalAllocateGsivForSecondaryInterrupt)(
    _In_reads_bytes_(OwnerNameLength) PCCHAR OwnerName,
    _In_ USHORT OwnerNameLength,
    _Out_ PULONG Gsiv
);

typedef
NTSTATUS
(NTAPI *pHalInterruptVectorDataToGsiv)(
    _In_ PINTERRUPT_VECTOR_DATA VectorData,
    _Out_ PULONG Gsiv
);

typedef enum _PCI_BUSMASTER_RID_TYPE
{
    BusmasterRidFromDeviceRid,
    BusmasterRidFromBridgeRid,
    BusmasterRidFromMultipleBridges
} PCI_BUSMASTER_RID_TYPE, *PPCI_BUSMASTER_RID_TYPE;

typedef struct _PCI_BUSMASTER_DESCRIPTOR
{
    PCI_BUSMASTER_RID_TYPE Type;
    ULONG Segment;
    union {
        struct {
            UCHAR Bus;
            UCHAR Device;
            UCHAR Function;
            UCHAR Reserved;
        } DeviceRid;
        struct {
            UCHAR Bus;
            UCHAR Device;
            UCHAR Function;
            UCHAR Reserved;
        } BridgeRid;
        struct {
            UCHAR SecondaryBus;
            UCHAR SubordinateBus;
        } MultipleBridges;
    } DUMMYSTRUCTNAME;
} PCI_BUSMASTER_DESCRIPTOR, *PPCI_BUSMASTER_DESCRIPTOR;

typedef
NTSTATUS
(NTAPI *pHalAddInterruptRemapping)(
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _In_ PPCI_BUSMASTER_DESCRIPTOR BusMasterDescriptor,
    _In_range_(0, 3) UCHAR PhantomBits,
    _Inout_updates_(VectorCount) PINTERRUPT_VECTOR_DATA VectorData,
    _In_ ULONG VectorCount
);

typedef
VOID
(NTAPI *pHalRemoveInterruptRemapping)(
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _In_ PPCI_BUSMASTER_DESCRIPTOR BusMasterDescriptor,
    _In_range_(0, 3) UCHAR PhantomBits,
    _Inout_updates_(VectorCount) PINTERRUPT_VECTOR_DATA VectorData,
    _In_ ULONG VectorCount
);

typedef
VOID
(NTAPI *pHalSaveAndDisableHvEnlightenment)(
    VOID
);

typedef
VOID
(NTAPI *pHalRestoreHvEnlightenment)(
    VOID
);

typedef
VOID
(NTAPI *pHalFlushIoBuffersExternalCache)(
    _In_ PMDL  Mdl,
    _In_ BOOLEAN  ReadOperation
);

typedef
VOID
(NTAPI *pHalFlushIoRectangleExternalCache)(
    _In_ PMDL Mdl,
    _In_ ULONG StartOffset,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ ULONG Stride,
    _In_ BOOLEAN ReadOperation
);

typedef
VOID
(NTAPI *pHalFlushExternalCache)(
    _In_ BOOLEAN Invalidate
);

typedef
VOID
(NTAPI *pHalFlushAndInvalidatePageExternalCache)(
    _In_ PHYSICAL_ADDRESS PhysicalAddress
);

typedef
NTSTATUS
(NTAPI *pHalPciEarlyRestore)(
    _In_ SYSTEM_POWER_STATE SleepState
);

typedef
VOID
(NTAPI *pHalPciLateRestore)(
    VOID
);

typedef
NTSTATUS
(NTAPI *pHalGetProcessorId)(
    _In_ ULONG ProcessorIndex,
    _Out_ ULONG *Identifier
);

typedef struct _HAL_PMC_COUNTERS *PMC_HANDLE;

typedef
NTSTATUS
(NTAPI *pHalAllocatePmcCounterSet)(
    _In_ ULONG ProcessorIndex,
    _In_reads_(SourceCount) KPROFILE_SOURCE *SourceList,
    _In_ ULONG SourceCount,
    _Out_ PMC_HANDLE *Handle
);

typedef
VOID
(NTAPI *pHalFreePmcCounterSet)(
    _In_ PMC_HANDLE Handle
);

typedef
VOID
(NTAPI *pHalCollectPmcCounters)(
    _In_ PMC_HANDLE Handle,
    _Out_ PULONG64 Data
);

typedef
ULONGLONG
(NTAPI *pHalTimerQueryCycleCounter)(
    _Out_opt_ PULONGLONG CycleCounterFrequency
);

typedef
VOID
(NTAPI *pHalGetNextTickDuration)(
    PKPRCB Unknown1,
    BOOLEAN Unknown2,
    ULONG Unknown3,
    ULONG64 Unknown4,
    PULONGLONG Unknown5
);

typedef
NTSTATUS
(NTAPI *pHalProcessorHalt)(
    _In_ ULONG Flags,
    _Inout_opt_ PVOID Context,
    _In_ /* PPROCESSOR_HALT_ROUTINE */ PVOID Halt
);

typedef
VOID
(NTAPI *pHalPciMarkHiberPhase)(
    VOID
);

typedef
NTSTATUS
(NTAPI *pHalQueryProcessorRestartEntryPoint)(
    _Out_ PPHYSICAL_ADDRESS EntryPoint
);

typedef
NTSTATUS
(NTAPI *pHalRequestInterrupt)(
    _In_ ULONG Gsiv
);

typedef
VOID
(NTAPI *pHalPowerEarlyRestore)(
    _In_ ULONG Phase
);

typedef
NTSTATUS
(NTAPI *pHalUpdateCapsule)(
    _In_ PVOID CapsuleHeaderArray,
    _In_ ULONG CapsuleCount,
    _In_opt_ PHYSICAL_ADDRESS ScatterGatherList
);

typedef
NTSTATUS
(NTAPI *pHalQueryCapsuleCapabilities)(
    _In_ PVOID CapsuleHeaderArray,
    _In_ ULONG CapsuleCount,
    _Out_ PULONGLONG MaximumCapsuleSize,
    _Out_ PULONG ResetType
);

typedef
BOOLEAN
(NTAPI *pHalPciMultiStageResumeCapable)(
    VOID
);

typedef
VOID
(NTAPI *pHalDmaFreeCrashDumpRegisters)(
    _In_ ULONG Phase
);

typedef
BOOLEAN
(NTAPI *pHalAcpiAoacCapable)(
    VOID
);

#if (NTDDI_VERSION >= NTDDI_WIN10)
typedef
NTSTATUS
(NTAPI *pHalInterruptSetDestination)(
    _In_ ULONG Gsiv,
    _In_ PINTERRUPT_VECTOR_DATA VectorData,
    _In_ PGROUP_AFFINITY TargetProcessors
);
#else
typedef
NTSTATUS
(NTAPI *pHalInterruptSetDestination)(
    _In_ PINTERRUPT_CONNECTION_DATA ConnectionData,
    _In_ PGROUP_AFFINITY TargetProcessors
);
#endif

typedef union _HAL_UNMASKED_INTERRUPT_FLAGS
{
    struct {
        USHORT SecondaryInterrupt: 1;
        USHORT Reserved: 15;
    };
    USHORT AsUSHORT;
} HAL_UNMASKED_INTERRUPT_FLAGS, *PHAL_UNMASKED_INTERRUPT_FLAGS;

typedef struct _HAL_UNMASKED_INTERRUPT_INFORMATION
{
    USHORT Version;
    USHORT Size;
    HAL_UNMASKED_INTERRUPT_FLAGS Flags;
    KINTERRUPT_MODE Mode;
    KINTERRUPT_POLARITY Polarity;
    ULONG Gsiv;
    USHORT PinNumber;
    PVOID DeviceHandle;
} HAL_UNMASKED_INTERRUPT_INFORMATION, *PHAL_UNMASKED_INTERRUPT_INFORMATION;

typedef
BOOLEAN
(NTAPI HAL_ENUMERATE_INTERRUPT_SOURCE_CALLBACK)(
    _In_ PVOID Context,
    _In_ PHAL_UNMASKED_INTERRUPT_INFORMATION InterruptInformation
);

typedef HAL_ENUMERATE_INTERRUPT_SOURCE_CALLBACK
    *PHAL_ENUMERATE_INTERRUPT_SOURCE_CALLBACK;

typedef
NTSTATUS
(NTAPI *pHalEnumerateUnmaskedInterrupts)(
    _In_ PHAL_ENUMERATE_INTERRUPT_SOURCE_CALLBACK Callback,
    _In_ PVOID Context,
    _Out_ PHAL_UNMASKED_INTERRUPT_INFORMATION InterruptInformation
);

typedef
PVOID
(NTAPI *pHalAcpiGetMultiNode)(
    VOID
);

typedef
void
(NTAPI HALREBOOTHANDLER)(
    _In_ ULONG ProcessorNumber,
    _Inout_opt_ volatile LONG* ProcessorsStarted
);

typedef HALREBOOTHANDLER *PHALREBOOTHANDLER;

typedef
PHALREBOOTHANDLER
(NTAPI *pHalPowerSetRebootHandler)(
    _In_opt_ PHALREBOOTHANDLER NewHandler
);

typedef
NTSTATUS
(NTAPI *pHalTimerWatchdogStart)(
    VOID
);

#if (NTDDI_VERSION >= NTDDI_WIN10)
typedef
VOID
(NTAPI *pHalTimerWatchdogResetCountdown)(
    _In_ LOGICAL SetWakeTimer
);
#else
typedef
VOID
(NTAPI *pHalTimerWatchdogResetCountdown)(
    VOID
);
#endif

typedef
NTSTATUS
(NTAPI *pHalTimerWatchdogStop)(
    VOID
);

typedef
BOOLEAN
(NTAPI *pHalTimerWatchdogGeneratedLastReset)(
    VOID
);

typedef
NTSTATUS
(NTAPI *pHalTimerWatchdogTriggerSystemReset)(
    _In_ BOOLEAN ResetViaClockInterrupt
);

typedef
NTSTATUS
(NTAPI *pHalInterruptGetHighestPriorityInterrupt)(
    _Out_ PULONG HighestPendingVector,
    _Out_ PBOOLEAN SingleInterrupt
);

typedef
NTSTATUS
(NTAPI *pHalProcessorOn)(
    _In_ ULONG NtNumber
);

typedef
NTSTATUS
(NTAPI *pHalProcessorOff)(
    VOID
);

typedef
NTSTATUS
(NTAPI *pHalProcessorFreeze)(
    VOID
);

typedef
NTSTATUS
(NTAPI *pHalDmaLinkDeviceObjectByToken)(
    _In_ ULONG_PTR Token,
    _In_opt_ PDEVICE_OBJECT DeviceObject
);

typedef
NTSTATUS
(NTAPI *pHalDmaCheckAdapterToken)(
    _In_ ULONG_PTR Token
);

typedef
NTSTATUS
(NTAPI *pHalTimerConvertAuxiliaryCounterToPerformanceCounter)(
    _In_ ULONG64 AuxiliaryCounterValue,
    _Out_ PULONG64 PerformanceCounterValueOut,
    _Out_opt_ PULONG64 ConversionErrorOut
);

typedef
NTSTATUS
(NTAPI *pHalTimerConvertPerformanceCounterToAuxiliaryCounter)(
    _In_ ULONG64 PerformanceCounterValue,
    _Out_ PULONG64 AuxiliaryCounterValueOut,
    _Out_opt_ PULONG64 ConversionErrorOut
);

typedef
NTSTATUS
(NTAPI *pHalTimerQueryAuxiliaryCounterFrequency)(
    _Out_opt_ PULONG64 AuxiliaryCounterFrequencyOut
);

typedef
NTSTATUS
(NTAPI *pHalConnectThermalInterrupt)(
    _In_ PKSERVICE_ROUTINE InterruptService
);

typedef
BOOLEAN
(NTAPI *pHalIsEFIRuntimeActive)(
    VOID
);

//
// HAL Bus Handler Callback Types
//
typedef
NTSTATUS
(NTAPI *PADJUSTRESOURCELIST)(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PBUS_HANDLER RootHandler,
    _Inout_ PIO_RESOURCE_REQUIREMENTS_LIST* pResourceList
);

typedef
NTSTATUS
(NTAPI *PASSIGNSLOTRESOURCES)(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PBUS_HANDLER RootHandler,
    _In_ PUNICODE_STRING RegistryPath,
    _In_opt_ PUNICODE_STRING DriverClassName,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG SlotNumber,
    _Inout_ PCM_RESOURCE_LIST* AllocatedResources
);

typedef
ULONG
(NTAPI *PGETSETBUSDATA)(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PBUS_HANDLER RootHandler,
    _In_ ULONG SlotNumber,
    _In_ PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length
);

typedef
ULONG
(NTAPI *PGETINTERRUPTVECTOR)(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PBUS_HANDLER RootHandler,
    _In_ ULONG BusInterruptLevel,
    _In_ ULONG BusInterruptVector,
    _Out_ PKIRQL Irql,
    _Out_ PKAFFINITY Affinity
);

typedef
BOOLEAN
(NTAPI *PTRANSLATEBUSADDRESS)(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PBUS_HANDLER RootHandler,
    _In_ PHYSICAL_ADDRESS BusAddress,
    _Inout_ PULONG AddressSpace,
    _Out_ PPHYSICAL_ADDRESS TranslatedAddress
);

//
// HAL Private dispatch Table
//
// See Version table at:
// https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/ntos/hal/hal_private_dispatch.htm
//
#if (NTDDI_VERSION < NTDDI_WINXP)
#define HAL_PRIVATE_DISPATCH_VERSION        1
#elif (NTDDI_VERSION < NTDDI_LONGHORN)
#define HAL_PRIVATE_DISPATCH_VERSION        2
#elif (NTDDI_VERSION < NTDDI_VISTASP1)
#define HAL_PRIVATE_DISPATCH_VERSION        5
#elif (NTDDI_VERSION < NTDDI_VISTASP2)
#define HAL_PRIVATE_DISPATCH_VERSION        6
#elif (NTDDI_VERSION < NTDDI_WIN7)
#define HAL_PRIVATE_DISPATCH_VERSION        7
#elif (NTDDI_VERSION < NTDDI_WIN8)
#define HAL_PRIVATE_DISPATCH_VERSION        13
#elif (NTDDI_VERSION < NTDDI_WINBLUE)
#define HAL_PRIVATE_DISPATCH_VERSION        21
#elif (NTDDI_VERSION < NTDDI_WIN10)
#define HAL_PRIVATE_DISPATCH_VERSION        23
#elif (NTDDI_VERSION >= NTDDI_WIN10)
#define HAL_PRIVATE_DISPATCH_VERSION        32
#else
/* Not yet defined */
#endif
typedef struct _HAL_PRIVATE_DISPATCH
{
    ULONG Version;
    pHalHandlerForBus HalHandlerForBus;
    pHalHandlerForConfigSpace HalHandlerForConfigSpace;
    pHalLocateHiberRanges HalLocateHiberRanges;
    pHalRegisterBusHandler HalRegisterBusHandler;
    pHalSetWakeEnable HalSetWakeEnable;
    pHalSetWakeAlarm HalSetWakeAlarm;
    pHalTranslateBusAddress HalPciTranslateBusAddress;
    pHalAssignSlotResources HalPciAssignSlotResources;
    pHalHaltSystem HalHaltSystem;
    pHalFindBusAddressTranslation HalFindBusAddressTranslation;
    pHalResetDisplay HalResetDisplay;
#if (NTDDI_VERSION >= NTDDI_WS03)
    pHalAllocateMapRegisters HalAllocateMapRegisters;
#endif
#if (NTDDI_VERSION >= NTDDI_WINXP)
    pKdSetupPciDeviceForDebugging KdSetupPciDeviceForDebugging;
    pKdReleasePciDeviceForDebugging KdReleasePciDeviceforDebugging;
    pKdGetAcpiTablePhase0 KdGetAcpiTablePhase0;
    pKdCheckPowerButton KdCheckPowerButton;
    pHalVectorToIDTEntry HalVectorToIDTEntry;
    pKdMapPhysicalMemory64 KdMapPhysicalMemory64;
    pKdUnmapVirtualAddress KdUnmapVirtualAddress;
#endif
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    pKdGetPciDataByOffset KdGetPciDataByOffset;
    pKdSetPciDataByOffset KdSetPciDataByOffset;
    pHalGetInterruptVector HalGetInterruptVectorOverride;
    pHalGetVectorInput HalGetVectorInputOverride;
    pHalLoadMicrocode HalLoadMicrocode;
    pHalUnloadMicrocode HalUnloadMicrocode;
    pHalPostMicrocodeUpdate HalPostMicrocodeUpdate;
#endif
#if (NTDDI_VERSION >= NTDDI_VISTASP1)
    pHalAllocateMessageTarget HalAllocateMessageTargetOverride;
    pHalFreeMessageTarget HalFreeMessageTargetOverride;
    pHalDpReplaceBegin HalDpReplaceBegin;
    pHalDpReplaceTarget HalDpReplaceTarget;
    pHalDpReplaceControl HalDpReplaceControl;
    pHalDpReplaceEnd HalDpReplaceEnd;
    pHalPrepareForBugcheck HalPrepareForBugcheck;
#endif
#if (NTDDI_VERSION >= NTDDI_WIN7)
    pHalQueryWakeTime HalQueryWakeTime;
    pHalReportIdleStateUsage HalReportIdleStateUsage;
    pHalTscSynchronization HalTscSynchronization;
    pHalWheaInitProcessorGenericSection HalWheaInitProcessorGenericSection;
    pHalStopLegacyUsbInterrupts HalStopLegacyUsbInterrupts;
#endif
#if (NTDDI_VERSION >= NTDDI_VISTASP2)
    pHalReadWheaPhysicalMemory HalReadWheaPhysicalMemory;
    pHalWriteWheaPhysicalMemory HalWriteWheaPhysicalMemory;
#endif
#if (NTDDI_VERSION >= NTDDI_WIN7)
    pHalDpMaskLevelTriggeredInterrupts HalDpMaskLevelTriggeredInterrupts;
    pHalDpUnmaskLevelTriggeredInterrupts HalDpUnmaskLevelTriggeredInterrupts;
    pHalDpGetInterruptReplayState HalDpGetInterruptReplayState;
    pHalDpReplayInterrupts HalDpReplayInterrupts;
    pHalQueryIoPortAccessSupported HalQueryIoPortAccessSupported;
#endif
#if (NTDDI_VERSION >= NTDDI_WIN8)
    pKdSetupIntegratedDeviceForDebugging KdSetupIntegratedDeviceForDebugging;
    pKdReleaseIntegratedDeviceForDebugging KdReleaseIntegratedDeviceForDebugging;
    pHalGetEnlightenmentInformation HalGetEnlightenmentInformation;
    pHalAllocateEarlyPages HalAllocateEarlyPages;
    pHalMapEarlyPages HalMapEarlyPages;
#if (NTDDI_VERSION == NTDDI_WIN8)
    pHalGetClockOwner HalGetClockOwner;
    pHalGetClockConfiguration HalGetClockConfiguration;
#elif (NTDDI_VERSION >= NTDDI_WINBLUE)
    PVOID Dummy1;
    PVOID Dummy2;
#endif
    pHalNotifyProcessorFreeze HalNotifyProcessorFreeze;
    pHalPrepareProcessorForIdle HalPrepareProcessorForIdle;
    pHalRegisterLogRoutine HalRegisterLogRoutine;
    pHalResumeProcessorFromIdle HalResumeProcessorFromIdle;
    PVOID Dummy;
    pHalVectorToIDTEntryEx HalVectorToIDTEntryEx;
    pHalSecondaryInterruptQueryPrimaryInformation HalSecondaryInterruptQueryPrimaryInformation;
    pHalMaskInterrupt HalMaskInterrupt;
    pHalUnmaskInterrupt HalUnmaskInterrupt;
    pHalIsInterruptTypeSecondary HalIsInterruptTypeSecondary;
    pHalAllocateGsivForSecondaryInterrupt HalAllocateGsivForSecondaryInterrupt;
    pHalAddInterruptRemapping HalAddInterruptRemapping;
    pHalRemoveInterruptRemapping HalRemoveInterruptRemapping;
    pHalSaveAndDisableHvEnlightenment HalSaveAndDisableHvEnlightenment;
    pHalRestoreHvEnlightenment HalRestoreHvEnlightenment;
    pHalFlushIoBuffersExternalCache HalFlushIoBuffersExternalCache;
    pHalFlushExternalCache HalFlushExternalCache;
    pHalPciEarlyRestore HalPciEarlyRestore;
    pHalGetProcessorId HalGetProcessorId;
    pHalAllocatePmcCounterSet HalAllocatePmcCounterSet;
    pHalCollectPmcCounters HalCollectPmcCounters;
    pHalFreePmcCounterSet HalFreePmcCounterSet;
    pHalProcessorHalt HalProcessorHalt;
    pHalTimerQueryCycleCounter HalTimerQueryCycleCounter;
#if (NTDDI_VERSION == NTDDI_WIN8)
    pHalGetNextTickDuration HalGetNextTickDuration;
#elif (NTDDI_VERSION >= NTDDI_WINBLUE)
    PVOID Dummy3;
#endif
    pHalPciMarkHiberPhase HalPciMarkHiberPhase;
    pHalQueryProcessorRestartEntryPoint HalQueryProcessorRestartEntryPoint;
    pHalRequestInterrupt HalRequestInterrupt;
    pHalEnumerateUnmaskedInterrupts HalEnumerateUnmaskedInterrupts;
    pHalFlushAndInvalidatePageExternalCache HalFlushAndInvalidatePageExternalCache;
    pKdEnumerateDebuggingDevices KdEnumerateDebuggingDevices;
    pHalFlushIoRectangleExternalCache HalFlushIoRectangleExternalCache;
    pHalPowerEarlyRestore HalPowerEarlyRestore;
    pHalQueryCapsuleCapabilities HalQueryCapsuleCapabilities;
    pHalUpdateCapsule HalUpdateCapsule;
    pHalPciMultiStageResumeCapable HalPciMultiStageResumeCapable;
    pHalDmaFreeCrashDumpRegisters HalDmaFreeCrashDumpRegisters;
    pHalAcpiAoacCapable HalAcpiAoacCapable;
#endif
#if (NTDDI_VERSION >= NTDDI_WINBLUE)
    pHalInterruptSetDestination HalInterruptSetDestination;
    pHalGetClockConfiguration HalGetClockConfiguration;
    pHalClockTimerActivate HalClockTimerActivate;
    pHalClockTimerInitialize HalClockTimerInitialize;
    pHalClockTimerStop HalClockTimerStop;
    pHalClockTimerArm HalClockTimerArm;
    pHalTimerOnlyClockInterruptPending HalTimerOnlyClockInterruptPending;
    pHalAcpiGetMultiNode HalAcpiGetMultiNode;
    pHalPowerSetRebootHandler HalPowerSetRebootHandler;
    pHalIommuRegisterDispatchTable HalIommuRegisterDispatchTable;
    pHalTimerWatchdogStart HalTimerWatchdogStart;
    pHalTimerWatchdogResetCountdown HalTimerWatchdogResetCountdown;
    pHalTimerWatchdogStop HalTimerWatchdogStop;
    pHalTimerWatchdogGeneratedLastReset HalTimerWatchdogGeneratedLastReset;
    pHalTimerWatchdogTriggerSystemReset HalTimerWatchdogTriggerSystemReset;
#endif
#if (NTDDI_VERSION >= NTDDI_WIN10)
    pHalInterruptVectorDataToGsiv HalInterruptVectorDataToGsiv;
    pHalInterruptGetHighestPriorityInterrupt HalInterruptGetHighestPriorityInterrupt;
    pHalProcessorOn HalProcessorOn;
    pHalProcessorOff HalProcessorOff;
    pHalProcessorFreeze HalProcessorFreeze;
    pHalDmaLinkDeviceObjectByToken HalDmaLinkDeviceObjectByToken;
    pHalDmaCheckAdapterToken HalDmaCheckAdapterToken;
    pHalPciLateRestore HalPciLateRestore;
    pHalTimerConvertPerformanceCounterToAuxiliaryCounter HalTimerConvertPerformanceCounterToAuxiliaryCounter;
    pHalTimerConvertAuxiliaryCounterToPerformanceCounter HalTimerConvertAuxiliaryCounterToPerformanceCounter;
    pHalTimerQueryAuxiliaryCounterFrequency HalTimerQueryAuxiliaryCounterFrequency;
    pHalConnectThermalInterrupt HalConnectThermalInterrupt;
    pHalIsEFIRuntimeActive HalIsEFIRuntimeActive;
#endif
} HAL_PRIVATE_DISPATCH, *PHAL_PRIVATE_DISPATCH;

typedef struct _IOMMU_DEVICE_PATH
{
    GUID BusTypeGuid;
    ULONG UniqueIdLength;
    ULONG PathLength;
    PVOID UniqueId;
    PVOID Path;
} IOMMU_DEVICE_PATH, *PIOMMU_DEVICE_PATH;

typedef union _IOMMU_SVM_CAPABILITIES
{
    struct {
        ULONG AtsCapability : 1;
        ULONG PriCapability : 1;
        ULONG PasidCapability : 1;
        struct {
            ULONG PasidMaxWidth : 5;
            ULONG PasidExePerm : 1;
            ULONG PasidPrivMode : 1;
            ULONG AtsPageAlignedRequest : 1;
            ULONG AtsGlobalInvalidate : 1;
            ULONG AtsInvalidateQueueDepth : 5;
        } CapReg;
        ULONG Rsvd : 15;
    } DUMMYSTRUCTNAME;
    ULONG AsULONG;
} IOMMU_SVM_CAPABILITIES, *PIOMMU_SVM_CAPABILITIES;

//
// HAL IOMMU function Types
//
typedef
BOOLEAN
(NTAPI *pHalIommuSupportEnabled)(
    VOID
);

typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *pHalIommuGetConfiguration)(
    _In_ ULONG Domain,
    _Out_ PULONG PageRequestQueues,
    _Out_ PULONG MaximumAsids,
    _Out_ PVOID *SystemContext
);

typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *pHalIommuGetLibraryContext)(
    _In_ ULONG Pasid,
    _In_ ULONG Domain,
    _Out_ PVOID *LibraryContext
);

typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *pHalIommuMapDevice)(
    _In_ PVOID LibraryContext,
    _In_ PIOMMU_DEVICE_PATH DevicePath,
    _In_ PIOMMU_SVM_CAPABILITIES DeviceCapabilities,
    _Out_ PVOID *DeviceHandle
);

typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *pHalIommuEnableDevicePasid)(
    _In_ PVOID LibraryContext,
    _In_ PVOID DeviceHandle
);

typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *pHalIommuSetAddressSpace)(
    _In_ PVOID LibraryContext,
    _In_ ULONG_PTR DirectoryBase
);

typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *pHalIommuDisableDevicePasid)(
    _In_ PVOID LibraryContext,
    _In_ PVOID DeviceHandle
);

typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *pHalIommuUnmapDevice)(
    _In_ PVOID SystemContext,
    _In_ PVOID DeviceHandle
);

typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *pHalIommuFreeLibraryContext)(
    _In_ PVOID LibraryContext
);

typedef
_IRQL_requires_max_(HIGH_LEVEL)
VOID
(NTAPI *pHalIommuFlushTb)(
    _In_ PVOID LibraryContext,
    _In_ ULONG Number,
    _In_reads_(Number) KTB_FLUSH_VA Virtual[]
);

typedef
_IRQL_requires_max_(HIGH_LEVEL)
VOID
(NTAPI *pHalIommuFlushAllPasid)(
    _In_ PVOID LibraryContext,
    _In_ ULONG Number,
    _In_reads_(Number) KTB_FLUSH_VA Virtual[]
);

typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN
(NTAPI *pHalIommuProcessPageRequestQueue)(
    _In_ ULONG Index
);

typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *pHalIommuDevicePowerChange)(
    _In_ PVOID SystemContext,
    _In_ PVOID DeviceHandle,
    _In_ BOOLEAN PowerActive
);

typedef
_IRQL_requires_max_(HIGH_LEVEL)
VOID
(NTAPI *pHalIommuFaultRoutine)(
    _In_ ULONG Index
);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
PVOID
(NTAPI *pHalIommuReferenceAsid)(
    _In_ ULONG Asid
);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *pHalIommuDereferenceAsid)(
    _In_ ULONG Asid
);

typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *pHalIommuServicePageFault)(
    _In_ ULONG_PTR FaultStatus,
    _In_ PVOID FaultingVa,
    _In_ ULONG Asid
);

//
// HAL IOMMU dispatch Table
//
typedef struct _HAL_IOMMU_DISPATCH
{
    pHalIommuSupportEnabled HalIommuSupportEnabled;
    pHalIommuGetConfiguration HalIommuGetConfiguration;
    pHalIommuGetLibraryContext HalIommuGetLibraryContext;
    pHalIommuMapDevice HalIommuMapDevice;
    pHalIommuEnableDevicePasid HalIommuEnableDevicePasid;
    pHalIommuSetAddressSpace HalIommuSetAddressSpace;
    pHalIommuDisableDevicePasid HalIommuDisableDevicePasid;
    pHalIommuUnmapDevice HalIommuUnmapDevice;
    pHalIommuFreeLibraryContext HalIommuFreeLibraryContext;
    pHalIommuFlushTb HalIommuFlushTb;
    pHalIommuFlushAllPasid HalIommuFlushAllPasid;
    pHalIommuProcessPageRequestQueue HalIommuProcessPageRequestQueue;
    pHalIommuFaultRoutine HalIommuFaultRoutine;
    pHalIommuReferenceAsid HalIommuReferenceAsid;
    pHalIommuDereferenceAsid HalIommuDereferenceAsid;
    pHalIommuServicePageFault HalIommuServicePageFault;
    pHalIommuDevicePowerChange HalIommuDevicePowerChange;
} HAL_IOMMU_DISPATCH, *PHAL_IOMMU_DISPATCH;

extern PHAL_IOMMU_DISPATCH HalIommuDispatch;

//
// HAL Supported Range
//
#define HAL_SUPPORTED_RANGE_VERSION 1
typedef struct _SUPPORTED_RANGE
{
    struct _SUPPORTED_RANGE *Next;
    ULONG SystemAddressSpace;
    LONGLONG SystemBase;
    LONGLONG Base;
    LONGLONG Limit;
} SUPPORTED_RANGE, *PSUPPORTED_RANGE;

typedef struct _SUPPORTED_RANGES
{
    USHORT Version;
    BOOLEAN Sorted;
    UCHAR Reserved;
    ULONG NoIO;
    SUPPORTED_RANGE IO;
    ULONG NoMemory;
    SUPPORTED_RANGE Memory;
    ULONG NoPrefetchMemory;
    SUPPORTED_RANGE PrefetchMemory;
    ULONG NoDma;
    SUPPORTED_RANGE Dma;
} SUPPORTED_RANGES, *PSUPPORTED_RANGES;

//
// HAL Bus Handler
//
#define HAL_BUS_HANDLER_VERSION 1
typedef struct _BUS_HANDLER
{
    ULONG Version;
    INTERFACE_TYPE InterfaceType;
    BUS_DATA_TYPE ConfigurationType;
    ULONG BusNumber;
    PDEVICE_OBJECT DeviceObject;
    struct _BUS_HANDLER *ParentHandler;
    PVOID BusData;
    ULONG DeviceControlExtensionSize;
    PSUPPORTED_RANGES BusAddresses;
    ULONG Reserved[4];
    PGETSETBUSDATA GetBusData;
    PGETSETBUSDATA SetBusData;
    PADJUSTRESOURCELIST AdjustResourceList;
    PASSIGNSLOTRESOURCES AssignSlotResources;
    PGETINTERRUPTVECTOR GetInterruptVector;
    PTRANSLATEBUSADDRESS TranslateBusAddress;
    PVOID Spare1;
    PVOID Spare2;
    PVOID Spare3;
    PVOID Spare4;
    PVOID Spare5;
    PVOID Spare6;
    PVOID Spare7;
    PVOID Spare8;
} BUS_HANDLER;

//
// HAL Chip Hacks
//
#define HAL_PCI_CHIP_HACK_BROKEN_ACPI_TIMER        0x01
#define HAL_PCI_CHIP_HACK_DISABLE_HIBERNATE        0x02
#define HAL_PCI_CHIP_HACK_DISABLE_ACPI_IRQ_ROUTING 0x04
#define HAL_PCI_CHIP_HACK_USB_SMI_DISABLE          0x08

//
// Kernel Exports
//
#if !defined(_NTSYSTEM_) && (defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_))
extern NTSYSAPI PHAL_PRIVATE_DISPATCH HalPrivateDispatchTable;
#define HALPRIVATEDISPATCH ((PHAL_PRIVATE_DISPATCH)&HalPrivateDispatchTable)
#else
extern NTSYSAPI HAL_PRIVATE_DISPATCH HalPrivateDispatchTable;
#define HALPRIVATEDISPATCH (&HalPrivateDispatchTable)
#endif

//
// HAL Exports
//
extern NTHALAPI PUCHAR KdComPortInUse;

//
// HAL Constants
//
#define HAL_IRQ_TRANSLATOR_VERSION 0x0

//
// BIOS call structure
//
typedef struct _X86_BIOS_REGISTERS
{
    ULONG Eax;
    ULONG Ecx;
    ULONG Edx;
    ULONG Ebx;
    ULONG Ebp;
    ULONG Esi;
    ULONG Edi;
    USHORT SegDs;
    USHORT SegEs;
} X86_BIOS_REGISTERS, *PX86_BIOS_REGISTERS;

#endif
#endif



