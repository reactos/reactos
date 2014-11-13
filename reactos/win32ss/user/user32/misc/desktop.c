/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/desktop.c
 * PURPOSE:         Desktops
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-06-2001  CSH  Created
 */

#include <user32.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(user32);

/*********************************************************************
 * desktop class descriptor
 */
#if 0 // Kept for referencing.
const struct builtin_class_descr DESKTOP_builtin_class =
{
  WC_DESKTOP,           /* name */
  CS_DBLCLKS,           /* style */
  NULL,                 /* procA (winproc is Unicode only) */
  DesktopWndProc,       /* procW */
  0,                    /* extra */
  IDC_ARROW,            /* cursor */
  (HBRUSH)(COLOR_BACKGROUND+1)    /* brush */
};
#endif

LRESULT
WINAPI
DesktopWndProcW(HWND Wnd,
                UINT Msg,
                WPARAM wParam,
                LPARAM lParam)
{
   TRACE("Desktop W Class Atom! hWnd 0x%x, Msg %d\n", Wnd, Msg);

   switch(Msg)
   {
      case WM_ERASEBKGND:
      case WM_NCCREATE:
      case WM_CREATE:
      case WM_CLOSE:
      case WM_DISPLAYCHANGE:
      case WM_PAINT:
      case WM_SYSCOLORCHANGE:
        {
          LRESULT lResult;
          NtUserMessageCall( Wnd, Msg, wParam, lParam, (ULONG_PTR)&lResult, FNID_DESKTOP, FALSE);
          TRACE("Desktop lResult %d\n", lResult);
          return lResult;
        }

      case WM_PALETTECHANGED:
          if (Wnd == (HWND)wParam) break;
      case WM_QUERYNEWPALETTE:
        {
          HDC hdc = GetWindowDC( Wnd );
          PaintDesktop(hdc);
          ReleaseDC( Wnd, hdc );
          break;
        }

      case WM_SETCURSOR:
          return (LRESULT)SetCursor(LoadCursorW(0, (LPCWSTR)IDC_ARROW));

      default:
          return DefWindowProcW(Wnd, Msg, wParam, lParam);
   }
   return 0;
}

VOID
WINAPI
LogFontA2W(LPLOGFONTW pW, CONST LOGFONTA *pA)
{
#define COPYS(f,len) MultiByteToWideChar ( CP_THREAD_ACP, 0, pA->f, len, pW->f, len )
#define COPYN(f) pW->f = pA->f

  COPYN(lfHeight);
  COPYN(lfWidth);
  COPYN(lfEscapement);
  COPYN(lfOrientation);
  COPYN(lfWeight);
  COPYN(lfItalic);
  COPYN(lfUnderline);
  COPYN(lfStrikeOut);
  COPYN(lfCharSet);
  COPYN(lfOutPrecision);
  COPYN(lfClipPrecision);
  COPYN(lfQuality);
  COPYN(lfPitchAndFamily);
  COPYS(lfFaceName,LF_FACESIZE);

#undef COPYN
#undef COPYS
}

VOID
WINAPI
LogFontW2A(LPLOGFONTA pA, CONST LOGFONTW *pW)
{
#define COPYS(f,len) WideCharToMultiByte ( CP_THREAD_ACP, 0, pW->f, len, pA->f, len, NULL, NULL )
#define COPYN(f) pA->f = pW->f

  COPYN(lfHeight);
  COPYN(lfWidth);
  COPYN(lfEscapement);
  COPYN(lfOrientation);
  COPYN(lfWeight);
  COPYN(lfItalic);
  COPYN(lfUnderline);
  COPYN(lfStrikeOut);
  COPYN(lfCharSet);
  COPYN(lfOutPrecision);
  COPYN(lfClipPrecision);
  COPYN(lfQuality);
  COPYN(lfPitchAndFamily);
  COPYS(lfFaceName,LF_FACESIZE);

#undef COPYN
#undef COPYS
}

int WINAPI
RealGetSystemMetrics(int nIndex)
{
  //GetConnected();
  //FIXME("Global Server Data -> %x\n",gpsi);
  if (nIndex < 0 || nIndex >= SM_CMETRICS) return 0;
  return gpsi->aiSysMet[nIndex];
}

