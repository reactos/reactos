/*
 *
 * PROJECT:         fontext.dll
 * FILE:            dll/shellext/fontext/fontext.c
 * PURPOSE:         fontext.dll
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 * UPDATE HISTORY:
 *      10-06-2008  Created
 */

#include "fontext.h"

static HINSTANCE hInstance;

HRESULT WINAPI
DllCanUnloadNow(VOID)
{
    DPRINT1("DllCanUnloadNow() stubs\n");
    return S_OK;
}

HRESULT WINAPI
DllGetClassObject(REFCLSID rclsid,
                  REFIID riid,
                  LPVOID *ppv)
{
    DPRINT1("DllGetClassObject() stubs\n");
    return S_OK;
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD dwReason,
        LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hInstance = hinstDLL;
            DisableThreadLibraryCalls(hInstance);
            break;
    }

    return TRUE;
}
