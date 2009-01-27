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
    HIMC (WINAPI* pImmAssociateContext) (HWND, HIMC);
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
BOOL WINAPI IMM_ImmIsIME(HKL hKL)
{
    return 0;
}

/* See comment for IMM_ImmIsIME */
HIMC WINAPI IMM_ImmAssociateContext(HWND hwnd, HIMC himc)
{
    return 0;
}

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

    pImmApiTable->pImmAssociateContext = (HIMC (WINAPI*)(HWND, HIMC)) GetProcAddress(hImmInstance, "ImmAssociateContext");
    if (!pImmApiTable->pImmAssociateContext)
        pImmApiTable->pImmAssociateContext = IMM_ImmAssociateContext;

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
