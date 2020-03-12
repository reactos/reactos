/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell change notification
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

#define INVALID_REG_ID 0

#define WM_SHELL_GETNOTIFWND (WM_USER+25) // 0x419

#define WM_NOTIF_BANG (WM_USER + 1) // 0x401
#define WM_NOTIF_UNREG (WM_USER + 2) // 0x402
#define WM_NOTIF_DELIVERY (WM_USER + 3) // 0x403
#define WM_NOTIF_SUSPEND (WM_USER + 6) // 0x406

#define DWORD_ALIGNMENT(offset) \
    ((((offset) + sizeof(DWORD) - 1) / sizeof(DWORD)) * sizeof(DWORD))

typedef struct OLDDELIVERYWORKER
{
    HWND hwnd;
    UINT uMsg;
} OLDDELIVERYWORKER, *LPOLDDELIVERYWORKER;

typedef struct DELITICKET
{
    DWORD dwMagic;
    LONG wEventId;
    UINT uFlags;
    DWORD dwTick;
    DWORD ibOffset1; // offset to pidl1
    DWORD ibOffset2; // offset to pidl2
} DELITICKET, *LPDELITICKET;

typedef struct NOTIFSHARE
{
    DWORD dwMagic;
    DWORD cbSize;
    UINT nRegID;
    HWND hwnd;
    UINT uMsg;
    LONG fSources;
    LONG fEvents;
    BOOL fRecursive;
    UINT ibPidl;
} NOTIFSHARE, *LPNOTIFSHARE;

typedef struct CHANGE
{
    DWORD dwMagic;
    LPITEMIDLIST pidl1;
    LPITEMIDLIST pidl2;
    LPDELITICKET pTicket;
} CHANGE, *LPCHANGE;

#define DELIVERY_MAGIC 0xDEADFACE
#define NOTIFSHARE_MAGIC 0xB0B32D1E
#define CHANGE_MAGIC 0xFACEB00C

EXTERN_C HWND
DoGetNotifWindow(BOOL bCreate);

EXTERN_C HWND
DoCreateNotifWindow(HWND hwndShell);

EXTERN_C UINT
DoGetNextRegID(void);

EXTERN_C void
DoRemoveChangeNotifyClientsByProcess(DWORD dwPID);

EXTERN_C void
DoNotifyFreeSpace(LPCITEMIDLIST pidl1,LPCITEMIDLIST pidl2);

EXTERN_C HWND
DoHireOldDeliveryWorker(HWND hwnd, UINT wMsg);

EXTERN_C LPCHANGE
DoGetChangeFromTicket(HANDLE hDelivery, DWORD dwOwnerPID);

EXTERN_C HANDLE
DoCreateNotifShare(ULONG nRegID, HWND hwnd, UINT wMsg, ULONG fSources,
                   LONG fEvents, LONG fRecursive, LPCITEMIDLIST pidl, DWORD dwOwnerPID);

EXTERN_C HANDLE
DoCreateDelivery(LONG wEventId, UINT uFlags, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2,
                 DWORD dwOwnerPID, DWORD dwTick);

EXTERN_C void
DoChangeDelivery(LONG wEventId, UINT uFlags, LPITEMIDLIST pidl1, LPITEMIDLIST pidl2,
                 DWORD dwTick);

#ifdef __cplusplus // C++

class CWorker : public CMessageMap
{
public:
    static LRESULT CALLBACK
    WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        CWorker *pThis = (CWorker *)GetWindowLongPtr(hwnd, 0);
        if (pThis)
        {
            LRESULT lResult = 0;
            BOOL bHandled = pThis->ProcessWindowMessage(hwnd, uMsg, wParam, lParam, lResult, 0);
            if (bHandled)
                return lResult;
        }
        return 0;
    }

    BOOL CreateWorker(HWND hwndParent, DWORD dwExStyle, DWORD dwStyle)
    {
        m_hWnd = SHCreateWorkerWindowW(WindowProc, hwndParent, dwExStyle, dwStyle,
                                       NULL, (LONG_PTR)this);
        return m_hWnd != NULL;
    }

protected:
    HWND m_hWnd;
};

class CChangeNotify : public CWorker
{
public:
    struct ITEM
    {
        UINT nRegID;
        DWORD dwUserPID;
        HANDLE hShare;
    };

    CChangeNotify() : m_nNextRegID(INVALID_REG_ID)
    {
    }

    ~CChangeNotify()
    {
    }

    operator HWND()
    {
        return m_hWnd;
    }

    void SetHWND(HWND hwnd)
    {
        m_hWnd = hwnd;
    }

    BOOL AddItem(UINT nRegID, DWORD dwUserPID, HANDLE hShare);
    BOOL RemoveItem(UINT nRegID, DWORD dwOwnerPID);
    void RemoveItemsByProcess(DWORD dwOwnerPID, DWORD dwUserPID);

    UINT GetNextRegID();
    BOOL DoDelivery(HANDLE hDelivery, LPCHANGE pChange);

    BOOL ShouldNotify(LPCHANGE pChange, LPDELITICKET pTicket, LPNOTIFSHARE pShared);
    BOOL DoNotify(LPCHANGE pChange, LPDELITICKET pTicket, LPNOTIFSHARE pShared);

    LRESULT OnBang(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnUnReg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDelivery(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSuspendResume(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    BEGIN_MSG_MAP(CChangeNotify)
        MESSAGE_HANDLER(WM_NOTIF_BANG, OnBang)
        MESSAGE_HANDLER(WM_NOTIF_UNREG, OnUnReg)
        MESSAGE_HANDLER(WM_NOTIF_DELIVERY, OnDelivery)
        MESSAGE_HANDLER(WM_NOTIF_SUSPEND, OnSuspendResume)
    END_MSG_MAP()

protected:
    UINT m_nNextRegID;
    CSimpleArray<ITEM> m_items;
};

#endif  // C++
