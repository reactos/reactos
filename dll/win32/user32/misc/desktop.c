/* $Id$
 *
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

#define DESKTOP_CLASS_ATOM   MAKEINTATOMA(32769)  /* Desktop */
static LRESULT WINAPI DesktopWndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );

/*********************************************************************
 * desktop class descriptor
 */
const struct builtin_class_descr DESKTOP_builtin_class =
{
  (LPCWSTR) DESKTOP_CLASS_ATOM,   /* name */
  CS_DBLCLKS,           /* style */
  NULL,                 /* procA (winproc is Unicode only) */
  (WNDPROC) DesktopWndProc,       /* procW */
  0,                    /* extra */
  IDC_ARROW,            /* cursor */
  (HBRUSH)(COLOR_BACKGROUND+1)    /* brush */
};

static
LRESULT
WINAPI
DesktopWndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    FIXME("Desktop Class Atom!\n");
    if (message == WM_NCCREATE) return TRUE;
    return 0;  /* all other messages are ignored */
}

VOID
STDCALL
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
STDCALL
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

/*
 * @implemented
 */
int STDCALL
GetSystemMetrics(int nIndex)
{
  return(NtUserGetSystemMetrics(nIndex));
}


/*
 * @unimplemented
 */
BOOL STDCALL SetDeskWallpaper(LPCSTR filename)
{
	return SystemParametersInfoA(SPI_SETDESKWALLPAPER,0,(PVOID)filename,TRUE);
}
/*
 * @implemented
 */
