#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#define N_TIMEOUT	3

/*******************************************************************************/

typedef struct _TEST *PTEST;

typedef VOID (*PFNTEST)(PTEST Test, HANDLE hEvent);

typedef struct _TEST
{
  TCHAR *description;
  BOOL Result;
  PFNTEST Routine;
  int id;
} TEST;

static TEST Tests[3];

VOID RunTests(VOID)
{
  int i, nTests;
  static HANDLE hEvent;

  hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  if(hEvent == NULL)
  {
    _tprintf(_T("Unable to create event!"));
    return;
  }

  nTests = sizeof(Tests) / sizeof(TEST);

  for(i = 0; i < nTests; i++)
  {
    Tests[i].id = i + 1;

    if(Tests[i].Routine == NULL)
    {
      continue;
    }

    _tprintf(_T("+++ TEST %d: %s\n"), Tests[i].id, Tests[i].description);

    Tests[i].Routine(&Tests[i], hEvent);

    WaitForSingleObject(hEvent, INFINITE);

    _tprintf(_T("\n\n"));
  }

  CloseHandle(hEvent);
}

VOID PrintTestResults(VOID)
{
  int i, nTests, nsuccess = 0, nfailed = 0;
  TCHAR *status;

  nTests = sizeof(Tests) / sizeof(TEST);

  for(i = 0; i < nTests; i++)
  {
    if(Tests[i].Routine == NULL)
    {
      status = _T("SKIPPED");
    }
    else if(Tests[i].Result == TRUE)
    {
      status = _T("SUCCESS");
      nsuccess++;
    }
    else
    {
      status = _T("FAILED ");
      nfailed++;
    }

    _tprintf(_T("Test %d: %s %s\n"), i, status, Tests[i].description);
  }

  _tprintf(_T("\nTests succeeded: %d, failed: %d\n"), nsuccess, nfailed);
  if(nfailed == 0)
  {
    _tprintf(_T("  ALL TESTS SUCCESSFUL!\n"));
  }
}

/*******************************************************************************/

typedef struct _TESTINFO
{
  PTEST Test;
  int secsleft;
  HANDLE hTimer;
  HANDLE hEvent;
  /* additional stuff */
  union
  {
    struct
    {
      /* nothing */
    } Test1;
    struct
    {
      HANDLE hWaitEvent;
    } Test2;
    struct
    {
      HANDLE hWaitEvent;
      HANDLE hNotification;
    } Test3;
  };
} TESTINFO, *PTESTINFO;

VOID CALLBACK TimerCallback1(PVOID Param, BOOLEAN Fired)
{
  PTESTINFO Info = (PTESTINFO)Param;

  _tprintf(_T("[%d]TimerCallback(0x%x, %d) called (%d)\n"), (int)Info->Test->id, (int)Info->hTimer, (int)Fired, --Info->secsleft);

  if(Info->secsleft == 0)
  {
    BOOL stat;

    _tprintf(_T("[%d]Timout finished, delete timer queue..."), (int)Info->Test->id);
    stat = DeleteTimerQueueTimer(NULL, Info->hTimer, NULL);
    if(stat)
      _tprintf(_T("returned OK -> test FAILED!\n"));
    else
    {
      int error = GetLastError();

      switch(error)
      {
        case ERROR_IO_PENDING:
          _tprintf(_T("OK, Overlapped I/O operation in progress\n"));
          /* this test is only successful in this case */
          Info->Test->Result = TRUE;
          break;
        default:
          _tprintf(_T("Failed, LastError: %d\n"), (int)GetLastError());
          break;
      }
    }

    /* set the event to continue tests */
    SetEvent(Info->hEvent);
  }
}

VOID Test1(PTEST Test, HANDLE hEvent)
{
  static TESTINFO Info;

  Info.Test = Test;
  Info.hEvent = hEvent;
  Info.secsleft = N_TIMEOUT;

  if(!CreateTimerQueueTimer(&Info.hTimer, NULL, TimerCallback1, &Info, 1000, 1000, 0))
  {
    _tprintf(_T("[%d]CreateTimerQueueTimer() failed, LastError: %d!"), (int)Info.Test->id, (int)GetLastError());
    /* we failed, set the event to continue tests */
    SetEvent(hEvent);
    return;
  }

  _tprintf(_T("[%d]CreateTimerQueueTimer() created timer 0x%x, countdown (%d sec)...\n"), (int)Info.Test->id, (int)Info.hTimer, (int)Info.secsleft);
}

/*******************************************************************************/

VOID CALLBACK TimerCallback2(PVOID Param, BOOLEAN Fired)
{
  PTESTINFO Info = (PTESTINFO)Param;

  _tprintf(_T("[%d]TimerCallback(0x%x, %d) called (%d)\n"), (int)Info->Test->id, (int)Info->hTimer, (int)Fired, --Info->secsleft);

  if(Info->secsleft == 0)
  {
    /* set the event to continue tests */
    SetEvent(Info->Test2.hWaitEvent);

    /* sleep a bit */
    Sleep(1500);
  }
}

