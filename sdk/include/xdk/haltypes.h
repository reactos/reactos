/* Hardware Abstraction Layer Types */

$if (_NTDDK_)
typedef BOOLEAN
(NTAPI *PHAL_RESET_DISPLAY_PARAMETERS)(
  _In_ ULONG Columns,
  _In_ ULONG Rows);

typedef PBUS_HANDLER
(FASTCALL *pHalHandlerForBus)(
  _In_ INTERFACE_TYPE InterfaceType,
  _In_ ULONG BusNumber);

typedef VOID
(FASTCALL *pHalReferenceBusHandler)(
  _In_ PBUS_HANDLER BusHandler);

typedef enum _HAL_QUERY_INFORMATION_CLASS {
  HalInstalledBusInformation,
  HalProfileSourceInformation,
  HalInformationClassUnused1,
  HalPowerInformation,
  HalProcessorSpeedInformation,
  HalCallbackInformation,
  HalMapRegisterInformation,
  HalMcaLogInformation,
  HalFrameBufferCachingInformation,
  HalDisplayBiosInformation,
  HalProcessorFeatureInformation,
  HalNumaTopologyInterface,
  HalErrorInformation,
  HalCmcLogInformation,
  HalCpeLogInformation,
  HalQueryMcaInterface,
  HalQueryAMLIIllegalIOPortAddresses,
  HalQueryMaxHotPlugMemoryAddress,
  HalPartitionIpiInterface,
  HalPlatformInformation,
  HalQueryProfileSourceList,
  HalInitLogInformation,
  HalFrequencyInformation,
  HalProcessorBrandString,
  HalHypervisorInformation,
  HalPlatformTimerInformation,
  HalAcpiAuditInformation
} HAL_QUERY_INFORMATION_CLASS, *PHAL_QUERY_INFORMATION_CLASS;

typedef enum _HAL_SET_INFORMATION_CLASS {
  HalProfileSourceInterval,
  HalProfileSourceInterruptHandler,
  HalMcaRegisterDriver,
  HalKernelErrorHandler,
  HalCmcRegisterDriver,
  HalCpeRegisterDriver,
  HalMcaLog,
  HalCmcLog,
  HalCpeLog,
  HalGenerateCmcInterrupt,
  HalProfileSourceTimerHandler,
  HalEnlightenment,
  HalProfileDpgoSourceInterruptHandler
} HAL_SET_INFORMATION_CLASS, *PHAL_SET_INFORMATION_CLASS;

typedef NTSTATUS
(NTAPI *pHalQuerySystemInformation)(
  _In_ HAL_QUERY_INFORMATION_CLASS InformationClass,
  _In_ ULONG BufferSize,
  _Inout_updates_bytes_to_(BufferSize, *ReturnedLength) PVOID Buffer,
  _Out_ PULONG ReturnedLength);

typedef NTSTATUS
(NTAPI *pHalSetSystemInformation)(
  _In_ HAL_SET_INFORMATION_CLASS InformationClass,
  _In_ ULONG BufferSize,
  _In_ PVOID Buffer);

typedef VOID
(FASTCALL *pHalExamineMBR)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ ULONG SectorSize,
  _In_ ULONG MBRTypeIdentifier,
  _Out_ PVOID *Buffer);

typedef NTSTATUS
(FASTCALL *pHalIoReadPartitionTable)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ ULONG SectorSize,
  _In_ BOOLEAN ReturnRecognizedPartitions,
  _Out_ struct _DRIVE_LAYOUT_INFORMATION **PartitionBuffer);

typedef NTSTATUS
(FASTCALL *pHalIoSetPartitionInformation)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ ULONG SectorSize,
  _In_ ULONG PartitionNumber,
  _In_ ULONG PartitionType);

typedef NTSTATUS
(FASTCALL *pHalIoWritePartitionTable)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ ULONG SectorSize,
  _In_ ULONG SectorsPerTrack,
  _In_ ULONG NumberOfHeads,
  _In_ struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer);

