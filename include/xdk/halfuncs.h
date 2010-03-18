
#if defined(USE_DMA_MACROS) && !defined(_NTHAL_) && (defined(_NTDDK_) || defined(_NTDRIVER_)) || defined(_WDM_INCLUDED_)
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

#endif

