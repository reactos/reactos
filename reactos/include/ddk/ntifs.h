#ifndef __INCLUDE_DDK_NTIFS_H
#define __INCLUDE_DDK_NTIFS_H

NTSTATUS STDCALL
CcRosInitializeFileCache (PFILE_OBJECT	FileObject,
		          ULONG		CacheSegmentSize);
NTSTATUS STDCALL
CcRosReleaseFileCache (PFILE_OBJECT	FileObject);

#include <ddk/cctypes.h>

#include <ddk/ccfuncs.h>

#include <ddk/fstypes.h>
#include <ddk/fsfuncs.h>

#endif /* __INCLUDE_DDK_NTIFS_H */
