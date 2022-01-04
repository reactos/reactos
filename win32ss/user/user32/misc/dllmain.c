#include <user32.h>
#include <ndk/cmfuncs.h>
#include <strsafe.h>

#define MAX_USER_MODE_DRV_BUFFER 526

//
// UMPD Packet Header should match win32ss/include/ntumpd.h
//
typedef struct _UMPDPKTHEAD
{
    INT       Size;
    INT       Index;
    INT       RetSize;
    DWORD     Reserved;
    HUMPD     humpd;
    ULONG_PTR Buffer[];
} UMPDPKTHEAD, *PUMPDPKTHEAD;

INT WINAPI GdiPrinterThunk(PUMPDPKTHEAD,PVOID,INT);

WINE_DEFAULT_DEBUG_CHANNEL(user32);

#define KEY_LENGTH 1024

static ULONG User32TlsIndex;
HINSTANCE User32Instance;

PPROCESSINFO g_ppi = NULL;
SHAREDINFO gSharedInfo = {0};
PSERVERINFO gpsi = NULL;
PUSER_HANDLE_TABLE gHandleTable = NULL;
PUSER_HANDLE_ENTRY gHandleEntries = NULL;
BOOLEAN gfLogonProcess  = FALSE;
BOOLEAN gfServerProcess = FALSE;
BOOLEAN gfFirstThread   = TRUE;
HICON hIconSmWindows = NULL, hIconWindows = NULL;

WCHAR szAppInit[KEY_LENGTH];

BOOL
GetDllList(VOID)
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
LoadAppInitDlls(VOID)
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
UnloadAppInitDlls(VOID)
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
    User32DeliverUserAPC,
    User32CallDDEPostFromKernel,
    User32CallDDEGetFromKernel,
    User32CallOBMFromKernel,
    User32CallLPKFromKernel,
    User32CallUMPDFromKernel,
};



VOID
WINAPI
GdiProcessSetup(VOID);

BOOL
WINAPI
ClientThreadSetupHelper(BOOL IsCallback)
{
    /*
     * Normally we are called by win32k so the win32 thread pointers
     * should be valid as they are set in win32k::InitThreadCallback.
     */
    PCLIENTINFO ClientInfo = GetWin32ClientInfo();
    BOOLEAN IsFirstThread = _InterlockedExchange8((PCHAR)&gfFirstThread, FALSE);

    TRACE("In ClientThreadSetup(IsCallback == %s, gfServerProcess = %s, IsFirstThread = %s)\n",
          IsCallback ? "TRUE" : "FALSE", gfServerProcess ? "TRUE" : "FALSE", IsFirstThread ? "TRUE" : "FALSE");

    if (IsFirstThread)
        GdiProcessSetup();

    /* Check for already initialized thread, and bail out if so */
    if (ClientInfo->CI_flags & CI_INITTHREAD)
    {
        ERR("ClientThreadSetup: Thread already initialized.\n");
        return FALSE;
    }

    /*
     * CSRSS couldn't use user32::DllMain CSR server-to-server call to connect
     * to win32k. So it is delayed to a manually-call to ClientThreadSetup.
     * Also this needs to be done only for the first thread (since the connection
     * is per-process).
     */
    if (gfServerProcess && IsFirstThread)
    {
        NTSTATUS Status;
        USERCONNECT UserCon;

        RtlZeroMemory(&UserCon, sizeof(UserCon));

        /* Minimal setup of the connect info structure */
        UserCon.ulVersion = USER_VERSION;
        // UserCon.dwDispatchCount;

        /* Connect to win32k */
        Status = NtUserProcessConnect(NtCurrentProcess(),
                                      &UserCon,
                                      sizeof(UserCon));
        if (!NT_SUCCESS(Status)) return FALSE;

        /* Retrieve data */
        g_ppi = ClientInfo->ppi; // Snapshot PI, used as pointer only!
        gSharedInfo = UserCon.siClient;
        gpsi = gSharedInfo.psi;
        gHandleTable = gSharedInfo.aheList;
        /* ReactOS-Specific! */ gHandleEntries = SharedPtrToUser(gHandleTable->handles);

        // ERR("1 SI 0x%x : HT 0x%x : D 0x%x\n",
        //     gSharedInfo.psi, gSharedInfo.aheList, gSharedInfo.ulSharedDelta);
    }

    TRACE("Checkpoint (register PFN)\n");
    if (!RegisterClientPFN())
    {
        ERR("RegisterClientPFN failed\n");
        return FALSE;
    }

    /* Mark this thread as initialized */
    ClientInfo->CI_flags |= CI_INITTHREAD;

    /* Initialization that should be done once per process */
    if (IsFirstThread)
    {
        TRACE("Checkpoint (Allocating TLS)\n");

        /* Allocate an index for user32 thread local data */
        User32TlsIndex = TlsAlloc();
        if (User32TlsIndex == TLS_OUT_OF_INDEXES)
            return FALSE;

        // HAAAAAAAAAACK!!!!!!
        // ASSERT(gpsi);
        if (!gpsi) ERR("AAAAAAAAAAAHHHHHHHHHHHHHH!!!!!!!! gpsi == NULL !!!!\n");
        if (gpsi)
        {
        TRACE("Checkpoint (MessageInit)\n");

        if (MessageInit())
        {
            TRACE("Checkpoint (MenuInit)\n");
            if (MenuInit())
            {
                TRACE("Checkpoint initialization done OK\n");
                InitializeCriticalSection(&U32AccelCacheLock);
                LoadAppInitDlls();
                return TRUE;
            }
            MessageCleanup();
        }

        TlsFree(User32TlsIndex);
        return FALSE;
        }
    }

    return TRUE;
}