typedef NTSTATUS
(NTAPI *pHalQueryBusSlots)(
  _In_ PBUS_HANDLER BusHandler,
  _In_ ULONG BufferSize,
  _Out_ PULONG SlotNumbers,
  _Out_ PULONG ReturnedLength);

typedef NTSTATUS
(NTAPI *pHalInitPnpDriver)(VOID);

typedef struct _PM_DISPATCH_TABLE {
  ULONG Signature;
  ULONG Version;
  PVOID Function[1];
} PM_DISPATCH_TABLE, *PPM_DISPATCH_TABLE;

typedef NTSTATUS
(NTAPI *pHalInitPowerManagement)(
  _In_ PPM_DISPATCH_TABLE PmDriverDispatchTable,
  _Out_ PPM_DISPATCH_TABLE *PmHalDispatchTable);

typedef struct _DMA_ADAPTER*
(NTAPI *pHalGetDmaAdapter)(
  _In_ PVOID Context,
  _In_ struct _DEVICE_DESCRIPTION *DeviceDescriptor,
  _Out_ PULONG NumberOfMapRegisters);

typedef NTSTATUS
(NTAPI *pHalGetInterruptTranslator)(
  _In_ INTERFACE_TYPE ParentInterfaceType,
  _In_ ULONG ParentBusNumber,
  _In_ INTERFACE_TYPE BridgeInterfaceType,
  _In_ USHORT Size,
  _In_ USHORT Version,
  _Out_ PTRANSLATOR_INTERFACE Translator,
  _Out_ PULONG BridgeBusNumber);

typedef NTSTATUS
(NTAPI *pHalStartMirroring)(VOID);

typedef NTSTATUS
(NTAPI *pHalEndMirroring)(
  _In_ ULONG PassNumber);

typedef NTSTATUS
(NTAPI *pHalMirrorPhysicalMemory)(
  _In_ PHYSICAL_ADDRESS PhysicalAddress,
  _In_ LARGE_INTEGER NumberOfBytes);

typedef NTSTATUS
(NTAPI *pHalMirrorVerify)(
  _In_ PHYSICAL_ADDRESS PhysicalAddress,
  _In_ LARGE_INTEGER NumberOfBytes);

typedef BOOLEAN
(NTAPI *pHalTranslateBusAddress)(
  _In_ INTERFACE_TYPE InterfaceType,
  _In_ ULONG BusNumber,
  _In_ PHYSICAL_ADDRESS BusAddress,
  _Inout_ PULONG AddressSpace,
  _Out_ PPHYSICAL_ADDRESS TranslatedAddress);

typedef NTSTATUS
(NTAPI *pHalAssignSlotResources)(
  _In_ PUNICODE_STRING RegistryPath,
  _In_opt_ PUNICODE_STRING DriverClassName,
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ INTERFACE_TYPE BusType,
  _In_ ULONG BusNumber,
  _In_ ULONG SlotNumber,
  _Inout_ PCM_RESOURCE_LIST *AllocatedResources);

typedef VOID
(NTAPI *pHalHaltSystem)(VOID);

typedef BOOLEAN
(NTAPI *pHalResetDisplay)(VOID);

typedef struct _MAP_REGISTER_ENTRY {
  PVOID MapRegister;
  BOOLEAN WriteToDevice;
} MAP_REGISTER_ENTRY, *PMAP_REGISTER_ENTRY;

typedef UCHAR
(NTAPI *pHalVectorToIDTEntry)(
  ULONG Vector);

typedef BOOLEAN
(NTAPI *pHalFindBusAddressTranslation)(
  _In_ PHYSICAL_ADDRESS BusAddress,
  _Inout_ PULONG AddressSpace,
  _Out_ PPHYSICAL_ADDRESS TranslatedAddress,
  _Inout_ PULONG_PTR Context,
  _In_ BOOLEAN NextBus);

typedef VOID
(NTAPI *pHalEndOfBoot)(VOID);

typedef PVOID
(NTAPI *pHalGetAcpiTable)(
  _In_ ULONG Signature,
  _In_opt_ PCSTR OemId,
  _In_opt_ PCSTR OemTableId);

