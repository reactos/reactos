$if (_NTIFS_)
/* Common Cache Functions */

#define CcIsFileCached(FO) (                                                         \
    ((FO)->SectionObjectPointer != NULL) &&                                          \
    (((PSECTION_OBJECT_POINTERS)(FO)->SectionObjectPointer)->SharedCacheMap != NULL) \
)

extern ULONG CcFastMdlReadWait;

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTKERNELAPI
VOID
NTAPI
CcInitializeCacheMap(
  IN PFILE_OBJECT FileObject,
  IN PCC_FILE_SIZES FileSizes,
  IN BOOLEAN PinAccess,
  IN PCACHE_MANAGER_CALLBACKS Callbacks,
  IN PVOID LazyWriteContext);

NTKERNELAPI
BOOLEAN
NTAPI
CcUninitializeCacheMap(
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER TruncateSize OPTIONAL,
  IN PCACHE_UNINITIALIZE_EVENT UninitializeCompleteEvent OPTIONAL);

NTKERNELAPI
VOID
NTAPI
CcSetFileSizes(
  IN PFILE_OBJECT FileObject,
  IN PCC_FILE_SIZES FileSizes);

NTKERNELAPI
VOID
NTAPI
CcSetDirtyPageThreshold(
  IN PFILE_OBJECT FileObject,
  IN ULONG DirtyPageThreshold);

NTKERNELAPI
VOID
NTAPI
CcFlushCache(
  IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
  IN PLARGE_INTEGER FileOffset OPTIONAL,
  IN ULONG Length,
  OUT PIO_STATUS_BLOCK IoStatus OPTIONAL);

NTKERNELAPI
LARGE_INTEGER
NTAPI
CcGetFlushedValidData(
  IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
  IN BOOLEAN BcbListHeld);

NTKERNELAPI
BOOLEAN
NTAPI
CcZeroData(
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER StartOffset,
  IN PLARGE_INTEGER EndOffset,
  IN BOOLEAN Wait);

NTKERNELAPI
PVOID
NTAPI
CcRemapBcb(
  IN PVOID Bcb);

NTKERNELAPI
VOID
NTAPI
CcRepinBcb(
  IN PVOID Bcb);

NTKERNELAPI
VOID
NTAPI
CcUnpinRepinnedBcb(
  IN PVOID Bcb,
  IN BOOLEAN WriteThrough,
  OUT PIO_STATUS_BLOCK IoStatus);

NTKERNELAPI
PFILE_OBJECT
NTAPI
CcGetFileObjectFromSectionPtrs(
  IN PSECTION_OBJECT_POINTERS SectionObjectPointer);

NTKERNELAPI
PFILE_OBJECT
NTAPI
CcGetFileObjectFromBcb(
  IN PVOID Bcb);

NTKERNELAPI
BOOLEAN
NTAPI
CcCanIWrite(
  IN PFILE_OBJECT FileObject,
  IN ULONG BytesToWrite,
  IN BOOLEAN Wait,
  IN BOOLEAN Retrying);

NTKERNELAPI
VOID
NTAPI
CcDeferWrite(
  IN PFILE_OBJECT FileObject,
  IN PCC_POST_DEFERRED_WRITE PostRoutine,
  IN PVOID Context1,
  IN PVOID Context2,
  IN ULONG BytesToWrite,
  IN BOOLEAN Retrying);

NTKERNELAPI
BOOLEAN
NTAPI
CcCopyRead(
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  IN BOOLEAN Wait,
  OUT PVOID Buffer,
  OUT PIO_STATUS_BLOCK IoStatus);

NTKERNELAPI
VOID
NTAPI
CcFastCopyRead(
  IN PFILE_OBJECT FileObject,
  IN ULONG FileOffset,
  IN ULONG Length,
  IN ULONG PageCount,
  OUT PVOID Buffer,
  OUT PIO_STATUS_BLOCK IoStatus);

NTKERNELAPI
BOOLEAN
NTAPI
CcCopyWrite(
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  IN BOOLEAN Wait,
  IN PVOID Buffer);

NTKERNELAPI
VOID
NTAPI
CcFastCopyWrite(
  IN PFILE_OBJECT FileObject,
  IN ULONG FileOffset,
  IN ULONG Length,
  IN PVOID Buffer);

NTKERNELAPI
VOID
NTAPI
CcMdlRead(
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  OUT PMDL *MdlChain,
  OUT PIO_STATUS_BLOCK IoStatus);

NTKERNELAPI
VOID
NTAPI
CcMdlReadComplete(
  IN PFILE_OBJECT FileObject,
  IN PMDL MdlChain);

NTKERNELAPI
VOID
NTAPI
CcPrepareMdlWrite(
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  OUT PMDL *MdlChain,
  OUT PIO_STATUS_BLOCK IoStatus);

NTKERNELAPI
VOID
NTAPI
CcMdlWriteComplete(
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN PMDL MdlChain);

NTKERNELAPI
VOID
NTAPI
CcScheduleReadAhead(
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length);

NTKERNELAPI
NTSTATUS
NTAPI
CcWaitForCurrentLazyWriterActivity(
  VOID);

NTKERNELAPI
VOID
NTAPI
CcSetReadAheadGranularity(
  IN PFILE_OBJECT FileObject,
  IN ULONG Granularity);

