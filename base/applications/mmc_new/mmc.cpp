/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Entrypoint of the application
 * COPYRIGHT:   Copyright 2006-2007 Thomas Weidenmueller
 *              Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"


class CMMCModule : public CAtlExeModuleT<CMMCModule>
{
public:
    CMainWnd m_MainWnd;

    HRESULT PreMessageLoop(int nShowCmd)
    {
        HRESULT hr = RegisterClassObjects(CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE);
        if (SUCCEEDED(hr))
        {
            WNDPROC ret = NULL;
            if (!CConsoleWnd::GetWndClassInfo().Register(&ret))
                return E_FAIL;

            LPCWSTR Filename = NULL;

            if (!m_MainWnd.Create(NULL, NULL, TEXT(""),
                                  WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_WINDOWEDGE,
                                  0U, (LPVOID)Filename))
                return E_FAIL;
            
            m_MainWnd.ShowWindow(nShowCmd);
        }
        return hr;
    }


    void RunMessageLoop()
    {
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            if (!TranslateMDISysAccel(m_MainWnd.m_MDIClient, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    static HRESULT InitializeCom()
    {
        return ::CoInitialize(NULL);
    }
};

static CMMCModule g_AtlModule;

int WINAPI
_tWinMain(HINSTANCE /*hInstance*/,
          HINSTANCE /*hPrevInstance*/,
          LPTSTR /*lpCmdLine*/,
          int nCmdShow)
{
    if (GetEnvironmentVariableA("I_REALIZE_MMC_NEW_IS_NOT_READY_YET", NULL, 0) == 0)
    {
        MessageBoxW(NULL, L"This application is not ready for use yet.", L"mmc_new", MB_OK);
        return -1;
    }


    InitCommonControls();

    return g_AtlModule.WinMain(nCmdShow);
}