VOID Test2(PTEST Test, HANDLE hEvent)
{
  static TESTINFO Info;
  BOOL stat;

  Info.Test = Test;
  Info.hEvent = hEvent;
  Info.secsleft = N_TIMEOUT;

  Info.Test2.hWaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  if(Info.Test2.hWaitEvent == NULL)
  {
    _tprintf(_T("[%d]Unable to create event!\n"), (int)Info.Test->id);
    return;
  }

  if(!CreateTimerQueueTimer(&Info.hTimer, NULL, TimerCallback2, &Info, 1000, 1000, 0))
  {
    _tprintf(_T("[%d]CreateTimerQueueTimer() failed, LastError: %d!"), (int)Info.Test->id, (int)GetLastError());

    CloseHandle(Info.Test2.hWaitEvent);
    /* we failed, set the event to continue tests */
    SetEvent(hEvent);
    return;
  }

  _tprintf(_T("[%d]CreateTimerQueueTimer() created timer 0x%x, countdown (%d sec)...\n"), (int)Test->id, (int)Info.hTimer, (int)Info.secsleft);

  WaitForSingleObject(Info.Test2.hWaitEvent, INFINITE);

  _tprintf(_T("[%d]Timout finished, delete timer queue..."), (int)Test->id);
  stat = DeleteTimerQueueTimer(NULL, Info.hTimer, INVALID_HANDLE_VALUE);
  if(stat)
  {
    _tprintf(_T("OK\n"));
    /* this test is only successful in this case */
    Test->Result = TRUE;
  }
  else
  {
    int error = GetLastError();

    switch(error)
    {
      case ERROR_IO_PENDING:
        _tprintf(_T("FAILED, Overlapped I/O operation in progress\n"));
        break;
      default:
        _tprintf(_T("Failed, LastError: %d\n"), (int)GetLastError());
        break;
    }
  }

  SetEvent(Info.hEvent);
}

/*******************************************************************************/

VOID CALLBACK TimerCallback3(PVOID Param, BOOLEAN Fired)
{
  PTESTINFO Info = (PTESTINFO)Param;

  _tprintf(_T("[%d]TimerCallback(0x%x, %d) called (%d)\n"), (int)Info->Test->id, (int)Info->hTimer, (int)Fired, --Info->secsleft);

  if(Info->secsleft == 0)
  {
    /* set the event to continue tests */
    SetEvent(Info->Test3.hWaitEvent);

    /* sleep a bit */
    Sleep(1500);
  }
}

VOID Test3(PTEST Test, HANDLE hEvent)
{
  static TESTINFO Info;
  BOOL stat;

  Info.Test = Test;
  Info.hEvent = hEvent;
  Info.secsleft = N_TIMEOUT;

  Info.Test3.hWaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  if(Info.Test3.hWaitEvent == NULL)
  {
    _tprintf(_T("[%d]Unable to create event!\n"), (int)Info.Test->id);
    return;
  }

  Info.Test3.hNotification = CreateEvent(NULL, FALSE, FALSE, NULL);
  if(Info.Test3.hNotification == NULL)
  {
    _tprintf(_T("[%d]Unable to create notification event!\n"), (int)Info.Test->id);
    return;
  }

  if(!CreateTimerQueueTimer(&Info.hTimer, NULL, TimerCallback3, &Info, 1000, 1000, 0))
  {
    _tprintf(_T("[%d]CreateTimerQueueTimer() failed, LastError: %d!"), (int)Info.Test->id, (int)GetLastError());

    CloseHandle(Info.Test3.hWaitEvent);
    CloseHandle(Info.Test3.hNotification);
    /* we failed, set the event to continue tests */
    SetEvent(hEvent);
    return;
  }

  _tprintf(_T("[%d]CreateTimerQueueTimer() created timer 0x%x, countdown (%d sec)...\n"), (int)Test->id, (int)Info.hTimer, (int)Info.secsleft);

  WaitForSingleObject(Info.Test3.hWaitEvent, INFINITE);

  _tprintf(_T("[%d]Timout finished, delete timer queue..."), (int)Test->id);
  stat = DeleteTimerQueueTimer(NULL, Info.hTimer, Info.Test3.hNotification);
  if(stat)
  {
    _tprintf(_T("returned OK -> test FAILED!\n"));
  }
  else
  {
    int error = GetLastError();

    switch(error)
    {
      case ERROR_IO_PENDING:
        _tprintf(_T("OK, Overlapped I/O operation in progress\n"));
        /* this test is only successful in this case */
        Test->Result = TRUE;
        break;
      default:
        _tprintf(_T("Failed, LastError: %d\n"), (int)GetLastError());
        break;
    }
  }

  WaitForSingleObject(Info.Test3.hNotification, INFINITE);

  CloseHandle(Info.Test3.hWaitEvent);
  CloseHandle(Info.Test3.hNotification);

  SetEvent(Info.hEvent);
}

/*******************************************************************************/

VOID
InitTests(VOID)
{
  ZeroMemory(Tests, sizeof(Tests));

  Tests[0].description = _T("non-blocking DeleteTimerQueueTimer() call from callback");
  Tests[0].Routine = Test1;

  Tests[1].description = _T("blocking DeleteTimerQueueTimer() call");
  Tests[1].Routine = Test2;

  Tests[2].description = _T("blocking DeleteTimerQueueTimer() call with specified event");
  Tests[2].Routine = Test3;
}

int main(int argc, char* argv[])
{
  _tprintf(_T("+++ TimerQueue test running +++\n\n"));

  InitTests();

  RunTests();

  _tprintf(_T("\n+++ RESULTS +++\n"));

  PrintTestResults();

  return 0;
}
