/*
 * PROJECT:         ReactOS kernel
 * FILE:            regtests/shared/regtests.c
 * PURPOSE:         Regression testing framework
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-07-2003  CSH  Created
 */
#include <roscfg.h>
#define NTOS_MODE_USER
#include <ntos.h>
#include "regtests.h"

#define NDEBUG
#include <debug.h>

LIST_ENTRY AllTests;

int
DriverTest()
{
  /* Dummy */
  return 0;
}


int
_regtestsTest()
{
  /* Dummy */
  return 0;
}


VOID
InitializeTests()
{
  InitializeListHead(&AllTests);
}

VOID
PerformTest(PROS_TEST Test)
{
  char TestName[200];
  char Buffer[200];
  int Result;

  memset(TestName, 0, sizeof(TestName));
  memset(Buffer, 0, sizeof(Buffer));

  if (!((Test->Routine)(TESTCMD_TESTNAME, TestName) == 0))
    {
      strcpy(TestName, "Unnamed");
    }

#ifdef SEH
  __try {
#endif
    Result = (Test->Routine)(TESTCMD_RUN, Buffer);
#ifdef SEH
  } __except(EXCEPTION_EXECUTE_HANDLER) {
    Result = TS_FAILED;
    strcpy(Buffer, "Failed due to exception");
  }
#endif

  if (Result != TS_OK)
    {
      DbgPrint("ROSREGTEST: (%s) Status: Failed (%s)\n", TestName, Buffer);
    }
  else
    {
      DbgPrint("ROSREGTEST: (%s) Status: Success\n", TestName);
    }
}

VOID
PerformTests()
{
  PLIST_ENTRY CurrentEntry;
  PLIST_ENTRY NextEntry;
  PROS_TEST Current;

  CurrentEntry = AllTests.Flink;
  while (CurrentEntry != &AllTests)
    {
      NextEntry = CurrentEntry->Flink;
      Current = CONTAINING_RECORD(CurrentEntry, ROS_TEST, ListEntry);
      PerformTest(Current);
      CurrentEntry = NextEntry;
    }
}

VOID
AddTest(TestRoutine Routine)
{
  PROS_TEST Test;

  Test = (PROS_TEST) AllocateMemory(sizeof(ROS_TEST));
  if (Test == NULL)
    {
      DbgPrint("Out of memory");
      return;
    }

  Test->Routine = Routine;

  InsertTailList(&AllTests, &Test->ListEntry);
}
