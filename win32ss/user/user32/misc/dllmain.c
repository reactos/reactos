#include <user32.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(user32);

#define KEY_LENGTH 1024

static ULONG User32TlsIndex;
HINSTANCE User32Instance;

PPROCESSINFO g_ppi = NULL;
PUSER_HANDLE_TABLE gHandleTable = NULL;
PUSER_HANDLE_ENTRY gHandleEntries = NULL;
PSERVERINFO gpsi = NULL;
ULONG_PTR g_ulSharedDelta;
BOOLEAN gfServerProcess = FALSE;

WCHAR szAppInit[KEY_LENGTH];

BOOL
GetDllList()
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Attributes;
    BOOL bRet = FALSE;
    BOOL bLoad;
    HANDLE hKey = NULL;
    DWORD dwSize;
    PKEY_VALUE_PARTIAL_INFORMATION kvpInfo = NULL;

    UNICODE_STRING szKeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows");
    UNICODE_STRING szLoadName = RTL_CONSTANT_STRING(L"LoadAppInit_DLLs");
    UNICODE_STRING szDllsName = RTL_CONSTANT_STRING(L"AppInit_DLLs");

    InitializeObjectAttributes(&Attributes, &szKeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenKey(&hKey, KEY_READ, &Attributes);
    if (NT_SUCCESS(Status))
    {
        dwSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD);
        kvpInfo = HeapAlloc(GetProcessHeap(), 0, dwSize);
        if (!kvpInfo)
            goto end;

        Status = NtQueryValueKey(hKey,
                                 &szLoadName,
                                 KeyValuePartialInformation,
                                 kvpInfo,
                                 dwSize,
                                 &dwSize);
        if (!NT_SUCCESS(Status))
            goto end;

        RtlMoveMemory(&bLoad,
                      kvpInfo->Data,
                      kvpInfo->DataLength);

        HeapFree(GetProcessHeap(), 0, kvpInfo);
        kvpInfo = NULL;

        if (bLoad)
        {
            Status = NtQueryValueKey(hKey,
                                     &szDllsName,
                                     KeyValuePartialInformation,
                                     NULL,
                                     0,
                                     &dwSize);
            if (Status != STATUS_BUFFER_TOO_SMALL)
                goto end;

            kvpInfo = HeapAlloc(GetProcessHeap(), 0, dwSize);
            if (!kvpInfo)
                goto end;

            Status = NtQueryValueKey(hKey,
                                     &szDllsName,
                                     KeyValuePartialInformation,
                                     kvpInfo,
                                     dwSize,
                                     &dwSize);
            if (NT_SUCCESS(Status))
            {
                LPWSTR lpBuffer = (LPWSTR)kvpInfo->Data;
                if (*lpBuffer != UNICODE_NULL)
                {
                    INT bytesToCopy, nullPos;

                    bytesToCopy = min(kvpInfo->DataLength, KEY_LENGTH * sizeof(WCHAR));

                    if (bytesToCopy != 0)
                    {
                        RtlMoveMemory(szAppInit,
                                      kvpInfo->Data,
                                      bytesToCopy);

                        nullPos = (bytesToCopy / sizeof(WCHAR)) - 1;

                        /* ensure string is terminated */
                        szAppInit[nullPos] = UNICODE_NULL;

                        bRet = TRUE;
                    }
                }
            }
        }
    }

end:
    if (hKey)
        NtClose(hKey);

    if (kvpInfo)
        HeapFree(GetProcessHeap(), 0, kvpInfo);

    return bRet;
}


VOID
LoadAppInitDlls()
{
    szAppInit[0] = UNICODE_NULL;

    if (GetDllList())
    {
        WCHAR buffer[KEY_LENGTH];
        LPWSTR ptr;
		size_t i;

        RtlCopyMemory(buffer, szAppInit, KEY_LENGTH);

		for (i = 0; i < KEY_LENGTH; ++ i)
		{
			if(buffer[i] == L' ' || buffer[i] == L',')
				buffer[i] = 0;
		}

		for (i = 0; i < KEY_LENGTH; )
		{
			if(buffer[i] == 0)
				++ i;
			else
			{
				ptr = buffer + i;
				i += wcslen(ptr);
				LoadLibraryW(ptr);
			}
		}
    }
}

VOID
UnloadAppInitDlls()
{
    if (szAppInit[0] != UNICODE_NULL)
    {
        WCHAR buffer[KEY_LENGTH];
        HMODULE hModule;
        LPWSTR ptr;
		size_t i;

        RtlCopyMemory(buffer, szAppInit, KEY_LENGTH * sizeof(WCHAR));

		for (i = 0; i < KEY_LENGTH; ++ i)
		{
			if(buffer[i] == L' ' || buffer[i] == L',')
				buffer[i] = 0;
		}

		for (i = 0; i < KEY_LENGTH; )
		{
			if(buffer[i] == 0)
				++ i;
			else
			{
				ptr = buffer + i;
				i += wcslen(ptr);
				hModule = GetModuleHandleW(ptr);
				FreeLibrary(hModule);
			}
		}
    }
}

BOOL
InitThread(VOID)
{
   return TRUE;
}

VOID
CleanupThread(VOID)
{
}

