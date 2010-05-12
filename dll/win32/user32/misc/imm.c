/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            dll/win32/user32/misc/imm.c
 * PURPOSE:         User32.dll Imm functions
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 * UPDATE HISTORY:
 *      01/27/2009  Created
 */

#include <user32.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);


typedef struct
{
    BOOL (WINAPI* pImmIsIME) (HKL);
    LRESULT (WINAPI* pImmEscapeA) (HKL, HIMC, UINT, LPVOID);
    LRESULT (WINAPI* pImmEscapeW) (HKL, HIMC, UINT, LPVOID);
    LONG (WINAPI* pImmGetCompositionStringA) (HIMC, DWORD, LPVOID, DWORD);
    LONG (WINAPI* pImmGetCompositionStringW) (HIMC, DWORD, LPVOID, DWORD);
    BOOL (WINAPI* pImmGetCompositionFontA) (HIMC, LPLOGFONTA);
    BOOL (WINAPI* pImmGetCompositionFontW) (HIMC, LPLOGFONTW);
    BOOL (WINAPI* pImmSetCompositionFontA)(HIMC, LPLOGFONTA);
    BOOL (WINAPI* pImmSetCompositionFontW)(HIMC, LPLOGFONTW);
    BOOL (WINAPI* pImmGetCompositionWindow) (HIMC, LPCOMPOSITIONFORM);
    BOOL (WINAPI* pImmSetCompositionWindow) (HIMC, LPCOMPOSITIONFORM);
    HIMC (WINAPI* pImmAssociateContext) (HWND, HIMC);
    BOOL (WINAPI* pImmReleaseContext) (HWND, HIMC);
    HIMC (WINAPI* pImmGetContext) (HWND);
    HWND (WINAPI* pImmGetDefaultIMEWnd) (HWND);
    BOOL (WINAPI* pImmNotifyIME) (HIMC, DWORD, DWORD, DWORD);
} Imm32ApiTable;

Imm32ApiTable *pImmApiTable = {0};
HINSTANCE hImmInstance = NULL;


