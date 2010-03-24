/* Hardware Abstraction Layer Functions */

#if (NTDDI_VERSION >= NTDDI_WIN2K)

#if defined(USE_DMA_MACROS) && !defined(_NTHAL_) && (defined(_NTDDK_) || defined(_NTDRIVER_)) || defined(_WDM_INCLUDED_)
$if (_WDMDDK_)

FORCEINLINE
PVOID
NTAPI
HalAllocateCommonBuffer(
  IN PDMA_ADAPTER DmaAdapter,
  IN ULONG Length,
  OUT PPHYSICAL_ADDRESS LogicalAddress,
  IN BOOLEAN CacheEnabled)
{
  PALLOCATE_COMMON_BUFFER allocateCommonBuffer;
  PVOID commonBuffer;

  allocateCommonBuffer = *(DmaAdapter)->DmaOperations->AllocateCommonBuffer;
  ASSERT( allocateCommonBuffer != NULL );
  commonBuffer = allocateCommonBuffer( DmaAdapter, Length, LogicalAddress, CacheEnabled );
  return commonBuffer;
}

FORCEINLINE
VOID
NTAPI
HalFreeCommonBuffer(
  IN PDMA_ADAPTER DmaAdapter,
  IN ULONG Length,
  IN PHYSICAL_ADDRESS LogicalAddress,
  IN PVOID VirtualAddress,
  IN BOOLEAN CacheEnabled)
{
  PFREE_COMMON_BUFFER freeCommonBuffer;

  freeCommonBuffer = *(DmaAdapter)->DmaOperations->FreeCommonBuffer;
  ASSERT( freeCommonBuffer != NULL );
  freeCommonBuffer( DmaAdapter, Length, LogicalAddress, VirtualAddress, CacheEnabled );
}

FORCEINLINE
ULONG
NTAPI
HalReadDmaCounter(
  IN PDMA_ADAPTER DmaAdapter)
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
  IN PDMA_ADAPTER DmaAdapter)
{
  PGET_DMA_ALIGNMENT getDmaAlignment;
  ULONG alignment;

  getDmaAlignment = *(DmaAdapter)->DmaOperations->GetDmaAlignment;
  ASSERT( getDmaAlignment != NULL );
  alignment = getDmaAlignment( DmaAdapter );
  return alignment;
}

$endif
$if (_NTDDK_)

/* Nothing here */

#else /* USE_DMA_MACROS ... */

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
VOID
NTAPI
IoFreeAdapterChannel(
  IN PADAPTER_OBJECT AdapterObject);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
BOOLEAN
NTAPI
IoFlushAdapterBuffers(
  IN PADAPTER_OBJECT AdapterObject,
  IN PMDL Mdl,
  IN PVOID MapRegisterBase,
  IN PVOID CurrentVa,
  IN ULONG Length,
  IN BOOLEAN WriteToDevice);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
VOID
NTAPI
IoFreeMapRegisters(
  IN PADAPTER_OBJECT AdapterObject,
  IN PVOID MapRegisterBase,
  IN ULONG NumberOfMapRegisters);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
PVOID
NTAPI
HalAllocateCommonBuffer(
  IN PADAPTER_OBJECT AdapterObject,
  IN ULONG Length,
  OUT PPHYSICAL_ADDRESS LogicalAddress,
  IN BOOLEAN CacheEnabled);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
VOID
NTAPI
HalFreeCommonBuffer(
  IN PADAPTER_OBJECT AdapterObject,
  IN ULONG Length,
  IN PHYSICAL_ADDRESS LogicalAddress,
  IN PVOID VirtualAddress,
  IN BOOLEAN CacheEnabled);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
ULONG
NTAPI
HalReadDmaCounter(
  IN PADAPTER_OBJECT AdapterObject);

NTHALAPI
NTSTATUS
NTAPI
HalAllocateAdapterChannel(
  IN PADAPTER_OBJECT  AdapterObject,
  IN PWAIT_CONTEXT_BLOCK  Wcb,
  IN ULONG  NumberOfMapRegisters,
  IN PDRIVER_CONTROL  ExecutionRoutine);

$endif /* _NTDDK_ */
#endif /* USE_DMA_MACROS ... */
$if (_NTDDK_)

