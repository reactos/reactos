#include <ddk/ntddk.h>
#include <windows.h>

#include "regtests.h"

static int RunTest(char *Buffer)
{
  VOID *pmem1, *sysaddr1, *sysaddr2;
  ULONG AllocSize;
  PMDL Mdl;

  /* Allocate memory for use in testing */
  AllocSize = 1024;
  pmem1 = ExAllocatePool(NonPagedPool,
                         AllocSize);

  /* MmCreateMdl test */
  Mdl = NULL;
  Mdl = MmCreateMdl(NULL,
                    pmem1,
                    AllocSize);
  if (Mdl == NULL)
  {
    strcpy(Buffer, "MmCreateMdl() failed for Mdl\n");
    return TS_FAILED;
  }

  /* MmBuildMdlForNonPagedPool test */
  MmBuildMdlForNonPagedPool(Mdl);

  /* MmGetSystemAddressForMdl test for buffer built by MmBuildMdlForNonPagedPool */
  sysaddr1 = MmGetSystemAddressForMdl(Mdl);
  if (sysaddr1 == NULL)
  {
    strcpy(Buffer, "MmGetSystemAddressForMdl() failed for Mdl after MmBuildMdlForNonPagedPool\n");
    return TS_FAILED;
  }

  /* MmIsNonPagedSystemAddressValid test */
  if (MmIsNonPagedSystemAddressValid(sysaddr1) == FALSE)
  {
    strcpy(Buffer, "MmIsNonPagedSystemAddressValid() failed for Mdl for sysaddr1\n");
    return TS_FAILED;
  }

  /* MmGetSystemAddressForMdlSafe test */
  sysaddr2 = MmGetSystemAddressForMdlSafe(Mdl, HighPagePriority);
  if (sysaddr2 == NULL)
  {
    strcpy(Buffer, "MmGetSystemAddressForMdlSafe() failed for Mdl after MmBuildMdlForNonPagedPool\n");
    return TS_FAILED;
  }

  /* MmIsNonPagedSystemAddressValid test */
  if (MmIsNonPagedSystemAddressValid(sysaddr2) == FALSE)
  {
    strcpy(Buffer, "MmIsNonPagedSystemAddressValid() failed for Mdl for sysaddr2\n");
    return TS_FAILED;
  }

  /* Free memory used in test */
  ExFreePool(pmem1);

  return TS_OK;
}

int
Mdl_2Test(int Command, char *Buffer)
{
  switch (Command)
    {
      case TESTCMD_RUN:
        return RunTest(Buffer);
      case TESTCMD_TESTNAME:
        strcpy(Buffer, "Kernel Memory MDL API (2)");
        return TS_OK;
      default:
        break;
    }
  return TS_FAILED;
}