/*
 * @implemented
 */
int WINAPI
GetSystemMetrics(int nIndex)
{
   BOOL Hook;
   int Ret = 0;

   if (!gpsi) // Fixme! Hax! Need Timos delay load support?
   {
      return RealGetSystemMetrics(nIndex);
   }

   LoadUserApiHook();

   Hook = BeginIfHookedUserApiHook();

   /* Bypass SEH and go direct. */
   if (!Hook) return RealGetSystemMetrics(nIndex);

   _SEH2_TRY
   {
      Ret = guah.GetSystemMetrics(nIndex);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
   }
   _SEH2_END;

   EndUserApiHook();

   return Ret;
}

/*
 * @unimplemented
 */
BOOL WINAPI SetDeskWallpaper(LPCSTR filename)
{
	return SystemParametersInfoA(SPI_SETDESKWALLPAPER,0,(PVOID)filename,TRUE);
}

BOOL WINAPI
RealSystemParametersInfoA(UINT uiAction,
		      UINT uiParam,
		      PVOID pvParam,
		      UINT fWinIni)
{
  switch (uiAction)
    {

      case SPI_GETNONCLIENTMETRICS:
        {
           LPNONCLIENTMETRICSA pnclma = (LPNONCLIENTMETRICSA)pvParam;
           NONCLIENTMETRICSW nclmw;
           if(pnclma->cbSize != sizeof(NONCLIENTMETRICSA))
           {
               SetLastError(ERROR_INVALID_PARAMETER);
               return FALSE;
           }
           nclmw.cbSize = sizeof(NONCLIENTMETRICSW);

           if (!SystemParametersInfoW(uiAction, sizeof(NONCLIENTMETRICSW),
                                      &nclmw, fWinIni))
             return FALSE;

           pnclma->iBorderWidth = nclmw.iBorderWidth;
           pnclma->iScrollWidth = nclmw.iScrollWidth;
           pnclma->iScrollHeight = nclmw.iScrollHeight;
           pnclma->iCaptionWidth = nclmw.iCaptionWidth;
           pnclma->iCaptionHeight = nclmw.iCaptionHeight;
           pnclma->iSmCaptionWidth = nclmw.iSmCaptionWidth;
           pnclma->iSmCaptionHeight = nclmw.iSmCaptionHeight;
           pnclma->iMenuWidth = nclmw.iMenuWidth;
           pnclma->iMenuHeight = nclmw.iMenuHeight;
           LogFontW2A(&(pnclma->lfCaptionFont), &(nclmw.lfCaptionFont));
           LogFontW2A(&(pnclma->lfSmCaptionFont), &(nclmw.lfSmCaptionFont));
           LogFontW2A(&(pnclma->lfMenuFont), &(nclmw.lfMenuFont));
           LogFontW2A(&(pnclma->lfStatusFont), &(nclmw.lfStatusFont));
           LogFontW2A(&(pnclma->lfMessageFont), &(nclmw.lfMessageFont));
           return TRUE;
        }
      case SPI_SETNONCLIENTMETRICS:
        {
           LPNONCLIENTMETRICSA pnclma = (LPNONCLIENTMETRICSA)pvParam;
           NONCLIENTMETRICSW nclmw;
           if(pnclma->cbSize != sizeof(NONCLIENTMETRICSA))
           {
               SetLastError(ERROR_INVALID_PARAMETER);
               return FALSE;
           }
           nclmw.cbSize = sizeof(NONCLIENTMETRICSW);
           nclmw.iBorderWidth = pnclma->iBorderWidth;
           nclmw.iScrollWidth = pnclma->iScrollWidth;
           nclmw.iScrollHeight = pnclma->iScrollHeight;
           nclmw.iCaptionWidth = pnclma->iCaptionWidth;
           nclmw.iCaptionHeight = pnclma->iCaptionHeight;
           nclmw.iSmCaptionWidth = pnclma->iSmCaptionWidth;
           nclmw.iSmCaptionHeight = pnclma->iSmCaptionHeight;
           nclmw.iMenuWidth = pnclma->iMenuWidth;
           nclmw.iMenuHeight = pnclma->iMenuHeight;
           LogFontA2W(&(nclmw.lfCaptionFont), &(pnclma->lfCaptionFont));
           LogFontA2W(&(nclmw.lfSmCaptionFont), &(pnclma->lfSmCaptionFont));
           LogFontA2W(&(nclmw.lfMenuFont), &(pnclma->lfMenuFont));
           LogFontA2W(&(nclmw.lfStatusFont), &(pnclma->lfStatusFont));
           LogFontA2W(&(nclmw.lfMessageFont), &(pnclma->lfMessageFont));

           return SystemParametersInfoW(uiAction, sizeof(NONCLIENTMETRICSW),
                                        &nclmw, fWinIni);
        }
      case SPI_GETICONMETRICS:
          {
              LPICONMETRICSA picma = (LPICONMETRICSA)pvParam;
              ICONMETRICSW icmw;
              if(picma->cbSize != sizeof(ICONMETRICSA))
              {
                  SetLastError(ERROR_INVALID_PARAMETER);
                  return FALSE;
              }
              icmw.cbSize = sizeof(ICONMETRICSW);
              if (!SystemParametersInfoW(uiAction, sizeof(ICONMETRICSW),
                                        &icmw, fWinIni))
                  return FALSE;

              picma->iHorzSpacing = icmw.iHorzSpacing;
              picma->iVertSpacing = icmw.iVertSpacing;
              picma->iTitleWrap = icmw.iTitleWrap;
              LogFontW2A(&(picma->lfFont), &(icmw.lfFont));
              return TRUE;
          }
      case SPI_SETICONMETRICS:
          {
              LPICONMETRICSA picma = (LPICONMETRICSA)pvParam;
              ICONMETRICSW icmw;
              if(picma->cbSize != sizeof(ICONMETRICSA))
              {
                  SetLastError(ERROR_INVALID_PARAMETER);
                  return FALSE;
              }
              icmw.cbSize = sizeof(ICONMETRICSW);
              icmw.iHorzSpacing = picma->iHorzSpacing;
              icmw.iVertSpacing = picma->iVertSpacing;
              icmw.iTitleWrap = picma->iTitleWrap;
              LogFontA2W(&(icmw.lfFont), &(picma->lfFont));

              return SystemParametersInfoW(uiAction, sizeof(ICONMETRICSW),
                                           &icmw, fWinIni);
          }
      case SPI_GETICONTITLELOGFONT:
        {
           LOGFONTW lfw;
           if (!SystemParametersInfoW(uiAction, 0, &lfw, fWinIni))
             return FALSE;
           LogFontW2A(pvParam, &lfw);
           return TRUE;
        }
      case SPI_SETICONTITLELOGFONT:
          {
              LPLOGFONTA plfa = (LPLOGFONTA)pvParam;
              LOGFONTW lfw;
              LogFontA2W(&lfw,plfa);
              return SystemParametersInfoW(uiAction, 0, &lfw, fWinIni);
          }
      case SPI_GETDESKWALLPAPER:
      {
        BOOL Ret;
        WCHAR awc[MAX_PATH];
        UNICODE_STRING ustrWallpaper;
        ANSI_STRING astrWallpaper;

        Ret = NtUserSystemParametersInfo(SPI_GETDESKWALLPAPER, MAX_PATH, awc, fWinIni);
        RtlInitUnicodeString(&ustrWallpaper, awc);
        RtlUnicodeStringToAnsiString(&astrWallpaper, &ustrWallpaper, TRUE);

        RtlCopyMemory(pvParam, astrWallpaper.Buffer, uiParam);
        RtlFreeAnsiString(&astrWallpaper);
        return Ret;
      }

      case SPI_SETDESKWALLPAPER:
      {
          UNICODE_STRING ustrWallpaper;
          BOOL Ret;

          if (pvParam)
          {
            if (!RtlCreateUnicodeStringFromAsciiz(&ustrWallpaper, pvParam))
            {
                ERR("RtlCreateUnicodeStringFromAsciiz failed\n");
                return FALSE;
            }
            pvParam = &ustrWallpaper;
          }

          Ret = NtUserSystemParametersInfo(SPI_SETDESKWALLPAPER, uiParam, pvParam, fWinIni);

          if (pvParam)
            RtlFreeUnicodeString(&ustrWallpaper);

          return Ret;
      }
    }
    return NtUserSystemParametersInfo(uiAction, uiParam, pvParam, fWinIni);
}

