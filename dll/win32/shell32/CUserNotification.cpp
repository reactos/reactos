/*
 * Copyright 2018 Hermes Belusca-Maito
 *
 * Pass on icon notification messages to the systray implementation
 * in the currently running shell.
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

#include <mmsystem.h>
#undef PlaySound

WINE_DEFAULT_DEBUG_CHANNEL(shell_notify);


/* Use Windows-compatible window callback message */
#define WM_TRAYNOTIFY   (WM_USER + 100)

/* Notification icon ID */
#define ID_NOTIFY_ICON  0

/* Balloon timers */
#define ID_BALLOON_TIMEOUT      1
#define ID_BALLOON_DELAYREMOVE  2
#define ID_BALLOON_QUERYCONT    3
#define ID_BALLOON_SHOWTIME     4

#define BALLOON_DELAYREMOVE_TIMEOUT 250 // milliseconds


CUserNotification::CUserNotification() :
    m_hWorkerWnd(NULL),
    m_hIcon(NULL),
    m_dwInfoFlags(0),
    m_uShowTime(15000),
    m_uInterval(10000),
    m_cRetryCount(-1),
    m_uContinuePoolInterval(0),
    m_bIsShown(FALSE),
    m_hRes(S_OK),
    m_pqc(NULL)
{
}

CUserNotification::~CUserNotification()
{
    /* If we have a notification window... */
    if (m_hWorkerWnd)
    {
        /* ... remove the notification icon and destroy the window */
        RemoveIcon();
        ::DestroyWindow(m_hWorkerWnd);
        m_hWorkerWnd = NULL;
    }

    /* Destroy our local icon copy */
    if (m_hIcon)
        ::DestroyIcon(m_hIcon);
}

VOID CUserNotification::RemoveIcon()
{
    NOTIFYICONDATAW nid = {0};

    nid.cbSize = NOTIFYICONDATAW_V3_SIZE; // sizeof(nid);
    nid.hWnd = m_hWorkerWnd;
    nid.uID  = ID_NOTIFY_ICON;

    /* Remove the notification icon */
    ::Shell_NotifyIconW(NIM_DELETE, &nid);
}

VOID CUserNotification::DelayRemoveIcon(IN HRESULT hRes)
{
    /* Set the return value for CUserNotification::Show() and defer icon removal */
    m_hRes = hRes;
    ::SetTimer(m_hWorkerWnd, ID_BALLOON_DELAYREMOVE,
               BALLOON_DELAYREMOVE_TIMEOUT, NULL);
}

VOID CUserNotification::TimeoutIcon()
{
    /*
     * The balloon timed out, we need to wait before showing it again.
     * If we retried too many times, delete the notification icon.
     */
    if (m_cRetryCount > 0)
    {
        /* Decrement the retry count */
        --m_cRetryCount;

        /* Set the timeout interval timer */
        ::SetTimer(m_hWorkerWnd, ID_BALLOON_TIMEOUT, m_uInterval, NULL);
    }
    else
    {
        /* No other retry: delete the notification icon */
        DelayRemoveIcon(HRESULT_FROM_WIN32(ERROR_CANCELLED));
    }
}

VOID CUserNotification::SetUpNotifyData(
    IN UINT uFlags,
    IN OUT PNOTIFYICONDATAW pnid)
{
    pnid->cbSize = NOTIFYICONDATAW_V3_SIZE; // sizeof(nid);
    pnid->hWnd = m_hWorkerWnd;
    pnid->uID  = ID_NOTIFY_ICON;
    // pnid->uVersion = NOTIFYICON_VERSION;

    if (uFlags & NIF_MESSAGE)
    {
        pnid->uFlags |= NIF_MESSAGE;
        pnid->uCallbackMessage = WM_TRAYNOTIFY;
    }

    if (uFlags & NIF_ICON)
    {
        pnid->uFlags |= NIF_ICON;
        /* Use a default icon if we do not have one already */
        pnid->hIcon = (m_hIcon ? m_hIcon : LoadIcon(NULL, IDI_WINLOGO));
    }

    if (uFlags & NIF_TIP)
    {
        pnid->uFlags |= NIF_TIP;
        ::StringCchCopyW(pnid->szTip, _countof(pnid->szTip), m_szTip);
    }

    if (uFlags & NIF_INFO)
    {
        pnid->uFlags |= NIF_INFO;

        // pnid->uTimeout    = m_uShowTime; // NOTE: Deprecated
        pnid->dwInfoFlags = m_dwInfoFlags;

        ::StringCchCopyW(pnid->szInfo, _countof(pnid->szInfo), m_szInfo);
        ::StringCchCopyW(pnid->szInfoTitle, _countof(pnid->szInfoTitle), m_szInfoTitle);
    }
}


/* IUserNotification Implementation */

