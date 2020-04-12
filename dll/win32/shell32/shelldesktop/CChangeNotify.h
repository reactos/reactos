/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell change notification
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#pragma once

#define INVALID_REG_ID 0 /* invalid registration ID */

#define WM_SHELL_GETWORKERWND (WM_USER + 25) /* 0x419 */
#define WM_OLDWORKER_HANDOVER (WM_USER + 1) /* 0x401 */
#define WM_WORKER_REGISTER (WM_USER + 1) /* 0x401 */
#define WM_WORKER_UNREGISTER (WM_USER + 2) /* 0x402 */
#define WM_WORKER_TICKET (WM_USER + 3) /* 0x403 */
#define WM_WORKER_SUSPEND (WM_USER + 6) /* 0x406 */
#define WM_WORKER_REMOVEBYPID (WM_USER + 7) /* 0x407 */

#define DWORD_ALIGNMENT(offset) \
    ((((offset) + sizeof(DWORD) - 1) / sizeof(DWORD)) * sizeof(DWORD))

/* delivery ticket */
typedef struct DELITICKET
{
    DWORD dwMagic;   /* same as DELITICKET_MAGIC */
    LONG wEventId;   /* event id of SHChangeNotify() */
    UINT uFlags;     /* flags of SHChangeNotify() */
    DWORD dwTick;    /* value of GetTickCount() */
    DWORD ibOffset1; /* offset to pidl1 */
    DWORD ibOffset2; /* offset to pidl2 */
    /* followed by pidl1 and pidl2 */
} DELITICKET, *LPDELITICKET;

/* registration entry */
typedef struct REGENTRY
{
    DWORD dwMagic;      /* same as REGENTRY_MAGIC */
    DWORD cbSize;       /* the real size of this structure */
    UINT nRegID;        /* the registration ID */
    HWND hwnd;          /* the target window */
    UINT uMsg;          /* the message ID used in notification */
    INT fSources;       /* the source flags */
    LONG fEvents;       /* the event flags */
    BOOL fRecursive;    /* is it recursive? */
    HWND hwndOldWorker; /* old worker (if any) */
    UINT ibPidl;        /* offset to the PIDL */
    /* followed by a PIDL */
} REGENTRY, *LPREGENTRY;

/* handbag */
typedef struct HANDBAG
{
    DWORD dwMagic;          /* same as HANDBAG_MAGIC */
    LPITEMIDLIST pidls[2];  /* two PIDLs */
    LPDELITICKET pTicket;   /* the ticket */
} HANDBAG, *LPHANDBAG;

#define DELITICKET_MAGIC 0xDEADFACE
#define REGENTRY_MAGIC 0xB0B32D1E
#define HANDBAG_MAGIC 0xFACEB00C

#ifdef __cplusplus

/////////////////////////////////////////////////////////////////////////////
// CWorker --- base class of CChangeNotify

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

/////////////////////////////////////////////////////////////////////////////
// CChangeNotify --- a class of new delivery worker

class CChangeNotify : public CWorker
{
public:
    CChangeNotify();
    virtual ~CChangeNotify();

    operator HWND()
    {
        return m_hWnd;
    }

    BOOL AddItem(UINT nRegID, DWORD dwUserPID, HANDLE hRegEntry, HWND hwndOldWorker);
    BOOL RemoveItemsByRegID(UINT nRegID, DWORD dwOwnerPID);
    void RemoveItemsByProcess(DWORD dwOwnerPID, DWORD dwUserPID);

    UINT GetNextRegID();
    BOOL DoTicket(HANDLE hTicket, DWORD dwOwnerPID);
    BOOL ShouldNotify(LPDELITICKET pTicket, LPREGENTRY pRegEntry);

    LRESULT OnRegister(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnUnRegister(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTicket(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSuspendResume(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnRemoveByPID(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    BEGIN_MSG_MAP(CChangeNotify)
        MESSAGE_HANDLER(WM_WORKER_REGISTER, OnRegister)
        MESSAGE_HANDLER(WM_WORKER_UNREGISTER, OnUnRegister)
        MESSAGE_HANDLER(WM_WORKER_TICKET, OnTicket)
        MESSAGE_HANDLER(WM_WORKER_SUSPEND, OnSuspendResume)
        MESSAGE_HANDLER(WM_WORKER_REMOVEBYPID, OnRemoveByPID);
    END_MSG_MAP()

protected:
    UINT m_nNextRegID;
    CChangeNotifyImpl *m_pimpl;
};
#endif
