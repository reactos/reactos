/*
 * PROJECT:         ReactOS user32.dll
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            dll/win32/user32/windows/class.c
 * PURPOSE:         Window classes
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

#include <user32.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(user32);

/*
 * @implemented
 */
BOOL
WINAPI
GetClassInfoExA(
  HINSTANCE hInstance,
  LPCSTR lpszClass,
  LPWNDCLASSEXA lpwcx)
{
    UNICODE_STRING ClassName = {0};
    BOOL Ret;
    LPCSTR pszMenuName;

    TRACE("%p class/atom: %s/%04x %p\n", hInstance,
        IS_ATOM(lpszClass) ? NULL : lpszClass,
        IS_ATOM(lpszClass) ? lpszClass : 0,
        lpwcx);

    //HACKHACK: This is ROS-specific and should go away
    lpwcx->cbSize = sizeof(*lpwcx);

    if (hInstance == User32Instance)
    {
        hInstance = NULL;
    }

    if (lpszClass == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!RegisterDefaultClasses)
    {
        ERR("GetClassInfoExA RegisterSystemControls\n");
        RegisterSystemControls();
    }

    if (IS_ATOM(lpszClass))
    {
        ClassName.Buffer = (PWSTR)((ULONG_PTR)lpszClass);
    }
    else
    {
        if (!RtlCreateUnicodeStringFromAsciiz(&ClassName,
                                              lpszClass))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }

    Ret = NtUserGetClassInfo(hInstance,
                             &ClassName,
                             (LPWNDCLASSEXW)lpwcx,
                             (LPWSTR *)&pszMenuName,
                             TRUE);
    if (Ret)
    {
       lpwcx->lpszClassName = lpszClass;
//       lpwcx->lpszMenuName  = pszMenuName;
    }

    if (!IS_ATOM(lpszClass))
    {
        RtlFreeUnicodeString(&ClassName);
    }

    return Ret;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetClassInfoExW(
  HINSTANCE hInstance,
  LPCWSTR lpszClass,
  LPWNDCLASSEXW lpwcx)
{
    UNICODE_STRING ClassName = {0};
    BOOL Ret;
    LPWSTR pszMenuName;

    TRACE("%p class/atom: %S/%04x %p\n", hInstance,
        IS_ATOM(lpszClass) ? NULL : lpszClass,
        IS_ATOM(lpszClass) ? lpszClass : 0,
        lpwcx);

    //HACKHACK: This is ROS-specific and should go away
    lpwcx->cbSize = sizeof(*lpwcx);

    if (hInstance == User32Instance)
    {
        hInstance = NULL;
    }

    if (lpszClass == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!RegisterDefaultClasses)
    {
       ERR("GetClassInfoExW RegisterSystemControls\n");
       RegisterSystemControls();
    }

    if (IS_ATOM(lpszClass))
    {
        ClassName.Buffer = (PWSTR)((ULONG_PTR)lpszClass);
    }
    else
    {
        RtlInitUnicodeString(&ClassName,
                             lpszClass);
    }

    Ret = NtUserGetClassInfo( hInstance,
                              &ClassName,
                              lpwcx,
                              &pszMenuName,
                              FALSE);
    if (Ret)
    {
       lpwcx->lpszClassName = lpszClass;
//       lpwcx->lpszMenuName  = pszMenuName;
    }
    return Ret;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetClassInfoA(
  HINSTANCE hInstance,
  LPCSTR lpClassName,
  LPWNDCLASSA lpWndClass)
{
    WNDCLASSEXA wcex;
    BOOL retval;

    retval = GetClassInfoExA(hInstance, lpClassName, &wcex);
    if (retval)
    {
        lpWndClass->style         = wcex.style;
        lpWndClass->lpfnWndProc   = wcex.lpfnWndProc;
        lpWndClass->cbClsExtra    = wcex.cbClsExtra;
        lpWndClass->cbWndExtra    = wcex.cbWndExtra;
        lpWndClass->hInstance     = wcex.hInstance;
        lpWndClass->hIcon         = wcex.hIcon;
        lpWndClass->hCursor       = wcex.hCursor;
        lpWndClass->hbrBackground = wcex.hbrBackground;
        lpWndClass->lpszMenuName  = wcex.lpszMenuName;
        lpWndClass->lpszClassName = wcex.lpszClassName;
    }

    return retval;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetClassInfoW(
  HINSTANCE hInstance,
  LPCWSTR lpClassName,
  LPWNDCLASSW lpWndClass)
{
    WNDCLASSEXW wcex;
    BOOL retval;

    retval = GetClassInfoExW(hInstance, lpClassName, &wcex);
    if (retval)
    {
        lpWndClass->style         = wcex.style;
        lpWndClass->lpfnWndProc   = wcex.lpfnWndProc;
        lpWndClass->cbClsExtra    = wcex.cbClsExtra;
        lpWndClass->cbWndExtra    = wcex.cbWndExtra;
        lpWndClass->hInstance     = wcex.hInstance;
        lpWndClass->hIcon         = wcex.hIcon;
        lpWndClass->hCursor       = wcex.hCursor;
        lpWndClass->hbrBackground = wcex.hbrBackground;
        lpWndClass->lpszMenuName  = wcex.lpszMenuName;
        lpWndClass->lpszClassName = wcex.lpszClassName;
    }
    return retval;
}

//
// Based on find_winproc... Fixes many whine tests......
//
ULONG_PTR FASTCALL
IntGetClsWndProc(PWND pWnd, PCLS Class, BOOL Ansi)
{
  INT i;
  ULONG_PTR gcpd, Ret = 0;
 // If server side, sweep through proc list and return the client side proc.
  if (Class->CSF_flags & CSF_SERVERSIDEPROC)
  {  // Always scan through the list due to wine class "deftest".
     for ( i = FNID_FIRST; i <= FNID_SWITCH; i++)
     {
         if (GETPFNSERVER(i) == Class->lpfnWndProc)
         {
            if (Ansi)
               Ret = (ULONG_PTR)GETPFNCLIENTA(i);
            else
               Ret = (ULONG_PTR)GETPFNCLIENTW(i);
         }
     }
     return Ret;
  }
  // Set return proc.
  Ret = (ULONG_PTR)Class->lpfnWndProc;
  // Return the proc if one of the FnId default class type.
  if (Class->fnid <= FNID_GHOST && Class->fnid >= FNID_BUTTON)
  {
     if (Ansi)
     { // If match return the right proc by type.
        if (GETPFNCLIENTW(Class->fnid) == Class->lpfnWndProc)
           Ret = (ULONG_PTR)GETPFNCLIENTA(Class->fnid);
     }
     else
     {
        if (GETPFNCLIENTA(Class->fnid) == Class->lpfnWndProc)
           Ret = (ULONG_PTR)GETPFNCLIENTW(Class->fnid);
     }
  }
  // Return on change or Ansi/Unicode proc equal.
  if ( Ret != (ULONG_PTR)Class->lpfnWndProc ||
       Ansi == !!(Class->CSF_flags & CSF_ANSIPROC) )
     return Ret;

  /* We have an Ansi and Unicode swap! If Ansi create Unicode proc handle.
     This will force CallWindowProc to deal with it. */
  gcpd = NtUserGetCPD( UserHMGetHandle(pWnd),
                       (Ansi ? UserGetCPDA2U : UserGetCPDU2A )|UserGetCPDWndtoCls,
                       Ret);

  return (gcpd ? gcpd : Ret);
}

//
// Based on IntGetClsWndProc
//
ULONG_PTR FASTCALL
IntGetWndProc(PWND pWnd, BOOL Ansi)
{
  INT i;
  ULONG_PTR gcpd, Ret = 0;
  PCLS Class = DesktopPtrToUser(pWnd->pcls);

  if (!Class) return Ret;

  if (pWnd->state & WNDS_SERVERSIDEWINDOWPROC)
  {
     for ( i = FNID_FIRST; i <= FNID_SWITCH; i++)
     {
         if (GETPFNSERVER(i) == pWnd->lpfnWndProc)
         {
            if (Ansi)
               Ret = (ULONG_PTR)GETPFNCLIENTA(i);
            else
               Ret = (ULONG_PTR)GETPFNCLIENTW(i);
         }
     }
     return Ret;
  }
  // Wine Class tests: 
  /*  Edit controls are special - they return a wndproc handle when
      GetWindowLongPtr is called with a different A/W.
      On the other hand there is no W->A->W conversion so this control
      is treated specially.
   */
  if (Class->fnid == FNID_EDIT)
     Ret = (ULONG_PTR)pWnd->lpfnWndProc;
  else
  {
     // Set return proc.
     Ret = (ULONG_PTR)pWnd->lpfnWndProc;

     if (Class->fnid <= FNID_GHOST && Class->fnid >= FNID_BUTTON)
     {
        if (Ansi)
        {
           if (GETPFNCLIENTW(Class->fnid) == pWnd->lpfnWndProc)
              Ret = (ULONG_PTR)GETPFNCLIENTA(Class->fnid);
        }
        else
        {
           if (GETPFNCLIENTA(Class->fnid) == pWnd->lpfnWndProc)
              Ret = (ULONG_PTR)GETPFNCLIENTW(Class->fnid);
        }
     }
     // Return on the change.
     if ( Ret != (ULONG_PTR)pWnd->lpfnWndProc)
        return Ret;
  }

  if ( Ansi == !!(pWnd->state & WNDS_ANSIWINDOWPROC) )
     return Ret;

  gcpd = NtUserGetCPD( UserHMGetHandle(pWnd),
                      (Ansi ? UserGetCPDA2U : UserGetCPDU2A )|UserGetCPDWindow,
                       Ret);

  return (gcpd ? gcpd : Ret);
}

/*
 * @implemented
 */
DWORD WINAPI
GetClassLongA(HWND hWnd, int nIndex)
{
    PWND Wnd;
    PCLS Class;
    ULONG_PTR Ret = 0;

    TRACE("%p %d\n", hWnd, nIndex);

    Wnd = ValidateHwnd(hWnd);
    if (!Wnd)
        return 0;

    _SEH2_TRY
    {
        Class = DesktopPtrToUser(Wnd->pcls);
        if (Class != NULL)
        {
            if (nIndex >= 0)
            {
                if (nIndex + sizeof(ULONG_PTR) < nIndex ||
                    nIndex + sizeof(ULONG_PTR) > Class->cbclsExtra)
                {
                    SetLastError(ERROR_INVALID_PARAMETER);
                }
                else
                    Ret = *(PULONG_PTR)((ULONG_PTR)(Class + 1) + nIndex);
            }
            else
            {
                switch (nIndex)
                {
                    case GCL_CBWNDEXTRA:
                        Ret = (ULONG_PTR)Class->cbwndExtra;
                        break;

                    case GCL_CBCLSEXTRA:
                        Ret = (ULONG_PTR)Class->cbclsExtra;
                        break;

                    case GCL_HBRBACKGROUND:
                        Ret = (ULONG_PTR)Class->hbrBackground;
                        if (Ret != 0 && Ret < 0x4000)
                            Ret = (ULONG_PTR)GetSysColorBrush((ULONG)Ret - 1);
                        break;

                    case GCL_HMODULE:
                        //ERR("Cls 0x%x GCL_HMODULE 0x%x\n", Wnd->pcls, Class->hModule);
                        Ret = (ULONG_PTR)Class->hModule;
                        break;

                    case GCL_MENUNAME:
                        Ret = (ULONG_PTR)Class->lpszClientAnsiMenuName;
                        break;

                    case GCL_STYLE:
                        Ret = (ULONG_PTR)Class->style;
                        break;

                    case GCW_ATOM:
                        Ret = (ULONG_PTR)Class->atomClassName;
                        break;

                    case GCLP_HCURSOR:
                        /* FIXME - get handle from pointer to CURSOR object */
                        Ret = (ULONG_PTR)Class->hCursor;
                        break;

                    case GCLP_HICON:
                        /* FIXME - get handle from pointer to ICON object */
                        Ret = (ULONG_PTR)Class->hIcon;
                        break;

                    case GCLP_HICONSM:
                        /* FIXME - get handle from pointer to ICON object */
                        Ret = (ULONG_PTR)Class->hIconSm;
                        break;

                    case GCLP_WNDPROC:
                        Ret = IntGetClsWndProc(Wnd, Class, TRUE);
                        break;

                    default:
                        SetLastError(ERROR_INVALID_INDEX);
                        break;
                }
            }
        }
        else
        {
            WARN("Invalid class for hwnd 0x%p!\n", hWnd);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Ret = 0;
    }
    _SEH2_END;

    return Ret;
}

/*
 * @implemented
 */
DWORD WINAPI
GetClassLongW ( HWND hWnd, int nIndex )
{
    PWND Wnd;
    PCLS Class;
    ULONG_PTR Ret = 0;

    TRACE("%p %d\n", hWnd, nIndex);

    Wnd = ValidateHwnd(hWnd);
    if (!Wnd)
        return 0;

    _SEH2_TRY
    {
        Class = DesktopPtrToUser(Wnd->pcls);
        if (Class != NULL)
        {
            if (nIndex >= 0)
            {
                if (nIndex + sizeof(ULONG_PTR) < nIndex ||
                    nIndex + sizeof(ULONG_PTR) > Class->cbclsExtra)
                {
                    SetLastError(ERROR_INVALID_PARAMETER);
                }
                else
                    Ret = *(PULONG_PTR)((ULONG_PTR)(Class + 1) + nIndex);
            }
            else
            {
                switch (nIndex)
                {
                    case GCL_CBWNDEXTRA:
                        Ret = (ULONG_PTR)Class->cbwndExtra;
                        break;

                    case GCL_CBCLSEXTRA:
                        Ret = (ULONG_PTR)Class->cbclsExtra;
                        break;

                    case GCL_HBRBACKGROUND:
                        Ret = (ULONG_PTR)Class->hbrBackground;
                        if (Ret != 0 && Ret < 0x4000)
                            Ret = (ULONG_PTR)GetSysColorBrush((ULONG)Ret - 1);
                        break;

                    case GCL_HMODULE:
                        Ret = (ULONG_PTR)Class->hModule;
                        break;

                    case GCL_MENUNAME:
                        Ret = (ULONG_PTR)Class->lpszClientUnicodeMenuName;
                        break;

                    case GCL_STYLE:
                        Ret = (ULONG_PTR)Class->style;
                        break;

                    case GCW_ATOM:
                        Ret = (ULONG_PTR)Class->atomClassName;
                        break;

                    case GCLP_HCURSOR:
                        /* FIXME - get handle from pointer to CURSOR object */
                        Ret = (ULONG_PTR)Class->hCursor;
                        break;

                    case GCLP_HICON:
                        /* FIXME - get handle from pointer to ICON object */
                        Ret = (ULONG_PTR)Class->hIcon;
                        break;

                    case GCLP_HICONSM:
                        /* FIXME - get handle from pointer to ICON object */
                        Ret = (ULONG_PTR)Class->hIconSm;
                        break;

                    case GCLP_WNDPROC:
                        Ret = IntGetClsWndProc(Wnd, Class, FALSE);
                        break;

                    default:
                        SetLastError(ERROR_INVALID_INDEX);
                        break;
                }
            }
        }
        else
        {
            WARN("Invalid class for hwnd 0x%p!\n", hWnd);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    return Ret;
}


/*
 * @implemented
 */
int WINAPI
GetClassNameA(
  HWND hWnd,
  LPSTR lpClassName,
  int nMaxCount)
{
    ANSI_STRING ClassName;
    int Result;

    ClassName.MaximumLength = nMaxCount;
    ClassName.Buffer = lpClassName;

    Result = NtUserGetClassName(hWnd,
                                (PUNICODE_STRING)&ClassName,
                                TRUE);

    TRACE("%p class/atom: %s/%04x %x\n", hWnd,
        IS_ATOM(lpClassName) ? NULL : lpClassName,
        IS_ATOM(lpClassName) ? lpClassName : 0,
        nMaxCount);

    return Result;
}


/*
 * @implemented
 */
int
WINAPI
GetClassNameW(
  HWND hWnd,
  LPWSTR lpClassName,
  int nMaxCount)
{
    UNICODE_STRING ClassName;
    int Result;

    ClassName.MaximumLength = nMaxCount * sizeof(WCHAR);
    ClassName.Buffer = lpClassName;

    Result = NtUserGetClassName(hWnd,
                                &ClassName,
                                FALSE);

    TRACE("%p class/atom: %S/%04x %x\n", hWnd,
        IS_ATOM(lpClassName) ? NULL : lpClassName,
        IS_ATOM(lpClassName) ? lpClassName : 0,
        nMaxCount);

    return Result;
}


/*
 * @implemented
 */
WORD
WINAPI
GetClassWord(
  HWND hwnd,
  int offset)
{
    PWND Wnd;
    PCLS class;
    WORD retvalue = 0;

    if (offset < 0) return GetClassLongA( hwnd, offset );

    Wnd = ValidateHwnd(hwnd);
    if (!Wnd)
        return 0;

    class = DesktopPtrToUser(Wnd->pcls);
    if (class == NULL) return 0;

    if (offset <= class->cbclsExtra - sizeof(WORD))
        memcpy( &retvalue, (char *)(class + 1) + offset, sizeof(retvalue) );
    else
        SetLastError( ERROR_INVALID_INDEX );

    return retvalue;
}


/*
 * @implemented
 */
LONG
WINAPI
GetWindowLongA ( HWND hWnd, int nIndex )
{
    PWND Wnd;

    Wnd = ValidateHwnd(hWnd);
    if (Wnd == NULL)
        return 0;

    if (nIndex >= 0)
    {
        if ((DWORD)nIndex + sizeof(LONG) > Wnd->cbwndExtra)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }
        return *((LONG *)((PCHAR)(Wnd + 1) + nIndex));
    }
    else
    {
        switch (nIndex)
        {
            case GWL_EXSTYLE:
                return Wnd->ExStyle;
            case GWL_STYLE:
                return Wnd->style;
            case GWL_HINSTANCE:
                return (LONG)Wnd->hModule;
            case GWL_ID:
                return Wnd->IDMenu;
            case GWL_USERDATA:
                return Wnd->dwUserData;

            case GWL_HWNDPARENT:
            {
                HWND parent = GetAncestor( hWnd, GA_PARENT );
                if (parent == GetDesktopWindow()) parent = GetWindow( hWnd, GW_OWNER );
                return (LONG)parent;
            }
            case GWL_WNDPROC:
                if (!TestWindowProcess(Wnd))
                {
                   SetLastError(ERROR_ACCESS_DENIED);
                   return 0;
                }
                return IntGetWndProc(Wnd, TRUE);

            default:
                SetLastError(ERROR_INVALID_PARAMETER);
                return 0;
        }
    }
}


/*
 * @implemented
 */
LONG
WINAPI
GetWindowLongW(HWND hWnd, int nIndex)
{
    PWND Wnd;

    Wnd = ValidateHwnd(hWnd);
    if (Wnd == NULL)
        return 0;

    if (nIndex >= 0)
    {
        if ((DWORD)nIndex + sizeof(LONG) > Wnd->cbwndExtra)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }
        return *((LONG *)((PCHAR)(Wnd + 1) + nIndex));
    }
    else
    {
        switch (nIndex)
        {
            case GWL_EXSTYLE:
                return Wnd->ExStyle;
            case GWL_STYLE:
                return Wnd->style;
            case GWL_HINSTANCE:
                return (LONG)Wnd->hModule;
            case GWL_ID:
                return Wnd->IDMenu;
            case GWL_USERDATA:
                return Wnd->dwUserData;

            case GWL_HWNDPARENT:
            {
                HWND parent = GetAncestor( hWnd, GA_PARENT );
                if (parent == GetDesktopWindow()) parent = GetWindow( hWnd, GW_OWNER );
                return (LONG)parent;
            }
            case GWL_WNDPROC:
                if (!TestWindowProcess(Wnd))
                {
                   SetLastError(ERROR_ACCESS_DENIED);
                   return 0;
                }
                return IntGetWndProc(Wnd, FALSE);

            default:
                SetLastError(ERROR_INVALID_PARAMETER);
                return 0;
        }
    }
}

/*
 * @implemented
 */
WORD
WINAPI
GetWindowWord(HWND hWnd, int nIndex)
{
  return (WORD)GetWindowLongW(hWnd, nIndex);
}

/*
 * @implemented
 */
WORD
WINAPI
SetWindowWord ( HWND hWnd,int nIndex,WORD wNewWord )
{
  return (WORD)NtUserSetWindowLong ( hWnd, nIndex, (LONG)wNewWord, TRUE );
}

/*
 * @implemented
 */
UINT
WINAPI
RealGetWindowClassW(
  HWND  hwnd,
  LPWSTR pszType,
  UINT  cchType)
{
	/* FIXME: Implement correct functionality of RealGetWindowClass */
	return GetClassNameW(hwnd,pszType,cchType);
}


/*
 * @implemented
 */
UINT
WINAPI
RealGetWindowClassA(
  HWND  hwnd,
  LPSTR pszType,
  UINT  cchType)
{
	/* FIXME: Implement correct functionality of RealGetWindowClass */
	return GetClassNameA(hwnd,pszType,cchType);
}

/*
 * Create a small icon based on a standard icon
 */
static HICON
CreateSmallIcon(HICON StdIcon)
{
   HICON SmallIcon = NULL;
   ICONINFO StdInfo;
   int SmallIconWidth;
   int SmallIconHeight;
   BITMAP StdBitmapInfo;
   HDC hInfoDc = NULL;
   HDC hSourceDc = NULL;
   HDC hDestDc = NULL;
   ICONINFO SmallInfo;
   HBITMAP OldSourceBitmap = NULL;
   HBITMAP OldDestBitmap = NULL;

   SmallInfo.hbmColor = NULL;
   SmallInfo.hbmMask = NULL;

   /* We need something to work with... */
   if (NULL == StdIcon)
   {
      goto cleanup;
   }

   SmallIconWidth = GetSystemMetrics(SM_CXSMICON);
   SmallIconHeight = GetSystemMetrics(SM_CYSMICON);
   if (! GetIconInfo(StdIcon, &StdInfo))
   {
      ERR("Failed to get icon info for icon 0x%x\n", StdIcon);
      goto cleanup;
   }
   if (! GetObjectW(StdInfo.hbmMask, sizeof(BITMAP), &StdBitmapInfo))
   {
      ERR("Failed to get bitmap info for icon 0x%x bitmap 0x%x\n",
              StdIcon, StdInfo.hbmColor);
      goto cleanup;
   }
   if (StdBitmapInfo.bmWidth == SmallIconWidth &&
       StdBitmapInfo.bmHeight == SmallIconHeight)
   {
      /* Icon already has the correct dimensions */
      return StdIcon;
   }

   /* Get a handle to a info DC and handles to DCs which can be used to
      select a bitmap into. This is done to avoid triggering a switch to
      graphics mode (if we're currently in text/blue screen mode) */
   hInfoDc = CreateICW(NULL, NULL, NULL, NULL);
   if (NULL == hInfoDc)
   {
      ERR("Failed to create info DC\n");
      goto cleanup;
   }
   hSourceDc = CreateCompatibleDC(NULL);
   if (NULL == hSourceDc)
   {
      ERR("Failed to create source DC\n");
      goto cleanup;
   }
   hDestDc = CreateCompatibleDC(NULL);
   if (NULL == hDestDc)
   {
      ERR("Failed to create dest DC\n");
      goto cleanup;
   }

   OldSourceBitmap = SelectObject(hSourceDc, StdInfo.hbmColor);
   if (NULL == OldSourceBitmap)
   {
      ERR("Failed to select source color bitmap\n");
      goto cleanup;
   }
   SmallInfo.hbmColor = CreateCompatibleBitmap(hInfoDc, SmallIconWidth,
                                              SmallIconHeight);
   if (NULL == SmallInfo.hbmColor)
   {
      ERR("Failed to create color bitmap\n");
      goto cleanup;
   }
   OldDestBitmap = SelectObject(hDestDc, SmallInfo.hbmColor);
   if (NULL == OldDestBitmap)
   {
      ERR("Failed to select dest color bitmap\n");
      goto cleanup;
   }
   if (! StretchBlt(hDestDc, 0, 0, SmallIconWidth, SmallIconHeight,
                    hSourceDc, 0, 0, StdBitmapInfo.bmWidth,
                    StdBitmapInfo.bmHeight, SRCCOPY))
   {
     ERR("Failed to stretch color bitmap\n");
     goto cleanup;
   }

   if (NULL == SelectObject(hSourceDc, StdInfo.hbmMask))
   {
      ERR("Failed to select source mask bitmap\n");
      goto cleanup;
   }
   SmallInfo.hbmMask = CreateBitmap(SmallIconWidth, SmallIconHeight, 1, 1,
                                    NULL);
   if (NULL == SmallInfo.hbmMask)
   {
      ERR("Failed to create mask bitmap\n");
      goto cleanup;
   }
   if (NULL == SelectObject(hDestDc, SmallInfo.hbmMask))
   {
      ERR("Failed to select dest mask bitmap\n");
      goto cleanup;
   }
   if (! StretchBlt(hDestDc, 0, 0, SmallIconWidth, SmallIconHeight,
                    hSourceDc, 0, 0, StdBitmapInfo.bmWidth,
                    StdBitmapInfo.bmHeight, SRCCOPY))
   {
      ERR("Failed to stretch mask bitmap\n");
      goto cleanup;
   }

   SmallInfo.fIcon = TRUE;
   SmallInfo.xHotspot = SmallIconWidth / 2;
   SmallInfo.yHotspot = SmallIconHeight / 2;
   SmallIcon = CreateIconIndirect(&SmallInfo);
   if (NULL == SmallIcon)
   {
      ERR("Failed to create icon\n");
      goto cleanup;
   }

cleanup:
   if (NULL != SmallInfo.hbmMask)
   {
      DeleteObject(SmallInfo.hbmMask);
   }
   if (NULL != OldDestBitmap)
   {
      SelectObject(hDestDc, OldDestBitmap);
   }
   if (NULL != SmallInfo.hbmColor)
   {
      DeleteObject(SmallInfo.hbmColor);
   }
   if (NULL != hDestDc)
   {
      DeleteDC(hDestDc);
   }
   if (NULL != OldSourceBitmap)
   {
      SelectObject(hSourceDc, OldSourceBitmap);
   }
   if (NULL != hSourceDc)
   {
      DeleteDC(hSourceDc);
   }
   if (NULL != hInfoDc)
   {
      DeleteDC(hInfoDc);
   }

   return SmallIcon;
}


ATOM WINAPI
RegisterClassExWOWW(WNDCLASSEXW *lpwcx,
                    LPDWORD pdwWowData,
                    WORD fnID,
                    DWORD dwFlags,
                    BOOL ChkRegCls)
{
   ATOM Atom;
   WNDCLASSEXW WndClass;
   UNICODE_STRING ClassName;
   UNICODE_STRING MenuName = {0};
   CLSMENUNAME clsMenuName;
   ANSI_STRING AnsiMenuName;

   if (lpwcx == NULL || lpwcx->cbSize != sizeof(WNDCLASSEXW) ||
       lpwcx->cbClsExtra < 0 || lpwcx->cbWndExtra < 0 ||
       lpwcx->lpszClassName == NULL)
   {
      TRACE("RegisterClassExWOWW Invalid Parameter Error!\n");
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
   }

   if (ChkRegCls)
   {
      if (!RegisterDefaultClasses) RegisterSystemControls();
   }
   /*
    * On real Windows this looks more like:
    *    if (lpwcx->hInstance == User32Instance &&
    *        *(PULONG)((ULONG_PTR)NtCurrentTeb() + 0x6D4) & 0x400)
    * But since I have no idea what the magic field in the
    * TEB structure means, I rather decided to omit that.
    * -- Filip Navara

       GetWin32ClientInfo()->dwExpWinVer & (WINVER == 0x400)
    */
   if (lpwcx->hInstance == User32Instance)
   {
      TRACE("RegisterClassExWOWW User32Instance!\n");
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
   }
   /* Yes, this is correct. We should modify the passed structure. */
   if (lpwcx->hInstance == NULL)
      ((WNDCLASSEXW*)lpwcx)->hInstance = GetModuleHandleW(NULL);

   RtlCopyMemory(&WndClass, lpwcx, sizeof(WNDCLASSEXW));

   if (NULL == WndClass.hIconSm)
   {
      WndClass.hIconSm = CreateSmallIcon(WndClass.hIcon);
   }

   if (WndClass.lpszMenuName != NULL)
   {
      if (!IS_INTRESOURCE(WndClass.lpszMenuName))
      {
         if (WndClass.lpszMenuName[0])
         {
            RtlInitUnicodeString(&MenuName, WndClass.lpszMenuName);
            RtlUnicodeStringToAnsiString( &AnsiMenuName, &MenuName, TRUE);
         }
      }
      else
      {
         MenuName.Buffer = (LPWSTR)WndClass.lpszMenuName;
         AnsiMenuName.Buffer = (PCHAR)WndClass.lpszMenuName;
      }
   }

   if (IS_ATOM(WndClass.lpszClassName))
   {
      ClassName.Length =
      ClassName.MaximumLength = 0;
      ClassName.Buffer = (LPWSTR)WndClass.lpszClassName;
   }
   else
   {
      RtlInitUnicodeString(&ClassName, WndClass.lpszClassName);
   }

   clsMenuName.pszClientAnsiMenuName = AnsiMenuName.Buffer;
   clsMenuName.pwszClientUnicodeMenuName = MenuName.Buffer;
   clsMenuName.pusMenuName = &MenuName;
   
   Atom = NtUserRegisterClassExWOW( &WndClass,
                                    &ClassName,  
                                     NULL, //PUNICODE_STRING ClsNVersion,
                                    &clsMenuName,
                                     fnID,
                                     dwFlags,
                                     pdwWowData);

   TRACE("atom=%04x wndproc=%p hinst=%p bg=%p style=%08x clsExt=%d winExt=%d class=%p\n",
          Atom, lpwcx->lpfnWndProc, lpwcx->hInstance, lpwcx->hbrBackground,
          lpwcx->style, lpwcx->cbClsExtra, lpwcx->cbWndExtra, WndClass);

   return Atom;
}

/*
 * @implemented
 */
ATOM WINAPI
RegisterClassExA(CONST WNDCLASSEXA *lpwcx)
{
   RTL_ATOM Atom;
   WNDCLASSEXW WndClass;
   WCHAR mname[MAX_BUFFER_LEN];
   WCHAR cname[MAX_BUFFER_LEN];

   RtlCopyMemory(&WndClass, lpwcx, sizeof(WNDCLASSEXA));

   if (WndClass.lpszMenuName != NULL)
   {
      if (!IS_INTRESOURCE(WndClass.lpszMenuName))
      {
         if (WndClass.lpszMenuName[0])
         {
            if (!MultiByteToWideChar( CP_ACP, 0, lpwcx->lpszMenuName, -1, mname, MAX_ATOM_LEN + 1 )) return 0;

            WndClass.lpszMenuName = mname;
         }
      }
   }

   if (!IS_ATOM(WndClass.lpszClassName))
   {
      if (!MultiByteToWideChar( CP_ACP, 0, lpwcx->lpszClassName, -1, cname, MAX_ATOM_LEN + 1 )) return 0;

      WndClass.lpszClassName = cname;
   }

   Atom = RegisterClassExWOWW( &WndClass,
                                0,
                                0,
                                CSF_ANSIPROC,
                                TRUE);

   TRACE("A atom=%04x wndproc=%p hinst=%p bg=%p style=%08x clsExt=%d winExt=%d class=%p\n",
          Atom, lpwcx->lpfnWndProc, lpwcx->hInstance, lpwcx->hbrBackground,
          lpwcx->style, lpwcx->cbClsExtra, lpwcx->cbWndExtra, WndClass);

   return (ATOM)Atom;
}

/*
 * @implemented
 */
ATOM WINAPI
RegisterClassExW(CONST WNDCLASSEXW *lpwcx)
{
   ATOM Atom;

   Atom = RegisterClassExWOWW( (WNDCLASSEXW *)lpwcx, 0, 0, 0, TRUE);

   TRACE("W atom=%04x wndproc=%p hinst=%p bg=%p style=%08x clsExt=%d winExt=%d\n",
          Atom, lpwcx->lpfnWndProc, lpwcx->hInstance, lpwcx->hbrBackground,
          lpwcx->style, lpwcx->cbClsExtra, lpwcx->cbWndExtra);

   return Atom;
}

/*
 * @implemented
 */
ATOM WINAPI
RegisterClassA(CONST WNDCLASSA *lpWndClass)
{
   WNDCLASSEXA Class;

   if (lpWndClass == NULL)
      return 0;

   RtlCopyMemory(&Class.style, lpWndClass, sizeof(WNDCLASSA));
   Class.cbSize = sizeof(WNDCLASSEXA);
   Class.hIconSm = NULL;

   return RegisterClassExA(&Class);
}

/*
 * @implemented
 */
ATOM WINAPI
RegisterClassW(CONST WNDCLASSW *lpWndClass)
{
   WNDCLASSEXW Class;

   if (lpWndClass == NULL)
      return 0;

   RtlCopyMemory(&Class.style, lpWndClass, sizeof(WNDCLASSW));
   Class.cbSize = sizeof(WNDCLASSEXW);
   Class.hIconSm = NULL;

   return RegisterClassExW(&Class);
}

/*
 * @implemented
 */
DWORD
WINAPI
SetClassLongA (HWND hWnd,
               int nIndex,
               LONG dwNewLong)
{
    PSTR lpStr = (PSTR)dwNewLong;
    UNICODE_STRING Value = {0};
    BOOL Allocated = FALSE;
    DWORD Ret;

    /* FIXME - portability!!!! */

    if (nIndex == GCL_MENUNAME && lpStr != NULL)
    {
        if (!IS_INTRESOURCE(lpStr))
        {
            if (!RtlCreateUnicodeStringFromAsciiz(&Value,
                                                  lpStr))
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return 0;
            }

            Allocated = TRUE;
        }
        else
            Value.Buffer = (PWSTR)lpStr;

        dwNewLong = (LONG)&Value;
    }
    else if (nIndex == GCW_ATOM && lpStr != NULL)
    {
        if (!IS_ATOM(lpStr))
        {
            if (!RtlCreateUnicodeStringFromAsciiz(&Value,
                                                  lpStr))
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return 0;
            }

            Allocated = TRUE;
        }
        else
            Value.Buffer = (PWSTR)lpStr;

        dwNewLong = (LONG)&Value;
    }

    Ret = (DWORD)NtUserSetClassLong(hWnd,
                                    nIndex,
                                    dwNewLong,
                                    TRUE);

    if (Allocated)
    {
        RtlFreeUnicodeString(&Value);
    }

    return Ret;
}


/*
 * @implemented
 */
DWORD
WINAPI
SetClassLongW(HWND hWnd,
              int nIndex,
              LONG dwNewLong)
{
    PWSTR lpStr = (PWSTR)dwNewLong;
    UNICODE_STRING Value = {0};

    TRACE("%p %d %lx\n", hWnd, nIndex, dwNewLong);

    /* FIXME - portability!!!! */

    if (nIndex == GCL_MENUNAME && lpStr != NULL)
    {
        if (!IS_INTRESOURCE(lpStr))
        {
            RtlInitUnicodeString(&Value,
                                 lpStr);
        }
        else
            Value.Buffer = lpStr;

        dwNewLong = (LONG)&Value;
    }
    else if (nIndex == GCW_ATOM && lpStr != NULL)
    {
        if (!IS_ATOM(lpStr))
        {
            RtlInitUnicodeString(&Value,
                                 lpStr);
        }
        else
            Value.Buffer = lpStr;

        dwNewLong = (LONG)&Value;
    }

    return (DWORD)NtUserSetClassLong(hWnd,
                                     nIndex,
                                     dwNewLong,
                                     FALSE);
}


/*
 * @implemented
 */
WORD
WINAPI
SetClassWord(
  HWND hWnd,
  int nIndex,
  WORD wNewWord)
/*
 * NOTE: Obsoleted in 32-bit windows
 */
{
    if ((nIndex < 0) && (nIndex != GCW_ATOM))
        return 0;

    return (WORD) SetClassLongW ( hWnd, nIndex, wNewWord );
}


/*
 * @implemented
 */
LONG
WINAPI
SetWindowLongA(
  HWND hWnd,
  int nIndex,
  LONG dwNewLong)
{
  return NtUserSetWindowLong(hWnd, nIndex, dwNewLong, TRUE);
}


/*
 * @implemented
 */
LONG
WINAPI
SetWindowLongW(
  HWND hWnd,
  int nIndex,
  LONG dwNewLong)
{
  return NtUserSetWindowLong(hWnd, nIndex, dwNewLong, FALSE);
}


/*
 * @implemented
 */
BOOL
WINAPI
UnregisterClassA(
  LPCSTR lpClassName,
  HINSTANCE hInstance)
{
    UNICODE_STRING ClassName = {0};
    BOOL Ret;

    TRACE("class/atom: %s/%04x %p\n",
        IS_ATOM(lpClassName) ? NULL : lpClassName,
        IS_ATOM(lpClassName) ? lpClassName : 0,
        hInstance);

    if (!IS_ATOM(lpClassName))
    {
        if (!RtlCreateUnicodeStringFromAsciiz(&ClassName,
                                              lpClassName))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
    }
    else
        ClassName.Buffer = (PWSTR)((ULONG_PTR)lpClassName);

    Ret = NtUserUnregisterClass(&ClassName,
                                hInstance,
                                0);

    if (!IS_ATOM(lpClassName))
        RtlFreeUnicodeString(&ClassName);

    return Ret;
}


/*
 * @implemented
 */
BOOL
WINAPI
UnregisterClassW(
  LPCWSTR lpClassName,
  HINSTANCE hInstance)
{
    UNICODE_STRING ClassName = {0};

    TRACE("class/atom: %S/%04x %p\n",
        IS_ATOM(lpClassName) ? NULL : lpClassName,
        IS_ATOM(lpClassName) ? lpClassName : 0,
        hInstance);

    if (!IS_ATOM(lpClassName))
    {
        RtlInitUnicodeString(&ClassName,
                             lpClassName);
    }
    else
        ClassName.Buffer = (PWSTR)((ULONG_PTR)lpClassName);

    return NtUserUnregisterClass(&ClassName,
                                 hInstance,
                                 0);
}

/* EOF */
