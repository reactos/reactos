$if (_NTIFS_)
/* Common Cache Functions */

#define CcIsFileCached(FO) (                                                         \
    ((FO)->SectionObjectPointer != NULL) &&                                          \
    (((PSECTION_OBJECT_POINTERS)(FO)->SectionObjectPointer)->SharedCacheMap != NULL) \
)

extern NTKERNELAPI ULONG CcFastMdlReadWait;

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTKERNELAPI
VOID
NTAPI
CcInitializeCacheMap(
  _In_ PFILE_OBJECT FileObject,
  _In_ PCC_FILE_SIZES FileSizes,
  _In_ BOOLEAN PinAccess,
  _In_ PCACHE_MANAGER_CALLBACKS Callbacks,
  _In_ PVOID LazyWriteContext);

NTKERNELAPI
BOOLEAN
NTAPI
CcUninitializeCacheMap(
  _In_ PFILE_OBJECT FileObject,
  _In_opt_ PLARGE_INTEGER TruncateSize,
  _In_opt_ PCACHE_UNINITIALIZE_EVENT UninitializeCompleteEvent);

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
  _In_ PFILE_OBJECT FileObject,
  _In_ ULONG DirtyPageThreshold);

NTKERNELAPI
VOID
NTAPI
CcFlushCache(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
  _In_opt_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _Out_opt_ PIO_STATUS_BLOCK IoStatus);

NTKERNELAPI
LARGE_INTEGER
NTAPI
CcGetFlushedValidData(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
  _In_ BOOLEAN BcbListHeld);

NTKERNELAPI
BOOLEAN
NTAPI
CcZeroData(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER StartOffset,
  _In_ PLARGE_INTEGER EndOffset,
  _In_ BOOLEAN Wait);

NTKERNELAPI
PVOID
NTAPI
CcRemapBcb(
  _In_ PVOID Bcb);

NTKERNELAPI
VOID
NTAPI
CcRepinBcb(
  _In_ PVOID Bcb);

NTKERNELAPI
VOID
NTAPI
CcUnpinRepinnedBcb(
  _In_ PVOID Bcb,
  _In_ BOOLEAN WriteThrough,
  _Out_ PIO_STATUS_BLOCK IoStatus);

NTKERNELAPI
PFILE_OBJECT
NTAPI
CcGetFileObjectFromSectionPtrs(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer);

NTKERNELAPI
PFILE_OBJECT
NTAPI
CcGetFileObjectFromBcb(
  _In_ PVOID Bcb);

NTKERNELAPI
BOOLEAN
NTAPI
CcCanIWrite(
  _In_opt_ PFILE_OBJECT FileObject,
  _In_ ULONG BytesToWrite,
  _In_ BOOLEAN Wait,
  _In_ BOOLEAN Retrying);

NTKERNELAPI
VOID
NTAPI
CcDeferWrite(
  _In_ PFILE_OBJECT FileObject,
  _In_ PCC_POST_DEFERRED_WRITE PostRoutine,
  _In_ PVOID Context1,
  _In_ PVOID Context2,
  _In_ ULONG BytesToWrite,
  _In_ BOOLEAN Retrying);

NTKERNELAPI
BOOLEAN
NTAPI
CcCopyRead(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ BOOLEAN Wait,
  _Out_writes_bytes_(Length) PVOID Buffer,
  _Out_ PIO_STATUS_BLOCK IoStatus);

NTKERNELAPI
VOID
NTAPI
CcFastCopyRead(
  _In_ PFILE_OBJECT FileObject,
  _In_ ULONG FileOffset,
  _In_ ULONG Length,
  _In_ ULONG PageCount,
  _Out_writes_bytes_(Length) PVOID Buffer,
  _Out_ PIO_STATUS_BLOCK IoStatus);

NTKERNELAPI
BOOLEAN
NTAPI
CcCopyWrite(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ BOOLEAN Wait,
  _In_reads_bytes_(Length) PVOID Buffer);

NTKERNELAPI
VOID
NTAPI
CcFastCopyWrite(
  _In_ PFILE_OBJECT FileObject,
  _In_ ULONG FileOffset,
  _In_ ULONG Length,
  _In_reads_bytes_(Length) PVOID Buffer);

NTKERNELAPI
VOID
NTAPI
CcMdlRead(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _Out_ PMDL *MdlChain,
  _Out_ PIO_STATUS_BLOCK IoStatus);

NTKERNELAPI
VOID
NTAPI
CcMdlReadComplete(
  _In_ PFILE_OBJECT FileObject,
  _In_ PMDL MdlChain);

NTKERNELAPI
VOID
NTAPI
CcPrepareMdlWrite(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _Out_ PMDL *MdlChain,
  _Out_ PIO_STATUS_BLOCK IoStatus);

NTKERNELAPI
VOID
NTAPI
CcMdlWriteComplete(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ PMDL MdlChain);

NTKERNELAPI
VOID
NTAPI
CcScheduleReadAhead(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length);

NTKERNELAPI
NTSTATUS
NTAPI
CcWaitForCurrentLazyWriterActivity(VOID);

NTKERNELAPI
VOID
NTAPI
CcSetReadAheadGranularity(
  _In_ PFILE_OBJECT FileObject,
  _In_ ULONG Granularity);

