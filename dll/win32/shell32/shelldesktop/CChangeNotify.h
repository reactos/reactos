/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell change notification
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#pragma once

#define INVALID_REG_ID 0

#define WM_GETDELIWORKERWND (WM_USER + 25) /* 0x419 */
#define WM_OLDDELI_HANDOVER (WM_USER + 1) /* 0x401 */
#define WM_NOTIF_REG (WM_USER + 1) /* 0x401 */
#define WM_NOTIF_UNREG (WM_USER + 2) /* 0x402 */
#define WM_NOTIF_DELIVERY (WM_USER + 3) /* 0x403 */
#define WM_NOTIF_SUSPEND (WM_USER + 6) /* 0x406 */
#define WM_NOTIF_REMOVEBYPID (WM_USER + 7) /* 0x407 */

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
    DWORD ibOffset1; /* offset to pidl1 */
    DWORD ibOffset2; /* offset to pidl2 */
} DELITICKET, *LPDELITICKET;

typedef struct NOTIFSHARE
{
    DWORD dwMagic;
    DWORD cbSize;
    UINT nRegID;
    HWND hwnd;
    HWND hwndOldWorker;
    UINT uMsg;
    INT fSources;
    LONG fEvents;
    BOOL fRecursive;
    UINT ibPidl;
} NOTIFSHARE, *LPNOTIFSHARE;

typedef struct HANDBAG
{
    DWORD dwMagic;
    LPITEMIDLIST pidls[2];
    LPDELITICKET pTicket;
} HANDBAG, *LPHANDBAG;

#define DELITICKET_MAGIC 0xDEADFACE
#define NOTIFSHARE_MAGIC 0xB0B32D1E
#define HANDBAG_MAGIC 0xFACEB00C

EXTERN_C HWND DoGetNewDeliveryWorker(void);
EXTERN_C void DoNotifyFreeSpace(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
EXTERN_C HWND DoHireOldDeliveryWorker(HWND hwnd, UINT wMsg);
EXTERN_C LPHANDBAG DoGetHandbagFromTicket(HANDLE hTicket, DWORD dwOwnerPID);

EXTERN_C HANDLE
DoCreateNotifShare(ULONG nRegID, HWND hwnd, UINT wMsg, INT fSources,
                   LONG fEvents, LONG fRecursive, LPCITEMIDLIST pidl,
                   DWORD dwOwnerPID, HWND hwndOldWorker);

EXTERN_C HANDLE
DoCreateDeliTicket(LONG wEventId, UINT uFlags, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2,
                   DWORD dwOwnerPID, DWORD dwTick);

EXTERN_C void
DoTransportChange(LONG wEventId, UINT uFlags, LPITEMIDLIST pidl1, LPITEMIDLIST pidl2,
                  DWORD dwTick);

#ifdef __cplusplus
class CWorker : public CMessageMap
{
public:
    CWorker() : m_hWnd(NULL)
    {
    }

    virtual ~CWorker()
    {
    }

    static LRESULT CALLBACK
    WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL CreateWorker(HWND hwndParent, DWORD dwExStyle, DWORD dwStyle);

protected:
    HWND m_hWnd;
};

struct CChangeNotifyImpl;

class CChangeNotify : public CWorker
{
public:
    struct ITEM
    {
        UINT nRegID;
        DWORD dwUserPID;
        HANDLE hShare;
        HWND hwndOldWorker;
    };

    CChangeNotify();
    virtual ~CChangeNotify();

    operator HWND()
    {
        return m_hWnd;
    }

    BOOL AddItem(UINT nRegID, DWORD dwUserPID, HANDLE hShare, HWND hwndOldWorker);
    BOOL RemoveItem(UINT nRegID, DWORD dwOwnerPID);
    void RemoveItemsByProcess(DWORD dwOwnerPID, DWORD dwUserPID);

    UINT GetNextRegID();
    BOOL DoDelivery(HANDLE hTicket, DWORD dwOwnerPID);

    BOOL ShouldNotify(LPDELITICKET pTicket, LPNOTIFSHARE pShared);
    BOOL DoNotify(LPHANDBAG pHandbag, LPDELITICKET pTicket, LPNOTIFSHARE pShared);

    LRESULT OnReg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnUnReg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDelivery(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSuspendResume(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnRemoveByPID(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    BEGIN_MSG_MAP(CChangeNotify)
        MESSAGE_HANDLER(WM_NOTIF_REG, OnReg)
        MESSAGE_HANDLER(WM_NOTIF_UNREG, OnUnReg)
        MESSAGE_HANDLER(WM_NOTIF_DELIVERY, OnDelivery)
        MESSAGE_HANDLER(WM_NOTIF_SUSPEND, OnSuspendResume)
        MESSAGE_HANDLER(WM_NOTIF_REMOVEBYPID, OnRemoveByPID);
    END_MSG_MAP()

protected:
    UINT m_nNextRegID;
    CChangeNotifyImpl *m_pimpl;
};
#endif
