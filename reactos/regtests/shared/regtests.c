/*
 * PROJECT:         ReactOS kernel
 * FILE:            regtests/shared/regtests.c
 * PURPOSE:         Regression testing framework
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-07-2003  CSH  Created
 */
#include <roscfg.h>
#include <stdio.h>
#define NTOS_MODE_USER
#include <ntos.h>
#include "regtests.h"
#include <string.h>

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
PerformTest(TestOutputRoutine OutputRoutine, PROS_TEST Test, LPSTR TestName)
{
  char OutputBuffer[200];
  char Buffer[200];
  char Name[200];
  int Result;

  memset(Name, 0, sizeof(Name));
  memset(Buffer, 0, sizeof(Buffer));

  if (!((Test->Routine)(TESTCMD_TESTNAME, Name) == 0))
    {
      if (TestName != NULL)
        {
          return;
        }
      strcpy(Name, "Unnamed");
    }

  if (TestName != NULL)
    {
      if (_stricmp(Name, TestName) != 0)
        {
          return;
        }
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
      sprintf(OutputBuffer, "ROSREGTEST: (%s) Status: Failed (%s)\n", Name, Buffer);
    }
  else
    {
      sprintf(OutputBuffer, "ROSREGTEST: (%s) Status: Success\n", Name);
    }
  if (OutputRoutine != NULL)
    {
      (*OutputRoutine)(OutputBuffer);
    }
  else
    {
      DbgPrint(OutputBuffer);
    }
}

VOID
PerformTests(TestOutputRoutine OutputRoutine, LPSTR TestName)
{
  PLIST_ENTRY CurrentEntry;
  PLIST_ENTRY NextEntry;
  PROS_TEST Current;

  CurrentEntry = AllTests.Flink;
  while (CurrentEntry != &AllTests)
    {
      NextEntry = CurrentEntry->Flink;
      Current = CONTAINING_RECORD(CurrentEntry, ROS_TEST, ListEntry);
      PerformTest(OutputRoutine, Current, TestName);
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
