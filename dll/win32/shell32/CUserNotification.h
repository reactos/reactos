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

#ifndef _USERNOTIFICATION_H_
#define _USERNOTIFICATION_H_

#undef PlaySound

class CUserNotification :
    public CComCoClass<CUserNotification, &CLSID_UserNotification>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IUserNotification
//  public IUserNotification2   // On Vista+
{
private:
    HWND  m_hWorkerWnd;
    HICON m_hIcon;
    DWORD m_dwInfoFlags;
    UINT  m_uShowTime;
    UINT  m_uInterval;
    UINT  m_cRetryCount;
    UINT  m_uContinuePoolInterval;
    BOOL  m_bIsShown;
    HRESULT m_hRes;
    IQueryContinue* m_pqc;
    CStringW m_szTip;
    CStringW m_szInfo;
    CStringW m_szInfoTitle;

private:
    VOID RemoveIcon();
    VOID DelayRemoveIcon(IN HRESULT hRes);
    VOID TimeoutIcon();

    VOID SetUpNotifyData(
        IN UINT uFlags,
        IN OUT PNOTIFYICONDATAW pnid);

    static LRESULT CALLBACK
    WorkerWndProc(
        IN HWND hWnd,
        IN UINT uMsg,
        IN WPARAM wParam,
        IN LPARAM lParam);

public:
    CUserNotification();
    ~CUserNotification();

    // IUserNotification
    virtual HRESULT STDMETHODCALLTYPE SetBalloonInfo(
        IN LPCWSTR pszTitle,
        IN LPCWSTR pszText,
        IN DWORD dwInfoFlags);

    virtual HRESULT STDMETHODCALLTYPE SetBalloonRetry(
        IN DWORD dwShowTime,  // Time intervals in milliseconds
        IN DWORD dwInterval,
        IN UINT cRetryCount);

    virtual HRESULT STDMETHODCALLTYPE SetIconInfo(
        IN HICON hIcon,
        IN LPCWSTR pszToolTip);

    // Blocks until the notification times out.
    virtual HRESULT STDMETHODCALLTYPE Show(
        IN IQueryContinue* pqc,
        IN DWORD dwContinuePollInterval);

    virtual HRESULT STDMETHODCALLTYPE PlaySound(
        IN LPCWSTR pszSoundName);

#if 0
    // IUserNotification2
    // Blocks until the notification times out.
    virtual HRESULT STDMETHODCALLTYPE Show(
        IN IQueryContinue* pqc,
        IN DWORD dwContinuePollInterval,
        IN IUserNotificationCallback* pSink);
#endif

    DECLARE_REGISTRY_RESOURCEID(IDR_USERNOTIFICATION)
    DECLARE_NOT_AGGREGATABLE(CUserNotification)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CUserNotification)
        COM_INTERFACE_ENTRY_IID(IID_IUserNotification , IUserNotification )
    //  COM_INTERFACE_ENTRY_IID(IID_IUserNotification2, IUserNotification2)
    END_COM_MAP()
};

#endif /* _USERNOTIFICATION_H_ */
