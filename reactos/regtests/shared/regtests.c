/*
 * PROJECT:         ReactOS kernel
 * FILE:            regtests/shared/regtests.c
 * PURPOSE:         Regression testing framework
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-07-2003  CSH  Created
 */
#include <roscfg.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define NTOS_MODE_USER
#include <ntos.h>
#include "regtests.h"

#define NDEBUG
#include <debug.h>

int _Result;
char *_Buffer;

static LIST_ENTRY AllTests;

void *_alloca(size_t size)
{
  void *ret;

  asm ("movl %1, %%eax\n"
       "addl $3, %%eax\n"
       "andl $-4, %%eax\n"
       "subl %%eax, %%esp\n"
       "movl %%esp, %0\n"
       : "=m" (ret) : "m" (size) : "eax");

  return ret;
}

VOID
InitializeTests()
{
  InitializeListHead(&AllTests);
}

VOID
PerformTest(TestOutputRoutine OutputRoutine, PROS_TEST Test, LPSTR TestName)
{
  char OutputBuffer[5000];
  char Buffer[5000];
  char Name[200];

  memset(Buffer, 0, sizeof(Buffer));
  memset(Name, 0, sizeof(Name));

  _Result = TS_OK;
  _Buffer = Name;
  (Test->Routine)(TESTCMD_TESTNAME);
  if (_Result != TS_OK)
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
    _Result = TS_OK;
    _Buffer = Buffer;
    (Test->Routine)(TESTCMD_RUN);
#ifdef SEH
  } __except(EXCEPTION_EXECUTE_HANDLER) {
    _Result = TS_FAILED;
    strcpy(Buffer, "Failed due to exception");
  }
#endif

  if (_Result != TS_OK)
    {
      sprintf(OutputBuffer, "ROSREGTEST: |%s| Status: Failed (%s)\n", Name, Buffer);
    }
  else
    {
      sprintf(OutputBuffer, "ROSREGTEST: |%s| Status: Success\n", Name);
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

  Test = (PROS_TEST) malloc(sizeof(ROS_TEST));
  if (Test == NULL)
    {
      DbgPrint("Out of memory");
      return;
    }

  Test->Routine = Routine;

  InsertTailList(&AllTests, &Test->ListEntry);
}

PVOID STDCALL
FrameworkGetHook(ULONG index)
{
  return FrameworkGetHookInternal(index);
}
