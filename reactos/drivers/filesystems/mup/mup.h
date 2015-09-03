#ifndef _MUP_PCH_
#define _MUP_PCH_

#include <wdm.h>
#include <ntifs.h>
#include <pseh/pseh2.h>
#include <ndk/muptypes.h>

#ifdef __GNUC__
#define INIT_SECTION __attribute__((section ("INIT")))
#define INIT_FUNCTION INIT_SECTION
#else
#define INIT_SECTION /* Done via alloc_text for MSC */
#define INIT_FUNCTION INIT_SECTION
#endif

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define IO_METHOD_FROM_CTL_CODE(C) (C & 0x00000003)

#define TAG_MUP ' puM'

#define DFS_MAGIC_CCB (PVOID)0x11444653
#define FILE_SIMPLE_RIGHTS_MASK (FILE_ALL_ACCESS & ~STANDARD_RIGHTS_REQUIRED &~ SYNCHRONIZE)

#define NODE_TYPE_VCB 0x1
#define NODE_TYPE_UNC 0x2
#define NODE_TYPE_PFX 0x3
#define NODE_TYPE_FCB 0x4
#define NODE_TYPE_CCB 0x5
#define NODE_TYPE_MIC 0x6
#define NODE_TYPE_MQC 0x8

#define NODE_STATUS_HEALTHY 0x1
#define NODE_STATUS_CLEANUP 0x2

typedef struct _MUP_VCB
{
    ULONG NodeType;
    ULONG NodeStatus;
    LONG NodeReferences;
    ULONG NodeSize;
    SHARE_ACCESS ShareAccess;
} MUP_VCB, *PMUP_VCB;

typedef struct _MUP_FCB
{
    ULONG NodeType;
    ULONG NodeStatus;
    LONG NodeReferences;
    ULONG NodeSize;
    PFILE_OBJECT FileObject;
    LIST_ENTRY CcbList;
} MUP_FCB, *PMUP_FCB;

typedef struct _MUP_CCB
{
    ULONG NodeType;
    ULONG NodeStatus;
    LONG NodeReferences;
    ULONG NodeSize;
    PMUP_FCB Fcb;
    LIST_ENTRY CcbListEntry;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
} MUP_CCB, *PMUP_CCB;

typedef struct _MUP_MIC
{
    ULONG NodeType;
    ULONG NodeStatus;
    LONG NodeReferences;
    ULONG NodeSize;
    PIRP Irp;
    NTSTATUS LastSuccess;
    NTSTATUS LastFailed;
    PMUP_FCB Fcb;
} MUP_MIC, *PMUP_MIC;

typedef struct _MUP_UNC
{
    ULONG NodeType;
    ULONG NodeStatus;
    LONG NodeReferences;
    ULONG NodeSize;
    LIST_ENTRY ProviderListEntry;
    UNICODE_STRING DeviceName;
    HANDLE DeviceHandle;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    ULONG ProviderOrder;
    BOOLEAN MailslotsSupported;
    BOOLEAN Registered;
} MUP_UNC, *PMUP_UNC;

typedef struct _MUP_PFX
{
    ULONG NodeType;
    ULONG NodeStatus;
    LONG NodeReferences;
    ULONG NodeSize;
    UNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry;
    UNICODE_STRING AcceptedPrefix;
    ULONG Reserved;
    LARGE_INTEGER ValidityTimeout;
    PMUP_UNC UncProvider;
    BOOLEAN ExternalAlloc;
    BOOLEAN InTable;
    BOOLEAN KeepExtraRef;
    BOOLEAN Padding;
    LIST_ENTRY PrefixListEntry;
} MUP_PFX, *PMUP_PFX;

typedef struct _MUP_MQC
{
    ULONG NodeType;
    ULONG NodeStatus;
    LONG NodeReferences;
    ULONG NodeSize;
    PIRP Irp;
    PFILE_OBJECT FileObject;
    PMUP_UNC LatestProvider;
    ERESOURCE QueryPathListLock;
    PMUP_PFX Prefix;
    NTSTATUS LatestStatus;
    LIST_ENTRY QueryPathList;
    LIST_ENTRY MQCListEntry;
} MUP_MQC, *PMUP_MQC;

typedef struct _FORWARDED_IO_CONTEXT
{
    PMUP_CCB Ccb;
    PMUP_MIC MasterIoContext;
    WORK_QUEUE_ITEM WorkQueueItem;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
} FORWARDED_IO_CONTEXT, *PFORWARDED_IO_CONTEXT;

typedef struct _QUERY_PATH_CONTEXT
{
    PMUP_MQC MasterQueryContext;
    PMUP_UNC UncProvider;
    PQUERY_PATH_REQUEST QueryPathRequest;
    LIST_ENTRY QueryPathListEntry;
    PIRP Irp;
} QUERY_PATH_CONTEXT, *PQUERY_PATH_CONTEXT;

#endif /* _MUP_PCH_ */
