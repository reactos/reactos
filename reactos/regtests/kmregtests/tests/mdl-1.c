#include <ddk/ntddk.h>
#include <windows.h>

#include "regtests.h"

static int RunTest(char *Buffer)
{
  VOID *pmem1, *MdlPfnArray, *MdlVirtAddr;
  ULONG MdlSize, MdlOffset, AllocSize;
  PMDL Mdl;

  /* Allocate memory for use in testing */
  AllocSize = 512;
  pmem1 = ExAllocatePool(NonPagedPool,
                         AllocSize);

  /* MmSizeOfMdl test */
  MdlSize = 0;
  MdlSize = MmSizeOfMdl(pmem1,
                        AllocSize);
  if (MdlSize <= sizeof(MDL))
  {
    strcpy(Buffer, "MmSizeOfMdl() failed\n");
    return TS_FAILED;
  }

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

  /* MmGetMdlByteCount test */
  MdlSize = 0;
  MdlSize = MmGetMdlByteCount(Mdl);
  if (MdlSize != AllocSize)
  {
    strcpy(Buffer, "MmGetMdlByteCount() failed for Mdl\n");
    return TS_FAILED;
  }

  /* MmGetMdlByteOffset test */
  MdlOffset = MmGetMdlByteOffset(Mdl);

  /* MmGetMdlPfnArray test */
  MdlPfnArray = NULL;
  MdlPfnArray = MmGetMdlPfnArray(Mdl);
  if (MdlPfnArray == NULL)
  {
    strcpy(Buffer, "MmGetMdlPfnArray() failed for Mdl\n");
    return TS_FAILED;
  }

  /* MmGetMdlVirtualAddress test */
  MdlVirtAddr = NULL;
  MdlVirtAddr = MmGetMdlVirtualAddress(Mdl);
  if (MdlVirtAddr == NULL)
  {
    strcpy(Buffer, "MmGetMdlVirtualAddress() failed for Mdl\n");
    return TS_FAILED;
  }

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
        strcpy(Buffer, "Kernel Memory MDL API (1)");
        return TS_OK;
      default:
        break;
    }
  return TS_FAILED;
}
