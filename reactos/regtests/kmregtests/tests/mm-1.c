#include <ddk/ntddk.h>
#include <windows.h>

#include "regtests.h"

static int RunTest(char *Buffer)
{
  VOID *pmem1, *pmem2, *pmem3, *pmem4, *pmem5;
  PHYSICAL_ADDRESS LowestAcceptableAddress, HighestAcceptableAddress, BoundryAddressMultiple,
                   PhysicalAddress;
  ULONG AllocSize1, AllocSize2, AllocSize3, AllocSize4, AllocSize5, MemSize;
  BOOL Server, BoolVal;

  /* Various ways to allocate memory */
  HighestAcceptableAddress.QuadPart = 0x00000000FFFFFF; /* 16MB */
  AllocSize1 = 512;
  pmem1 = 0;
  pmem1 = MmAllocateContiguousMemory(AllocSize1, HighestAcceptableAddress);
  FAIL_IF_EQUAL(pmem1, 0, "MmAllocateContiguousMemory() for pmem1 failed");

  LowestAcceptableAddress.QuadPart = 0x00000000F00000; /* 15MB */
  HighestAcceptableAddress.QuadPart = 0x00000000FFFFFF; /* 16MB */
  BoundryAddressMultiple.QuadPart = 512;
  AllocSize2 = 512;
  pmem2 = 0;
  pmem2 = MmAllocateContiguousMemorySpecifyCache(AllocSize2,
                                                 LowestAcceptableAddress, HighestAcceptableAddress,
                                                 BoundryAddressMultiple, MmNonCached);
  FAIL_IF_EQUAL(pmem2, 0, "MmAllocateContiguousMemorySpecifyCache() for pmem2 failed");

  LowestAcceptableAddress.QuadPart = 0x00000000000000; /* 15MB */
  HighestAcceptableAddress.QuadPart = 0x0000000F000000; /* 250MB */
  BoundryAddressMultiple.QuadPart = 1024;
  AllocSize3 = 512;
  pmem3 = 0;
  pmem3 = MmAllocateContiguousMemorySpecifyCache(AllocSize3,
                                                 LowestAcceptableAddress, HighestAcceptableAddress,
                                                 BoundryAddressMultiple, MmCached);
  FAIL_IF_EQUAL(pmem3, 0, "MmAllocateContiguousMemorySpecifyCache() for pmem3 failed");

  LowestAcceptableAddress.QuadPart = 0x00000000000000; /* 0MB */
  HighestAcceptableAddress.QuadPart = 0x00000000FFFFFF; /* 16MB */
  BoundryAddressMultiple.QuadPart = 4096;
  AllocSize4 = 512;
  pmem4 = 0;
  pmem4 = MmAllocateContiguousMemorySpecifyCache(AllocSize4,
                                                 LowestAcceptableAddress, HighestAcceptableAddress,
                                                 BoundryAddressMultiple, MmWriteCombined);
  FAIL_IF_EQUAL(pmem4, 0, "MmAllocateContiguousMemorySpecifyCache() for pmem4 failed");

  AllocSize5 = 1048576; /* 1MB */
  pmem5 = 0;
  pmem5 = MmAllocateNonCachedMemory(AllocSize5);
  FAIL_IF_EQUAL(pmem5, 0, "MmAllocateNonCachedMemory() for pmem5 failed");

  /* Memory checking functions */
  PhysicalAddress.QuadPart = 0;
  PhysicalAddress = MmGetPhysicalAddress(pmem1);
  FAIL_IF_EQUAL(PhysicalAddress.QuadPart, 0, "MmGetPhysicalAddress() failed");

  BoolVal = MmIsAddressValid(pmem1);
  FAIL_IF_FALSE(BoolVal, "MmIsAddressValid() failed for pmem1");

  BoolVal = MmIsAddressValid(pmem2);
  FAIL_IF_FALSE(BoolVal, "MmIsAddressValid() failed for pmem2");

  BoolVal = MmIsAddressValid(pmem3);
  FAIL_IF_FALSE(BoolVal, "MmIsAddressValid() failed for pmem3");

  BoolVal = MmIsAddressValid(pmem4);
  FAIL_IF_FALSE(BoolVal, "MmIsAddressValid() failed for pmem4");

  BoolVal = MmIsAddressValid(pmem5);
  FAIL_IF_FALSE(BoolVal, "MmIsAddressValid() failed for pmem5");

  BoolVal = MmIsNonPagedSystemAddressValid(pmem1);
  FAIL_IF_FALSE(BoolVal, "MmIsNonPagedSystemAddressValid() failed for pmem1");

  BoolVal = MmIsNonPagedSystemAddressValid(pmem2);
  FAIL_IF_FALSE(BoolVal, "MmIsNonPagedSystemAddressValid() failed for pmem2");

  BoolVal = MmIsNonPagedSystemAddressValid(pmem3);
  FAIL_IF_FALSE(BoolVal, "MmIsNonPagedSystemAddressValid() failed for pmem3");

  BoolVal = MmIsNonPagedSystemAddressValid(pmem4);
  FAIL_IF_FALSE(BoolVal, "MmIsNonPagedSystemAddressValid() failed for pmem4");

  BoolVal = MmIsNonPagedSystemAddressValid(pmem5);
  FAIL_IF_FALSE(BoolVal, "MmIsNonPagedSystemAddressValid() failed for pmem5");

  /* Misc functions */
  Server = MmIsThisAnNtAsSystem();
  MemSize = 0;
  MemSize = MmQuerySystemSize();
  if (MemSize != MmSmallSystem &&
      MemSize != MmMediumSystem &&
      MemSize != MmLargeSystem)
  {
    strcpy(Buffer, "MmQuerySystemSize() failed\n");
    return TS_FAILED;
  }

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

  return TS_OK;
}

int
Mm_1Test(int Command, char *Buffer)
{
  DISPATCHER("Kernel Core Memory API");
}
