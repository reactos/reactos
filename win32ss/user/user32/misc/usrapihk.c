/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/misc/usrapihk.c
 * PURPOSE:         User32.dll User32 Api Hook interface and support functions
 * PROGRAMMER:
 *
 *
 *
 * Information:
 *   http://www.reactos.org/wiki/RegisterUserApiHook
 *
 */

#include <user32.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

BOOL WINAPI RealAdjustWindowRectEx(LPRECT,DWORD,BOOL,DWORD);
LRESULT WINAPI RealDefWindowProcA(HWND,UINT,WPARAM,LPARAM);
LRESULT WINAPI RealDefWindowProcW(HWND,UINT,WPARAM,LPARAM); 
BOOL WINAPI RealDrawFrameControl(HDC,LPRECT,UINT,UINT);
BOOL WINAPI RealGetScrollInfo(HWND,INT,LPSCROLLINFO);
int WINAPI RealGetSystemMetrics(int);
BOOL WINAPI RealMDIRedrawFrame(HWND,DWORD);
INT WINAPI RealSetScrollInfo(HWND,int,LPCSCROLLINFO,BOOL);
BOOL WINAPI RealSystemParametersInfoA(UINT,UINT,PVOID,UINT);
BOOL WINAPI RealSystemParametersInfoW(UINT,UINT,PVOID,UINT);
DWORD WINAPI GetRealWindowOwner(HWND);
BOOL WINAPI RealUserDrawCaption(HWND hWnd, HDC hDC, LPCRECT lpRc, UINT uFlags);

/* GLOBALS *******************************************************************/

DWORD gcLoadUserApiHook = 0;
LONG gcCallUserApiHook = 0;
DWORD gfUserApiHook = 0;
HINSTANCE ghmodUserApiHook = NULL;
USERAPIHOOKPROC gpfnInitUserApi;
RTL_CRITICAL_SECTION gcsUserApiHook;
// API Hooked Message group bitmaps
BYTE grgbDwpLiteHookMsg[128];
BYTE grgbWndLiteHookMsg[128];
BYTE grgbDlgLiteHookMsg[128];

/* INTERNAL ******************************************************************/

/*
   Pre and Post Message handler stub.
 */
LRESULT
WINAPI
DefaultOWP(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, ULONG_PTR lResult, PDWORD pData)
{
  return 0;
}

/*
   Check for API Hooked Message Overrides. Using message bitmapping.. One bit
   corresponds to one message number.
 */
BOOL
FASTCALL
IsMsgOverride( UINT Msg, PUAHOWP puaowpOverride)
{
  UINT nMsg = Msg / 8; // Group Indexed, (Msg 1024) / 8 = (0 -> 127) bytes Max

  if ( puaowpOverride && nMsg < puaowpOverride->Size )
  {
     return (puaowpOverride->MsgBitArray[nMsg] & (1 << (Msg & WM_SETFOCUS)));
  }
  return FALSE;
}

VOID
FASTCALL
CopyMsgMask(PUAHOWP Dest, PUAHOWP Src, PVOID hkmsg, DWORD Size)
{
  DWORD nSize;

  if ( Src && Src->Size > 0 )
  {
     Dest->MsgBitArray = hkmsg;
     nSize = Src->Size;
     if ( Size < nSize) nSize = Size;
     Dest->Size = nSize;
     RtlCopyMemory(Dest->MsgBitArray, Src->MsgBitArray, nSize);
     return;
  }

  Dest->MsgBitArray = NULL;
  Dest->Size = 0;
  return;
}


BOOL
FASTCALL
IsInsideUserApiHook(VOID)
{
  if ( ghmodUserApiHook && gfUserApiHook ) return TRUE;
  return FALSE;
}

BOOL
FASTCALL
BeginIfHookedUserApiHook(VOID)
{
  InterlockedIncrement(&gcCallUserApiHook);
  if (IsInsideUserApiHook()) return TRUE;

  InterlockedDecrement(&gcCallUserApiHook);
  return FALSE;
}

BOOL
WINAPI
ForceResetUserApiHook(HINSTANCE hInstance)
{
  if ( ghmodUserApiHook == hInstance &&
       RtlIsThreadWithinLoaderCallout() )
  {
     ResetUserApiHook(&guah);
     gpfnInitUserApi = NULL;
     return TRUE;
  }
  return FALSE;
}

