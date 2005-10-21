#ifndef __INCLUDE_INTERNAL_CC_H
#define __INCLUDE_INTERNAL_CC_H

#define CACHE_VIEW_SIZE	(128 * 1024) // 128kB

struct _BCB;

typedef struct
{
   SECTION_DATA SectionData;
   PVOID BaseAddress;
   ULONG RefCount;
   struct _BCB* Bcb;
   LIST_ENTRY ListEntry;
} CACHE_VIEW, *PCACHE_VIEW;

typedef struct _BCB
{
  PFILE_OBJECT FileObject;
  CC_FILE_SIZES FileSizes;
  BOOLEAN PinAccess;
  PCACHE_MANAGER_CALLBACKS CallBacks;
  PVOID LazyWriterContext;
  ULONG RefCount;
  PCACHE_VIEW CacheView[2048];
  PVOID LargeCacheView;
  PSECTION_OBJECT Section;
#if defined(DBG) || defined(KDBG)
	BOOLEAN Trace; /* enable extra trace output for this BCB and it's cache segments */
#endif
} BCB, *PBCB;


typedef struct _INTERNAL_BCB
{
  PBCB Bcb;
  ULONG Index;
//  CSHORT RefCount; /* (At offset 0x34 on WinNT4) */
} INTERNAL_BCB, *PINTERNAL_BCB;

VOID
STDCALL
CcMdlReadCompleteDev(
    IN PMDL MdlChain,
    IN PFILE_OBJECT FileObject
);

VOID
STDCALL
CcMdlWriteCompleteDev(
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PFILE_OBJECT FileObject
);

VOID
NTAPI
CcInitView(VOID);

VOID
NTAPI
CcInit(VOID);

VOID
NTAPI
CcInitCacheZeroPage(VOID);

NTSTATUS
NTAPI
CcRosFlushDirtyPages(
    ULONG Target,
    PULONG Count
);

VOID
NTAPI
CcRosDereferenceCache(PFILE_OBJECT FileObject);

VOID
NTAPI
CcRosReferenceCache(PFILE_OBJECT FileObject);

VOID
NTAPI
CcRosSetRemoveOnClose(PSECTION_OBJECT_POINTERS SectionObjectPointer);

/*
 * Macro for generic cache manage bugchecking. Note that this macro assumes
 * that the file name including extension is always longer than 4 characters.
 */
#define KEBUGCHECKCC \
    KEBUGCHECKEX(CACHE_MANAGER, \
    (*(DWORD*)(__FILE__ + sizeof(__FILE__) - 4) << 16) | \
    (__LINE__ & 0xFFFF), 0, 0, 0)

#endif