BOOL STDCALL
SystemParametersInfoA(UINT uiAction,
		      UINT uiParam,
		      PVOID pvParam,
		      UINT fWinIni)
{
  switch (uiAction)
    {
      case SPI_GETHIGHCONTRAST:
      case SPI_SETHIGHCONTRAST:
      case SPI_GETSOUNDSENTRY:
      case SPI_SETSOUNDSENTRY:
        {
            /* FIXME: Support this accessibility SPI actions */
            FIXME("FIXME: Unsupported SPI Code: %lx \n",uiAction );
            return FALSE;
        }

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
        HKEY hKey;
        BOOL Ret = FALSE;

#if 0
        /* Get the desktop bitmap handle, this does NOT return the file name! */
        if(!NtUserSystemParametersInfo(SPI_GETDESKWALLPAPER, 0, &hbmWallpaper, 0))
        {
          /* Return an empty string, no wallpapaper is set */
          *(CHAR*)pvParam = '\0';
          return TRUE;
        }
#endif

        /* FIXME - Read the registry key for now, but what happens if the wallpaper was
                   changed without SPIF_UPDATEINIFILE?! */
        if(RegOpenKeyExW(HKEY_CURRENT_USER,
                         L"Control Panel\\Desktop",
                         0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
        {
          DWORD Type, Size;
          Size = uiParam;
          if(RegQueryValueExA(hKey,
                              "Wallpaper",
                              NULL,
                              &Type,
                              (LPBYTE)pvParam,
                              &Size) == ERROR_SUCCESS
             && Type == REG_SZ)
          {
            Ret = TRUE;
          }
          RegCloseKey(hKey);
        }
        return Ret;
      }
      case SPI_SETDESKWALLPAPER:
      {
        HBITMAP hNewWallpaper;
        BOOL Ret;
        LPSTR lpWallpaper = (LPSTR)pvParam;

        if(lpWallpaper != NULL && *lpWallpaper != '\0')
        {
          hNewWallpaper = LoadImageA(0, lpWallpaper, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
          if(hNewWallpaper == NULL)
          {
            return FALSE;
          }
        }
        else
        {
          hNewWallpaper = NULL;
          lpWallpaper = NULL;
        }

        /* Set the wallpaper bitmap */
        if(!NtUserSystemParametersInfo(SPI_SETDESKWALLPAPER, 0, &hNewWallpaper, fWinIni & SPIF_SENDCHANGE))
        {
          if(hNewWallpaper != NULL)
            DeleteObject(hNewWallpaper);
          return FALSE;
        }
        /* Do not use the bitmap handle anymore, it doesn't belong to our process anymore! */

        Ret = TRUE;
        if(fWinIni & SPIF_UPDATEINIFILE)
        {
          /* Save the path to the file in the registry */
          HKEY hKey;
          if(RegOpenKeyExW(HKEY_CURRENT_USER,
                           L"Control Panel\\Desktop",
                           0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
          {
            Ret = RegSetValueExA(hKey, "Wallpaper", 0, REG_SZ, (LPBYTE)(lpWallpaper != NULL ? lpWallpaper : ""),
                                 (lpWallpaper != NULL ? (lstrlenA(lpWallpaper) + 1) * sizeof(CHAR) : sizeof(CHAR)) == ERROR_SUCCESS);
            RegCloseKey(hKey);
          }
        }

        RedrawWindow(GetShellWindow(), NULL, NULL, RDW_INVALIDATE | RDW_ERASE);

        return Ret;
      }
    }
    return NtUserSystemParametersInfo(uiAction, uiParam, pvParam, fWinIni);
}


/*
 * @implemented
 */
BOOL STDCALL
SystemParametersInfoW(UINT uiAction,
		      UINT uiParam,
		      PVOID pvParam,
		      UINT fWinIni)
{
  switch(uiAction)
  {
    case SPI_GETHIGHCONTRAST:
    case SPI_SETHIGHCONTRAST:
    case SPI_GETSOUNDSENTRY:
    case SPI_SETSOUNDSENTRY:
       {
           /* FIXME: Support this accessibility SPI actions */
           FIXME("FIXME: Unsupported SPI Code: %lx \n",uiAction );
           return FALSE;
       }
    case SPI_GETDESKWALLPAPER:
    {
      HKEY hKey;
      BOOL Ret = FALSE;

#if 0
      /* Get the desktop bitmap handle, this does NOT return the file name! */
      if(!NtUserSystemParametersInfo(SPI_GETDESKWALLPAPER, 0, &hbmWallpaper, 0))
      {
        /* Return an empty string, no wallpapaper is set */
        *(WCHAR*)pvParam = L'\0';
        return TRUE;
      }
#endif

      /* FIXME - Read the registry key for now, but what happens if the wallpaper was
                 changed without SPIF_UPDATEINIFILE?! */
      if(RegOpenKeyExW(HKEY_CURRENT_USER,
                       L"Control Panel\\Desktop",
                       0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
      {
        DWORD Type, Size;
        Size = uiParam * sizeof(WCHAR);
        if(RegQueryValueExW(hKey,
                            L"Wallpaper",
                            NULL,
                            &Type,
                            (LPBYTE)pvParam,
                            &Size) == ERROR_SUCCESS
           && Type == REG_SZ)
        {
          Ret = TRUE;
        }
        RegCloseKey(hKey);
      }
      return Ret;
    }
    case SPI_SETDESKWALLPAPER:
    {
      HBITMAP hNewWallpaper;
      BOOL Ret;
      LPWSTR lpWallpaper = (LPWSTR)pvParam;

      if(lpWallpaper != NULL && *lpWallpaper != L'\0')
      {
        hNewWallpaper = LoadImageW(0, lpWallpaper, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

        if(hNewWallpaper == NULL)
        {
          return FALSE;
        }
      }
      else
      {
        hNewWallpaper = NULL;
        lpWallpaper = NULL;
      }

      /* Set the wallpaper bitmap */
      if(!NtUserSystemParametersInfo(SPI_SETDESKWALLPAPER, 0, &hNewWallpaper, fWinIni & SPIF_SENDCHANGE))
      {
        if(hNewWallpaper != NULL)
          DeleteObject(hNewWallpaper);
        return FALSE;
      }
      /* Do not use the bitmap handle anymore, it doesn't belong to our process anymore! */
      Ret = TRUE;
      if(fWinIni & SPIF_UPDATEINIFILE)
      {
        /* Save the path to the file in the registry */
        HKEY hKey;

        if(RegOpenKeyExW(HKEY_CURRENT_USER,
                         L"Control Panel\\Desktop",
                         0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
        {
          Ret = (RegSetValueExW(hKey, L"Wallpaper", 0, REG_SZ, (lpWallpaper != NULL ? (LPBYTE)lpWallpaper : (LPBYTE)L""),
                               (lpWallpaper != NULL ? (lstrlenW(lpWallpaper) + 1) * sizeof(WCHAR) : sizeof(WCHAR))) == ERROR_SUCCESS);
          RegCloseKey(hKey);
        }
      }

      RedrawWindow(GetShellWindow(), NULL, NULL, RDW_INVALIDATE | RDW_ERASE);

      return Ret;
    }
  }
  return NtUserSystemParametersInfo(uiAction, uiParam, pvParam, fWinIni);
}


/*
 * @implemented
 */
BOOL
STDCALL
CloseDesktop(
  HDESK hDesktop)
{
  return NtUserCloseDesktop(hDesktop);
}


/*
 * @implemented
 */
HDESK STDCALL
CreateDesktopA(LPCSTR lpszDesktop,
	       LPCSTR lpszDevice,
	       LPDEVMODEA pDevmode,
	       DWORD dwFlags,
	       ACCESS_MASK dwDesiredAccess,
	       LPSECURITY_ATTRIBUTES lpsa)
{
  ANSI_STRING DesktopNameA;
  UNICODE_STRING DesktopNameU;
  HDESK hDesktop;
  LPDEVMODEW DevmodeW;

  if (lpszDesktop != NULL)
    {
      RtlInitAnsiString(&DesktopNameA, (LPSTR)lpszDesktop);
      RtlAnsiStringToUnicodeString(&DesktopNameU, &DesktopNameA, TRUE);
    }
  else
    {
      RtlInitUnicodeString(&DesktopNameU, NULL);
    }

  DevmodeW = GdiConvertToDevmodeW(pDevmode);

  hDesktop = CreateDesktopW(DesktopNameU.Buffer,
			    NULL,
			    DevmodeW,
			    dwFlags,
			    dwDesiredAccess,
			    lpsa);

  RtlFreeUnicodeString(&DesktopNameU);
  return(hDesktop);
}


/*
 * @implemented
 */
HDESK STDCALL
CreateDesktopW(LPCWSTR lpszDesktop,
	       LPCWSTR lpszDevice,
	       LPDEVMODEW pDevmode,
	       DWORD dwFlags,
	       ACCESS_MASK dwDesiredAccess,
	       LPSECURITY_ATTRIBUTES lpsa)
{
  UNICODE_STRING DesktopName;
  HWINSTA hWinSta;
  HDESK hDesktop;

  hWinSta = NtUserGetProcessWindowStation();

  RtlInitUnicodeString(&DesktopName, lpszDesktop);

  hDesktop = NtUserCreateDesktop(&DesktopName,
				 dwFlags,
				 dwDesiredAccess,
				 lpsa,
				 hWinSta);

  return(hDesktop);
}


/*
 * @implemented
 */
BOOL
STDCALL
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
STDCALL
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
STDCALL
GetThreadDesktop(
  DWORD dwThreadId)
{
  return NtUserGetThreadDesktop(dwThreadId, 0);
}


/*
 * @implemented
 */
HDESK
STDCALL
OpenDesktopA(
  LPSTR lpszDesktop,
  DWORD dwFlags,
  BOOL fInherit,
  ACCESS_MASK dwDesiredAccess)
{
  ANSI_STRING DesktopNameA;
  UNICODE_STRING DesktopNameU;
  HDESK hDesktop;

	if (lpszDesktop != NULL) {
		RtlInitAnsiString(&DesktopNameA, lpszDesktop);
		RtlAnsiStringToUnicodeString(&DesktopNameU, &DesktopNameA, TRUE);
  } else {
    RtlInitUnicodeString(&DesktopNameU, NULL);
  }

  hDesktop = OpenDesktopW(
    DesktopNameU.Buffer,
    dwFlags,
    fInherit,
    dwDesiredAccess);

	RtlFreeUnicodeString(&DesktopNameU);

  return hDesktop;
}


/*
 * @implemented
 */
HDESK
STDCALL
OpenDesktopW(
  LPWSTR lpszDesktop,
  DWORD dwFlags,
  BOOL fInherit,
  ACCESS_MASK dwDesiredAccess)
{
  UNICODE_STRING DesktopName;

  RtlInitUnicodeString(&DesktopName, lpszDesktop);

  return NtUserOpenDesktop(
    &DesktopName,
    dwFlags,
    dwDesiredAccess);
}


/*
 * @implemented
 */
HDESK
STDCALL
OpenInputDesktop(
  DWORD dwFlags,
  BOOL fInherit,
  ACCESS_MASK dwDesiredAccess)
{
  return NtUserOpenInputDesktop(
    dwFlags,
    fInherit,
    dwDesiredAccess);
}


/*
 * @implemented
 */
BOOL
STDCALL
PaintDesktop(
  HDC hdc)
{
  return NtUserPaintDesktop(hdc);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetThreadDesktop(
  HDESK hDesktop)
{
  return NtUserSetThreadDesktop(hDesktop);
}


/*
 * @implemented
 */
BOOL
STDCALL
SwitchDesktop(
  HDESK hDesktop)
{
  return NtUserSwitchDesktop(hDesktop);
}


/*
 * @implemented
 */
BOOL STDCALL
SetShellWindowEx(HWND hwndShell, HWND hwndShellListView)
{
	return NtUserSetShellWindowEx(hwndShell, hwndShellListView);
}


/*
 * @implemented
 */
BOOL STDCALL
SetShellWindow(HWND hwndShell)
{
	return SetShellWindowEx(hwndShell, hwndShell);
}


/*
 * @implemented
 */
HWND STDCALL
GetShellWindow(VOID)
{
	return NtUserGetShellWindow();
}


/* EOF */