NTKERNELAPI
BOOLEAN
NTAPI
CcPinRead(
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  IN ULONG Flags,
  OUT PVOID *Bcb,
  OUT PVOID *Buffer);

NTKERNELAPI
BOOLEAN
NTAPI
CcPinMappedData(
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  IN ULONG Flags,
  IN OUT PVOID *Bcb);

NTKERNELAPI
BOOLEAN
NTAPI
CcPreparePinWrite(
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  IN BOOLEAN Zero,
  IN ULONG Flags,
  OUT PVOID *Bcb,
  OUT PVOID *Buffer);

NTKERNELAPI
VOID
NTAPI
CcSetDirtyPinnedData(
  IN PVOID BcbVoid,
  IN PLARGE_INTEGER Lsn OPTIONAL);

NTKERNELAPI
VOID
NTAPI
CcUnpinData(
  IN PVOID Bcb);

NTKERNELAPI
VOID
NTAPI
CcSetBcbOwnerPointer(
  IN PVOID Bcb,
  IN PVOID OwnerPointer);

NTKERNELAPI
VOID
NTAPI
CcUnpinDataForThread(
  IN PVOID Bcb,
  IN ERESOURCE_THREAD ResourceThreadId);

NTKERNELAPI
VOID
NTAPI
CcSetAdditionalCacheAttributes(
  IN PFILE_OBJECT FileObject,
  IN BOOLEAN DisableReadAhead,
  IN BOOLEAN DisableWriteBehind);

NTKERNELAPI
BOOLEAN
NTAPI
CcIsThereDirtyData(
  IN PVPB Vpb);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)

NTKERNELAPI
VOID
NTAPI
CcMdlWriteAbort(
  IN PFILE_OBJECT FileObject,
  IN PMDL MdlChain);

NTKERNELAPI
VOID
NTAPI
CcSetLogHandleForFile(
  IN PFILE_OBJECT FileObject,
  IN PVOID LogHandle,
  IN PFLUSH_TO_LSN FlushToLsnRoutine);

NTKERNELAPI
LARGE_INTEGER
NTAPI
CcGetDirtyPages(
  IN PVOID LogHandle,
  IN PDIRTY_PAGE_ROUTINE DirtyPageRoutine,
  IN PVOID Context1,
  IN PVOID Context2);

#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTKERNELAPI
BOOLEAN
NTAPI
CcMapData(
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  IN ULONG Flags,
  OUT PVOID *Bcb,
  OUT PVOID *Buffer);
#elif (NTDDI_VERSION >= NTDDI_WIN2K)
NTKERNELAPI
BOOLEAN
NTAPI
CcMapData(
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  IN BOOLEAN Wait,
  OUT PVOID *Bcb,
  OUT PVOID *Buffer);
#endif

#if (NTDDI_VERSION >= NTDDI_VISTA)

NTKERNELAPI
NTSTATUS
NTAPI
CcSetFileSizesEx(
  IN PFILE_OBJECT FileObject,
  IN PCC_FILE_SIZES FileSizes);

NTKERNELAPI
PFILE_OBJECT
NTAPI
CcGetFileObjectFromSectionPtrsRef(
  IN PSECTION_OBJECT_POINTERS SectionObjectPointer);

NTKERNELAPI
VOID
NTAPI
CcSetParallelFlushFile(
  IN PFILE_OBJECT FileObject,
  IN BOOLEAN EnableParallelFlush);

NTKERNELAPI
BOOLEAN
CcIsThereDirtyDataEx(
  IN PVPB Vpb,
  IN PULONG NumberOfDirtyPages OPTIONAL);

#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTKERNELAPI
VOID
NTAPI
CcCoherencyFlushAndPurgeCache(
  IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
  IN PLARGE_INTEGER FileOffset OPTIONAL,
  IN ULONG Length,
  OUT PIO_STATUS_BLOCK IoStatus,
  IN ULONG Flags OPTIONAL);
#endif

#define CcGetFileSizePointer(FO) (                                     \
    ((PLARGE_INTEGER)((FO)->SectionObjectPointer->SharedCacheMap) + 1) \
)

#if (NTDDI_VERSION >= NTDDI_VISTA)
NTKERNELAPI
BOOLEAN
NTAPI
CcPurgeCacheSection(
  IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
  IN PLARGE_INTEGER FileOffset OPTIONAL,
  IN ULONG Length,
  IN ULONG Flags);
#elif (NTDDI_VERSION >= NTDDI_WIN2K)
NTKERNELAPI
BOOLEAN
NTAPI
CcPurgeCacheSection(
  IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
  IN PLARGE_INTEGER FileOffset OPTIONAL,
  IN ULONG Length,
  IN BOOLEAN UninitializeCacheMaps);
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTKERNELAPI
BOOLEAN
NTAPI
CcCopyWriteWontFlush(
  IN PFILE_OBJECT FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length);
#else
#define CcCopyWriteWontFlush(FO, FOFF, LEN) ((LEN) <= 0x10000)
#endif

#define CcReadAhead(FO, FOFF, LEN) (                \
    if ((LEN) >= 256) {                             \
        CcScheduleReadAhead((FO), (FOFF), (LEN));   \
    }                                               \
)
$endif (_NTIFS_)
