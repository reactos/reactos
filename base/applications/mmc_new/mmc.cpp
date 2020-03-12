/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Entrypoint of the application
 * COPYRIGHT:   Copyright 2006-2007 Thomas Weidenmueller
 *              Copyright 2017-2020 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"


class CMMCModule : public CAtlExeModuleT<CMMCModule>
{
public:
    CMainWnd m_MainWnd;
    HACCEL m_hAccel;

    HRESULT PreMessageLoop(int nShowCmd)
    {
        HRESULT hr = RegisterClassObjects(CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        m_hAccel = LoadAcceleratorsW(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCEW(IDA_MMC));
        if (!m_hAccel)
        {
            DPRINT1("ERROR: Failed to load accelerators\n");
            return E_FAIL;
        }

        WNDPROC ret = NULL;
        if (!CConsoleWnd::GetWndClassInfo().Register(&ret))
        {
            DPRINT1("ERROR: Failed to register CConsoleWnd class\n");
            return E_FAIL;
        }

        LPCWSTR Filename = NULL;

        if (!m_MainWnd.Create(NULL, NULL, L"",
                                WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_WINDOWEDGE,
                                0U, (LPVOID)Filename))
        {
            DPRINT1("ERROR: Failed to create m_MainWnd\n");
            return E_FAIL;
        }

        m_MainWnd.ShowWindow(nShowCmd);
        return S_OK;
    }


    void RunMessageLoop()
    {
        MSG msg;
        BOOL bRet;
        while ((bRet = GetMessageW(&msg, NULL, 0, 0)) != 0)
        {
            if (bRet == -1)
            {
                DPRINT1("ERROR: GetMessage\n");
                break;
            }
            if (!TranslateMDISysAccel(m_MainWnd.m_MDIClient, &msg) &&
                !TranslateAcceleratorW(m_MainWnd, m_hAccel, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
    }

    // We want apartment threading.
    // The default UninitializeCom works fine, so we do not need to implement that
    static HRESULT InitializeCom()
    {
        return ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    }
};

static CMMCModule g_AtlModule;

int WINAPI
wWinMain(HINSTANCE /*hInstance*/,
          HINSTANCE /*hPrevInstance*/,
          LPWSTR /*lpCmdLine*/,
          int nCmdShow)
{
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES | ICC_COOL_CLASSES };

    if (GetEnvironmentVariableA("I_REALIZE_MMC_NEW_IS_NOT_READY_YET", NULL, 0) == 0)
    {
        MessageBoxW(NULL, L"This application is not ready for use yet.", L"mmc_new", MB_OK);
        return -1;
    }

    if (!InitCommonControlsEx(&icc))
    {
        DPRINT1("ERROR: InitCommonControlsEx\n");
    }

    return g_AtlModule.WinMain(nCmdShow);
}