#if defined(_IA64_)
typedef NTSTATUS
(*pHalGetErrorCapList)(
  _Inout_ PULONG CapsListLength,
  _Inout_updates_bytes_(*CapsListLength) PUCHAR ErrorCapList);

typedef NTSTATUS
(*pHalInjectError)(
  _In_ ULONG BufferLength,
  _In_reads_bytes_(BufferLength) PUCHAR Buffer);
#endif

typedef VOID
(NTAPI *PCI_ERROR_HANDLER_CALLBACK)(VOID);

typedef VOID
(NTAPI *pHalSetPciErrorHandlerCallback)(
  _In_ PCI_ERROR_HANDLER_CALLBACK Callback);

#if 1 /* Not present in WDK 7600 */
typedef VOID
(FASTCALL *pHalIoAssignDriveLetters)(
  _In_ struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
  _In_ PSTRING NtDeviceName,
  _Out_ PUCHAR NtSystemPath,
  _Out_ PSTRING NtSystemPathString);
#endif

typedef struct {
  ULONG Version;
  pHalQuerySystemInformation HalQuerySystemInformation;
  pHalSetSystemInformation HalSetSystemInformation;
  pHalQueryBusSlots HalQueryBusSlots;
  ULONG Spare1;
  pHalExamineMBR HalExamineMBR;
#if 1 /* Not present in WDK 7600 */
  pHalIoAssignDriveLetters HalIoAssignDriveLetters;
#endif
  pHalIoReadPartitionTable HalIoReadPartitionTable;
  pHalIoSetPartitionInformation HalIoSetPartitionInformation;
  pHalIoWritePartitionTable HalIoWritePartitionTable;
  pHalHandlerForBus HalReferenceHandlerForBus;
  pHalReferenceBusHandler HalReferenceBusHandler;
  pHalReferenceBusHandler HalDereferenceBusHandler;
  pHalInitPnpDriver HalInitPnpDriver;
  pHalInitPowerManagement HalInitPowerManagement;
  pHalGetDmaAdapter HalGetDmaAdapter;
  pHalGetInterruptTranslator HalGetInterruptTranslator;
  pHalStartMirroring HalStartMirroring;
  pHalEndMirroring HalEndMirroring;
  pHalMirrorPhysicalMemory HalMirrorPhysicalMemory;
  pHalEndOfBoot HalEndOfBoot;
  pHalMirrorVerify HalMirrorVerify;
  pHalGetAcpiTable HalGetCachedAcpiTable;
  pHalSetPciErrorHandlerCallback  HalSetPciErrorHandlerCallback;
#if defined(_IA64_)
  pHalGetErrorCapList HalGetErrorCapList;
  pHalInjectError HalInjectError;
#endif
} HAL_DISPATCH, *PHAL_DISPATCH;

#if !defined(_NTSYSTEM_) && (defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_))
__CREATE_NTOS_DATA_IMPORT_ALIAS(HalDispatchTable)
extern  PHAL_DISPATCH   HalDispatchTable;
#define HALDISPATCH     HalDispatchTable
#else
extern  HAL_DISPATCH    HalDispatchTable;
#define HALDISPATCH     (&HalDispatchTable)
#endif

// See Version table at:
// https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/ntos/hal/hal_dispatch.htm
#if (NTDDI_VERSION < NTDDI_WIN2K)
#define HAL_DISPATCH_VERSION            1
#elif (NTDDI_VERSION < NTDDI_WINXP)
#define HAL_DISPATCH_VERSION            2
#elif (NTDDI_VERSION < NTDDI_WIN7)
#define HAL_DISPATCH_VERSION            3
#else
#define HAL_DISPATCH_VERSION            4
#endif

