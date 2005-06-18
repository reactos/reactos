#ifndef __INCLUDE_DDK_NTIFS_H
#define __INCLUDE_DDK_NTIFS_H

#ifdef __USE_W32API

#include_next <ddk/ntifs.h>

#else /* __USE_W32API */

#include <ddk/cctypes.h>
#include <ddk/ccfuncs.h>

#include <ddk/fstypes.h>
#include <ddk/fsfuncs.h>

#endif

NTSTATUS STDCALL
CcRosInitializeFileCache (PFILE_OBJECT	FileObject,
		          ULONG		CacheSegmentSize);
NTSTATUS STDCALL
CcRosReleaseFileCache (PFILE_OBJECT	FileObject);

#define FSCTL_ROS_QUERY_LCN_MAPPING \
        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 63, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _ROS_QUERY_LCN_MAPPING
{
  LARGE_INTEGER LcnDiskOffset;
} ROS_QUERY_LCN_MAPPING, *PROS_QUERY_LCN_MAPPING;

#endif /* __INCLUDE_DDK_NTIFS_H */
