/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/misc/imm.c
 * PURPOSE:         User32.dll Imm functions
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 * UPDATE HISTORY:
 *      01/27/2009  Created
 */

#include <user32.h>

#include <winnls32.h>

#include <wine/debug.h>
#include <strsafe.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

#define IMM_INIT_MAGIC 0x19650412


Imm32ApiTable gImmApiEntries = {0};
HINSTANCE ghImm32 = NULL;
BOOL bImmInitializing = FALSE;
BOOL ImmApiTableZero = TRUE;

HRESULT WINAPI GetImmFileName(PWSTR lpBuffer, UINT uSize)
{
  UINT length;
  STRSAFE_LPWSTR Safe = lpBuffer;

  length = GetSystemDirectoryW(lpBuffer, uSize);
  if ( length && length < uSize )
  {
    StringCchCatW(Safe, uSize, L"\\");
    return StringCchCatW(Safe, uSize, L"IMM32.DLL");
  }
  return StringCchCopyW(Safe, uSize, L"IMM32.DLL");
}  

/*
 *  This function should not be implemented, it is used,
 *  if you can not load function from imm32.dll
 */
BOOL WINAPI IMM_ImmIsIME(HKL hKL) { return 0; }
HIMC WINAPI IMM_ImmAssociateContext(HWND hwnd, HIMC himc) { return 0; }
BOOL WINAPI IMM_ImmReleaseContext(HWND hwnd, HIMC himc) { return 0; }
LRESULT WINAPI IMM_ImmEscapeAW(HKL hkl, HIMC himc, UINT uint, LPVOID lpvoid) { return 0; }
LONG WINAPI IMM_ImmGetCompositionStringAW(HIMC himc, DWORD dword1, LPVOID lpvoid, DWORD dword2) { return 0; }
BOOL WINAPI IMM_ImmGetCompositionFontA(HIMC himc, LPLOGFONTA lplf) { return 0; }
BOOL WINAPI IMM_ImmGetCompositionFontW(HIMC himc, LPLOGFONTW lplf) { return 0; }
BOOL WINAPI IMM_ImmSetCompositionFontA(HIMC himc, LPLOGFONTA lplf) { return 0; }
BOOL WINAPI IMM_ImmSetCompositionFontW(HIMC himc, LPLOGFONTW lplf) { return 0; }
BOOL WINAPI IMM_ImmSetGetCompositionWindow(HIMC himc, LPCOMPOSITIONFORM lpcf) { return 0; }
HIMC WINAPI IMM_ImmGetContext(HWND hwnd) { return 0; }
HWND WINAPI IMM_ImmGetDefaultIMEWnd(HWND hwnd) { return 0; }
BOOL WINAPI IMM_ImmNotifyIME(HIMC himc, DWORD dword1, DWORD dword2, DWORD dword3) { return 0; }
BOOL WINAPI IMM_ImmRegisterClient(PVOID ptr, HINSTANCE hMod) { return 0; }
UINT WINAPI IMM_ImmProcessKey(HWND hwnd, HKL hkl, UINT Vk, LPARAM lParam, DWORD HotKey) { return 0; }

/*
 * @unimplemented
 */
