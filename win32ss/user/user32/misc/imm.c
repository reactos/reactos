/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/misc/imm.c
 * PURPOSE:         User32.dll Imm functions
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 * UPDATE HISTORY:
 *      01/27/2009  Created
 */

#include <user32.h>
#include <strsafe.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

#define IMM_INIT_MAGIC 0x19650412

HINSTANCE ghImm32 = NULL;
BOOL bImmInitializing = FALSE;

/* define stub functions */
#undef DEFINE_IMM_ENTRY
#define DEFINE_IMM_ENTRY(type, name, params, retval, retkind) \
    static type WINAPI IMMSTUB_##name params { IMM_RETURN_##retkind((type)retval); }
#include "immtable.h"

Imm32ApiTable gImmApiEntries = {
/* initialize by stubs */
#undef DEFINE_IMM_ENTRY
#define DEFINE_IMM_ENTRY(type, name, params, retval, retkind) \
    IMMSTUB_##name,
#include "immtable.h"
};

HRESULT WINAPI GetImmFileName(PWSTR lpBuffer, UINT uSize)
{
  UINT length;
  STRSAFE_LPWSTR Safe = lpBuffer;

  length = GetSystemDirectoryW(lpBuffer, uSize);
  if ( length && length < uSize )
  {
    StringCchCatW(Safe, uSize, L"\\");
    return StringCchCatW(Safe, uSize, L"imm32.dll");
  }
  return StringCchCopyW(Safe, uSize, L"imm32.dll");
}  

/*
 * @unimplemented
 */
BOOL WINAPI IntInitializeImmEntryTable(VOID)
{
    WCHAR ImmFile[MAX_PATH];
    HMODULE imm32 = ghImm32;

    GetImmFileName(ImmFile, sizeof(ImmFile));
    TRACE("File %ws\n",ImmFile);

    if (imm32 == NULL)
    {
       imm32 = GetModuleHandleW(ImmFile);
    }

    if (imm32 == NULL)
    {
        imm32 = ghImm32 = LoadLibraryW(ImmFile);
        if (imm32 == NULL)
        {
           ERR("Did not load!\n");
           return FALSE;
        }
        return TRUE;
    }

/* load imm procedures */
#undef DEFINE_IMM_ENTRY
#define DEFINE_IMM_ENTRY(type, name, params, retval, retkind) \
    do { \
        FN_##name proc = (FN_##name)GetProcAddress(imm32, #name); \
        if (proc) { \
            IMM_FN(name) = proc; \
        } \
    } while (0);
#include "immtable.h"

    return TRUE;
}

BOOL WINAPI InitializeImmEntryTable(VOID)
{
  bImmInitializing = TRUE;
  return IntInitializeImmEntryTable();
}

BOOL WINAPI User32InitializeImmEntryTable(DWORD magic)
{
    TRACE("Imm (%x)\n", magic);

    if (magic != IMM_INIT_MAGIC)
        return FALSE;

    if (IMM_FN(ImmIsIME) != IMMSTUB_ImmIsIME)
        return TRUE;

    IntInitializeImmEntryTable();

    if (ghImm32 == NULL && !bImmInitializing)
    {
       WCHAR ImmFile[MAX_PATH];
       GetImmFileName(ImmFile, sizeof(ImmFile));
       ghImm32 = LoadLibraryW(ImmFile);
       if (ghImm32 == NULL)
       {
          ERR("Did not load! 2\n");
          return FALSE;
       } 
    }

    return TRUE;
}

LRESULT WINAPI ImeWndProc_common( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, BOOL unicode ) // ReactOS
{
    PWND pWnd;
    PIMEUI pimeui;

    pWnd = ValidateHwnd(hwnd);
    if (pWnd)
    {
       if (!pWnd->fnid)
       {
          if (msg != WM_NCCREATE)
          {
             if (unicode)
                return DefWindowProcW(hwnd, msg, wParam, lParam);
             return DefWindowProcA(hwnd, msg, wParam, lParam);
          }
          NtUserSetWindowFNID(hwnd, FNID_IME);
          pimeui = HeapAlloc( GetProcessHeap(), 0, sizeof(IMEUI) );
          SetWindowLongPtrW(hwnd, 0, (LONG_PTR)pimeui);
       }
       else
       {
          if (pWnd->fnid != FNID_IME)
          {
             ERR("Wrong window class for Ime! fnId 0x%x\n",pWnd->fnid);
             return 0;
          }
          pimeui = ((PIMEWND)pWnd)->pimeui;
          if (pimeui == NULL)
          {
             ERR("Window is not set to IME!\n");
             return 0;
          }
       }
    }

    if (msg==WM_CREATE || msg==WM_NCCREATE)
        return TRUE;

    if (msg==WM_NCDESTROY)
    {
        HeapFree( GetProcessHeap(), 0, pimeui );
        SetWindowLongPtrW(hwnd, 0, 0);
        NtUserSetWindowFNID(hwnd, FNID_DESTROY);
    }

    if (unicode)
       return DefWindowProcW(hwnd, msg, wParam, lParam);
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

LRESULT WINAPI ImeWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return ImeWndProc_common(hwnd, msg, wParam, lParam, FALSE);
}

