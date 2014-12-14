/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

class CDesktopThread
{
    HANDLE m_hEvent;
    HANDLE m_hDesktop;
    CComPtr<ITrayWindow> m_Tray;

    DWORD DesktopThreadProc()
    {
        CComPtr<IShellDesktopTray> pSdt;
        HANDLE hDesktop;
        HRESULT hRet;

        OleInitialize(NULL);

        hRet = m_Tray->QueryInterface(IID_PPV_ARG(IShellDesktopTray, &pSdt));
        if (!SUCCEEDED(hRet))
            return 1;

        hDesktop = _SHCreateDesktop(pSdt);
        if (hDesktop == NULL)
            return 1;

        if (!SetEvent(m_hEvent))
        {
            /* Failed to notify that we initialized successfully, kill ourselves
            to make the main thread wake up! */
            return 1;
        }

        _SHDesktopMessageLoop(hDesktop);

        /* FIXME: Properly rundown the main thread! */
        ExitProcess(0);

        return 0;
    }

    static DWORD CALLBACK s_DesktopThreadProc(IN OUT LPVOID lpParameter)
    {
        return reinterpret_cast<CDesktopThread*>(lpParameter)->DesktopThreadProc();
    }

public:
    CDesktopThread() :
        m_hEvent(NULL),
        m_hDesktop(NULL),
        m_Tray(NULL)
    {
    }

    HANDLE Initialize(IN OUT ITrayWindow *pTray)
    {
        HANDLE hThread;
        HANDLE Handles[2];

        m_Tray = pTray;

        m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!m_hEvent)
            return NULL;

        hThread = CreateThread(NULL, 0, s_DesktopThreadProc, (PVOID)this, 0, NULL);
        if (!hThread)
        {
            CloseHandle(m_hEvent);
            return NULL;
        }

        Handles[0] = hThread;
        Handles[1] = m_hEvent;

        for (;;)
        {
            DWORD WaitResult = MsgWaitForMultipleObjects(_countof(Handles), Handles, FALSE, INFINITE, QS_ALLEVENTS);
            if (WaitResult == WAIT_OBJECT_0 + _countof(Handles))
            {
                TrayProcessMessages(m_Tray);
            }
            else if (WaitResult != WAIT_FAILED && WaitResult != WAIT_OBJECT_0)
            {
                break;
            }
        }

        CloseHandle(hThread);
        CloseHandle(m_hEvent);

        // FIXME: Never assigned, will always return default value (NULL).
        return m_hDesktop;
    }

    void Destroy()
    {
        return;
    }

} * g_pDesktopWindowInstance;

HANDLE
DesktopCreateWindow(IN OUT ITrayWindow *Tray)
{
    if (!g_pDesktopWindowInstance)
    {
        g_pDesktopWindowInstance = new CDesktopThread();
    }
    
    if (!g_pDesktopWindowInstance)
        return NULL;

    return g_pDesktopWindowInstance->Initialize(Tray);
}

VOID
DesktopDestroyShellWindow(IN HANDLE hDesktop)
{
    if (g_pDesktopWindowInstance)
    {
        g_pDesktopWindowInstance->Destroy();
    }
}
