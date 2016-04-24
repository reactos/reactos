/* Hardware Abstraction Layer Functions */

#if (NTDDI_VERSION >= NTDDI_WIN2K)

#if defined(USE_DMA_MACROS) && !defined(_NTHAL_) && (defined(_NTDDK_) || defined(_NTDRIVER_)) || defined(_WDM_INCLUDED_)

$if (_WDMDDK_)
__drv_preferredFunction("AllocateCommonBuffer","Obsolete")
FORCEINLINE
PVOID
NTAPI
HalAllocateCommonBuffer(
  _In_ PDMA_ADAPTER DmaAdapter,
  _In_ ULONG Length,
  _Out_ PPHYSICAL_ADDRESS LogicalAddress,
  _In_ BOOLEAN CacheEnabled)
{
  PALLOCATE_COMMON_BUFFER allocateCommonBuffer;
  PVOID commonBuffer;

  allocateCommonBuffer = *(DmaAdapter)->DmaOperations->AllocateCommonBuffer;
  ASSERT( allocateCommonBuffer != NULL );
  commonBuffer = allocateCommonBuffer( DmaAdapter, Length, LogicalAddress, CacheEnabled );
  return commonBuffer;
}

__drv_preferredFunction("FreeCommonBuffer","Obsolete")
FORCEINLINE
VOID
NTAPI
HalFreeCommonBuffer(
  _In_ PDMA_ADAPTER DmaAdapter,
  _In_ ULONG Length,
  _In_ PHYSICAL_ADDRESS LogicalAddress,
  _In_ PVOID VirtualAddress,
  _In_ BOOLEAN CacheEnabled)
{
  PFREE_COMMON_BUFFER freeCommonBuffer;

  freeCommonBuffer = *(DmaAdapter)->DmaOperations->FreeCommonBuffer;
  ASSERT( freeCommonBuffer != NULL );
  freeCommonBuffer( DmaAdapter, Length, LogicalAddress, VirtualAddress, CacheEnabled );
}

__drv_preferredFunction("ReadDmaCounter","Obsolete")
FORCEINLINE
ULONG
NTAPI
HalReadDmaCounter(
  _In_ PDMA_ADAPTER DmaAdapter)
{
  PREAD_DMA_COUNTER readDmaCounter;
  ULONG counter;

  readDmaCounter = *(DmaAdapter)->DmaOperations->ReadDmaCounter;
  ASSERT( readDmaCounter != NULL );
  counter = readDmaCounter( DmaAdapter );
  return counter;
}

FORCEINLINE
ULONG
HalGetDmaAlignment(
  _In_ PDMA_ADAPTER DmaAdapter)
{
  PGET_DMA_ALIGNMENT getDmaAlignment;
  ULONG alignment;

  getDmaAlignment = *(DmaAdapter)->DmaOperations->GetDmaAlignment;
  ASSERT( getDmaAlignment != NULL );
  alignment = getDmaAlignment( DmaAdapter );
  return alignment;
}

$endif  (_WDMDDK_)
$if (_NTDDK_)
/* Nothing here */

#else /* USE_DMA_MACROS ... */

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
VOID
NTAPI
IoFreeAdapterChannel(
  _In_ PADAPTER_OBJECT AdapterObject);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
BOOLEAN
NTAPI
IoFlushAdapterBuffers(
  _In_ PADAPTER_OBJECT AdapterObject,
  _In_ PMDL Mdl,
  _In_ PVOID MapRegisterBase,
  _In_ PVOID CurrentVa,
  _In_ ULONG Length,
  _In_ BOOLEAN WriteToDevice);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
VOID
NTAPI
IoFreeMapRegisters(
  _In_ PADAPTER_OBJECT AdapterObject,
  _In_ PVOID MapRegisterBase,
  _In_ ULONG NumberOfMapRegisters);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
PVOID
NTAPI
HalAllocateCommonBuffer(
  _In_ PADAPTER_OBJECT AdapterObject,
  _In_ ULONG Length,
  _Out_ PPHYSICAL_ADDRESS LogicalAddress,
  _In_ BOOLEAN CacheEnabled);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
VOID
NTAPI
HalFreeCommonBuffer(
  _In_ PADAPTER_OBJECT AdapterObject,
  _In_ ULONG Length,
  _In_ PHYSICAL_ADDRESS LogicalAddress,
  _In_ PVOID VirtualAddress,
  _In_ BOOLEAN CacheEnabled);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
ULONG
NTAPI
HalReadDmaCounter(
  _In_ PADAPTER_OBJECT AdapterObject);

NTHALAPI
NTSTATUS
NTAPI
HalAllocateAdapterChannel(
  _In_ PADAPTER_OBJECT AdapterObject,
  _In_ PWAIT_CONTEXT_BLOCK Wcb,
  _In_ ULONG NumberOfMapRegisters,
  _In_ PDRIVER_CONTROL ExecutionRoutine);

$endif (_NTDDK_)
#endif /* USE_DMA_MACROS ... */

$if (_NTDDK_)
#if !defined(NO_LEGACY_DRIVERS)
NTHALAPI
NTSTATUS
NTAPI
HalAssignSlotResources(
  _In_ PUNICODE_STRING RegistryPath,
  _In_opt_ PUNICODE_STRING DriverClassName,
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ INTERFACE_TYPE BusType,
  _In_ ULONG BusNumber,
  _In_ ULONG SlotNumber,
  _Inout_ PCM_RESOURCE_LIST *AllocatedResources);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTHALAPI
ULONG
NTAPI
HalGetInterruptVector(
  _In_ INTERFACE_TYPE InterfaceType,
  _In_ ULONG BusNumber,
  _In_ ULONG BusInterruptLevel,
  _In_ ULONG BusInterruptVector,
  _Out_ PKIRQL Irql,
  _Out_ PKAFFINITY Affinity);

NTHALAPI
ULONG
NTAPI
HalSetBusData(
  _In_ BUS_DATA_TYPE BusDataType,
  _In_ ULONG BusNumber,
  _In_ ULONG SlotNumber,
  _In_reads_bytes_(Length) PVOID Buffer,
  _In_ ULONG Length);

NTHALAPI
ULONG
NTAPI
HalGetBusData(
  _In_ BUS_DATA_TYPE BusDataType,
  _In_ ULONG BusNumber,
  _In_ ULONG SlotNumber,
  _Out_writes_bytes_(Length) PVOID Buffer,
  _In_ ULONG Length);

NTHALAPI
BOOLEAN
NTAPI
HalMakeBeep(
  _In_ ULONG Frequency);
#endif /* !defined(NO_LEGACY_DRIVERS) */

_IRQL_requires_max_(PASSIVE_LEVEL)
NTHALAPI
PADAPTER_OBJECT
NTAPI
HalGetAdapter(
  _In_ PDEVICE_DESCRIPTION DeviceDescription,
  _Out_ PULONG NumberOfMapRegisters);

VOID
NTAPI
HalPutDmaAdapter(
  _In_ PADAPTER_OBJECT DmaAdapter);

NTHALAPI
VOID
NTAPI
HalAcquireDisplayOwnership(
  _In_ PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters);

NTHALAPI
ULONG
NTAPI
HalGetBusDataByOffset(
  _In_ BUS_DATA_TYPE BusDataType,
  _In_ ULONG BusNumber,
  _In_ ULONG SlotNumber,
  _Out_writes_bytes_(Length) PVOID Buffer,
  _In_ ULONG Offset,
  _In_ ULONG Length);

NTHALAPI
ULONG
NTAPI
HalSetBusDataByOffset(
  _In_ BUS_DATA_TYPE BusDataType,
  _In_ ULONG BusNumber,
  _In_ ULONG SlotNumber,
  _In_reads_bytes_(Length) PVOID Buffer,
  _In_ ULONG Offset,
  _In_ ULONG Length);

NTHALAPI
BOOLEAN
NTAPI
HalTranslateBusAddress(
  _In_ INTERFACE_TYPE InterfaceType,
  _In_ ULONG BusNumber,
  _In_ PHYSICAL_ADDRESS BusAddress,
  _Inout_ PULONG AddressSpace,
  _Out_ PPHYSICAL_ADDRESS TranslatedAddress);

NTHALAPI
PVOID
NTAPI
HalAllocateCrashDumpRegisters(
  _In_ PADAPTER_OBJECT AdapterObject,
  _Inout_ PULONG NumberOfMapRegisters);

NTSTATUS
NTAPI
HalGetScatterGatherList(
  _In_ PADAPTER_OBJECT DmaAdapter,
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PMDL Mdl,
  _In_ PVOID CurrentVa,
  _In_ ULONG Length,
  _In_ PDRIVER_LIST_CONTROL ExecutionRoutine,
  _In_ PVOID Context,
  _In_ BOOLEAN WriteToDevice);

VOID
NTAPI
HalPutScatterGatherList(
  _In_ PADAPTER_OBJECT DmaAdapter,
  _In_ PSCATTER_GATHER_LIST ScatterGather,
  _In_ BOOLEAN WriteToDevice);

$endif (_NTDDK_)
#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

$if (_NTDDK_)
#if (NTDDI_VERSION >= NTDDI_WINXP)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
FASTCALL
HalExamineMBR(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ ULONG SectorSize,
  _In_ ULONG MBRTypeIdentifier,
  _Out_ PVOID *Buffer);
#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_WIN7)

NTSTATUS
NTAPI
HalAllocateHardwareCounters(
  _In_reads_(GroupCount) PGROUP_AFFINITY GroupAffinty,
  _In_ ULONG GroupCount,
  _In_ PPHYSICAL_COUNTER_RESOURCE_LIST ResourceList,
  _Out_ PHANDLE CounterSetHandle);

NTSTATUS
NTAPI
HalFreeHardwareCounters(
  _In_ HANDLE CounterSetHandle);

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

#if defined(_IA64_)
#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTHALAPI
ULONG
NTAPI
HalGetDmaAlignmentRequirement(VOID);
#endif
#endif /* defined(_IA64_) */

#if defined(_M_IX86) || defined(_M_AMD64)
#define HalGetDmaAlignmentRequirement() 1L
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)

typedef struct _WHEA_ERROR_SOURCE_DESCRIPTOR *PWHEA_ERROR_SOURCE_DESCRIPTOR;
typedef struct _WHEA_ERROR_RECORD *PWHEA_ERROR_RECORD;

NTHALAPI
VOID
NTAPI
HalBugCheckSystem(
  _In_ PWHEA_ERROR_SOURCE_DESCRIPTOR ErrorSource,
  _In_ PWHEA_ERROR_RECORD ErrorRecord);

#else

typedef struct _WHEA_ERROR_RECORD *PWHEA_ERROR_RECORD;

NTHALAPI
VOID
NTAPI
HalBugCheckSystem(
  _In_ PWHEA_ERROR_RECORD ErrorRecord);

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

$endif (_NTDDK_)
