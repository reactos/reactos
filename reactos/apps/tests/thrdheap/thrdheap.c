#include <windows.h>
#include <stdio.h>
 
#define MAX_THREADS	10
#define STAT_PRINT_INTERVAL	250
 
typedef struct _THREADDATA
{
  DWORD id;
  HANDLE hThread;
  struct _THREADDATA **entry;
} THREADDATA, *PTHREADDATA;
 
static CRITICAL_SECTION LockThreadList;
static PTHREADDATA ThreadList[MAX_THREADS];
static LONG ThreadsCount = 0;
static ULONG ThreadsCreatedCount = 0;
 
DWORD WINAPI
TestThread(PTHREADDATA ThreadData)
{
  EnterCriticalSection(&LockThreadList);
  *(ThreadData->entry) = NULL;
  LeaveCriticalSection(&LockThreadList);
 
  InterlockedDecrement(&ThreadsCount);
  CloseHandle(ThreadData->hThread);
  HeapFree(GetProcessHeap(), 0, ThreadData);
  return 0;
}
 
BOOL CreateThreads(VOID)
{
  int x;
  PTHREADDATA td;
 
  if(ThreadsCount >= MAX_THREADS)
  {
    return FALSE;
  }
 
  for(x = 0; x < MAX_THREADS; x++)
  {
    if(ThreadList[x] == 0)
    {
      if(!(td = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(THREADDATA))))
      {
        printf("Unable to allocate memory for a THREADDATA structure!\n");
        return FALSE;
      }
 
      td->entry = &ThreadList[x];
 
      if(!(td->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)TestThread,
                                      td, 0, &td->id)))
      {
        printf("Failed to create a thread\n");
 
        /* free the memory, we couldn't find a free slot for the thread */
        HeapFree(GetProcessHeap(), 0, td);
        return FALSE;
      }
      InterlockedIncrement(&ThreadsCount);
 
      ThreadList[x] = td;
 
      /* close the handles later */
      return TRUE;
    }
  }
 
  return FALSE;
}
 
int main(int argc, char* argv[])
{  
  HANDLE Handles[MAX_THREADS];
  int x, n;
  BOOL PrintInfo;
 
  InitializeCriticalSection(&LockThreadList);
  ZeroMemory(ThreadList, MAX_THREADS * sizeof(PTHREADDATA));
  
  printf("ReactOS Heap/Thread Stress-Test\nType Ctrl+C to stop the infinite test\n\n");
 
  for(;;)
  {
    PrintInfo = FALSE;
    
    EnterCriticalSection(&LockThreadList);
    while(CreateThreads())
    {
      if(++ThreadsCreatedCount % STAT_PRINT_INTERVAL == 0)
      {
        PrintInfo = TRUE;
      }
    }
 
    /* build a list of handles */
    n = 0;
    for(x = 0; x < MAX_THREADS; x++)
    {
      if(ThreadList[x] != NULL)
        Handles[n++] = ThreadList[x]->hThread;
    }
 
    LeaveCriticalSection(&LockThreadList);
    WaitForMultipleObjects(MAX_THREADS, Handles, FALSE, INFINITE);
    if(PrintInfo)
    {
      printf("Created %d threads\r", (int)ThreadsCreatedCount);
    }
  }
}