NTKERNELAPI
BOOLEAN
NTAPI
CcPinRead(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ ULONG Flags,
  _Outptr_ PVOID *Bcb,
  _Outptr_result_bytebuffer_(Length) PVOID *Buffer);

NTKERNELAPI
BOOLEAN
NTAPI
CcPinMappedData(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ ULONG Flags,
  _Inout_ PVOID *Bcb);

NTKERNELAPI
BOOLEAN
NTAPI
CcPreparePinWrite(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ BOOLEAN Zero,
  _In_ ULONG Flags,
  _Outptr_ PVOID *Bcb,
  _Outptr_result_bytebuffer_(Length) PVOID *Buffer);

NTKERNELAPI
VOID
NTAPI
CcSetDirtyPinnedData(
  _In_ PVOID BcbVoid,
  _In_opt_ PLARGE_INTEGER Lsn);

NTKERNELAPI
VOID
NTAPI
CcUnpinData(
  _In_ PVOID Bcb);

NTKERNELAPI
VOID
NTAPI
CcSetBcbOwnerPointer(
  _In_ PVOID Bcb,
  _In_ PVOID OwnerPointer);

NTKERNELAPI
VOID
NTAPI
CcUnpinDataForThread(
  _In_ PVOID Bcb,
  _In_ ERESOURCE_THREAD ResourceThreadId);

NTKERNELAPI
VOID
NTAPI
CcSetAdditionalCacheAttributes(
  _In_ PFILE_OBJECT FileObject,
  _In_ BOOLEAN DisableReadAhead,
  _In_ BOOLEAN DisableWriteBehind);

NTKERNELAPI
BOOLEAN
NTAPI
CcIsThereDirtyData(
  _In_ PVPB Vpb);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)

NTKERNELAPI
VOID
NTAPI
CcMdlWriteAbort(
  _In_ PFILE_OBJECT FileObject,
  _In_ PMDL MdlChain);

NTKERNELAPI
VOID
NTAPI
CcSetLogHandleForFile(
  _In_ PFILE_OBJECT FileObject,
  _In_ PVOID LogHandle,
  _In_ PFLUSH_TO_LSN FlushToLsnRoutine);

NTKERNELAPI
LARGE_INTEGER
NTAPI
CcGetDirtyPages(
  _In_ PVOID LogHandle,
  _In_ PDIRTY_PAGE_ROUTINE DirtyPageRoutine,
  _In_ PVOID Context1,
  _In_ PVOID Context2);

#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
_Success_(return!=FALSE)
NTKERNELAPI
BOOLEAN
NTAPI
CcMapData(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ ULONG Flags,
  _Outptr_ PVOID *Bcb,
  _Outptr_result_bytebuffer_(Length) PVOID *Buffer);
#elif (NTDDI_VERSION >= NTDDI_WIN2K)
NTKERNELAPI
BOOLEAN
NTAPI
CcMapData(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ BOOLEAN Wait,
  _Outptr_ PVOID *Bcb,
  _Outptr_result_bytebuffer_(Length) PVOID *Buffer);
#endif

#if (NTDDI_VERSION >= NTDDI_VISTA)

NTKERNELAPI
NTSTATUS
NTAPI
CcSetFileSizesEx(
  _In_ PFILE_OBJECT FileObject,
  _In_ PCC_FILE_SIZES FileSizes);

NTKERNELAPI
PFILE_OBJECT
NTAPI
CcGetFileObjectFromSectionPtrsRef(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer);

NTKERNELAPI
VOID
NTAPI
CcSetParallelFlushFile(
  _In_ PFILE_OBJECT FileObject,
  _In_ BOOLEAN EnableParallelFlush);

NTKERNELAPI
BOOLEAN
CcIsThereDirtyDataEx(
  _In_ PVPB Vpb,
  _In_opt_ PULONG NumberOfDirtyPages);

#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTKERNELAPI
VOID
NTAPI
CcCoherencyFlushAndPurgeCache(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
  _In_opt_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _Out_ PIO_STATUS_BLOCK IoStatus,
  _In_opt_ ULONG Flags);
#endif

#define CcGetFileSizePointer(FO) (                                     \
    ((PLARGE_INTEGER)((FO)->SectionObjectPointer->SharedCacheMap) + 1) \
)

#if (NTDDI_VERSION >= NTDDI_VISTA)
NTKERNELAPI
BOOLEAN
NTAPI
CcPurgeCacheSection(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
  _In_opt_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ ULONG Flags);
#elif (NTDDI_VERSION >= NTDDI_WIN2K)
NTKERNELAPI
BOOLEAN
NTAPI
CcPurgeCacheSection(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
  _In_opt_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ BOOLEAN UninitializeCacheMaps);
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTKERNELAPI
BOOLEAN
NTAPI
CcCopyWriteWontFlush(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length);
#else
#define CcCopyWriteWontFlush(FO, FOFF, LEN) ((LEN) <= 0x10000)
#endif

#define CcReadAhead(FO, FOFF, LEN) (                \
    if ((LEN) >= 256) {                             \
        CcScheduleReadAhead((FO), (FOFF), (LEN));   \
    }                                               \
)
$endif (_NTIFS_)