VOID
FASTCALL
ResetUserApiHook(PUSERAPIHOOK puah)
{
  // Setup Structure.....
  puah->size = sizeof(USERAPIHOOK);
  puah->DefWindowProcA = RealDefWindowProcA;
  puah->DefWindowProcW = RealDefWindowProcW;
  puah->DefWndProcArray.MsgBitArray = NULL;
  puah->DefWndProcArray.Size = 0;
  puah->GetScrollInfo = RealGetScrollInfo;
  puah->SetScrollInfo = RealSetScrollInfo;
  puah->EnableScrollBar = NtUserEnableScrollBar;
  puah->AdjustWindowRectEx = RealAdjustWindowRectEx;
  puah->SetWindowRgn = NtUserSetWindowRgn;
  puah->PreWndProc = DefaultOWP;
  puah->PostWndProc = DefaultOWP;
  puah->WndProcArray.MsgBitArray = NULL;
  puah->WndProcArray.Size = 0;
  puah->PreDefDlgProc = DefaultOWP;
  puah->PostDefDlgProc = DefaultOWP;
  puah->DlgProcArray.MsgBitArray = NULL;
  puah->DlgProcArray.Size = 0;
  puah->GetSystemMetrics = RealGetSystemMetrics;
  puah->SystemParametersInfoA = RealSystemParametersInfoA;
  puah->SystemParametersInfoW = RealSystemParametersInfoW;
  puah->ForceResetUserApiHook = ForceResetUserApiHook;
  puah->DrawFrameControl = RealDrawFrameControl;
  puah->DrawCaption = RealUserDrawCaption;
  puah->MDIRedrawFrame = RealMDIRedrawFrame;
  puah->GetRealWindowOwner = GetRealWindowOwner;
}

BOOL
FASTCALL
EndUserApiHook(VOID)
{
  HMODULE hModule;
  USERAPIHOOKPROC pfn;  
  BOOL Ret = FALSE;

  if ( !InterlockedDecrement(&gcCallUserApiHook) )
  {
     if ( !gcLoadUserApiHook )
     {
        RtlEnterCriticalSection(&gcsUserApiHook);

        pfn = gpfnInitUserApi;
        hModule = ghmodUserApiHook;
        ghmodUserApiHook = NULL;
        gpfnInitUserApi = NULL;

        RtlLeaveCriticalSection(&gcsUserApiHook);

        if ( pfn ) Ret = pfn(uahStop, 0);

        if ( hModule ) Ret = FreeLibrary(hModule);
     }
  }
  return Ret;
}

BOOL
WINAPI
ClearUserApiHook(HINSTANCE hInstance)
{
  HMODULE hModule;
  USERAPIHOOKPROC pfn = NULL, pfn1 = NULL;

  RtlEnterCriticalSection(&gcsUserApiHook);
  hModule = ghmodUserApiHook;
  if ( ghmodUserApiHook == hInstance )
  {
     pfn1 = gpfnInitUserApi;
     if ( --gcLoadUserApiHook == 0 )
     {
        gfUserApiHook = 0;
        ResetUserApiHook(&guah);
        if ( gcCallUserApiHook )
        {
           hInstance = NULL;
           pfn1 = NULL;
           pfn = gpfnInitUserApi;
           gcLoadUserApiHook = 1;
        }
        else
        {
           hInstance = hModule;
           ghmodUserApiHook = NULL;
           gpfnInitUserApi = NULL;
        }
     }
  }
  RtlLeaveCriticalSection(&gcsUserApiHook);

  if ( pfn )
  {
     pfn(uahShutdown, 0); // Shutdown.

     RtlEnterCriticalSection(&gcsUserApiHook);
     pfn1 = gpfnInitUserApi;

     if ( --gcLoadUserApiHook == 0 )
     {
        if ( gcCallUserApiHook )
        {
           hInstance = NULL;
           pfn1 = NULL;
        }
        else
        {
           hInstance = ghmodUserApiHook;
           ghmodUserApiHook = NULL;
           gpfnInitUserApi = NULL;
        }
     }
     RtlLeaveCriticalSection(&gcsUserApiHook);
  }

  if ( pfn1 ) pfn1(uahStop, 0);

  return hInstance != 0;
}

