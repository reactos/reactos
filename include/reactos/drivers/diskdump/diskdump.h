#ifndef __DISKDUMP_H
#define __DISKDUMP_H

#include <ntddscsi.h>
#include <ketypes.h>

#define MM_CORE_DUMP_HEADER_MAGIC         (0xdeafbead)
#define MM_CORE_DUMP_HEADER_VERSION       (0x1)

typedef struct _MM_CORE_DUMP_HEADER
{
  ULONG Magic;
  ULONG Version;
  ULONG Type;
  KTRAP_FRAME TrapFrame;
  ULONG BugCheckCode;
  ULONG BugCheckParameters[4];
  PVOID FaultingStackBase;
  ULONG FaultingStackSize;
  ULONG PhysicalMemorySize;
} MM_CORE_DUMP_HEADER, *PMM_CORE_DUMP_HEADER;

typedef struct MM_CORE_DUMP_FUNCTIONS
{
  NTSTATUS (NTAPI *DumpPrepare)(PDEVICE_OBJECT DeviceObject, PDUMP_POINTERS DumpPointers);
  NTSTATUS (NTAPI *DumpInit)(VOID);
  NTSTATUS (NTAPI *DumpWrite)(LARGE_INTEGER Address, PMDL Mdl);
  NTSTATUS (NTAPI *DumpFinish)(VOID);
} MM_CORE_DUMP_FUNCTIONS, *PMM_CORE_DUMP_FUNCTIONS;

#endif /* __DISKDUMP_H */
