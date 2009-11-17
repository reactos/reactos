#ifndef __INCLUDE_DDK_NTIFS_H
#define __INCLUDE_DDK_NTIFS_H

VOID NTAPI
CcRosInitializeFileCache(PFILE_OBJECT FileObject,
                         ULONG CacheSegmentSize,
                         PCACHE_MANAGER_CALLBACKS CallBacks,
                         PVOID LazyWriterContext);

NTSTATUS NTAPI
CcRosReleaseFileCache (PFILE_OBJECT	FileObject);

struct _BCB;

VOID
NTAPI
CcRosTraceCacheMap (
	struct _BCB* Bcb,
	BOOLEAN Trace );

#define FSCTL_ROS_QUERY_LCN_MAPPING \
        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 63, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _ROS_QUERY_LCN_MAPPING
{
  LARGE_INTEGER LcnDiskOffset;
} ROS_QUERY_LCN_MAPPING, *PROS_QUERY_LCN_MAPPING;

#endif /* __INCLUDE_DDK_NTIFS_H */
