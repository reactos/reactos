#define UNICODE

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>
#include <stdio.h>

#define DBG
#define NDEBUG
#include <debug.h>

static volatile DWORD z;
static volatile DWORD x=0;

static NTSTATUS STDCALL
thread_1(PVOID Param)
{
  DWORD y=0;

  for(;;)
  {
   z++;
   if(x>50)
   {
     Sleep(100);
     x=0;y++;
     if(y==3) return(0);
   }
  }
}

int
main(int argc, char *argv[])
{
  HANDLE thread;
  DWORD thread_id;
  CONTEXT context;
  DWORD z = 0;

  context.ContextFlags=CONTEXT_CONTROL;
  
  while (z < 50)
    {
      z++;
      thread=CreateThread(NULL,
			  0x1000,
			  (LPTHREAD_START_ROUTINE)thread_1,
			  NULL,
			  0,
			  &thread_id);
      
      if(!thread)
	{
	  printf("Error: could not create thread ...\n");
	  ExitProcess(0);
	}
      
      Sleep(1000);
      
      printf("T");
      if ((z % 5) == 0)
	{
	  TerminateThread(thread, 0);
	}
      printf("C");
      GetThreadContext(thread, &context);
      printf("S");
      SuspendThread(thread);
      printf("R");
      ResumeThread(thread);      
      TerminateThread(thread, 0);
    }

  ExitProcess(0);
  return(0);
}
