#include <ddk/ntddk.h>
#include <ddk/winddi.h>

#include "regtests.h"

static int RunTest(char *Buffer)
{
  VOID *pmem1, *pmem2;
  ULONG AllocSize1, AllocSize2;
  ULONG AllocTag1, AllocTag2;
  HANDLE Handle1, Handle2;

  /* Allocate memory with EngAllocMem */
  pmem1 = 0;
  AllocSize1 = 1024;
  AllocTag1 = "zyxD";
  pmem1 = EngAllocMem(FL_ZERO_MEMORY, AllocSize1, AllocTag1);
  FAIL_IF_EQUAL(pmem1, 0, "EngAllocMem() for pmem1 failed");

  /* Allocate memory with EngAllocMem */
  pmem2 = 0;
  AllocSize2 = 1024;
  AllocTag2 = "zyxD";
  pmem2 = EngAllocUserMem(AllocSize2, AllocTag2);
  FAIL_IF_EQUAL(pmem1, 0, "EngAllocUserMem() for pmem2 failed");

  /* Lock down memory with EngSecureMem
  ** Dependant functions in ntoskrnl.exe are currently unimplemented
  Handle1 = EngSecureMem(pmem1, AllocSize1);
  FAIL_IF_NULL(pmem1, "EngSecureMem() for pmem1 failed");
  Handle2 = EngSecureMem(pmem2, AllocSize2);
  FAIL_IF_NULL(pmem2, "EngSecureMem() for pmem2 failed"); */

  /* Unlock down memory with EngSecureMem
  ** Dependant functions in ntoskrnl.exe are currently unimplemented
  EngUnsecureMem(Handle1);
  EngUnsecureMem(Handle2); */

  /* Free memory with EngFreeMem */
  EngFreeMem(pmem1);

  /* Free memory with EngFreeUserMem */
  EngFreeUserMem(pmem2);

  return TS_OK;
}

int
Eng_mem_1Test(int Command, char *Buffer)
{
  DISPATCHER("Win32k Engine Memory API");
}