#define HalDispatchTableVersion         HALDISPATCH->Version
#define HalQuerySystemInformation       HALDISPATCH->HalQuerySystemInformation
#define HalSetSystemInformation         HALDISPATCH->HalSetSystemInformation
#define HalQueryBusSlots                HALDISPATCH->HalQueryBusSlots
#define HalReferenceHandlerForBus       HALDISPATCH->HalReferenceHandlerForBus
#define HalReferenceBusHandler          HALDISPATCH->HalReferenceBusHandler
#define HalDereferenceBusHandler        HALDISPATCH->HalDereferenceBusHandler
#define HalInitPnpDriver                HALDISPATCH->HalInitPnpDriver
#define HalInitPowerManagement          HALDISPATCH->HalInitPowerManagement
#define HalGetDmaAdapter                HALDISPATCH->HalGetDmaAdapter
#define HalGetInterruptTranslator       HALDISPATCH->HalGetInterruptTranslator
#define HalStartMirroring               HALDISPATCH->HalStartMirroring
#define HalEndMirroring                 HALDISPATCH->HalEndMirroring
#define HalMirrorPhysicalMemory         HALDISPATCH->HalMirrorPhysicalMemory
#define HalEndOfBoot                    HALDISPATCH->HalEndOfBoot
#define HalMirrorVerify                 HALDISPATCH->HalMirrorVerify
#define HalGetCachedAcpiTable           HALDISPATCH->HalGetCachedAcpiTable
#define HalSetPciErrorHandlerCallback   HALDISPATCH->HalSetPciErrorHandlerCallback
#if defined(_IA64_)
#define HalGetErrorCapList              HALDISPATCH->HalGetErrorCapList
#define HalInjectError                  HALDISPATCH->HalInjectError
#endif

typedef struct _HAL_BUS_INFORMATION {
  INTERFACE_TYPE BusType;
  BUS_DATA_TYPE ConfigurationType;
  ULONG BusNumber;
  ULONG Reserved;
} HAL_BUS_INFORMATION, *PHAL_BUS_INFORMATION;

typedef struct _HAL_PROFILE_SOURCE_INFORMATION {
  KPROFILE_SOURCE Source;
  BOOLEAN Supported;
  ULONG Interval;
} HAL_PROFILE_SOURCE_INFORMATION, *PHAL_PROFILE_SOURCE_INFORMATION;

typedef struct _HAL_PROFILE_SOURCE_INFORMATION_EX {
  KPROFILE_SOURCE Source;
  BOOLEAN Supported;
  ULONG_PTR Interval;
  ULONG_PTR DefInterval;
  ULONG_PTR MaxInterval;
  ULONG_PTR MinInterval;
} HAL_PROFILE_SOURCE_INFORMATION_EX, *PHAL_PROFILE_SOURCE_INFORMATION_EX;

typedef struct _HAL_PROFILE_SOURCE_INTERVAL {
  KPROFILE_SOURCE Source;
  ULONG_PTR Interval;
} HAL_PROFILE_SOURCE_INTERVAL, *PHAL_PROFILE_SOURCE_INTERVAL;

typedef struct _HAL_PROFILE_SOURCE_LIST {
  KPROFILE_SOURCE Source;
  PWSTR Description;
} HAL_PROFILE_SOURCE_LIST, *PHAL_PROFILE_SOURCE_LIST;

typedef enum _HAL_DISPLAY_BIOS_INFORMATION {
  HalDisplayInt10Bios,
  HalDisplayEmulatedBios,
  HalDisplayNoBios
} HAL_DISPLAY_BIOS_INFORMATION, *PHAL_DISPLAY_BIOS_INFORMATION;

typedef struct _HAL_POWER_INFORMATION {
  ULONG TBD;
} HAL_POWER_INFORMATION, *PHAL_POWER_INFORMATION;

typedef struct _HAL_PROCESSOR_SPEED_INFO {
  ULONG ProcessorSpeed;
} HAL_PROCESSOR_SPEED_INFORMATION, *PHAL_PROCESSOR_SPEED_INFORMATION;

typedef struct _HAL_CALLBACKS {
  PCALLBACK_OBJECT SetSystemInformation;
  PCALLBACK_OBJECT BusCheck;
} HAL_CALLBACKS, *PHAL_CALLBACKS;

typedef struct _HAL_PROCESSOR_FEATURE {
  ULONG UsableFeatureBits;
} HAL_PROCESSOR_FEATURE;

