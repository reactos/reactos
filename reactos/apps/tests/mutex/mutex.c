#include <ddk/ntddk.h>
#include <windows.h>
#include <string.h>
#include <stdio.h>


HANDLE OutputHandle;
HANDLE InputHandle;

HANDLE hThread[2];
DWORD dwCounter = 0;
HANDLE hMutex;


void dprintf(char* fmt, ...)
{
   va_list args;
   char buffer[255];

   va_start(args,fmt);
   vsprintf(buffer,fmt,args);
   WriteConsoleA(OutputHandle, buffer, strlen(buffer), NULL, NULL);
   va_end(args);
}


DWORD WINAPI thread1(LPVOID crap)
{
  DWORD dwError = 0;
  DWORD i;

  dprintf("Thread 1 running!\n");

  for (i = 0; i < 10; i++)
    {
      dwError = WaitForSingleObject(hMutex, INFINITE);
      if (dwError == WAIT_FAILED)
	{
	  dprintf("Thread2: WaitForSingleObject failed!\n");
	  return 1;
	}
      else if (dwError == WAIT_ABANDONED_0)
	{
	  dprintf("Thread2: WaitForSingleObject returned WAIT_ABANDONED_0\n");
	}

      dprintf("Thread1: dwCounter : %lu -->", dwCounter);
      dwCounter++;
      dprintf(" %lu\n", dwCounter);
      ReleaseMutex(hMutex);
    }

  return 1;
}

DWORD WINAPI thread2(LPVOID crap)
{
  DWORD dwError = 0;
  DWORD i;
  dprintf("Thread 2 running!\n");

  for (i = 0; i < 10; i++)
    {
      dwError = WaitForSingleObject(hMutex, INFINITE);
      if (dwError == WAIT_FAILED)
	{
	  dprintf("Thread2: WaitForSingleObject failed!\n");
	  return 1;
	}
      else if (dwError == WAIT_ABANDONED_0)
	{
	  dprintf("Thread2: WaitForSingleObject returned WAIT_ABANDONED_0\n");
	}

      dprintf("Thread2: dwCounter : %lu -->", dwCounter);
      dwCounter++;
      dprintf(" %lu\n", dwCounter);
      ReleaseMutex(hMutex);
    }

  return 1;
}


int main(int argc, char* argv[])
{
  DWORD dwError;
  DWORD id1,id2;

  AllocConsole();
  InputHandle = GetStdHandle(STD_INPUT_HANDLE);
  OutputHandle =  GetStdHandle(STD_OUTPUT_HANDLE);

  dprintf("Calling CreateMutex()\n");
  hMutex = CreateMutexW(NULL, FALSE, L"TestMutex");
  if (hMutex == INVALID_HANDLE_VALUE)
    {
      dprintf("CreateMutex() failed! Error: %lu\n", GetLastError);
      return 0;
    }
  dprintf("CreateMutex() succeeded!\n");

  hThread[0] = CreateThread(0, 0, thread1, 0, 0, &id1);
  hThread[1] = CreateThread(0, 0, thread2, 0, 0, &id2);

  dprintf("Calling WaitForMultipleObject()\n");
  dwError = WaitForMultipleObjects(2, hThread, TRUE, INFINITE);
  dprintf("WaitForMultipleObject() Error: %lu\n", dwError);

  CloseHandle(hThread[0]);
  CloseHandle(hThread[1]);

  CloseHandle(hMutex);

  return 0;
}
