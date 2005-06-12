/*
 * PROJECT:         ReactOS kernel
 * FILE:            regtests/shared/regtests.c
 * PURPOSE:         Regression testing framework
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-07-2003  CSH  Created
 */
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#define NTOS_MODE_USER
#include <ntos.h>
#include <pseh.h>
#include "regtests.h"

#define NDEBUG
#include <debug.h>

typedef struct _PERFORM_TEST_ARGS
{
  TestOutputRoutine OutputRoutine;
  PROS_TEST Test;
  LPSTR TestName;
} PERFORM_TEST_ARGS;

int _Result;
char *_Buffer;

static LIST_ENTRY AllTests;

VOID
InitializeTests()
{
  InitializeListHead(&AllTests);
}

char*
FormatExecutionTime(char *buffer, LPFILETIME time)
{
  ULONG milliseconds = time->dwLowDateTime / 10000;
  
  sprintf(buffer,
	      "%ldms",
	      milliseconds);
  return buffer;
}

DWORD WINAPI
PerformTest(PVOID _arg)
{
  PERFORM_TEST_ARGS *Args = (PERFORM_TEST_ARGS *)_arg;
  TestOutputRoutine OutputRoutine = Args->OutputRoutine;
  PROS_TEST Test = Args->Test;
  LPSTR TestName = Args->TestName;
  HANDLE hThread;
  FILETIME time;
  FILETIME ExecutionTime;
  char OutputBuffer[5000];
  char Buffer[5000];
  char Format[100];

  hThread = GetCurrentThread();
  _SetThreadPriority(hThread, THREAD_PRIORITY_IDLE);

  memset(Buffer, 0, sizeof(Buffer));
  
  _SEH_TRY {
    _Result = TS_OK;
    _Buffer = Buffer;
    (Test->Routine)(TESTCMD_RUN);
  } _SEH_HANDLE {
    _Result = TS_FAILED;
    sprintf(Buffer, "due to exception 0x%lx", _SEH_GetExceptionCode());
  } _SEH_END;

  if (_Result == TS_OK)
    {
      if (!_GetThreadTimes(hThread,
  	                       &time,
  	                       &time,
  	                       &time,
  	                       &ExecutionTime))
      {
        ExecutionTime.dwLowDateTime = 10;
        ExecutionTime.dwHighDateTime = 10;
      }
      sprintf(OutputBuffer,
              "[%s] Success [%s]\n",
              TestName,
              FormatExecutionTime(Format,
                                  &ExecutionTime));
    }
  else
      sprintf(OutputBuffer, "[%s] Failed (%s)\n", TestName, Buffer);

  if (OutputRoutine != NULL)
      (*OutputRoutine)(OutputBuffer);
  else
      DbgPrint(OutputBuffer);
  return 1;
}

VOID
PerformTests(TestOutputRoutine OutputRoutine, LPSTR TestName)
{
  PLIST_ENTRY CurrentEntry;
  PLIST_ENTRY NextEntry;
  PROS_TEST Current;
  PERFORM_TEST_ARGS Args;
  HANDLE hThread;
  char OutputBuffer[1024];
  char Name[200];
  DWORD TimeOut;

  Args.OutputRoutine = OutputRoutine;
  Args.TestName = Name;
  
  CurrentEntry = AllTests.Flink;
  for (; CurrentEntry != &AllTests; CurrentEntry = NextEntry)
    {
      NextEntry = CurrentEntry->Flink;
      Current = CONTAINING_RECORD(CurrentEntry, ROS_TEST, ListEntry);
      Args.Test = Current;

      /* Get name of test */
      memset(Name, 0, sizeof(Name));

      _Result = TS_OK;
      _Buffer = Name;
      (Current->Routine)(TESTCMD_TESTNAME);
      if (_Result != TS_OK)
        {
          if (TestName != NULL)
            continue;
          strcpy(Name, "Unnamed");
        }

      if ((TestName != NULL) && (_stricmp(Name, TestName) != 0))
        continue;

      /* Get timeout for test */
      TimeOut = 0;
      _Result = TS_OK;
      _Buffer = (char *)&TimeOut;
      (Current->Routine)(TESTCMD_TIMEOUT);
      if (_Result != TS_OK || TimeOut == INFINITE)
          TimeOut = 5000;

      /* Run test in thread */
      hThread = _CreateThread(NULL, 0, PerformTest, (PVOID)&Args, 0, NULL);
      if (hThread == NULL)
          sprintf(OutputBuffer,
                  "[%s] Failed (CreateThread() failed: %d)\n",
                  Name, (unsigned int)_GetLastError());
      else if (_WaitForSingleObject(hThread, TimeOut) == WAIT_TIMEOUT)
        {
          if (!_TerminateThread(hThread, 0))
              sprintf(OutputBuffer,
                      "[%s] Failed (timed out after %dms; TerminateThread() failed: %d)\n",
                      Name, (int)TimeOut, (unsigned int)_GetLastError());
          else
              sprintf(OutputBuffer, "[%s] Failed (timed out after %dms)\n", Name, (int)TimeOut);          
      	  _CloseHandle(hThread);
        }
      else
        {
      	  _CloseHandle(hThread);
          continue;
        }

      if (OutputRoutine != NULL)
          (*OutputRoutine)(OutputBuffer);
      else
          DbgPrint(OutputBuffer);
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
