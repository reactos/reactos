#include <user32.h>

#include <wine/debug.h>

static ULONG User32TlsIndex;
HINSTANCE User32Instance;
PUSER_HANDLE_TABLE gHandleTable = NULL;


PUSER32_THREAD_DATA
User32GetThreadData()
{
   return ((PUSER32_THREAD_DATA)TlsGetValue(User32TlsIndex));
}

BOOL
InitThread(VOID)
{
   PUSER32_THREAD_DATA ThreadData;

   ThreadData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                          sizeof(USER32_THREAD_DATA));
   if (ThreadData == NULL)
      return FALSE;
   if (!TlsSetValue(User32TlsIndex, ThreadData))
      return FALSE;
   return TRUE;
}

VOID
CleanupThread(VOID)
{
   PUSER32_THREAD_DATA ThreadData;

   ThreadData = (PUSER32_THREAD_DATA)TlsGetValue(User32TlsIndex);
   HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, ThreadData);
   TlsSetValue(User32TlsIndex, 0);
}

BOOL
Init(VOID)
{
   /* Set up the kernel callbacks. */
   NtCurrentTeb()->ProcessEnvironmentBlock->KernelCallbackTable[USER32_CALLBACK_WINDOWPROC] =
      (PVOID)User32CallWindowProcFromKernel;
   NtCurrentTeb()->ProcessEnvironmentBlock->KernelCallbackTable[USER32_CALLBACK_SENDASYNCPROC] =
      (PVOID)User32CallSendAsyncProcForKernel;
   NtCurrentTeb()->ProcessEnvironmentBlock->KernelCallbackTable[USER32_CALLBACK_LOADSYSMENUTEMPLATE] =
      (PVOID)User32LoadSysMenuTemplateForKernel;
   NtCurrentTeb()->ProcessEnvironmentBlock->KernelCallbackTable[USER32_CALLBACK_LOADDEFAULTCURSORS] =
      (PVOID)User32SetupDefaultCursors;
   NtCurrentTeb()->ProcessEnvironmentBlock->KernelCallbackTable[USER32_CALLBACK_HOOKPROC] =
      (PVOID)User32CallHookProcFromKernel;
   {
     PW32THREADINFO ti = (PW32THREADINFO)NtCurrentTeb()->Win32ThreadInfo;
     PW32PROCESSINFO pi = ti->pi;
     gHandleTable = (PUSER_HANDLE_TABLE) pi->UserHandleTable;
   }
   /* Allocate an index for user32 thread local data. */
   User32TlsIndex = TlsAlloc();
   if (User32TlsIndex != TLS_OUT_OF_INDEXES)
   {
      if (MessageInit())
      {
         if (MenuInit())
         {
            InitializeCriticalSection(&U32AccelCacheLock);
            GdiDllInitialize(NULL, DLL_PROCESS_ATTACH, NULL);
            InitStockObjects();

            return TRUE;
         }
         MessageCleanup();
      }
      TlsFree(User32TlsIndex);
   }

   return FALSE;
}

VOID
Cleanup(VOID)
{
   DeleteCriticalSection(&U32AccelCacheLock);
   MenuCleanup();
   MessageCleanup();
   DeleteFrameBrushes();
   GdiDllInitialize(NULL, DLL_PROCESS_DETACH, NULL);
   TlsFree(User32TlsIndex);
}

INT STDCALL
DllMain(
   IN PVOID hInstanceDll,
   IN ULONG dwReason,
   IN PVOID reserved)
{
   switch (dwReason)
   {
      case DLL_PROCESS_ATTACH:
         User32Instance = hInstanceDll;
         if (!NtUserRegisterUserModule(hInstanceDll))
             return FALSE;

         hProcessHeap = RtlGetProcessHeap();
         if (!Init())
            return FALSE;
         if (!InitThread())
         {
            Cleanup();
            return FALSE;
         }
     
         /* Initialize message spying */
        if (!SPY_Init()) return FALSE;

         break;

      case DLL_THREAD_ATTACH:
         if (!InitThread())
            return FALSE;
         break;

      case DLL_THREAD_DETACH:
         CleanupThread();
         break;

      case DLL_PROCESS_DETACH:
         CleanupThread();
         Cleanup();
         break;
   }

   return TRUE;
}
