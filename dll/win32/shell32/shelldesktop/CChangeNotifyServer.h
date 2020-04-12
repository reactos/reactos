/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell change notification
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#pragma once

/////////////////////////////////////////////////////////////////////////////
// CChangeNotifyServer is a delivery worker window that is managed by CDesktopBrowser.
// The process of CChangeNotifyServer is same as the process of CDesktopBrowser.
// The caller process of SHChangeNotify function might be different from the
// process of CChangeNotifyServer.
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// The shared memory block can be allocated by shlwapi!SHAllocShared function.
//
// HANDLE SHAllocShared(LPCVOID lpData, DWORD dwSize, DWORD dwProcessId);
// LPVOID SHLockShared(HANDLE hData, DWORD dwProcessId);
// LPVOID SHLockSharedEx(HANDLE hData, DWORD dwProcessId, BOOL bWriteAccess);
// BOOL SHUnlockShared(LPVOID lpData);
// BOOL SHFreeShared(HANDLE hData, DWORD dwProcessId);
//
// The shared memory block is managed by the pair of a HANDLE value and an owner PID.
// If the pair is known, it can be accessed by SHLockShared(Ex) function
// from another process.
/////////////////////////////////////////////////////////////////////////////

#define INVALID_REG_ID 0 /* invalid registration ID */

// This message is handled by CDesktopBrowser and returns
// the window of CChangeNotifyServer which responds to the CN_* messages
#define WM_DESKTOP_GET_CNOTIFY_SERVER (WM_USER + 25) /* 0x419 */

// The following messages are implemented by CChangeNotifyServer
// CChangeNotifyServer lives in the context of explorer and delivers notifications
// across all processes that register themselves with the SHChangeNotifyRegister api
#define CN_REGISTER (WM_USER + 1) /* 0x401 */
#define CN_UNREGISTER (WM_USER + 2) /* 0x402 */
#define CN_DELIVER_NOTIFICATION (WM_USER + 3) /* 0x403 */
#define CN_SUSPEND_RESUME (WM_USER + 6) /* 0x406 */
#define CN_UNREGISTER_PROCESS (WM_USER + 7) /* 0x407 */

// This message is implemented by the broker window which lives in the context of the client
#define WM_BROKER_NOTIFICATION (WM_USER + 1) /* 0x401 */

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
    HWND hwndBroker;    /* broker window (if any) */
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

HRESULT CChangeNotifyServer_CreateInstance(REFIID riid, void **ppv);
