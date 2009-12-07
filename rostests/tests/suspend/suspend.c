#define UNICODE
#define WIN32_NO_STATUS
#include <windows.h>
#include <stdio.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#define NDEBUG
#include <debug.h>

static volatile DWORD z;
static volatile DWORD x=0;

static NTSTATUS WINAPI
thread_1(PVOID Param)
{
  DWORD y=0;

  for(;;)
  {
   z++;
   if(x>50)
   {
     printf("I should have been suspended for years :-)\n");
     Sleep(100);
     x=0;y++;
     if(y==3) ExitProcess(0);
   }
  }
}

int
main(int argc, char *argv[])
{
  HANDLE thread;
  DWORD thread_id;
  CONTEXT context;

  context.ContextFlags=CONTEXT_CONTROL;

  z=0;
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

  SuspendThread(thread);

  for(;;)
  {
    printf("%lx ", z);
    Sleep(100);x++;
    if(x>100 && GetThreadContext(thread, &context))
    {
      printf("EIP: %lx\n", context.Eip);
      printf("Calling resumethread ... \n");
      ResumeThread(thread);
    }
  }

  ExitProcess(0);
  return(0);
}
