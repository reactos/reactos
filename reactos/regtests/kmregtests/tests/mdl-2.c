#include <ddk/ntddk.h>
#include <windows.h>

#include "regtests.h"

static int RunTest(char *Buffer)
{
  VOID *pmem1, *sysaddr1, *sysaddr2;
  ULONG AllocSize;
  BOOLEAN BoolVal;
  PMDL Mdl;

  /* Allocate memory for use in testing */
  AllocSize = 1024;
  pmem1 = ExAllocatePool(NonPagedPool, AllocSize);

  /* MmCreateMdl test */
  Mdl = NULL;
  Mdl = MmCreateMdl(NULL, pmem1, AllocSize);
  FAIL_IF_NULL(Mdl, "MmCreateMdl() failed for Mdl");

  /* MmBuildMdlForNonPagedPool test */
  MmBuildMdlForNonPagedPool(Mdl);

  /* MmGetSystemAddressForMdl test for buffer built by MmBuildMdlForNonPagedPool */
  sysaddr1 = MmGetSystemAddressForMdl(Mdl);
  FAIL_IF_NULL(sysaddr1, "MmGetSystemAddressForMdl() failed for Mdl after MmBuildMdlForNonPagedPool");

  /* MmIsNonPagedSystemAddressValid test */
  BoolVal = MmIsNonPagedSystemAddressValid(sysaddr1);
  FAIL_IF_FALSE(BoolVal, "MmIsNonPagedSystemAddressValid() failed for Mdl for sysaddr1");

  /* MmGetSystemAddressForMdlSafe test */
  sysaddr2 = MmGetSystemAddressForMdlSafe(Mdl, HighPagePriority);
  FAIL_IF_NULL(sysaddr2, "MmGetSystemAddressForMdlSafe() failed for Mdl after MmBuildMdlForNonPagedPool");

  /* MmIsNonPagedSystemAddressValid test */
  BoolVal = MmIsNonPagedSystemAddressValid(sysaddr2);
  FAIL_IF_FALSE(BoolVal, "MmIsNonPagedSystemAddressValid() failed for Mdl for sysaddr2");

  /* Free memory used in test */
  ExFreePool(pmem1);

  return TS_OK;
}

int
Mdl_2Test(int Command, char *Buffer)
{
  DISPATCHER("Kernel Memory MDL API (2)");
}