BOOL WINAPI
RealSystemParametersInfoW(UINT uiAction,
		      UINT uiParam,
		      PVOID pvParam,
		      UINT fWinIni)
{
  switch(uiAction)
  {

    case SPI_SETDESKWALLPAPER:
      {
          UNICODE_STRING ustrWallpaper;

          RtlInitUnicodeString(&ustrWallpaper, pvParam);
          return NtUserSystemParametersInfo(SPI_SETDESKWALLPAPER, uiParam, &ustrWallpaper, fWinIni);
      }
  }
  return NtUserSystemParametersInfo(uiAction, uiParam, pvParam, fWinIni);
}


/*
 * @implemented
 */
BOOL WINAPI
SystemParametersInfoA(UINT uiAction,
		      UINT uiParam,
		      PVOID pvParam,
		      UINT fWinIni)
{
   BOOL Hook, Ret = FALSE;

   LoadUserApiHook();

   Hook = BeginIfHookedUserApiHook();

   /* Bypass SEH and go direct. */
   if (!Hook) return RealSystemParametersInfoA(uiAction, uiParam, pvParam, fWinIni);

   _SEH2_TRY
   {
      Ret = guah.SystemParametersInfoA(uiAction, uiParam, pvParam, fWinIni);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
   }
   _SEH2_END;

   EndUserApiHook();

   return Ret;
}

