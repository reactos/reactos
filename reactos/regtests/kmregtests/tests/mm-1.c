#include <ddk/ntddk.h>
#include <windows.h>

#include "regtests.h"

static int RunTest(char *Buffer)
{
  VOID *pmem1, *pmem2, *pmem3, *pmem4, *pmem5;
  PHYSICAL_ADDRESS LowestAcceptableAddress, HighestAcceptableAddress, BoundryAddressMultiple,
                   PhysicalAddress;
  ULONG AllocSize1, AllocSize2, AllocSize3, AllocSize4, AllocSize5, MemSize;
  BOOL Server;

  /* Various ways to allocate memory */
  HighestAcceptableAddress.QuadPart = 0x00000000FFFFFF; /* 16MB */
  AllocSize1 = 512;
  pmem1 = MmAllocateContiguousMemory(AllocSize1,
                                     HighestAcceptableAddress);
  if (pmem1 == 0)
  {
    DbgPrint("MmAllocateContiguousMemory() for pmem1 failed\n");
    return TS_FAILED;
  }

  LowestAcceptableAddress.QuadPart = 0x00000000F00000; /* 15MB */
  HighestAcceptableAddress.QuadPart = 0x00000000FFFFFF; /* 16MB */
  BoundryAddressMultiple.QuadPart = 512;
  AllocSize2 = 512;
  pmem2 = MmAllocateContiguousMemorySpecifyCache(AllocSize2,
                                                 LowestAcceptableAddress,
                                                 HighestAcceptableAddress,
                                                 BoundryAddressMultiple,
                                                 MmNonCached);
  if (pmem2 == 0)
  {
    DbgPrint("MmAllocateContiguousMemorySpecifyCache() for pmem2 failed\n");
    return TS_FAILED;
  }

  LowestAcceptableAddress.QuadPart = 0x00000000000000; /* 15MB */
  HighestAcceptableAddress.QuadPart = 0x0000000F000000; /* 250MB */
  BoundryAddressMultiple.QuadPart = 1024;
  AllocSize3 = 512;
  pmem3 = MmAllocateContiguousMemorySpecifyCache(AllocSize3,
                                                 LowestAcceptableAddress,
                                                 HighestAcceptableAddress,
                                                 BoundryAddressMultiple,
                                                 MmCached);
  if (pmem3 == 0)
  {
    DbgPrint("MmAllocateContiguousMemorySpecifyCache() for pmem3 failed\n");
    return TS_FAILED;
  }


  LowestAcceptableAddress.QuadPart = 0x00000000000000; /* 0MB */
  HighestAcceptableAddress.QuadPart = 0x00000000FFFFFF; /* 16MB */
  BoundryAddressMultiple.QuadPart = 4096;
  AllocSize4 = 512;
  pmem4 = MmAllocateContiguousMemorySpecifyCache(AllocSize4,
                                                 LowestAcceptableAddress,
                                                 HighestAcceptableAddress,
                                                 BoundryAddressMultiple,
                                                 MmWriteCombined);
  if (pmem4 == 0)
  {
    DbgPrint("MmAllocateContiguousMemorySpecifyCache() for pmem4 failed\n");
    return TS_FAILED;
  }

  AllocSize5 = 1048576; /* 1MB */
  pmem5 = MmAllocateNonCachedMemory(AllocSize5);
  if (pmem5 == 0)
  {
    DbgPrint("MmAllocateNonCachedMemory() for pmem5 failed\n");
    return TS_FAILED;
  }

  /* Memory checking functions */
  PhysicalAddress = MmGetPhysicalAddress(pmem1);
  if (PhysicalAddress.QuadPart == 0)
  {
    DbgPrint("MmGetPhysicalAddress() failed\n");
    return TS_FAILED;
  }

  if (MmIsAddressValid(pmem1) == FALSE)
  {
    DbgPrint("MmIsAddressValid() failed for pmem1\n");
    return TS_FAILED;
  }
  if (MmIsAddressValid(pmem2) == FALSE)
  {
    DbgPrint("MmIsAddressValid() failed for pmem2\n");
    return TS_FAILED;
  }
  if (MmIsAddressValid(pmem3) == FALSE)
  {
    DbgPrint("MmIsAddressValid() failed for pmem3\n");
    return TS_FAILED;
  }
  if (MmIsAddressValid(pmem4) == FALSE)
  {
    DbgPrint("MmIsAddressValid() failed for pmem4\n");
    return TS_FAILED;
  }
  if (MmIsAddressValid(pmem5) == FALSE)
  {
    DbgPrint("MmIsAddressValid() failed for pmem5\n");
    return TS_FAILED;
  }

  if (MmIsNonPagedSystemAddressValid(pmem1) == FALSE)
  {
    DbgPrint("MmIsAddressValid() failed for pmem1\n");
    return TS_FAILED;
  }
  if (MmIsNonPagedSystemAddressValid(pmem2) == FALSE)
  {
    DbgPrint("MmIsAddressValid() failed for pmem2\n");
    return TS_FAILED;
  }
  if (MmIsNonPagedSystemAddressValid(pmem3) == FALSE)
  {
    DbgPrint("MmIsAddressValid() failed for pmem3\n");
    return TS_FAILED;
  }
  if (MmIsNonPagedSystemAddressValid(pmem4) == FALSE)
  {
    DbgPrint("MmIsAddressValid() failed for pmem4\n");
    return TS_FAILED;
  }
  if (MmIsNonPagedSystemAddressValid(pmem5) == FALSE)
  {
    DbgPrint("MmIsAddressValid() failed for pmem5\n");
    return TS_FAILED;
  }

  /* Misc functions */
  Server = MmIsThisAnNtAsSystem();
  MemSize = MmQuerySystemSize();
  if (MemSize == 0)
  {
    DbgPrint("MmQuerySystemSize() failed\n");
    return TS_FAILED;
  }

  /* Page locking functions */

  /* Mapping functions */

  /* Free allocated memory */
  MmFreeContiguousMemory(pmem1);
  MmFreeContiguousMemorySpecifyCache(pmem2,
                                     AllocSize2,
                                     MmNonCached);
  MmFreeContiguousMemorySpecifyCache(pmem3,
                                     AllocSize3,
                                     MmCached);
  MmFreeContiguousMemorySpecifyCache(pmem4,
                                     AllocSize4,
                                     MmWriteCombined);
  MmFreeNonCachedMemory(pmem5,
                        AllocSize5);

  /* MDL operations */

  return TS_OK;
}

int
Mm_1Test(int Command, char *Buffer)
{
  switch (Command)
    {
      case TESTCMD_RUN:
        return RunTest(Buffer);
      case TESTCMD_TESTNAME:
        strcpy(Buffer, "Kernel Memory API");
        return TS_OK;
      default:
        break;
    }
  return TS_FAILED;
}
