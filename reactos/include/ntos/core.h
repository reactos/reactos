#ifndef __INCLUDE_NTOS_CORE_H
#define __INCLUDE_NTOS_CORE_H

#include "../../ntoskrnl/include/internal/ke.h"

#define MM_CORE_DUMP_HEADER_MAGIC         (0xdeafbead)
#define MM_CORE_DUMP_HEADER_VERSION       (0x1)
#define MM_CORE_DUMP_TYPE_MINIMAL         (0x1)
#define MM_CORE_DUMP_TYPE_FULL            (0x2)

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

typedef struct _MM_DUMP_POINTERS
{
  PVOID Context;
  NTSTATUS (*DeviceInit)(PVOID Context);
  NTSTATUS (*DeviceWrite)(PVOID Context, ULONG Block, PMDL Mdl);
  NTSTATUS (*DeviceFinish)(PVOID Context);
} MM_DUMP_POINTERS, *PMM_DUMP_POINTERS;

#define FSCTL_GET_DUMP_BLOCK_MAP                     (('R' << 24) | 0xF1)
#define IOCTL_GET_DUMP_POINTERS                      (('R' << 24) | 0xF2)

#endif /* __INCLUDE_NTOS_CORE_H */