/*
 * @implemented
 */
BOOL WINAPI
SystemParametersInfoW(UINT uiAction,
		      UINT uiParam,
		      PVOID pvParam,
		      UINT fWinIni)
{
   BOOL Hook, Ret = FALSE;

   LoadUserApiHook();

   Hook = BeginIfHookedUserApiHook();

   /* Bypass SEH and go direct. */
   if (!Hook) return RealSystemParametersInfoW(uiAction, uiParam, pvParam, fWinIni);

   _SEH2_TRY
   {
      Ret = guah.SystemParametersInfoW(uiAction, uiParam, pvParam, fWinIni);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
   }
   _SEH2_END;

   EndUserApiHook();

   return Ret;
}

/*
 * @implemented
 */
HDESK WINAPI
CreateDesktopA(LPCSTR lpszDesktop,
	       LPCSTR lpszDevice,
	       LPDEVMODEA pDevmode,
	       DWORD dwFlags,
	       ACCESS_MASK dwDesiredAccess,
	       LPSECURITY_ATTRIBUTES lpsa)
{
    UNICODE_STRING DesktopNameU;
    HDESK hDesktop;
    LPDEVMODEW DevmodeW = NULL;

    if (lpszDesktop)
    {
        /* After conversion, the buffer is zero-terminated */
        RtlCreateUnicodeStringFromAsciiz(&DesktopNameU, lpszDesktop);
    }
    else
    {
        RtlInitUnicodeString(&DesktopNameU, NULL);
    }

    if (pDevmode)
        DevmodeW = GdiConvertToDevmodeW(pDevmode);

    hDesktop = CreateDesktopW(DesktopNameU.Buffer,
                              NULL,
                              DevmodeW,
                              dwFlags,
                              dwDesiredAccess,
                              lpsa);

    /* Free the string, if it was allocated */
    if (lpszDesktop) RtlFreeUnicodeString(&DesktopNameU);

    return hDesktop;
}


/*
 * @implemented
 */