static BOOL IsInitialized()
{
    return (hImmInstance != NULL) ? TRUE : FALSE;
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

/*
 * @unimplemented
 */
BOOL WINAPI User32InitializeImmEntryTable(DWORD dwUnknown)
{
    UNIMPLEMENTED;

    if (dwUnknown != 0x19650412) /* FIXME */
        return FALSE;

    if (IsInitialized())
        return TRUE;

    hImmInstance = LoadLibraryW(L"imm32.dll");
    if (!hImmInstance)
        return FALSE;

    ZeroMemory(pImmApiTable, sizeof(Imm32ApiTable));

    pImmApiTable->pImmIsIME = (BOOL (WINAPI*)(HKL)) GetProcAddress(hImmInstance, "ImmIsIME");
    if (!pImmApiTable->pImmIsIME)
        pImmApiTable->pImmIsIME = IMM_ImmIsIME;

    pImmApiTable->pImmEscapeA = (LRESULT (WINAPI*)(HKL, HIMC, UINT, LPVOID)) GetProcAddress(hImmInstance, "ImmEscapeA");
    if (!pImmApiTable->pImmEscapeA)
        pImmApiTable->pImmEscapeA = IMM_ImmEscapeAW;

    pImmApiTable->pImmEscapeW = (LRESULT (WINAPI*)(HKL, HIMC, UINT, LPVOID)) GetProcAddress(hImmInstance, "ImmEscapeW");
    if (!pImmApiTable->pImmEscapeW)
        pImmApiTable->pImmEscapeW = IMM_ImmEscapeAW;

    pImmApiTable->pImmGetCompositionStringA = (LONG (WINAPI*)(HIMC, DWORD, LPVOID, DWORD)) GetProcAddress(hImmInstance, "ImmGetCompositionStringA");
    if (!pImmApiTable->pImmGetCompositionStringA)
        pImmApiTable->pImmGetCompositionStringA = IMM_ImmGetCompositionStringAW;

    pImmApiTable->pImmGetCompositionStringW = (LONG (WINAPI*)(HIMC, DWORD, LPVOID, DWORD)) GetProcAddress(hImmInstance, "ImmGetCompositionStringW");
    if (!pImmApiTable->pImmGetCompositionStringW)
        pImmApiTable->pImmGetCompositionStringW = IMM_ImmGetCompositionStringAW;

    pImmApiTable->pImmGetCompositionFontA = (BOOL (WINAPI*)(HIMC, LPLOGFONTA)) GetProcAddress(hImmInstance, "ImmGetCompositionFontA");
    if (!pImmApiTable->pImmGetCompositionFontA)
        pImmApiTable->pImmGetCompositionFontA = IMM_ImmGetCompositionFontA;

    pImmApiTable->pImmGetCompositionFontW = (BOOL (WINAPI*)(HIMC, LPLOGFONTW)) GetProcAddress(hImmInstance, "ImmGetCompositionFontW");
    if (!pImmApiTable->pImmGetCompositionFontW)
        pImmApiTable->pImmGetCompositionFontW = IMM_ImmGetCompositionFontW;

    pImmApiTable->pImmSetCompositionFontA = (BOOL (WINAPI*)(HIMC, LPLOGFONTA)) GetProcAddress(hImmInstance, "ImmSetCompositionFontA");
    if (!pImmApiTable->pImmSetCompositionFontA)
        pImmApiTable->pImmSetCompositionFontA = IMM_ImmSetCompositionFontA;

    pImmApiTable->pImmSetCompositionFontW = (BOOL (WINAPI*)(HIMC, LPLOGFONTW)) GetProcAddress(hImmInstance, "ImmSetCompositionFontW");
    if (!pImmApiTable->pImmSetCompositionFontW)
        pImmApiTable->pImmSetCompositionFontW = IMM_ImmSetCompositionFontW;

    pImmApiTable->pImmGetCompositionWindow = (BOOL (WINAPI*)(HIMC, LPCOMPOSITIONFORM)) GetProcAddress(hImmInstance, "ImmGetCompositionWindow");
    if (!pImmApiTable->pImmGetCompositionWindow)
        pImmApiTable->pImmGetCompositionWindow = IMM_ImmSetGetCompositionWindow;

    pImmApiTable->pImmSetCompositionWindow = (BOOL (WINAPI*)(HIMC, LPCOMPOSITIONFORM)) GetProcAddress(hImmInstance, "ImmSetCompositionWindow");
    if (!pImmApiTable->pImmSetCompositionWindow)
        pImmApiTable->pImmSetCompositionWindow = IMM_ImmSetGetCompositionWindow;

    pImmApiTable->pImmAssociateContext = (HIMC (WINAPI*)(HWND, HIMC)) GetProcAddress(hImmInstance, "ImmAssociateContext");
    if (!pImmApiTable->pImmAssociateContext)
        pImmApiTable->pImmAssociateContext = IMM_ImmAssociateContext;

    pImmApiTable->pImmReleaseContext = (BOOL (WINAPI*)(HWND, HIMC)) GetProcAddress(hImmInstance, "ImmReleaseContext");
    if (!pImmApiTable->pImmReleaseContext)
        pImmApiTable->pImmReleaseContext = IMM_ImmReleaseContext;

    pImmApiTable->pImmGetContext = (HIMC (WINAPI*)(HWND)) GetProcAddress(hImmInstance, "ImmGetContext");
    if (!pImmApiTable->pImmGetContext)
        pImmApiTable->pImmGetContext = IMM_ImmGetContext;

    pImmApiTable->pImmGetDefaultIMEWnd = (HWND (WINAPI*)(HWND)) GetProcAddress(hImmInstance, "ImmGetDefaultIMEWnd");
    if (!pImmApiTable->pImmGetDefaultIMEWnd)
        pImmApiTable->pImmGetDefaultIMEWnd = IMM_ImmGetDefaultIMEWnd;

    pImmApiTable->pImmNotifyIME = (BOOL (WINAPI*)(HIMC, DWORD, DWORD, DWORD)) GetProcAddress(hImmInstance, "ImmNotifyIME");
    if (!pImmApiTable->pImmNotifyIME)
        pImmApiTable->pImmNotifyIME = IMM_ImmNotifyIME;

    /*
     *  TODO: Load more functions from imm32.dll
     *  Function like IMPSetIMEW, IMPQueryIMEW etc. call functions
     *  from imm32.dll through pointers in the structure pImmApiTable.
     *  I do not know whether it is necessary to initialize a table
     *  of functions to load user32 (DLL_PROCESS_ATTACH)
     */

    return TRUE;
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
 * @unimplemented
 */
UINT
WINAPI
WINNLSGetIMEHotkey(HWND hwnd)
{
    UNIMPLEMENTED;
    return FALSE;
}