HRESULT STDMETHODCALLTYPE
CUserNotification::SetBalloonInfo(
    IN LPCWSTR pszTitle,
    IN LPCWSTR pszText,
    IN DWORD dwInfoFlags)
{
    NOTIFYICONDATAW nid = {0};

    m_szInfo      = pszText;
    m_szInfoTitle = pszTitle;
    m_dwInfoFlags = dwInfoFlags;

    /* Update the notification icon if we have one */
    if (!m_hWorkerWnd)
        return S_OK;

    /* Modify the notification icon */
    SetUpNotifyData(NIF_INFO, &nid);
    if (::Shell_NotifyIconW(NIM_MODIFY, &nid))
        return S_OK;
    else
        return E_FAIL;
}

HRESULT STDMETHODCALLTYPE
CUserNotification::SetBalloonRetry(
    IN DWORD dwShowTime,  // Time intervals in milliseconds
    IN DWORD dwInterval,
    IN UINT cRetryCount)
{
    m_uShowTime   = dwShowTime;
    m_uInterval   = dwInterval;
    m_cRetryCount = cRetryCount;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
CUserNotification::SetIconInfo(
    IN HICON hIcon,
    IN LPCWSTR pszToolTip)
{
    NOTIFYICONDATAW nid = {0};

    /* Destroy our local icon copy */
    if (m_hIcon)
        ::DestroyIcon(m_hIcon);

    if (hIcon)
    {
        /* Copy the icon from the user */
        m_hIcon = ::CopyIcon(hIcon);
    }
    else
    {
        /* Use the same icon as the one for the balloon if specified */
        UINT uIcon = (m_dwInfoFlags & NIIF_ICON_MASK);
        LPCWSTR pIcon = NULL;

        if (uIcon == NIIF_INFO)
            pIcon = IDI_INFORMATION;
        else if (uIcon == NIIF_WARNING)
            pIcon = IDI_WARNING;
        else if (uIcon == NIIF_ERROR)
            pIcon = IDI_ERROR;
        else if (uIcon == NIIF_USER)
            pIcon = NULL;

        m_hIcon = (pIcon ? ::LoadIconW(NULL, pIcon) : NULL);
    }

    m_szTip = pszToolTip;

    /* Update the notification icon if we have one */
    if (!m_hWorkerWnd)
        return S_OK;

    /* Modify the notification icon */
    SetUpNotifyData(NIF_ICON | NIF_TIP, &nid);
    if (::Shell_NotifyIconW(NIM_MODIFY, &nid))
        return S_OK;
    else
        return E_FAIL;
}


LRESULT CALLBACK
CUserNotification::WorkerWndProc(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    /* Retrieve the current user notification object stored in the window extra bits */
    CUserNotification* pThis = reinterpret_cast<CUserNotification*>(::GetWindowLongPtrW(hWnd, 0));
    ASSERT(pThis);
    ASSERT(hWnd == pThis->m_hWorkerWnd);

    TRACE("Msg = 0x%x\n", uMsg);
    switch (uMsg)
    {
        /*
         * We do not receive any WM_(NC)CREATE message since worker windows
         * are first created using the default window procedure DefWindowProcW.
         * The window procedure is changed only subsequently to the user one.
         * We however receive WM_(NC)DESTROY messages.
         */
        case WM_DESTROY:
        {
            /* Post a WM_QUIT message only if the Show() method's message loop is running */
            if (pThis->m_bIsShown)
                ::PostQuitMessage(0);
            return 0;
        }

        case WM_NCDESTROY:
        {
            ::SetWindowLongPtrW(hWnd, 0, (LONG_PTR)NULL);
            pThis->m_hWorkerWnd = NULL;
            return 0;
        }

        case WM_QUERYENDSESSION:
        {
            /*
             * User session is ending or a shutdown is occurring: perform cleanup.
             * Set the return value for CUserNotification::Show() and remove the notification.
             */
            pThis->m_hRes = HRESULT_FROM_WIN32(ERROR_CANCELLED);
            pThis->RemoveIcon();
            ::DestroyWindow(pThis->m_hWorkerWnd);
            return TRUE;
        }

        case WM_TIMER:
        {
            TRACE("WM_TIMER(0x%lx)\n", wParam);

            /* Destroy the associated timer */
            ::KillTimer(hWnd, (UINT_PTR)wParam);

            if (wParam == ID_BALLOON_TIMEOUT)
            {
                /* Timeout interval timer expired: display the balloon again */
                NOTIFYICONDATAW nid = {0};
                pThis->SetUpNotifyData(NIF_INFO, &nid);
                ::Shell_NotifyIconW(NIM_MODIFY, &nid);
            }
            else if (wParam == ID_BALLOON_DELAYREMOVE)
            {
                /* Delay-remove timer expired: remove the notification */
                pThis->RemoveIcon();
                ::DestroyWindow(pThis->m_hWorkerWnd);
            }
            else if (wParam == ID_BALLOON_QUERYCONT)
            {
                /*
                 * Query-continue timer expired: ask the user whether the
                 * notification should continue to be displayed or not.
                 */
                if (pThis->m_pqc && pThis->m_pqc->QueryContinue() == S_OK)
                {
                    /* The notification can be displayed */
                    ::SetTimer(hWnd, ID_BALLOON_QUERYCONT, pThis->m_uContinuePoolInterval, NULL);
                }
                else
                {
                    /* The notification should be removed */
                    pThis->DelayRemoveIcon(S_FALSE);
                }
            }
            else if (wParam == ID_BALLOON_SHOWTIME)
            {
                /* Show-time timer expired: wait before showing the balloon again */
                pThis->TimeoutIcon();
            }
            return 0;
        }

        /*
         * Shell User Notification message.
         * We use NOTIFYICON_VERSION == 0 or 3 callback version, with:
         * wParam == identifier of the taskbar icon in which the event occurred;
         * lParam == holds the mouse or keyboard message associated with the event.
         */
        case WM_TRAYNOTIFY:
        {
            TRACE("WM_TRAYNOTIFY - wParam = 0x%lx ; lParam = 0x%lx\n", wParam, lParam);
            ASSERT(wParam == ID_NOTIFY_ICON);

            switch (lParam)
            {
                case NIN_BALLOONSHOW:
                    TRACE("NIN_BALLOONSHOW\n");
                    break;

                case NIN_BALLOONHIDE:
                    TRACE("NIN_BALLOONHIDE\n");
                    break;

                /* The balloon timed out, or the user closed it by clicking on the 'X' button */
                case NIN_BALLOONTIMEOUT:
                {
                    TRACE("NIN_BALLOONTIMEOUT\n");
                    pThis->TimeoutIcon();
                    break;
                }

                /* The user clicked on the balloon: delete the notification icon */
                case NIN_BALLOONUSERCLICK:
                    TRACE("NIN_BALLOONUSERCLICK\n");
                    /* Fall back to icon click behaviour */

                /* The user clicked on the notification icon: delete it */
                case WM_LBUTTONDOWN:
                case WM_RBUTTONDOWN:
                {
                    pThis->DelayRemoveIcon(S_OK);
                    break;
                }

                default:
                    break;
            }

            return 0;
        }
    }

    return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}


// Blocks until the notification times out.
HRESULT STDMETHODCALLTYPE
CUserNotification::Show(
    IN IQueryContinue* pqc,
    IN DWORD dwContinuePollInterval)
{
    NOTIFYICONDATAW nid = {0};
    MSG msg;

    /* Create the hidden notification message worker window if we do not have one already */
    if (!m_hWorkerWnd)
    {
        m_hWorkerWnd = ::SHCreateWorkerWindowW(CUserNotification::WorkerWndProc,
                                               NULL, 0, 0, NULL, (LONG_PTR)this);
        if (!m_hWorkerWnd)
        {
            FAILED_UNEXPECTEDLY(E_FAIL);
            return E_FAIL;
        }

        /* Add and display the notification icon */
        SetUpNotifyData(NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_INFO, &nid);
        if (!::Shell_NotifyIconW(NIM_ADD, &nid))
        {
            ::DestroyWindow(m_hWorkerWnd);
            m_hWorkerWnd = NULL;
            return E_FAIL;
        }
    }

    m_hRes = S_OK;

    /* Set up the user-continue callback mechanism */
    m_pqc = pqc;
    if (pqc)
    {
        m_uContinuePoolInterval = dwContinuePollInterval;
        ::SetTimer(m_hWorkerWnd, ID_BALLOON_QUERYCONT, m_uContinuePoolInterval, NULL);
    }

    /* Control how long the balloon notification is displayed */
    if ((nid.uFlags & NIF_INFO) && !*nid.szInfo /* && !*nid.szInfoTitle */)
        ::SetTimer(m_hWorkerWnd, ID_BALLOON_SHOWTIME, m_uShowTime, NULL);

    /* Dispatch messsages to the worker window */
    m_bIsShown = TRUE;
    while (::GetMessageW(&msg, NULL, 0, 0))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }
    m_bIsShown = FALSE;

    /* Reset the user-continue callback mechanism */
    if (pqc)
    {
        ::KillTimer(m_hWorkerWnd, ID_BALLOON_QUERYCONT);
        m_uContinuePoolInterval = 0;
    }
    m_pqc = NULL;

    /* Return the notification error code */
    return m_hRes;
}

#if 0   // IUserNotification2
// Blocks until the notification times out.
HRESULT STDMETHODCALLTYPE
CUserNotification::Show(
    IN IQueryContinue* pqc,
    IN DWORD dwContinuePollInterval,
    IN IUserNotificationCallback* pSink)
{
    return S_OK;
}
#endif

HRESULT STDMETHODCALLTYPE
CUserNotification::PlaySound(
    IN LPCWSTR pszSoundName)
{
    /* Call the Win32 API - Ignore the PlaySoundW() return value as on Windows */
    ::PlaySoundW(pszSoundName,
                 NULL,
                 SND_ALIAS | SND_APPLICATION |
                 SND_NOSTOP | SND_NODEFAULT | SND_ASYNC);
    return S_OK;
}
