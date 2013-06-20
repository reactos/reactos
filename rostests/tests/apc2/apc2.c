
#include <windows.h>
#include <stdio.h>

VOID CALLBACK TimerApcProc(
  LPVOID lpArg,
  DWORD dwTimerLowValue,
  DWORD dwTimerHighValue )
{
  printf("APC Callback %lu\n", *(PDWORD)lpArg);
}


int main()
{
  HANDLE          hTimer;
  BOOL            bSuccess;
  LARGE_INTEGER   DueTime;
  DWORD           value = 1;

  hTimer = CreateWaitableTimer(NULL, FALSE, NULL );

  if (!hTimer)
  {
    printf("CreateWaitableTimer failed!\n");
    return 0;
  }

  DueTime.QuadPart = -(LONGLONG)(5 * 10000000);

  bSuccess = SetWaitableTimer(
           hTimer,
           &DueTime,
           2001 /*interval (using an odd number to be able to find it easy in kmode) */,
           TimerApcProc,
           &value /*callback argument*/,
           FALSE );

  if (!bSuccess)
  {
    printf("SetWaitableTimer failed!\n");
    return 0;
  }

  for (;value <= 10; value++ )
  {
    SleepEx(INFINITE, TRUE /*alertable*/ );
  }

  CloseHandle( hTimer );
  return 0;
}

