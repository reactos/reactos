#include <ddk/ntddk.h>
#include <windows.h>

#include "regtests.h"

static int RunTest(char *Buffer)
{
  VOID *pmem1;
  PHYSICAL_ADDRESS LowestAcceptableAddress, HighestAcceptableAddress, SkipBytes;
  SIZE_T TotalBytes;
  PMDL Mdl;

  /* Allocate memory for use in testing */
  pmem1 = ExAllocatePool(NonPagedPool,
                         512);

  /* MDL Testing */
  Mdl = NULL;
  Mdl = MmCreateMdl(NULL,
                    pmem1,
                    512);
  if (Mdl == NULL)
  {
    strcpy(Buffer, "MmCreateMdl() failed for Mdl\n");
    return TS_FAILED;
  }
  MmBuildMdlForNonPagedPool(Mdl);
  MmProbeAndLockPages(Mdl,
                      KernelMode,
                      IoReadAccess);
  MmUnlockPages(Mdl);
  ExFreePool(Mdl);

  SkipBytes.QuadPart = PAGE_SIZE * 4;
  TotalBytes = 4096;
  Mdl = NULL;
  Mdl = MmAllocatePagesForMdl(LowestAcceptableAddress,
                              HighestAcceptableAddress,
                              SkipBytes,
                              TotalBytes);
  if (Mdl == NULL)
  {
    strcpy(Buffer, "MmAllocatePagesForMdl() failed for Mdl\n");
    return TS_FAILED;
  }
  MmFreePagesFromMdl(Mdl);

  /* Free memory used in test */
  ExFreePool(pmem1);

  return TS_OK;
}

int
Mdl_1Test(int Command, char *Buffer)
{
  switch (Command)
    {
      case TESTCMD_RUN:
        return RunTest(Buffer);
      case TESTCMD_TESTNAME:
        strcpy(Buffer, "Kernel Memory MDL API");
        return TS_OK;
      default:
        break;
    }
  return TS_FAILED;
}
