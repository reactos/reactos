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

/* Is != NULL when we have loaded the IMM ourselves */
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

HRESULT
GetImmFileName(_Out_ LPWSTR lpBuffer,
               _In_ size_t cchBuffer)
{
    UINT length = GetSystemDirectoryW(lpBuffer, cchBuffer);
    if (length && length < cchBuffer)
    {
        StringCchCatW(lpBuffer, cchBuffer, L"\\");
        return StringCchCatW(lpBuffer, cchBuffer, L"imm32.dll");
    }
    return StringCchCopyW(lpBuffer, cchBuffer, L"imm32.dll");
}

/*
 * @unimplemented
 */
static BOOL IntInitializeImmEntryTable(VOID)
{
    WCHAR ImmFile[MAX_PATH];
    HMODULE imm32 = ghImm32;

    /* Check whether the IMM table has already been initialized */
    if (IMM_FN(ImmWINNLSEnableIME) != IMMSTUB_ImmWINNLSEnableIME)
        return TRUE;

    GetImmFileName(ImmFile, _countof(ImmFile));
    TRACE("File %S\n", ImmFile);

    /* If IMM32 is already loaded, use it without increasing reference count. */
    if (imm32 == NULL)
        imm32 = GetModuleHandleW(ImmFile);

    /*
     * Loading imm32.dll will call imm32!DllMain function.
     * imm32!DllMain calls User32InitializeImmEntryTable.
     * Thus, if imm32.dll was loaded, the table has been loaded.
     */
    if (imm32 == NULL)
    {
        imm32 = ghImm32 = LoadLibraryW(ImmFile);
        if (imm32 == NULL)
        {
            ERR("Did not load imm32.dll!\n");
            return FALSE;
        }
        return TRUE;
    }

/* load imm procedures */
#undef DEFINE_IMM_ENTRY
#define DEFINE_IMM_ENTRY(type, name, params, retval, retkind) \
    do { \
        FN_##name proc = (FN_##name)GetProcAddress(imm32, #name); \
        if (!proc) { \
            ERR("Could not load %s\n", #name); \
            return FALSE; \
        } \
        IMM_FN(name) = proc; \
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

    /* Check whether the IMM table has already been initialized */
    if (IMM_FN(ImmWINNLSEnableIME) != IMMSTUB_ImmWINNLSEnableIME)
        return TRUE;

    IntInitializeImmEntryTable();

    if (ghImm32 == NULL && !bImmInitializing)
    {
        WCHAR ImmFile[MAX_PATH];
        GetImmFileName(ImmFile, _countof(ImmFile));
        ghImm32 = LoadLibraryW(ImmFile);
        if (ghImm32 == NULL)
        {
            ERR("Did not load imm32.dll!\n");
            return FALSE;
        }
    }

    return IMM_FN(ImmRegisterClient)(&gSharedInfo, ghImm32);
}

static BOOL CheckIMCForWindow(HIMC hIMC, HWND hWnd)
{
    PIMC pIMC = ValidateHandle(hIMC, TYPE_INPUTCONTEXT);
    return pIMC && (!pIMC->hImeWnd || pIMC->hImeWnd == hWnd || !ValidateHwnd(pIMC->hImeWnd));
}

static BOOL ImeWnd_OnCreate(PIMEUI pimeui, LPCREATESTRUCT lpCS)
{
    PWND pParentWnd, pWnd = pimeui->spwnd;
    HIMC hIMC;

    if (!pWnd || (pWnd->style & (WS_DISABLED | WS_POPUP)) != (WS_DISABLED | WS_POPUP))
        return FALSE;

    pimeui->hIMC = NULL;
    pParentWnd = ValidateHwnd(lpCS->hwndParent);
    if (pParentWnd)
    {
        hIMC = pParentWnd->hImc;
        if (hIMC && CheckIMCForWindow(hIMC, UserHMGetHandle(pWnd)))
            pimeui->hIMC = hIMC;
    }

    pimeui->fShowStatus = FALSE;
    pimeui->nCntInIMEProc = 0;
    pimeui->fActivate = FALSE;
    pimeui->fDestroy = FALSE;
    pimeui->hwndIMC = NULL;
    pimeui->hKL = GetWin32ClientInfo()->hKL;
    pimeui->fCtrlShowStatus = TRUE;

    IMM_FN(ImmLoadIME)(pimeui->hKL);
    return TRUE;
}

static void ImeWnd_OnDestroy(PIMEUI pimeui)
{
    HWND hwndUI = pimeui->hwndUI;

    if (IsWindow(hwndUI))
    {
        pimeui->fDestroy = TRUE;
        NtUserDestroyWindow(hwndUI);
    }

    pimeui->fShowStatus = pimeui->fDestroy = FALSE;
    pimeui->hwndUI = NULL;
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
          pimeui->spwnd = pWnd;
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

    if (pimeui->nCntInIMEProc > 0)
    {
        switch (msg)
        {
            case WM_IME_CHAR:
            case WM_IME_COMPOSITIONFULL:
            case WM_IME_CONTROL:
            case WM_IME_REQUEST:
            case WM_IME_SELECT:
            case WM_IME_SETCONTEXT:
            case WM_IME_STARTCOMPOSITION:
            case WM_IME_COMPOSITION:
            case WM_IME_ENDCOMPOSITION:
                return 0;

            case WM_IME_NOTIFY:
                // TODO:
                return 0;

            case WM_IME_SYSTEM:
                // TODO:
                return 0;

            default:
            {
                if (unicode)
                    return DefWindowProcW(hwnd, msg, wParam, lParam);
                return DefWindowProcA(hwnd, msg, wParam, lParam);
            }
        }
    }

    switch (msg)
    {
        case WM_CREATE:
            return (ImeWnd_OnCreate(pimeui, (LPCREATESTRUCT)lParam) ? 0 : -1);

        case WM_DESTROY:
            ImeWnd_OnDestroy(pimeui);
            break;

        case WM_NCDESTROY:
            HeapFree(GetProcessHeap(), 0, pimeui);
            SetWindowLongPtrW(hwnd, 0, 0);
            NtUserSetWindowFNID(hwnd, FNID_DESTROY);
            break;

        case WM_ERASEBKGND:
            return TRUE;

        case WM_PAINT:
            break;

        case WM_COPYDATA:
            // TODO:
            break;

        case WM_IME_STARTCOMPOSITION:
        case WM_IME_COMPOSITION:
        case WM_IME_ENDCOMPOSITION:
            // TODO:
            break;

        case WM_IME_CONTROL:
            // TODO:
            break;

        case WM_IME_NOTIFY:
            // TODO:
            break;

        case WM_IME_REQUEST:
            break;

        case WM_IME_SELECT:
            // TODO:
            break;

        case WM_IME_SETCONTEXT:
            // TODO:
            break;

        case WM_IME_SYSTEM:
            // TODO:
            break;

        default:
        {
            if (unicode)
                return DefWindowProcW(hwnd, msg, wParam, lParam);
            return DefWindowProcA(hwnd, msg, wParam, lParam);
        }
    }

    return 0;
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
