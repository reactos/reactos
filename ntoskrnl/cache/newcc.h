#pragma once

typedef struct _NOCC_BCB
{
    /* Public part */
    PUBLIC_BCB Bcb;

    struct _NOCC_CACHE_MAP *Map;
    PSECTION SectionObject;
    LARGE_INTEGER FileOffset;
    ULONG Length;
    PVOID BaseAddress;
    BOOLEAN Dirty;
    PVOID OwnerPointer;

    /* Reference counts */
    ULONG RefCount;

    LIST_ENTRY ThisFileList;

    KEVENT ExclusiveWait;
    ULONG ExclusiveWaiter;
    BOOLEAN Exclusive;
} NOCC_BCB, *PNOCC_BCB;

typedef struct _NOCC_CACHE_MAP
{
    LIST_ENTRY Entry;
    LIST_ENTRY AssociatedBcb;
    LIST_ENTRY PrivateCacheMaps;
    ULONG NumberOfMaps;
    ULONG RefCount;
    CC_FILE_SIZES FileSizes;
    CACHE_MANAGER_CALLBACKS Callbacks;
    PVOID LazyContext;
    PVOID LogHandle;
    PFLUSH_TO_LSN FlushToLsn;
    ULONG ReadAheadGranularity;
} NOCC_CACHE_MAP, *PNOCC_CACHE_MAP;

VOID
NTAPI
CcPfInitializePrefetcher(VOID);

VOID
NTAPI
CcMdlReadComplete2(IN PFILE_OBJECT FileObject,
                   IN PMDL MemoryDescriptorList);

VOID
NTAPI
CcMdlWriteComplete2(IN PFILE_OBJECT FileObject,
                    IN PLARGE_INTEGER FileOffset,
                    IN PMDL MdlChain);

VOID
NTAPI
CcInitView(VOID);

BOOLEAN
NTAPI
CcpUnpinData(PNOCC_BCB Bcb,
             BOOLEAN ActuallyRelease);

VOID
NTAPI
CcShutdownSystem(VOID);

VOID
NTAPI
CcInitCacheZeroPage(VOID);

/* Called by section.c */
BOOLEAN
NTAPI
CcFlushImageSection(PSECTION_OBJECT_POINTERS SectionObjectPointer,
                    MMFLUSH_TYPE FlushType);

VOID
NTAPI
_CcpFlushCache(IN PNOCC_CACHE_MAP Map,
               IN OPTIONAL PLARGE_INTEGER FileOffset,
               IN ULONG Length,
               OUT OPTIONAL PIO_STATUS_BLOCK IoStatus,
               BOOLEAN Delete,
               const char *File,
               int Line);

#define CcpFlushCache(M,F,L,I,D) _CcpFlushCache(M,F,L,I,D,__FILE__,__LINE__)

BOOLEAN
NTAPI
CcGetFileSizes(PFILE_OBJECT FileObject,
               PCC_FILE_SIZES FileSizes);

ULONG
NTAPI
CcpCountCacheSections(PNOCC_CACHE_MAP Map);

BOOLEAN
NTAPI
CcpAcquireFileLock(PNOCC_CACHE_MAP Map);

VOID
NTAPI
CcpReleaseFileLock(PNOCC_CACHE_MAP Map);

/*
 * Macro for generic cache manage bugchecking. Note that this macro assumes
 * that the file name including extension is always longer than 4 characters.
 */
#define KEBUGCHECKCC \
    KEBUGCHECKEX(CACHE_MANAGER, \
    (*(ULONG*)(__FILE__ + sizeof(__FILE__) - 4) << 16) | \
    (__LINE__ & 0xFFFF), 0, 0, 0)

/* Private data */

#define CACHE_SINGLE_FILE_MAX (16)
#define CACHE_OVERALL_SIZE    (32 * 1024 * 1024)
#define CACHE_STRIPE          VACB_MAPPING_GRANULARITY
#define CACHE_SHIFT           18
#define CACHE_NUM_SECTIONS    (CACHE_OVERALL_SIZE / CACHE_STRIPE)
#define CACHE_ROUND_UP(x)     (((x) + (CACHE_STRIPE-1)) & ~(CACHE_STRIPE-1))
#define CACHE_ROUND_DOWN(x)   ((x) & ~(CACHE_STRIPE-1))
#define INVALID_CACHE         ((ULONG)~0)

extern NOCC_BCB CcCacheSections[CACHE_NUM_SECTIONS];
extern PRTL_BITMAP CcCacheBitmap;
extern FAST_MUTEX CcMutex;
extern KEVENT CcDeleteEvent;
extern ULONG CcCacheClockHand;
extern LIST_ENTRY CcPendingUnmap;
extern KEVENT CcpLazyWriteEvent;

#define CcpLock() _CcpLock(__FILE__,__LINE__)
#define CcpUnlock() _CcpUnlock(__FILE__,__LINE__)

extern VOID _CcpLock(const char *file, int line);
extern VOID _CcpUnlock(const char *file, int line);

extern VOID CcpReferenceCache(ULONG Sector);
extern VOID CcpDereferenceCache(ULONG Sector, BOOLEAN Immediate);

BOOLEAN
NTAPI
CcpMapData(IN PFILE_OBJECT FileObject,
           IN PLARGE_INTEGER FileOffset,
           IN ULONG Length,
           IN ULONG Flags,
           OUT PVOID *BcbResult,
           OUT PVOID *Buffer);

BOOLEAN
NTAPI
CcpPinMappedData(IN PNOCC_CACHE_MAP Map,
                 IN PLARGE_INTEGER FileOffset,
                 IN ULONG Length,
                 IN ULONG Flags,
                 IN OUT PVOID *Bcb);