/*
 * @implemented
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
    // above, the thread is marked as TIF_CSRSSTHREAD).
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

    // FIXME: Disabling this call is a HACK!! See also User32CallClientThreadSetupFromKernel...
    // return ClientThreadSetupHelper(FALSE);
    TRACE("ClientThreadSetup is not implemented\n");
    return TRUE;
}

BOOL
Init(PUSERCONNECT UserCon /*PUSERSRV_API_CONNECTINFO*/)
{
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("user32::Init(0x%p) -->\n", UserCon);

    RtlInitializeCriticalSection(&gcsUserApiHook);

    /* Initialize callback table in PEB data */
    NtCurrentPeb()->KernelCallbackTable = apfnDispatch;
    NtCurrentPeb()->PostProcessInitRoutine = NULL;

    // This is a HACK!! //
    gfServerProcess = FALSE;
    gfFirstThread   = TRUE;
    //// End of HACK!! ///

    /*
     * Retrieve data from the connect info structure if the initializing
     * process is not CSRSS. In case it is, this will be done from inside
     * ClientThreadSetup.
     */
    if (!gfServerProcess)
    {
        // FIXME: HACK!! We should fixup for the NtUserProcessConnect fixups
        // because it was made in the context of CSRSS process and not ours!!
        // So... as long as we don't fix that, we need to redo again a call
        // to NtUserProcessConnect... How perverse is that?!
        //
        // HACK(2): This call is necessary since we disabled
        // the CSR call in DllMain...
        {
            RtlZeroMemory(UserCon, sizeof(*UserCon));

            /* Minimal setup of the connect info structure */
            UserCon->ulVersion = USER_VERSION;
            // UserCon->dwDispatchCount;

            TRACE("HACK: Hackish NtUserProcessConnect call!!\n");
            /* Connect to win32k */
            Status = NtUserProcessConnect(NtCurrentProcess(),
                                          UserCon,
                                          sizeof(*UserCon));
            if (!NT_SUCCESS(Status)) return FALSE;
        }

        //
        // We continue as we should do normally...
        //

        /* Retrieve data */
        g_ppi = GetWin32ClientInfo()->ppi; // Snapshot PI, used as pointer only!
        gSharedInfo = UserCon->siClient;
        gpsi = gSharedInfo.psi;
        gHandleTable = gSharedInfo.aheList;
        /* ReactOS-Specific! */ gHandleEntries = SharedPtrToUser(gHandleTable->handles);
    }

    // FIXME: Yet another hack... This call should normally not be done here, but
    // instead in ClientThreadSetup, and in User32CallClientThreadSetupFromKernel as well.
    TRACE("HACK: Using Init-ClientThreadSetupHelper hack!!\n");
    if (!ClientThreadSetupHelper(FALSE))
    {
        TRACE("Init-ClientThreadSetupHelper hack failed!\n");
        return FALSE;
    }

    TRACE("<-- user32::Init()\n");

    return NT_SUCCESS(Status);
}

