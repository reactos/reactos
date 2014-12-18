#include <user32.h>

#include <ndk/cmfuncs.h>

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
BOOLEAN gfLogonProcess  = FALSE;
BOOLEAN gfServerProcess = FALSE;
HICON hIconSmWindows = NULL, hIconWindows = NULL;

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

        RtlCopyMemory(buffer, szAppInit, KEY_LENGTH * sizeof(WCHAR) );

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

PVOID apfnDispatch[USER32_CALLBACK_MAXIMUM + 1] =
{
    User32CallWindowProcFromKernel,
    User32CallSendAsyncProcForKernel,
    User32LoadSysMenuTemplateForKernel,
    User32SetupDefaultCursors,
    User32CallHookProcFromKernel,
    User32CallEventProcFromKernel,
    User32CallLoadMenuFromKernel,
    User32CallClientThreadSetupFromKernel,
    User32CallClientLoadLibraryFromKernel,
    User32CallGetCharsetInfo,
    User32CallCopyImageFromKernel,
    User32CallSetWndIconsFromKernel,
};

/*
* @unimplemented
*/
BOOL
WINAPI
ClientThreadSetup(VOID)
{
    //
    // This routine, in Windows, does a lot of what Init does, but in a radically
    // different way.
    //
    // In Windows, because CSRSS's threads have TIF_CSRSSTHREAD set (we have this
    // flag in ROS but not sure if we use it), the xxxClientThreadSetup callback
    // isn't made when CSRSS first loads WINSRV.DLL (which loads USER32.DLL).
    //
    // However, all the other calls are made as normal, and WINSRV.DLL loads
    // USER32.dll, the DllMain runs, and eventually the first NtUser system call is
    // made which initializes Win32k (and initializes the thread, but as mentioned
    // above, the thread is marked as TIF_CSRSSTHREAD.
    //
    // In the DllMain of User32, there is also a CsrClientConnectToServer call to
    // server 2 (winsrv). When this is done from CSRSS, the "InServer" flag is set,
    // so user32 will remember that it's running inside of CSRSS. Also, another
    // flag, called "FirstThread" is manually set by DllMain.
    //
    // Then, WINSRV finishes loading, and CSRSRV starts the API thread/loop. This
    // code then calls CsrConnectToUser, which calls... ClientThreadStartup. Now
    // this routine detects that it's in the server process, which means it's CSRSS
    // and that the callback never happened. It does some first-time-Win32k connection
    // initialization and caches a bunch of things -- if it's the first thread. It also
    // acquires a critical section to initialize GDI -- and then resets the first thread
    // flag.
    //
    // For now, we'll do none of this, but to support Windows' CSRSRV.DLL which calls
    // CsrConnectToUser, we'll pretend we "did something" here. Then the rest will
    // continue as normal.
    //

    GetConnected();

    UNIMPLEMENTED;
    return TRUE;
}