typedef NTSTATUS
(NTAPI *PHALIOREADWRITEHANDLER)(
  _In_ BOOLEAN fRead,
  _In_ ULONG dwAddr,
  _In_ ULONG dwSize,
  _Inout_ PULONG pdwData);

typedef struct _HAL_AMLI_BAD_IO_ADDRESS_LIST {
  ULONG BadAddrBegin;
  ULONG BadAddrSize;
  ULONG OSVersionTrigger;
  PHALIOREADWRITEHANDLER IOHandler;
} HAL_AMLI_BAD_IO_ADDRESS_LIST, *PHAL_AMLI_BAD_IO_ADDRESS_LIST;

#if defined(_X86_) || defined(_IA64_) || defined(_AMD64_)

typedef VOID
(NTAPI *PHALMCAINTERFACELOCK)(VOID);

typedef VOID
(NTAPI *PHALMCAINTERFACEUNLOCK)(VOID);

typedef NTSTATUS
(NTAPI *PHALMCAINTERFACEREADREGISTER)(
  _In_ UCHAR BankNumber,
  _Inout_ PVOID Exception);

typedef struct _HAL_MCA_INTERFACE {
  PHALMCAINTERFACELOCK Lock;
  PHALMCAINTERFACEUNLOCK Unlock;
  PHALMCAINTERFACEREADREGISTER ReadRegister;
} HAL_MCA_INTERFACE;

typedef enum {
  ApicDestinationModePhysical = 1,
  ApicDestinationModeLogicalFlat,
  ApicDestinationModeLogicalClustered,
  ApicDestinationModeUnknown
} HAL_APIC_DESTINATION_MODE, *PHAL_APIC_DESTINATION_MODE;

#if defined(_AMD64_)

struct _KTRAP_FRAME;
struct _KEXCEPTION_FRAME;

typedef ERROR_SEVERITY
(NTAPI *PDRIVER_EXCPTN_CALLBACK)(
  _In_ PVOID Context,
  _In_ struct _KTRAP_FRAME *TrapFrame,
  _In_ struct _KEXCEPTION_FRAME *ExceptionFrame,
  _In_ PMCA_EXCEPTION Exception);

#endif

#if defined(_X86_) || defined(_IA64_)
typedef
#if defined(_IA64_)
ERROR_SEVERITY
#else
VOID
#endif
(NTAPI *PDRIVER_EXCPTN_CALLBACK)(
  _In_ PVOID Context,
  _In_ PMCA_EXCEPTION BankLog);
#endif

typedef PDRIVER_EXCPTN_CALLBACK PDRIVER_MCA_EXCEPTION_CALLBACK;

typedef struct _MCA_DRIVER_INFO {
  PDRIVER_MCA_EXCEPTION_CALLBACK ExceptionCallback;
  PKDEFERRED_ROUTINE DpcCallback;
  PVOID DeviceContext;
} MCA_DRIVER_INFO, *PMCA_DRIVER_INFO;

typedef struct _HAL_ERROR_INFO {
  ULONG Version;
  ULONG InitMaxSize;
  ULONG McaMaxSize;
  ULONG McaPreviousEventsCount;
  ULONG McaCorrectedEventsCount;
  ULONG McaKernelDeliveryFails;
  ULONG McaDriverDpcQueueFails;
  ULONG McaReserved;
  ULONG CmcMaxSize;
  ULONG CmcPollingInterval;
  ULONG CmcInterruptsCount;
  ULONG CmcKernelDeliveryFails;
  ULONG CmcDriverDpcQueueFails;
  ULONG CmcGetStateFails;
  ULONG CmcClearStateFails;
  ULONG CmcReserved;
  ULONGLONG CmcLogId;
  ULONG CpeMaxSize;
  ULONG CpePollingInterval;
  ULONG CpeInterruptsCount;
  ULONG CpeKernelDeliveryFails;
  ULONG CpeDriverDpcQueueFails;
  ULONG CpeGetStateFails;
  ULONG CpeClearStateFails;
  ULONG CpeInterruptSources;
  ULONGLONG CpeLogId;
  ULONGLONG KernelReserved[4];
} HAL_ERROR_INFO, *PHAL_ERROR_INFO;