BOOL
WINAPI
InitUserApiHook(HINSTANCE hInstance, USERAPIHOOKPROC pfn)
{
  USERAPIHOOK uah;

  ResetUserApiHook(&uah);

  RtlEnterCriticalSection(&gcsUserApiHook);

  if (!pfn(uahLoadInit,&uah) ||  // Swap data, User32 to and Uxtheme from!
       uah.ForceResetUserApiHook != ForceResetUserApiHook ||
       uah.size <= 0 )
  {
     RtlLeaveCriticalSection(&gcsUserApiHook);
     return FALSE;
  }

  if ( ghmodUserApiHook )
  {
     if ( ghmodUserApiHook != hInstance )
     {
        RtlLeaveCriticalSection(&gcsUserApiHook);
        pfn(uahStop, 0);
        return FALSE;
     }
     gcLoadUserApiHook++;
  }
  else
  {
     ghmodUserApiHook = hInstance;
     // Do not over write GetRealWindowOwner.
     RtlCopyMemory(&guah, &uah, sizeof(USERAPIHOOK) - sizeof(LONG));
     gpfnInitUserApi = pfn;
     gcLoadUserApiHook = 1;
     gfUserApiHook = 1;
     // Copy Message Masks
     CopyMsgMask(&guah.DefWndProcArray,
                 &uah.DefWndProcArray,
                 &grgbDwpLiteHookMsg,
                  sizeof(grgbDwpLiteHookMsg));

     CopyMsgMask(&guah.WndProcArray,
                 &uah.WndProcArray,
                 &grgbWndLiteHookMsg,
                  sizeof(grgbWndLiteHookMsg));

     CopyMsgMask(&guah.DlgProcArray,
                 &uah.DlgProcArray,
                 &grgbDlgLiteHookMsg,
                  sizeof(grgbDlgLiteHookMsg));
  }
  RtlLeaveCriticalSection(&gcsUserApiHook);
  return TRUE;
}

BOOL
WINAPI
RealMDIRedrawFrame(HWND hWnd, DWORD flags)
{
  return NtUserxMDIRedrawFrame(hWnd);
}

BOOL
WINAPI
MDIRedrawFrame(HWND hWnd, DWORD flags)
{
   BOOL Hook, Ret = FALSE;

   LoadUserApiHook();

   Hook = BeginIfHookedUserApiHook();

   /* Bypass SEH and go direct. */
   if (!Hook) return RealMDIRedrawFrame(hWnd, flags);

   _SEH2_TRY
   {
      Ret = guah.MDIRedrawFrame(hWnd, flags);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       ERR("Got exception in hooked MDIRedrawFrame!\n");
   }
   _SEH2_END;

   EndUserApiHook();

   return Ret;
}

USERAPIHOOK guah =
{
  sizeof(USERAPIHOOK),
  RealDefWindowProcA,
  RealDefWindowProcW,
  {NULL, 0},
  RealGetScrollInfo,
  RealSetScrollInfo,
  NtUserEnableScrollBar,
  RealAdjustWindowRectEx,
  NtUserSetWindowRgn,
  DefaultOWP,
  DefaultOWP,
  {NULL, 0},
  DefaultOWP,
  DefaultOWP,
  {NULL, 0},
  RealGetSystemMetrics,
  RealSystemParametersInfoA,
  RealSystemParametersInfoW,
  ForceResetUserApiHook,
  RealDrawFrameControl,
  NtUserDrawCaption,
  RealMDIRedrawFrame,
  GetRealWindowOwner,
};

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL WINAPI RegisterUserApiHook(PUSERAPIHOOKINFO puah)
{
  UNICODE_STRING m_dllname1;
  UNICODE_STRING m_funname1;

  if (puah->m_size == sizeof(USERAPIHOOKINFO))
  {
     WARN("RegisterUserApiHook: %S and %S\n", puah->m_dllname1, puah->m_funname1);
     RtlInitUnicodeString(&m_dllname1, puah->m_dllname1);
     RtlInitUnicodeString(&m_funname1, puah->m_funname1);
     return NtUserRegisterUserApiHook( &m_dllname1, &m_funname1, 0, 0);
  }
  return FALSE;
}

/*
 * @implemented
 */
BOOL WINAPI UnregisterUserApiHook(VOID)
{
    return NtUserUnregisterUserApiHook();
}
