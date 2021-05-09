#ifndef _DFS_PCH_
#define _DFS_PCH_

#include <section_attribs.h>

#define DFS_OPEN_CONTEXT 0xFF444653
#define DFS_DOWNLEVEL_OPEN_CONTEXT 0x11444653
#define DFS_CSCAGENT_NAME_CONTEXT 0xAAAAAAAA
#define DFS_USER_NAME_CONTEXT 0xBBBBBBBB

typedef struct _DFS_NAME_CONTEXT_
{
    UNICODE_STRING UNCFileName;
    LONG NameContextType;
    ULONG Flags;
} DFS_NAME_CONTEXT, *PDFS_NAME_CONTEXT;

NTSTATUS
NTAPI
DfsVolumePassThrough(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
);

NTSTATUS
DfsFsdFileSystemControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
);

NTSTATUS
DfsFsdCreate(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
);

NTSTATUS
DfsFsdCleanup(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
);

NTSTATUS
DfsFsdClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
);

VOID
DfsUnload(
    PDRIVER_OBJECT DriverObject
);

NTSTATUS
DfsDriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
);

#endif
