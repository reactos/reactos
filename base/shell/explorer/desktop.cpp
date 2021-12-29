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
private:
    
    CComPtr<ITrayWindow> m_Tray;
    
    HANDLE m_hEvent;
    HANDLE m_hThread;

    DWORD DesktopThreadProc();
    static DWORD WINAPI s_DesktopThreadProc(LPVOID lpParameter);
    
public:
    
    CDesktopThread();
    virtual ~CDesktopThread();

    HRESULT Initialize(ITrayWindow* pTray);
    void Destroy();
};

/*******************************************************************/

CDesktopThread::CDesktopThread() :
    m_Tray(NULL),
    m_hEvent(NULL),
    m_hThread(NULL)
{
}

CDesktopThread::~CDesktopThread()
{
    if (m_hThread)
    {
        CloseHandle(m_hThread);
    }

    if (m_hEvent)
    {
        CloseHandle(m_hEvent);
    }
}

HRESULT CDesktopThread::Initialize(ITrayWindow* pTray)
{
    HANDLE Handles[2];

    m_Tray = pTray;
    
    m_hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

    if (!m_hEvent)
    {
        return E_FAIL;
    }

    m_hThread = CreateThread(NULL, 0, s_DesktopThreadProc, (LPVOID)this, 0, NULL);

    if (!m_hThread)
    {   
        CloseHandle(m_hEvent);
        
        m_hEvent = NULL;
        
        return E_FAIL;
    }

    Handles[0] = m_hThread;
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

    return S_OK;
}

void CDesktopThread::Destroy()
{
    if (m_hThread)
    {
        DWORD dwExitCode;

        GetExitCodeThread(m_hThread, &dwExitCode);

        if (dwExitCode == STILL_ACTIVE)
        {
            TerminateThread(m_hThread, 0);
        }
    }
}

DWORD CDesktopThread::DesktopThreadProc()
{
    CComPtr<IShellDesktopTray> pSdt;
    
    HANDLE hDesktop;
    HRESULT hRet;

    OleInitialize(NULL);

    hRet = m_Tray->QueryInterface(IID_PPV_ARG(IShellDesktopTray, &pSdt));

    if (!SUCCEEDED(hRet))
    {
        return 1;
    }

    hDesktop = _SHCreateDesktop(pSdt);

    if (!hDesktop)
    {
        return 1;
    }

    if (!SetEvent(m_hEvent))
    {
        /* Failed to notify that we initialized successfully, kill ourselves
           to make the main thread wake up! */
        return 1;
    }

    _SHDesktopMessageLoop(hDesktop);

    OleUninitialize();

    return 0;
}

DWORD WINAPI CDesktopThread::s_DesktopThreadProc(LPVOID lpParameter)
{
    CDesktopThread* pDesktopThread = reinterpret_cast<CDesktopThread*>(lpParameter);

    return pDesktopThread->DesktopThreadProc();
}

/*******************************************************************/

HANDLE
DesktopCreateWindow(IN OUT ITrayWindow *Tray)
{
    CDesktopThread* pDesktopThread = new CDesktopThread();

    HRESULT hres = pDesktopThread->Initialize(Tray);

    if (FAILED_UNEXPECTEDLY(hres))
    {
        delete pDesktopThread;
        
        return NULL;
    }

    return pDesktopThread;
}

VOID
DesktopDestroyShellWindow(IN HANDLE hDesktop)
{
    CDesktopThread* pDesktopThread = reinterpret_cast<CDesktopThread*>(hDesktop);

    if (!pDesktopThread)
    {
        return;
    }

    pDesktopThread->Destroy();
    
    delete pDesktopThread;
}
