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
  MdlSize = MmSizeOfMdl(pmem1, AllocSize);
  FAIL_IF_LESS_EQUAL(MdlSize, sizeof(MDL), "MmSizeOfMdl() failed");

  /* MmCreateMdl test */
  Mdl = NULL;
  Mdl = MmCreateMdl(NULL, pmem1, AllocSize);
  FAIL_IF_NULL(Mdl, "MmCreateMdl() failed for Mdl");

  /* MmGetMdlByteCount test */
  MdlSize = 0;
  MdlSize = MmGetMdlByteCount(Mdl);
  FAIL_IF_NOT_EQUAL(MdlSize, AllocSize, "MmGetMdlByteCount() failed for Mdl");

  /* MmGetMdlByteOffset test */
  MdlOffset = MmGetMdlByteOffset(Mdl);

  /* MmGetMdlPfnArray test */
  MdlPfnArray = NULL;
  MdlPfnArray = MmGetMdlPfnArray(Mdl);
  FAIL_IF_NULL(MdlPfnArray, "MmGetMdlPfnArray() failed for Mdl");

  /* MmGetMdlVirtualAddress test */
  MdlVirtAddr = NULL;
  MdlVirtAddr = MmGetMdlVirtualAddress(Mdl);
  FAIL_IF_NULL(MdlVirtAddr, "MmGetMdlVirtualAddress() failed for Mdl");

  /* Free memory used in test */
  ExFreePool(pmem1);

  return TS_OK;
}

int
Mdl_1Test(int Command, char *Buffer)
{
  DISPATCHER("Kernel Memory MDL API (1)");
}