BOOL WINAPI IntInitializeImmEntryTable(VOID)
{
    WCHAR ImmFile[MAX_PATH];
    HMODULE imm32 = ghImm32;;

    if (gImmApiEntries.pImmIsIME != 0)
    {
       ERR("Imm Api Table Init 1\n");
       return TRUE;
    }

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

    if (ImmApiTableZero)
    {
       ImmApiTableZero = FALSE;
       ZeroMemory(&gImmApiEntries, sizeof(Imm32ApiTable));
    }

    gImmApiEntries.pImmIsIME = (BOOL (WINAPI*)(HKL)) GetProcAddress(imm32, "ImmIsIME");
    if (!gImmApiEntries.pImmIsIME)
        gImmApiEntries.pImmIsIME = IMM_ImmIsIME;

    gImmApiEntries.pImmEscapeA = (LRESULT (WINAPI*)(HKL, HIMC, UINT, LPVOID)) GetProcAddress(imm32, "ImmEscapeA");
    if (!gImmApiEntries.pImmEscapeA)
        gImmApiEntries.pImmEscapeA = IMM_ImmEscapeAW;

    gImmApiEntries.pImmEscapeW = (LRESULT (WINAPI*)(HKL, HIMC, UINT, LPVOID)) GetProcAddress(imm32, "ImmEscapeW");
    if (!gImmApiEntries.pImmEscapeW)
        gImmApiEntries.pImmEscapeW = IMM_ImmEscapeAW;

    gImmApiEntries.pImmGetCompositionStringA = (LONG (WINAPI*)(HIMC, DWORD, LPVOID, DWORD)) GetProcAddress(imm32, "ImmGetCompositionStringA");
    if (!gImmApiEntries.pImmGetCompositionStringA)
        gImmApiEntries.pImmGetCompositionStringA = IMM_ImmGetCompositionStringAW;

    gImmApiEntries.pImmGetCompositionStringW = (LONG (WINAPI*)(HIMC, DWORD, LPVOID, DWORD)) GetProcAddress(imm32, "ImmGetCompositionStringW");
    if (!gImmApiEntries.pImmGetCompositionStringW)
        gImmApiEntries.pImmGetCompositionStringW = IMM_ImmGetCompositionStringAW;

    gImmApiEntries.pImmGetCompositionFontA = (BOOL (WINAPI*)(HIMC, LPLOGFONTA)) GetProcAddress(imm32, "ImmGetCompositionFontA");
    if (!gImmApiEntries.pImmGetCompositionFontA)
        gImmApiEntries.pImmGetCompositionFontA = IMM_ImmGetCompositionFontA;

    gImmApiEntries.pImmGetCompositionFontW = (BOOL (WINAPI*)(HIMC, LPLOGFONTW)) GetProcAddress(imm32, "ImmGetCompositionFontW");
    if (!gImmApiEntries.pImmGetCompositionFontW)
        gImmApiEntries.pImmGetCompositionFontW = IMM_ImmGetCompositionFontW;

    gImmApiEntries.pImmSetCompositionFontA = (BOOL (WINAPI*)(HIMC, LPLOGFONTA)) GetProcAddress(imm32, "ImmSetCompositionFontA");
    if (!gImmApiEntries.pImmSetCompositionFontA)
        gImmApiEntries.pImmSetCompositionFontA = IMM_ImmSetCompositionFontA;

    gImmApiEntries.pImmSetCompositionFontW = (BOOL (WINAPI*)(HIMC, LPLOGFONTW)) GetProcAddress(imm32, "ImmSetCompositionFontW");
    if (!gImmApiEntries.pImmSetCompositionFontW)
        gImmApiEntries.pImmSetCompositionFontW = IMM_ImmSetCompositionFontW;

    gImmApiEntries.pImmGetCompositionWindow = (BOOL (WINAPI*)(HIMC, LPCOMPOSITIONFORM)) GetProcAddress(imm32, "ImmGetCompositionWindow");
    if (!gImmApiEntries.pImmGetCompositionWindow)
        gImmApiEntries.pImmGetCompositionWindow = IMM_ImmSetGetCompositionWindow;

    gImmApiEntries.pImmSetCompositionWindow = (BOOL (WINAPI*)(HIMC, LPCOMPOSITIONFORM)) GetProcAddress(imm32, "ImmSetCompositionWindow");
    if (!gImmApiEntries.pImmSetCompositionWindow)
        gImmApiEntries.pImmSetCompositionWindow = IMM_ImmSetGetCompositionWindow;

    gImmApiEntries.pImmAssociateContext = (HIMC (WINAPI*)(HWND, HIMC)) GetProcAddress(imm32, "ImmAssociateContext");
    if (!gImmApiEntries.pImmAssociateContext)
        gImmApiEntries.pImmAssociateContext = IMM_ImmAssociateContext;

    gImmApiEntries.pImmReleaseContext = (BOOL (WINAPI*)(HWND, HIMC)) GetProcAddress(imm32, "ImmReleaseContext");
    if (!gImmApiEntries.pImmReleaseContext)
        gImmApiEntries.pImmReleaseContext = IMM_ImmReleaseContext;

    gImmApiEntries.pImmGetContext = (HIMC (WINAPI*)(HWND)) GetProcAddress(imm32, "ImmGetContext");
    if (!gImmApiEntries.pImmGetContext)
        gImmApiEntries.pImmGetContext = IMM_ImmGetContext;

    gImmApiEntries.pImmGetDefaultIMEWnd = (HWND (WINAPI*)(HWND)) GetProcAddress(imm32, "ImmGetDefaultIMEWnd");
    if (!gImmApiEntries.pImmGetDefaultIMEWnd)
        gImmApiEntries.pImmGetDefaultIMEWnd = IMM_ImmGetDefaultIMEWnd;

    gImmApiEntries.pImmNotifyIME = (BOOL (WINAPI*)(HIMC, DWORD, DWORD, DWORD)) GetProcAddress(imm32, "ImmNotifyIME");
    if (!gImmApiEntries.pImmNotifyIME)
        gImmApiEntries.pImmNotifyIME = IMM_ImmNotifyIME;

    /*
     *  TODO: Load more functions from imm32.dll
     *  Function like IMPSetIMEW, IMPQueryIMEW etc. call functions
     *  from imm32.dll through pointers in the structure gImmApiEntries.
     *  I do not know whether it is necessary to initialize a table
     *  of functions to load user32 (DLL_PROCESS_ATTACH)
     */

    gImmApiEntries.pImmRegisterClient = (BOOL (WINAPI*)(PVOID, HINSTANCE)) GetProcAddress(imm32, "ImmRegisterClient");
    if (!gImmApiEntries.pImmRegisterClient)
        gImmApiEntries.pImmRegisterClient = IMM_ImmRegisterClient;

    gImmApiEntries.pImmProcessKey = (UINT (WINAPI*)(HWND, HKL, UINT, LPARAM, DWORD)) GetProcAddress(imm32, "ImmProcessKey");
    if (!gImmApiEntries.pImmProcessKey)
        gImmApiEntries.pImmProcessKey = IMM_ImmProcessKey;

    return TRUE;
}

