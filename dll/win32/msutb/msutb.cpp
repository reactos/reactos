/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Supporting Language Bar back-end
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(msutb);

HINSTANCE g_hInst = NULL;
UINT g_wmTaskbarCreated = 0;
UINT g_uACP = CP_ACP;
DWORD g_dwOSInfo = 0;
CRITICAL_SECTION g_cs;

class CMsUtbModule : public CComModule
{
};

BEGIN_OBJECT_MAP(ObjectMap)
    //OBJECT_ENTRY(CLSID_..., C...)
END_OBJECT_MAP()

CMsUtbModule gModule;

/***********************************************************************
 *              GetLibTls (MSUTB.@)
 */
EXTERN_C LPVOID WINAPI
GetLibTls(VOID)
{
    FIXME("stub:()\n");
    return NULL;
}

/***********************************************************************
 *              GetPopupTipbar (MSUTB.@)
 */
EXTERN_C BOOL WINAPI
GetPopupTipbar(HWND hWnd, DWORD dwFlags)
{
    FIXME("stub:(%p, 0x%lX)\n", hWnd, dwFlags);
    return FALSE;
}

/***********************************************************************
 *              SetRegisterLangBand (MSUTB.@)
 */
EXTERN_C HRESULT WINAPI
SetRegisterLangBand(BOOL bRegister)
{
    FIXME("stub:(%d)\n", bRegister);
    return E_NOTIMPL;
}

/***********************************************************************
 *              ClosePopupTipbar (MSUTB.@)
 */
EXTERN_C VOID WINAPI
ClosePopupTipbar(VOID)
{
    FIXME("stub:()\n");
}

/***********************************************************************
 *              DllRegisterServer (MSUTB.@)
 */
STDAPI DllRegisterServer(VOID)
{
    TRACE("()\n");
    return gModule.DllRegisterServer(FALSE);
}

/***********************************************************************
 *              DllUnregisterServer (MSUTB.@)
 */
STDAPI DllUnregisterServer(VOID)
{
    TRACE("()\n");
    return gModule.DllUnregisterServer(FALSE);
}

/***********************************************************************
 *              DllCanUnloadNow (MSUTB.@)
 */
STDAPI DllCanUnloadNow(VOID)
{
    TRACE("()\n");
    return gModule.DllCanUnloadNow();
}

/***********************************************************************
 *              DllGetClassObject (MSUTB.@)
 */
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("()\n");
    return gModule.DllGetClassObject(rclsid, riid, ppv);
}

/**
 * @unimplemented
 */
VOID InitUIFLib(VOID)
{
    //FIXME
}

/**
 * @unimplemented
 */
VOID DoneUIFLib(VOID)
{
    //FIXME
}

/**
 * @implemented
 */
HRESULT APIENTRY
MsUtbCoCreateInstance(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD dwClsContext,
    REFIID iid,
    LPVOID *ppv)
{
    if (IsEqualCLSID(rclsid, CLSID_TF_CategoryMgr))
        return TF_CreateCategoryMgr((ITfCategoryMgr**)ppv);
    if (IsEqualCLSID(rclsid, CLSID_TF_DisplayAttributeMgr))
        return TF_CreateDisplayAttributeMgr((ITfDisplayAttributeMgr **)ppv);
    return cicRealCoCreateInstance(rclsid, pUnkOuter, dwClsContext, iid, ppv);
}

/**
 * @unimplemented
 */
BOOL ProcessAttach(HINSTANCE hinstDLL)
{
    ::InitializeCriticalSectionAndSpinCount(&g_cs, 0);

    g_hInst = hinstDLL;

    cicGetOSInfo(&g_uACP, &g_dwOSInfo);

    TFInitLib(MsUtbCoCreateInstance);
    InitUIFLib();

    //CTrayIconWnd::RegisterClassW(); //FIXME

    g_wmTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");

    gModule.Init(ObjectMap, hinstDLL, NULL);
    ::DisableThreadLibraryCalls(hinstDLL);

    return TRUE;
}

/**
 * @implemented
 */
VOID ProcessDetach(HINSTANCE hinstDLL)
{
    DoneUIFLib();
    TFUninitLib();
    ::DeleteCriticalSection(&g_cs);
    gModule.Term();
}

/**
 * @implemented
 */
EXTERN_C BOOL WINAPI
DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD dwReason,
    _Inout_opt_ LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            TRACE("(%p, %lu, %p)\n", hinstDLL, dwReason, lpvReserved);
            if (!ProcessAttach(hinstDLL))
            {
                ProcessDetach(hinstDLL);
                return FALSE;
            }
            break;
        }
        case DLL_PROCESS_DETACH:
        {
            ProcessDetach(hinstDLL);
            break;
        }
    }
    return TRUE;
}