VOID
Cleanup(VOID)
{
    UnloadAppInitDlls();
    DeleteCriticalSection(&U32AccelCacheLock);
    MenuCleanup();
    MessageCleanup();
    TlsFree(User32TlsIndex);
    DeleteFrameBrushes();
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

            USERSRV_API_CONNECTINFO ConnectInfo; // USERCONNECT

#if 0 // Disabling this code is a BIG HACK!!

            NTSTATUS Status;
            ULONG ConnectInfoSize = sizeof(ConnectInfo);
            WCHAR SessionDir[256];

            /* Cache the PEB and Session ID */
            PPEB Peb = NtCurrentPeb();
            ULONG SessionId = Peb->SessionId; // gSessionId

            TRACE("user32::DllMain\n");

            /* Don't bother us for each thread */
            DisableThreadLibraryCalls(hInstanceDll);

            RtlZeroMemory(&ConnectInfo, sizeof(ConnectInfo));

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

            TRACE("Checkpoint (call CSR)\n");

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

            TRACE("Checkpoint (CSR called)\n");

#endif

            User32Instance = hInstanceDll;

            /* Finish initialization */
            TRACE("Checkpoint (call Init)\n");
            if (!Init(&ConnectInfo))
                return FALSE;

            if (!gfServerProcess)
            {
                HINSTANCE hImm32 = NULL;

                if (gpsi && (gpsi->dwSRVIFlags & SRVINFO_IMM32))
                {
                    WCHAR szImmFile[MAX_PATH];
                    InitializeImmEntryTable();
                    GetImmFileName(szImmFile, _countof(szImmFile));
                    hImm32 = GetModuleHandleW(szImmFile);
                }

                if (!IMM_FN(ImmRegisterClient)(&gSharedInfo, hImm32))
                    return FALSE;
            }
            break;
        }

        case DLL_PROCESS_DETACH:
        {
            if (ghImm32)
                FreeLibrary(ghImm32);

            Cleanup();
            break;
        }
    }

    /* Finally, initialize GDI */
    return GdiDllInitialize(hInstanceDll, dwReason, reserved);
}

NTSTATUS
WINAPI
User32CallClientThreadSetupFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  TRACE("User32CallClientThreadSetupFromKernel -->\n");
  // FIXME: Disabling this call is a HACK!! See also ClientThreadSetup...
  // ClientThreadSetupHelper(TRUE);
  TRACE("<-- User32CallClientThreadSetupFromKernel\n");
  return ZwCallbackReturn(NULL, 0, STATUS_SUCCESS);
}

