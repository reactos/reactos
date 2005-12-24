#include <ntddk.h>
#include <winddi.h>

#include "regtests.h"

static void RunTest()
{
#if 0
  VOID *pmem1, *pmem2;
  ULONG AllocSize1, AllocSize2;
  ULONG AllocTag1, AllocTag2;
  HANDLE Handle1, Handle2;

  /* Allocate memory with EngAllocMem */
  pmem1 = 0;
  AllocSize1 = 1024;
  AllocTag1 = TAG('D','x','y','z');
  pmem1 = EngAllocMem(FL_ZERO_MEMORY, AllocSize1, AllocTag1);
  _AssertNotEqualValue(pmem1, NULL);

  /* Allocate memory with EngAllocMem */
  pmem2 = 0;
  AllocSize2 = 1024;
  AllocTag2 = TAG('D','x','y','z');
  pmem2 = EngAllocUserMem(AllocSize2, AllocTag2);
  _AssertNotEqualValue(pmem1, NULL);

  /* Lock down memory with EngSecureMem
  ** Dependant functions in ntoskrnl.exe are currently unimplemented
  Handle1 = EngSecureMem(pmem1, AllocSize1);
  _AssertNotEqualValue(pmem1, NULL);
  Handle2 = EngSecureMem(pmem2, AllocSize2);
  _AssertNotEqualValue(pmem2, NULL);

  /* Unlock down memory with EngSecureMem
  ** Dependant functions in ntoskrnl.exe are currently unimplemented
  EngUnsecureMem(Handle1);
  EngUnsecureMem(Handle2); */

  /* Free memory with EngFreeMem */
  EngFreeMem(pmem1);

  /* Free memory with EngFreeUserMem */
  EngFreeUserMem(pmem2);
#endif
}

_Dispatcher(Eng_mem_1Test, "Win32k Engine Memory API")