LRESULT WINAPI ImeWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return ImeWndProc_common(hwnd, msg, wParam, lParam, TRUE);
}

BOOL
WINAPI
UpdatePerUserImmEnabling(VOID)
{
  BOOL Ret = NtUserCallNoParam(NOPARAM_ROUTINE_UPDATEPERUSERIMMENABLING);
  if ( Ret )
  {
    if ( gpsi->dwSRVIFlags & SRVINFO_IMM32 )
    {
      HMODULE imm32 = GetModuleHandleW(L"imm32.dll");
      if ( !imm32 )
      {
        imm32 = LoadLibraryW(L"imm32.dll");
        if (!imm32)
        {
           ERR("UPUIE: Imm32 not installed!\n");
           Ret = FALSE;
        }
      }
    }
  }
  return Ret;
}

static const WCHAR imeW[] = {'I','M','E',0};

BOOL
WINAPI
RegisterIMEClass(VOID)
{
    WNDCLASSEXW WndClass;
    ATOM atom;

    ZeroMemory(&WndClass, sizeof(WndClass));

    WndClass.cbSize = sizeof(WndClass);
    WndClass.lpszClassName = imeW;
    WndClass.style = CS_GLOBALCLASS;
    WndClass.lpfnWndProc = ImeWndProcW;
    WndClass.cbWndExtra = sizeof(LONG_PTR);
    WndClass.hCursor = LoadCursorW(NULL, IDC_ARROW);

    atom = RegisterClassExWOWW( &WndClass,
                                 0,
                                 FNID_IME,
                                 0,
                                 FALSE);
    if (atom)
    {
       RegisterDefaultClasses |= ICLASS_TO_MASK(ICLS_IME);
       TRACE("Register IME Class!\n");
       return TRUE;
    }
    ERR("Failed to register IME Class!\n");
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI CliImmSetHotKey(DWORD dwID, UINT uModifiers, UINT uVirtualKey, HKL hKl)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
IMPSetIMEW(HWND hwnd, LPIMEPROW ime)
{
    return IMM_FN(ImmIMPSetIMEW)(hwnd, ime);
}

/*
 * @implemented
 */
BOOL
WINAPI
IMPQueryIMEW(LPIMEPROW ime)
{
    return IMM_FN(ImmIMPQueryIMEW)(ime);
}

/*
 * @implemented
 */
BOOL
WINAPI
IMPGetIMEW(HWND hwnd, LPIMEPROW ime)
{
    return IMM_FN(ImmIMPGetIMEW)(hwnd, ime);
}

/*
 * @implemented
 */
BOOL
WINAPI
IMPSetIMEA(HWND hwnd, LPIMEPROA ime)
{
    return IMM_FN(ImmIMPSetIMEA)(hwnd, ime);
}

/*
 * @implemented
 */
BOOL
WINAPI
IMPQueryIMEA(LPIMEPROA ime)
{
    return IMM_FN(ImmIMPQueryIMEA)(ime);
}

/*
 * @implemented
 */
BOOL
WINAPI
IMPGetIMEA(HWND hwnd, LPIMEPROA ime)
{
    return IMM_FN(ImmIMPGetIMEA)(hwnd, ime);
}

/*
 * @implemented
 */
LRESULT
WINAPI
SendIMEMessageExW(HWND hwnd, LPARAM lParam)
{
    return IMM_FN(ImmSendIMEMessageExW)(hwnd, lParam);
}

/*
 * @implemented
 */
LRESULT
WINAPI
SendIMEMessageExA(HWND hwnd, LPARAM lParam)
{
    return IMM_FN(ImmSendIMEMessageExA)(hwnd, lParam);
}

/*
 * @implemented
 */
BOOL
WINAPI
WINNLSEnableIME(HWND hwnd, BOOL enable)
{
    return IMM_FN(ImmWINNLSEnableIME)(hwnd, enable);
}

/*
 * @implemented
 */
BOOL
WINAPI
WINNLSGetEnableStatus(HWND hwnd)
{
    return IMM_FN(ImmWINNLSGetEnableStatus)(hwnd);
}

/*
 * @implemented
 */
UINT
WINAPI
WINNLSGetIMEHotkey(HWND hwnd)
{
    return FALSE;
}