BOOL WINAPI InitializeImmEntryTable(VOID)
{
  bImmInitializing = TRUE;
  return IntInitializeImmEntryTable();
}

BOOL WINAPI User32InitializeImmEntryTable(DWORD magic)
{
    TRACE("(%x)\n", magic);

    if (magic != IMM_INIT_MAGIC)
        return FALSE;

    if (gImmApiEntries.pImmIsIME != 0)
    {
       ERR("Imm Api Table Init 2\n");
       return TRUE;
    }

    IntInitializeImmEntryTable();

    if (ghImm32 == NULL && !bImmInitializing)
    {
       WCHAR ImmFile[MAX_PATH];
       ERR("IMM32 not installed!\n");
       GetImmFileName(ImmFile, sizeof(ImmFile));
       ERR("File %ws\n",ImmFile);
       ghImm32 = LoadLibraryW(ImmFile);
       if (ghImm32 == NULL)
       {
          ERR("Did not load! 2\n");
          return FALSE;
       } 
    }
#if 0 // For real Imm32.dll testing!!!!
    if (ghImm32 && !gImmApiEntries.pImmRegisterClient(&gSharedInfo, ghImm32))
    {
       ERR("Wine is stubed!\n");
    }
#endif
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
 * @unimplemented
 */
BOOL
WINAPI
IMPSetIMEW(HWND hwnd, LPIMEPROW ime)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
IMPQueryIMEW(LPIMEPROW ime)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
IMPGetIMEW(HWND hwnd, LPIMEPROW ime)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
IMPSetIMEA(HWND hwnd, LPIMEPROA ime)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
IMPQueryIMEA(LPIMEPROA ime)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
IMPGetIMEA(HWND hwnd, LPIMEPROA ime)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
LRESULT
WINAPI
SendIMEMessageExW(HWND hwnd, LPARAM lparam)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
LRESULT
WINAPI
SendIMEMessageExA(HWND hwnd, LPARAM lparam)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
WINNLSEnableIME(HWND hwnd, BOOL enable)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
WINNLSGetEnableStatus(HWND hwnd)
{
    UNIMPLEMENTED;
    return FALSE;
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
