#ifndef __INCLUDE_DDK_NTIFS_H
#define __INCLUDE_DDK_NTIFS_H

#if 0
typedef struct
{
   BOOLEAN Replace;
   HANDLE RootDir;
   ULONG FileNameLength;
   WCHAR FileName[1];
} FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;
#endif 

typedef struct _BCB
{
   LIST_ENTRY CacheSegmentListHead;
   PFILE_OBJECT FileObject;
   KSPIN_LOCK BcbLock;
} BCB, *PBCB;

NTSTATUS CcRequestCachePage(PBCB Bcb,
			    ULONG FileOffset,
			    PVOID* BaseAddress,
			    PBOOLEAN UptoDate);
NTSTATUS CcInitializeFileCache(PFILE_OBJECT FileObject,
			       PBCB* Bcb);

#include <ddk/cctypes.h>

#include <ddk/ccfuncs.h>

#endif /* __INCLUDE_DDK_NTIFS_H */