BOOL
Init(VOID)
{
    NTSTATUS Status;
    USERCONNECT UserCon;

    TRACE("user32::Init()\n");

    /* Set PEB data */
    NtCurrentPeb()->KernelCallbackTable = apfnDispatch;
    NtCurrentPeb()->PostProcessInitRoutine = NULL;

    /* Minimal setup of the connect info structure */
    UserCon.ulVersion = USER_VERSION;

    /* Connect to win32k */
    Status = NtUserProcessConnect(NtCurrentProcess(),
                                  &UserCon,
                                  sizeof(UserCon));
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Retrieve data */
    g_ppi = GetWin32ClientInfo()->ppi; // Snapshot PI, used as pointer only!
    g_ulSharedDelta = UserCon.siClient.ulSharedDelta;
    gpsi = SharedPtrToUser(UserCon.siClient.psi);
    gHandleTable = SharedPtrToUser(UserCon.siClient.aheList);
    gHandleEntries = SharedPtrToUser(gHandleTable->handles);

    RtlInitializeCriticalSection(&gcsUserApiHook);

    //ERR("1 SI 0x%x : HT 0x%x : D 0x%x\n", UserCon.siClient.psi, UserCon.siClient.aheList,  g_ulSharedDelta);

    /* Allocate an index for user32 thread local data */
    User32TlsIndex = TlsAlloc();
    if (User32TlsIndex == TLS_OUT_OF_INDEXES)
        return FALSE;

    if (MessageInit())
    {
        if (MenuInit())
        {
            InitializeCriticalSection(&U32AccelCacheLock);
            LoadAppInitDlls();
            return TRUE;
        }
        MessageCleanup();
    }

    TlsFree(User32TlsIndex);
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
        {

#define WIN_OBJ_DIR L"\\Windows"
#define SESSION_DIR L"\\Sessions"

            NTSTATUS Status;
            USERSRV_API_CONNECTINFO ConnectInfo; // USERCONNECT
            ULONG ConnectInfoSize = sizeof(ConnectInfo);
            WCHAR SessionDir[256];

            /* Cache the PEB and Session ID */
            PPEB Peb = NtCurrentPeb();
            ULONG SessionId = Peb->SessionId; // gSessionId

            /* Don't bother us for each thread */
            DisableThreadLibraryCalls(hInstanceDll);

            /* Minimal setup of the connect info structure */
            ConnectInfo.ulVersion = USER_VERSION;

            /* Setup the Object Directory path */
            if (!SessionId)
            {
                /* Use the raw path */
                wcscpy(SessionDir, WIN_OBJ_DIR);
            }
            else
            {
                /* Use the session path */
                swprintf(SessionDir,
                         L"%ws\\%ld%ws",
                         SESSION_DIR,
                         SessionId,
                         WIN_OBJ_DIR);
            }

            /* Connect to the USER Server */
            Status = CsrClientConnectToServer(SessionDir,
                                              USERSRV_SERVERDLL_INDEX,
                                              &ConnectInfo,
                                              &ConnectInfoSize,
                                              &gfServerProcess);
            if (!NT_SUCCESS(Status))
            {
                ERR("Failed to connect to CSR (Status %lx)\n", Status);
                return FALSE;
            }

            User32Instance = hInstanceDll;

            if (!RegisterClientPFN())
                return FALSE;

            if (!Init())
                return FALSE;

            break;
        }

        case DLL_PROCESS_DETACH:
        {
            if (hImmInstance)
                FreeLibrary(hImmInstance);

            Cleanup();
            break;
        }
    }

    /* Finally, initialize GDI */
    return GdiDllInitialize(hInstanceDll, dwReason, reserved);
}

// FIXME: This function IS A BIIG HACK!!
VOID
FASTCALL
GetConnected(VOID)
{
    NTSTATUS Status;
    USERCONNECT UserCon;

    TRACE("user32::GetConnected()\n");

    if ((PTHREADINFO)NtCurrentTeb()->Win32ThreadInfo == NULL)
        NtUserGetThreadState(THREADSTATE_GETTHREADINFO);

    if (gpsi && g_ppi) return;

    /* Minimal setup of the connect info structure */
    UserCon.ulVersion = USER_VERSION;

    /* Connect to win32k */
    Status = NtUserProcessConnect(NtCurrentProcess(),
                                  &UserCon,
                                  sizeof(UserCon));
    if (!NT_SUCCESS(Status)) return;

    /* Retrieve data */
    g_ppi = GetWin32ClientInfo()->ppi; // Snapshot PI, used as pointer only!
    g_ulSharedDelta = UserCon.siClient.ulSharedDelta;
    gpsi = SharedPtrToUser(UserCon.siClient.psi);
    gHandleTable = SharedPtrToUser(UserCon.siClient.aheList);
    gHandleEntries = SharedPtrToUser(gHandleTable->handles);
}

NTSTATUS
WINAPI
User32CallClientThreadSetupFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  TRACE("ClientThreadSetup\n");
  ClientThreadSetup();
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

NTSTATUS
WINAPI
User32CallSetWndIconsFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSETWNDICONS_CALLBACK_ARGUMENTS Common = Arguments;

  if (!hIconSmWindows)
  {
      hIconSmWindows = LoadImageW(0, IDI_WINLOGO, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
      hIconWindows   = LoadImageW(0, IDI_WINLOGO, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
  }
  Common->hIconSmWindows = hIconSmWindows;
  Common->hIconWindows = hIconWindows;
  ERR("hIconSmWindows %p hIconWindows %p \n",hIconSmWindows,hIconWindows);
  return ZwCallbackReturn(Arguments, ArgumentLength, STATUS_SUCCESS);
}