#if !defined(NO_LEGACY_DRIVERS)
NTHALAPI
NTSTATUS
NTAPI
HalAssignSlotResources(
  IN PUNICODE_STRING RegistryPath,
  IN PUNICODE_STRING DriverClassName,
  IN PDRIVER_OBJECT DriverObject,
  IN PDEVICE_OBJECT DeviceObject,
  IN INTERFACE_TYPE BusType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  IN OUT PCM_RESOURCE_LIST *AllocatedResources);

NTHALAPI
ULONG
NTAPI
HalGetInterruptVector(
  IN INTERFACE_TYPE InterfaceType,
  IN ULONG BusNumber,
  IN ULONG BusInterruptLevel,
  IN ULONG BusInterruptVector,
  OUT PKIRQL Irql,
  OUT PKAFFINITY Affinity);

NTHALAPI
ULONG
NTAPI
HalSetBusData(
  IN BUS_DATA_TYPE BusDataType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  IN PVOID Buffer,
  IN ULONG Length);

NTHALAPI
ULONG
NTAPI
HalGetBusData(
  IN BUS_DATA_TYPE BusDataType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  OUT PVOID Buffer,
  IN ULONG Length);

NTHALAPI
BOOLEAN
NTAPI
HalMakeBeep(
  IN ULONG Frequency);
#endif /* !defined(NO_LEGACY_DRIVERS) */

NTHALAPI
PADAPTER_OBJECT
NTAPI
HalGetAdapter(
  IN PDEVICE_DESCRIPTION DeviceDescription,
  OUT PULONG NumberOfMapRegisters);

VOID
NTAPI
HalPutDmaAdapter(
  IN PADAPTER_OBJECT DmaAdapter);

NTHALAPI
VOID
NTAPI
HalAcquireDisplayOwnership(
  IN PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters);

NTHALAPI
ULONG
NTAPI
HalGetBusDataByOffset(
  IN BUS_DATA_TYPE BusDataType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  OUT PVOID Buffer,
  IN ULONG Offset,
  IN ULONG Length);

NTHALAPI
ULONG
NTAPI
HalSetBusDataByOffset(
  IN BUS_DATA_TYPE BusDataType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  IN PVOID Buffer,
  IN ULONG Offset,
  IN ULONG Length);

NTHALAPI
BOOLEAN
NTAPI
HalTranslateBusAddress(
  IN INTERFACE_TYPE InterfaceType,
  IN ULONG BusNumber,
  IN PHYSICAL_ADDRESS BusAddress,
  IN OUT PULONG AddressSpace,
  OUT PPHYSICAL_ADDRESS TranslatedAddress);

NTHALAPI
PVOID
NTAPI
HalAllocateCrashDumpRegisters(
  IN PADAPTER_OBJECT AdapterObject,
  IN OUT PULONG NumberOfMapRegisters);

NTSTATUS
NTAPI
HalGetScatterGatherList(
  IN PADAPTER_OBJECT DmaAdapter,
  IN PDEVICE_OBJECT DeviceObject,
  IN PMDL Mdl,
  IN PVOID CurrentVa,
  IN ULONG Length,
  IN PDRIVER_LIST_CONTROL ExecutionRoutine,
  IN PVOID Context,
  IN BOOLEAN WriteToDevice);

VOID
NTAPI
HalPutScatterGatherList(
  IN PADAPTER_OBJECT DmaAdapter,
  IN PSCATTER_GATHER_LIST ScatterGather,
  IN BOOLEAN WriteToDevice);

$endif /* _NTDDK_ */
#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */
$if (_NTDDK_)

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTKERNELAPI
VOID
FASTCALL
HalExamineMBR(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG SectorSize,
  IN ULONG MBRTypeIdentifier,
  OUT PVOID *Buffer);
#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_WIN7)

NTSTATUS
NTAPI
HalAllocateHardwareCounters(
  IN PGROUP_AFFINITY GroupAffinty,
  IN ULONG GroupCount,
  IN PPHYSICAL_COUNTER_RESOURCE_LIST ResourceList,
  OUT PHANDLE CounterSetHandle);

NTSTATUS
NTAPI
HalFreeHardwareCounters(
  IN HANDLE CounterSetHandle);

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
  IN PWHEA_ERROR_SOURCE_DESCRIPTOR ErrorSource,
  IN PWHEA_ERROR_RECORD ErrorRecord);

#else

typedef struct _WHEA_ERROR_RECORD *PWHEA_ERROR_RECORD;

NTHALAPI
VOID
NTAPI
HalBugCheckSystem(
  IN PWHEA_ERROR_RECORD ErrorRecord);

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

$endif /* _NTDDK_ */