NTSTATUS
WINAPI
User32CallGetCharsetInfo(PVOID Arguments, ULONG ArgumentLength)
{
  BOOL Ret;
  PGET_CHARSET_INFO pgci = (PGET_CHARSET_INFO)Arguments;

  TRACE("GetCharsetInfo\n");

  Ret = TranslateCharsetInfo((DWORD *)(ULONG_PTR)pgci->Locale, &pgci->Cs, TCI_SRCLOCALE);

  return ZwCallbackReturn(Arguments, ArgumentLength, Ret ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

NTSTATUS
WINAPI
User32CallSetWndIconsFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSETWNDICONS_CALLBACK_ARGUMENTS Common = Arguments;

  if (!gpsi->hIconSmWindows)
  {
      Common->hIconSample    = LoadImageW(0, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
      Common->hIconHand      = LoadImageW(0, IDI_HAND,        IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
      Common->hIconQuestion  = LoadImageW(0, IDI_QUESTION,    IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
      Common->hIconBang      = LoadImageW(0, IDI_EXCLAMATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
      Common->hIconNote      = LoadImageW(0, IDI_ASTERISK,    IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
      Common->hIconWindows   = LoadImageW(0, IDI_WINLOGO,     IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
      Common->hIconSmWindows = LoadImageW(0, IDI_WINLOGO, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
      hIconWindows   = Common->hIconWindows;
      hIconSmWindows = Common->hIconSmWindows;
  }
  ERR("hIconSmWindows %p hIconWindows %p \n",hIconSmWindows,hIconWindows);
  return ZwCallbackReturn(Arguments, ArgumentLength, STATUS_SUCCESS);
}

NTSTATUS
WINAPI
User32DeliverUserAPC(PVOID Arguments, ULONG ArgumentLength)
{
  return ZwCallbackReturn(0, 0, STATUS_SUCCESS);
}

NTSTATUS
WINAPI
User32CallOBMFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  BITMAP bmp;
  PSETOBM_CALLBACK_ARGUMENTS Common = Arguments;

  GetObjectW(LoadBitmapW(0, MAKEINTRESOURCEW(OBM_CLOSE)), sizeof(bmp), &bmp);
  Common->oembmi[OBI_CLOSE].cx = bmp.bmWidth;
  Common->oembmi[OBI_CLOSE].cy = bmp.bmHeight;

  GetObjectW(LoadBitmapW(0, MAKEINTRESOURCEW(OBM_MNARROW)), sizeof(bmp), &bmp);
  Common->oembmi[OBI_MNARROW].cx = bmp.bmWidth;
  Common->oembmi[OBI_MNARROW].cy = bmp.bmHeight;

  GetObjectW(LoadBitmapW(0, MAKEINTRESOURCEW(OBM_DNARROW)), sizeof(bmp), &bmp);
  Common->oembmi[OBI_DNARROW].cx = bmp.bmWidth;
  Common->oembmi[OBI_DNARROW].cy = bmp.bmHeight;

  GetObjectW(LoadBitmapW(0, MAKEINTRESOURCEW(OBM_DNARROWI)), sizeof(bmp), &bmp);
  Common->oembmi[OBI_DNARROWI].cx = bmp.bmWidth;
  Common->oembmi[OBI_DNARROWI].cy = bmp.bmHeight;

  GetObjectW(LoadBitmapW(0, MAKEINTRESOURCEW(OBM_UPARROW)), sizeof(bmp), &bmp);
  Common->oembmi[OBI_UPARROW].cx = bmp.bmWidth;
  Common->oembmi[OBI_UPARROW].cy = bmp.bmHeight;

  GetObjectW(LoadBitmapW(0, MAKEINTRESOURCEW(OBM_UPARROWI)), sizeof(bmp), &bmp);
  Common->oembmi[OBI_UPARROWI].cx = bmp.bmWidth;
  Common->oembmi[OBI_UPARROWI].cy = bmp.bmHeight;

  return ZwCallbackReturn(Arguments, ArgumentLength, STATUS_SUCCESS);
}

NTSTATUS WINAPI User32CallLPKFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
    BOOL bResult;
    PLPK_CALLBACK_ARGUMENTS Argument;

    Argument = (PLPK_CALLBACK_ARGUMENTS)Arguments;

    Argument->lpString = (LPWSTR)((ULONG_PTR)Argument->lpString + (ULONG_PTR)Argument);

    bResult = ExtTextOutW(Argument->hdc,
                          Argument->x,
                          Argument->y,
                          Argument->flags,
                          (Argument->bRect) ? &Argument->rect : NULL,
                          Argument->lpString,
                          Argument->count,
                          NULL);

    return ZwCallbackReturn(&bResult, sizeof(BOOL), STATUS_SUCCESS);
}

NTSTATUS WINAPI User32CallUMPDFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
    DWORD Buffer[MAX_USER_MODE_DRV_BUFFER];
    INT cbSize = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    PUMPDPKTHEAD pkt, pktOut = NULL;

    pkt = (PUMPDPKTHEAD)Arguments;

    if ( pkt->RetSize <= sizeof(Buffer) )
    {
        pktOut = (PUMPDPKTHEAD)Buffer;

        if ( (GdiPrinterThunk( pkt, pktOut, pkt->RetSize ) == GDI_ERROR) )
        {
            pktOut = NULL;
            Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            cbSize = pkt->RetSize;
        }
    }
    else
    {
       Status = STATUS_NO_MEMORY;   
    }
    return ZwCallbackReturn( pktOut, cbSize, Status );
}
