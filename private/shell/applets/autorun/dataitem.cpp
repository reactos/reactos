#include <windows.h>
#include <shlwapi.h>
#include <commctrl.h>
#include "dataitem.h"
#include "resource.h"
#include "autorun.h"

#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))

CDataItem::CDataItem()
{
    m_pszTitle = m_pszMenuName = m_pszDescription = m_pszCmdLine = m_pszArgs = NULL;
    m_dwFlags = 0;
    m_chAccel = NULL;
}

CDataItem::~CDataItem()
{
    if ( m_pszTitle )
        delete [] m_pszTitle;
    if ( m_pszMenuName )
        delete [] m_pszMenuName;
    if ( m_pszDescription )
        delete [] m_pszDescription;
    if ( m_pszCmdLine )
        delete [] m_pszCmdLine;
    if ( m_pszArgs )
        delete [] m_pszArgs;
}

BOOL CDataItem::SetData( LPTSTR szTitle, LPTSTR szMenu, LPTSTR szDesc, LPTSTR szCmd, LPTSTR szArgs, DWORD dwFlags, int iImgIndex )
{
    TCHAR * psz;

    // This function should only be called once or else we will leak like a, like a, a thing that leaks a lot.
    ASSERT( NULL==m_pszTitle && NULL==m_pszMenuName && NULL==m_pszDescription && NULL==m_pszCmdLine && NULL==m_pszArgs );

    m_pszTitle = new TCHAR[lstrlen(szTitle)+1];
    lstrcpy( m_pszTitle, szTitle );

    if ( szMenu )
    {
        // menuname is allowed to remain NULL.  This is only used if you want the
        // text on the menu item to be different than the description. This could
        // be useful for localization where a shortened name might be required.
        m_pszMenuName = new TCHAR[lstrlen(szMenu)+1];
        lstrcpy( m_pszMenuName, szMenu );

        psz = StrChr(szMenu, TEXT('&'));
        if ( psz )
            m_chAccel = *(CharNext(psz));
    }

    m_pszDescription = new TCHAR[lstrlen(szDesc)+1];
    lstrcpy( m_pszDescription, szDesc );

    m_pszCmdLine = new TCHAR[lstrlen(szCmd)+1];
    lstrcpy( m_pszCmdLine, szCmd );

    if ( szArgs )
    {
        // Some commands don't have any args so this can remain NULL.  This is only used
        // if the executable requires arguments.
        m_pszArgs = new TCHAR[lstrlen(szArgs)+1];
        lstrcpy( m_pszArgs, szArgs );
    }

    m_dwFlags = dwFlags;
    m_iImage = iImgIndex;

    return TRUE;
}

BOOL CDataItem::Invoke(HWND hwnd)
{
    BOOL fResult;
    TCHAR szCmdLine[MAX_PATH*2];
    PROCESS_INFORMATION ei;
    STARTUPINFO si = {0};
    si.cb = sizeof(si);

    lstrcpy( szCmdLine, m_pszCmdLine );
    if ( m_pszArgs )
    {
        strcat( szCmdLine, TEXT(" ") );
        strcat( szCmdLine, m_pszArgs );
    }

    fResult = CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &ei);
    if (fResult)
    {
        if (NULL != ei.hProcess)
        {
            DWORD dwObject;

            // passing in a NULL HWND is used as a signal not to wait in this inner loop.
            while (hwnd)
            {
                dwObject = MsgWaitForMultipleObjects(1, &ei.hProcess, FALSE, INFINITE, QS_ALLINPUT);
                
                if (WAIT_OBJECT_0 == dwObject)
                {
                    break;
                }
                else if (WAIT_OBJECT_0+1 == dwObject)
                {
                    MSG msg;

                    while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
                    {
                        if ( WM_QUIT == msg.message )
                        {
                            CloseHandle(ei.hProcess);
                            return fResult;
                        }
                        else
                        {
                            GetMessage(&msg, NULL, 0, 0);

                            // IsDialogMessage cannot understand the concept of ownerdraw default pushbuttons.  It treats
                            // these attributes as mutually exclusive.  As a result, we handle this ourselves.  We want
                            // whatever control has focus to act as the default pushbutton.
                            if ( (WM_KEYDOWN == msg.message) && (VK_RETURN == msg.wParam) )
                            {
                                HWND hwndFocus = GetFocus();
                                if ( hwndFocus )
                                {
                                    SendMessage(hwnd, WM_COMMAND, MAKELONG(GetDlgCtrlID(hwndFocus), BN_CLICKED), (LPARAM)hwndFocus);
                                }
                                continue;
                            }

                            if ( IsDialogMessage(hwnd, &msg) )
                                continue;

                            TranslateMessage(&msg);
                            DispatchMessage(&msg);
                        }
                    }
                }
            }

            if ( !hwnd )
            {
                // A NULL hwnd means we were called in the mode by which we execute the item and then immediately
                // exit.  If our process exits before the other process is ready it'll end up in the wrong place
                // in the z-order.  To prevent this, when we're in "exit when done" mode we need to wait for the
                // process we created to be ready.  The way to do this is to call WaitForInputIdle.  This is really
                // only needed on NT5 and above due to the new "rude window activation" stuff, but since this API
                // is available all the way back to NT 3.1 we simply call it blindly.
                WaitForInputIdle(ei.hProcess, 20*1000);     // we wait a maximum of 20 seconds
            }

            CloseHandle(ei.hProcess);
        }
    }
    else
    {
        // do something if we fail to create a process?
    }

    return fResult;
}