#define HAL_MCE_INTERRUPTS_BASED ((ULONG)-1)
#define HAL_MCE_DISABLED          ((ULONG)0)

#define HAL_CMC_INTERRUPTS_BASED  HAL_MCE_INTERRUPTS_BASED
#define HAL_CMC_DISABLED          HAL_MCE_DISABLED

#define HAL_CPE_INTERRUPTS_BASED  HAL_MCE_INTERRUPTS_BASED
#define HAL_CPE_DISABLED          HAL_MCE_DISABLED

#define HAL_MCA_INTERRUPTS_BASED  HAL_MCE_INTERRUPTS_BASED
#define HAL_MCA_DISABLED          HAL_MCE_DISABLED

typedef VOID
(NTAPI *PDRIVER_CMC_EXCEPTION_CALLBACK)(
  _In_ PVOID Context,
  _In_ PCMC_EXCEPTION CmcLog);

typedef VOID
(NTAPI *PDRIVER_CPE_EXCEPTION_CALLBACK)(
  _In_ PVOID Context,
  _In_ PCPE_EXCEPTION CmcLog);

typedef struct _CMC_DRIVER_INFO {
  PDRIVER_CMC_EXCEPTION_CALLBACK ExceptionCallback;
  PKDEFERRED_ROUTINE DpcCallback;
  PVOID DeviceContext;
} CMC_DRIVER_INFO, *PCMC_DRIVER_INFO;

typedef struct _CPE_DRIVER_INFO {
  PDRIVER_CPE_EXCEPTION_CALLBACK ExceptionCallback;
  PKDEFERRED_ROUTINE DpcCallback;
  PVOID DeviceContext;
} CPE_DRIVER_INFO, *PCPE_DRIVER_INFO;

#endif // defined(_X86_) || defined(_IA64_) || defined(_AMD64_)

#if defined(_IA64_)

typedef NTSTATUS
(*HALSENDCROSSPARTITIONIPI)(
  _In_ USHORT ProcessorID,
  _In_ UCHAR HardwareVector);

typedef NTSTATUS
(*HALRESERVECROSSPARTITIONINTERRUPTVECTOR)(
  _Out_ PULONG Vector,
  _Out_ PKIRQL Irql,
  _Inout_ PGROUP_AFFINITY Affinity,
  _Out_ PUCHAR HardwareVector);

typedef VOID
(*HALFREECROSSPARTITIONINTERRUPTVECTOR)(
  _In_ ULONG Vector,
  _In_ PGROUP_AFFINITY Affinity);

typedef struct _HAL_CROSS_PARTITION_IPI_INTERFACE {
  HALSENDCROSSPARTITIONIPI HalSendCrossPartitionIpi;
  HALRESERVECROSSPARTITIONINTERRUPTVECTOR HalReserveCrossPartitionInterruptVector;
  HALFREECROSSPARTITIONINTERRUPTVECTOR HalFreeCrossPartitionInterruptVector;
} HAL_CROSS_PARTITION_IPI_INTERFACE;

#define HAL_CROSS_PARTITION_IPI_INTERFACE_MINIMUM_SIZE \
    FIELD_OFFSET(HAL_CROSS_PARTITION_IPI_INTERFACE,    \
                 HalFreeCrossPartitionInterruptVector)

#endif /* defined(_IA64_) */

typedef struct _HAL_PLATFORM_INFORMATION {
  ULONG PlatformFlags;
} HAL_PLATFORM_INFORMATION, *PHAL_PLATFORM_INFORMATION;

#define HAL_PLATFORM_DISABLE_WRITE_COMBINING      0x01L
#define HAL_PLATFORM_DISABLE_PTCG                 0x04L
#define HAL_PLATFORM_DISABLE_UC_MAIN_MEMORY       0x08L
#define HAL_PLATFORM_ENABLE_WRITE_COMBINING_MMIO  0x10L
#define HAL_PLATFORM_ACPI_TABLES_CACHED           0x20L

$endif (_NTDDK_)