HDESK WINAPI
CreateDesktopW(LPCWSTR lpszDesktop,
	       LPCWSTR lpszDevice,
	       LPDEVMODEW pDevmode,
	       DWORD dwFlags,
	       ACCESS_MASK dwDesiredAccess,
	       LPSECURITY_ATTRIBUTES lpsa)
{
  OBJECT_ATTRIBUTES oas;
  UNICODE_STRING DesktopName, DesktopDevice;
  HWINSTA hWinSta;
  HDESK hDesktop;
  ULONG Attributes = (OBJ_OPENIF|OBJ_CASE_INSENSITIVE);

  /* Retrive WinStation handle. */
  hWinSta = NtUserGetProcessWindowStation();

  /* Initialize the strings. */
  RtlInitUnicodeString(&DesktopName, lpszDesktop);
  RtlInitUnicodeString(&DesktopDevice, lpszDevice);

  /* Check for process is inherited, set flag if set. */
  if (lpsa && lpsa->bInheritHandle) Attributes |= OBJ_INHERIT;

  /* Initialize the attributes for the desktop. */
  InitializeObjectAttributes( &oas,
                              &DesktopName,
                               Attributes,
                               hWinSta,
                               lpsa ? lpsa->lpSecurityDescriptor : NULL);

  /* Send the request and call to win32k. */
  hDesktop = NtUserCreateDesktop( &oas,
                                  &DesktopDevice,
                                   pDevmode,
				   dwFlags,
				   dwDesiredAccess);

  return(hDesktop);
}


/*
 * @implemented
 */
BOOL
WINAPI
EnumDesktopsA(
  HWINSTA WindowStation,
  DESKTOPENUMPROCA EnumFunc,
  LPARAM Context)
{
   return EnumNamesA(WindowStation, EnumFunc, Context, TRUE);
}


/*
 * @implemented
 */
BOOL
WINAPI
EnumDesktopsW(
  HWINSTA WindowStation,
  DESKTOPENUMPROCW EnumFunc,
  LPARAM Context)
{
   return EnumNamesW(WindowStation, EnumFunc, Context, TRUE);
}


/*
 * @implemented
 */
HDESK
WINAPI
GetThreadDesktop(
  DWORD dwThreadId)
{
  return NtUserGetThreadDesktop(dwThreadId, 0);
}


/*
 * @implemented
 */
HDESK
WINAPI
OpenDesktopA(
  LPCSTR lpszDesktop,
  DWORD dwFlags,
  BOOL fInherit,
  ACCESS_MASK dwDesiredAccess)
{
    UNICODE_STRING DesktopNameU;
    HDESK hDesktop;

    if (lpszDesktop)
    {
        /* After conversion, the buffer is zero-terminated */
        RtlCreateUnicodeStringFromAsciiz(&DesktopNameU, lpszDesktop);
    }
    else
    {
        RtlInitUnicodeString(&DesktopNameU, NULL);
    }

    hDesktop = OpenDesktopW(DesktopNameU.Buffer,
                            dwFlags,
                            fInherit,
                            dwDesiredAccess);

    /* Free the string, if it was allocated */
    if (lpszDesktop) RtlFreeUnicodeString(&DesktopNameU);

    return hDesktop;
}


/*
 * @implemented
 */
HDESK
WINAPI
OpenDesktopW(
  LPCWSTR lpszDesktop,
  DWORD dwFlags,
  BOOL fInherit,
  ACCESS_MASK dwDesiredAccess)
{
  UNICODE_STRING DesktopName;
  OBJECT_ATTRIBUTES ObjectAttributes;

  RtlInitUnicodeString(&DesktopName, lpszDesktop);

  InitializeObjectAttributes(&ObjectAttributes,
                             &DesktopName,
                             OBJ_CASE_INSENSITIVE,
                             GetProcessWindowStation(),
                             0);

  if( fInherit )
  {
      ObjectAttributes.Attributes |= OBJ_INHERIT;
  }

  return NtUserOpenDesktop(&ObjectAttributes, dwFlags, dwDesiredAccess);
}


/*
 * @implemented
 */
BOOL WINAPI
SetShellWindow(HWND hwndShell)
{
	return SetShellWindowEx(hwndShell, hwndShell);
}


/*
 * @implemented
 */
HWND WINAPI
GetShellWindow(VOID)
{
   PDESKTOPINFO pdi;
   pdi = GetThreadDesktopInfo();
   if (pdi) return pdi->hShellWindow;
   return NULL;
}


/* EOF */