BOOL
Init(VOID)
{
   USERCONNECT UserCon;
   PVOID *KernelCallbackTable;
 
   /* Set up the kernel callbacks. */
   KernelCallbackTable = NtCurrentPeb()->KernelCallbackTable;
   KernelCallbackTable[USER32_CALLBACK_WINDOWPROC] =
      (PVOID)User32CallWindowProcFromKernel;
   KernelCallbackTable[USER32_CALLBACK_SENDASYNCPROC] =
      (PVOID)User32CallSendAsyncProcForKernel;
   KernelCallbackTable[USER32_CALLBACK_LOADSYSMENUTEMPLATE] =
      (PVOID)User32LoadSysMenuTemplateForKernel;
   KernelCallbackTable[USER32_CALLBACK_LOADDEFAULTCURSORS] =
      (PVOID)User32SetupDefaultCursors;
   KernelCallbackTable[USER32_CALLBACK_HOOKPROC] =
      (PVOID)User32CallHookProcFromKernel;
   KernelCallbackTable[USER32_CALLBACK_EVENTPROC] =
      (PVOID)User32CallEventProcFromKernel;
   KernelCallbackTable[USER32_CALLBACK_LOADMENU] =
      (PVOID)User32CallLoadMenuFromKernel;
   KernelCallbackTable[USER32_CALLBACK_CLIENTTHREADSTARTUP] =
      (PVOID)User32CallClientThreadSetupFromKernel;
   KernelCallbackTable[USER32_CALLBACK_CLIENTLOADLIBRARY] =
      (PVOID)User32CallClientLoadLibraryFromKernel;
   KernelCallbackTable[USER32_CALLBACK_GETCHARSETINFO] =
      (PVOID)User32CallGetCharsetInfo;

   NtUserProcessConnect( NtCurrentProcess(),
                         &UserCon,
                         sizeof(USERCONNECT));

   g_ppi = GetWin32ClientInfo()->ppi; // Snapshot PI, used as pointer only!
   g_ulSharedDelta = UserCon.siClient.ulSharedDelta;
   gpsi = SharedPtrToUser(UserCon.siClient.psi);
   gHandleTable = SharedPtrToUser(UserCon.siClient.aheList);
   gHandleEntries = SharedPtrToUser(gHandleTable->handles);

   RtlInitializeCriticalSection(&gcsUserApiHook);
   gfServerProcess = FALSE; // FIXME HAX! Used in CsrClientConnectToServer(,,,,&gfServerProcess);

   //CsrClientConnectToServer(L"\\Windows", 0, NULL, 0, &gfServerProcess);
   //ERR("1 SI 0x%x : HT 0x%x : D 0x%x\n", UserCon.siClient.psi, UserCon.siClient.aheList,  g_ulSharedDelta);

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
            LoadAppInitDlls();

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
   UnloadAppInitDlls();
   GdiDllInitialize(NULL, DLL_PROCESS_DETACH, NULL);
   TlsFree(User32TlsIndex);
}

INT WINAPI
DllMain(
   IN PVOID hInstanceDll,
   IN ULONG dwReason,
   IN PVOID reserved)
{
   switch (dwReason)
   {
      case DLL_PROCESS_ATTACH:
         User32Instance = hInstanceDll;
         if (!RegisterClientPFN())
         {
             return FALSE;
         }

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
         if (hImmInstance) FreeLibrary(hImmInstance);
         CleanupThread();
         Cleanup();
         break;
   }

   return TRUE;
}


VOID
FASTCALL
GetConnected(VOID)
{
  USERCONNECT UserCon;
//  ERR("GetConnected\n");

  if ((PTHREADINFO)NtCurrentTeb()->Win32ThreadInfo == NULL)
     NtUserGetThreadState(THREADSTATE_GETTHREADINFO);

  if (gpsi && g_ppi) return;
// FIXME HAX: Due to the "Dll Initialization Bug" we have to call this too.
  GdiDllInitialize(NULL, DLL_PROCESS_ATTACH, NULL);

  NtUserProcessConnect( NtCurrentProcess(),
                         &UserCon,
                         sizeof(USERCONNECT));

  g_ppi = GetWin32ClientInfo()->ppi;
  g_ulSharedDelta = UserCon.siClient.ulSharedDelta;
  gpsi = SharedPtrToUser(UserCon.siClient.psi);
  gHandleTable = SharedPtrToUser(UserCon.siClient.aheList);
  gHandleEntries = SharedPtrToUser(gHandleTable->handles);  
  
}

NTSTATUS
WINAPI
User32CallClientThreadSetupFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  ERR("GetConnected\n");
  return ZwCallbackReturn(NULL, 0, STATUS_SUCCESS);  
}

NTSTATUS
WINAPI
User32CallGetCharsetInfo(PVOID Arguments, ULONG ArgumentLength)
{
  BOOL Ret;
  PGET_CHARSET_INFO pgci = (PGET_CHARSET_INFO)Arguments;

  TRACE("GetCharsetInfo\n");

  Ret = TranslateCharsetInfo((DWORD *)pgci->Locale, &pgci->Cs, TCI_SRCLOCALE);

  return ZwCallbackReturn(Arguments, ArgumentLength, Ret ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);  
}
