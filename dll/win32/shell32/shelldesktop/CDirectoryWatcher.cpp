/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell change notification
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include "shelldesktop.h"
#include "CDirectoryWatcher.h"
#include <process.h>     // for _beginthreadex
#include <assert.h>      // for assert

WINE_DEFAULT_DEBUG_CHANNEL(shcn);

// Notify filesystem change
static inline void
NotifyFileSystemChange(LONG wEventId, LPCWSTR path1, LPCWSTR path2)
{
    SHChangeNotify(wEventId | SHCNE_INTERRUPT, SHCNF_PATHW | SHCNF_FLUSH, path1, path2);
}

// The handle of the APC thread
static HANDLE s_hThreadAPC = NULL;

// Terminate now?
static BOOL s_fTerminateAllWatchers = FALSE;

// the buffer for ReadDirectoryChangesW
#define BUFFER_SIZE 0x1000
static BYTE s_buffer[BUFFER_SIZE];

// The APC thread function for directory watch
static unsigned __stdcall DirectoryWatcherThreadFuncAPC(void *)
{
    while (!s_fTerminateAllWatchers)
    {
#if 1 // FIXME: This is a HACK
        WaitForSingleObjectEx(GetCurrentThread(), INFINITE, TRUE);
#else
        SleepEx(INFINITE, TRUE);
#endif
    }
    return 0;
}

// The APC procedure to add a CDirectoryWatcher and start the directory watch
static void NTAPI _AddDirectoryProcAPC(ULONG_PTR Parameter)
{
    CDirectoryWatcher *pDirectoryWatcher = (CDirectoryWatcher *)Parameter;
    assert(pDirectoryWatcher != NULL);

    pDirectoryWatcher->RestartWatching();
}

// The APC procedure to request termination of a CDirectoryWatcher
static void NTAPI _RequestTerminationAPC(ULONG_PTR Parameter)
{
    CDirectoryWatcher *pDirectoryWatcher = (CDirectoryWatcher *)Parameter;
    assert(pDirectoryWatcher != NULL);

    pDirectoryWatcher->QuitWatching();
}

// The APC procedure to request termination of all the directory watches
static void NTAPI _RequestAllTerminationAPC(ULONG_PTR Parameter)
{
    s_fTerminateAllWatchers = TRUE;
    CloseHandle(s_hThreadAPC);
    s_hThreadAPC = NULL;
}

CDirectoryWatcher::CDirectoryWatcher(LPCWSTR pszDirectoryPath, BOOL fSubTree)
    : m_fDead(FALSE)
    , m_fRecursive(fSubTree)
    , m_dir_list(pszDirectoryPath, fSubTree)
{
    TRACE("CDirectoryWatcher::CDirectoryWatcher: %p, '%S'\n", this, pszDirectoryPath);

    lstrcpynW(m_szDirectoryPath, pszDirectoryPath, MAX_PATH);

    // open the directory to watch changes (for ReadDirectoryChangesW)
    m_hDirectory = CreateFileW(pszDirectoryPath, FILE_LIST_DIRECTORY,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL, OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                               NULL);
}

/*static*/ CDirectoryWatcher *
CDirectoryWatcher::Create(LPCWSTR pszDirectoryPath, BOOL fSubTree)
{
    WCHAR szFullPath[MAX_PATH];
    GetFullPathNameW(pszDirectoryPath, _countof(szFullPath), szFullPath, NULL);

    CDirectoryWatcher *pDirectoryWatcher = new CDirectoryWatcher(szFullPath, fSubTree);
    if (pDirectoryWatcher->m_hDirectory == INVALID_HANDLE_VALUE)
    {
        ERR("CreateFileW failed\n");
        delete pDirectoryWatcher;
        pDirectoryWatcher = NULL;
    }
    return pDirectoryWatcher;
}

CDirectoryWatcher::~CDirectoryWatcher()
{
    TRACE("CDirectoryWatcher::~CDirectoryWatcher: %p, '%S'\n", this, m_szDirectoryPath);

    if (m_hDirectory != INVALID_HANDLE_VALUE)
        CloseHandle(m_hDirectory);
}

// convert the file action to an event
static DWORD
ConvertActionToEvent(DWORD Action, BOOL fDir)
{
    switch (Action)
    {
        case FILE_ACTION_ADDED:
            return (fDir ? SHCNE_MKDIR : SHCNE_CREATE);
        case FILE_ACTION_REMOVED:
            return (fDir ? SHCNE_RMDIR : SHCNE_DELETE);
        case FILE_ACTION_MODIFIED:
            return (fDir ? SHCNE_UPDATEDIR : SHCNE_UPDATEITEM);
        case FILE_ACTION_RENAMED_OLD_NAME:
            break;
        case FILE_ACTION_RENAMED_NEW_NAME:
            return (fDir ? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM);
        default:
            break;
    }
    return 0;
}

// Notify a filesystem notification using pDirectoryWatcher.
void CDirectoryWatcher::ProcessNotification()
{
    PFILE_NOTIFY_INFORMATION pInfo = (PFILE_NOTIFY_INFORMATION)s_buffer;
    WCHAR szName[MAX_PATH], szPath[MAX_PATH], szTempPath[MAX_PATH];
    DWORD dwEvent, cbName;
    BOOL fDir;
    TRACE("CDirectoryWatcher::ProcessNotification: enter\n");

    // for each entry in s_buffer
    szPath[0] = szTempPath[0] = 0;
    for (;;)
    {
        // get name (relative from m_szDirectoryPath)
        cbName = pInfo->FileNameLength;
        if (sizeof(szName) - sizeof(UNICODE_NULL) < cbName)
        {
            ERR("pInfo->FileName is longer than szName\n");
            break;
        }
        // NOTE: FILE_NOTIFY_INFORMATION.FileName is not null-terminated.
        ZeroMemory(szName, sizeof(szName));
        CopyMemory(szName, pInfo->FileName, cbName);

        // get full path
        lstrcpynW(szPath, m_szDirectoryPath, _countof(szPath));
        PathAppendW(szPath, szName);

        // convert to long pathname if it contains '~'
        if (StrChrW(szPath, L'~') != NULL)
        {
            if (GetLongPathNameW(szPath, szName, _countof(szName)) &&
                !PathIsRelativeW(szName))
            {
                lstrcpynW(szPath, szName, _countof(szPath));
            }
        }

        // convert action to event
        fDir = PathIsDirectoryW(szPath);
        dwEvent = ConvertActionToEvent(pInfo->Action, fDir);

        // convert SHCNE_DELETE to SHCNE_RMDIR if the path is a directory
        if (!fDir && (dwEvent == SHCNE_DELETE) && m_dir_list.ContainsPath(szPath))
        {
            fDir = TRUE;
            dwEvent = SHCNE_RMDIR;
        }

        // update m_dir_list
        switch (dwEvent)
        {
            case SHCNE_MKDIR:
                if (!PathIsDirectoryW(szPath) || !m_dir_list.AddPath(szPath))
                    dwEvent = 0;
                break;
            case SHCNE_CREATE:
                if (!PathFileExistsW(szPath) || PathIsDirectoryW(szPath))
                    dwEvent = 0;
                break;
            case SHCNE_RENAMEFOLDER:
                if (!PathIsDirectoryW(szPath) || !m_dir_list.RenamePath(szTempPath, szPath))
                    dwEvent = 0;
                break;
            case SHCNE_RENAMEITEM:
                if (!PathFileExistsW(szPath) || PathIsDirectoryW(szPath))
                    dwEvent = 0;
                break;
            case SHCNE_RMDIR:
                if (PathIsDirectoryW(szPath) || !m_dir_list.DeletePath(szPath))
                    dwEvent = 0;
                break;
            case SHCNE_DELETE:
                if (PathFileExistsW(szPath))
                    dwEvent = 0;
                break;
        }

        if (dwEvent != 0)
        {
            // notify
            if (pInfo->Action == FILE_ACTION_RENAMED_NEW_NAME)
                NotifyFileSystemChange(dwEvent, szTempPath, szPath);
            else
                NotifyFileSystemChange(dwEvent, szPath, NULL);
        }
        else if (pInfo->Action == FILE_ACTION_RENAMED_OLD_NAME)
        {
            // save path for next FILE_ACTION_RENAMED_NEW_NAME
            lstrcpynW(szTempPath, szPath, MAX_PATH);
        }

        if (pInfo->NextEntryOffset == 0)
            break; // there is no next entry

        // go next entry
        pInfo = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pInfo + pInfo->NextEntryOffset);
    }

    TRACE("CDirectoryWatcher::ProcessNotification: leave\n");
}

void CDirectoryWatcher::ReadCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered)
{
    // If the FSD doesn't support directory change notifications, there's no
    // no need to retry and requeue notification
    if (dwErrorCode == ERROR_INVALID_FUNCTION)
    {
        ERR("ERROR_INVALID_FUNCTION\n");
        return;
    }

    // Also, if the notify operation was canceled (like, user moved to another
    // directory), then, don't requeue notification.
    if (dwErrorCode == ERROR_OPERATION_ABORTED)
    {
        TRACE("ERROR_OPERATION_ABORTED\n");
        if (IsDead())
            delete this;
        return;
    }

    // is this watch dead?
    if (IsDead())
    {
        TRACE("IsDead()\n");
        delete this;
        return;
    }

    // This likely means overflow, so force whole directory refresh.
    if (dwNumberOfBytesTransfered == 0)
    {
        // do notify a SHCNE_UPDATEDIR
        NotifyFileSystemChange(SHCNE_UPDATEDIR, m_szDirectoryPath, NULL);
    }
    else
    {
        // do notify
        ProcessNotification();
    }

    // restart a watch
    RestartWatching();
}

// The completion routine of ReadDirectoryChangesW.
static void CALLBACK
_NotificationCompletion(DWORD dwErrorCode,
                        DWORD dwNumberOfBytesTransfered,
                        LPOVERLAPPED lpOverlapped)
{
    // MSDN: The hEvent member of the OVERLAPPED structure is not used by the
    // system in this case, so you can use it yourself. We do just this, storing
    // a pointer to the working struct in the overlapped structure.
    CDirectoryWatcher *pDirectoryWatcher = (CDirectoryWatcher *)lpOverlapped->hEvent;
    assert(pDirectoryWatcher != NULL);

    pDirectoryWatcher->ReadCompletion(dwErrorCode, dwNumberOfBytesTransfered);
}

// convert events to notification filter
static DWORD
GetFilterFromEvents(DWORD fEvents)
{
    // FIXME
    return (FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_CREATION |
            FILE_NOTIFY_CHANGE_SIZE);
}

// Restart a watch by using ReadDirectoryChangesW function
BOOL CDirectoryWatcher::RestartWatching()
{
    assert(this != NULL);

    if (IsDead())
    {
        delete this;
        return FALSE; // the watch is dead
    }

    // initialize the buffer and the overlapped
    ZeroMemory(s_buffer, sizeof(s_buffer));
    ZeroMemory(&m_overlapped, sizeof(m_overlapped));
    m_overlapped.hEvent = (HANDLE)this;

    // start the directory watch
    DWORD dwFilter = GetFilterFromEvents(SHCNE_ALLEVENTS);
    if (!ReadDirectoryChangesW(m_hDirectory, s_buffer, sizeof(s_buffer),
                               m_fRecursive, dwFilter, NULL,
                               &m_overlapped, _NotificationCompletion))
    {
        ERR("ReadDirectoryChangesW for '%S' failed (error: %ld)\n",
            m_szDirectoryPath, GetLastError());
        return FALSE; // failure
    }

    return TRUE; // success
}

BOOL CDirectoryWatcher::CreateAPCThread()
{
    if (s_hThreadAPC != NULL)
        return TRUE;

    unsigned tid;
    s_fTerminateAllWatchers = FALSE;
    s_hThreadAPC = (HANDLE)_beginthreadex(NULL, 0, DirectoryWatcherThreadFuncAPC,
                                          NULL, 0, &tid);
    return s_hThreadAPC != NULL;
}

BOOL CDirectoryWatcher::RequestAddWatcher()
{
    assert(this != NULL);

    // create an APC thread for directory watching
    if (!CreateAPCThread())
        return FALSE;

    // request adding the watch
    QueueUserAPC(_AddDirectoryProcAPC, s_hThreadAPC, (ULONG_PTR)this);
    return TRUE;
}

BOOL CDirectoryWatcher::RequestTermination()
{
    assert(this != NULL);

    if (s_hThreadAPC)
    {
        QueueUserAPC(_RequestTerminationAPC, s_hThreadAPC, (ULONG_PTR)this);
        return TRUE;
    }

    return FALSE;
}

/*static*/ void CDirectoryWatcher::RequestAllWatchersTermination()
{
    if (!s_hThreadAPC)
        return;

    // request termination of all directory watches
    QueueUserAPC(_RequestAllTerminationAPC, s_hThreadAPC, (ULONG_PTR)NULL);
}

void CDirectoryWatcher::QuitWatching()
{
    assert(this != NULL);

    m_fDead = TRUE;
    CancelIo(m_hDirectory);
}

BOOL CDirectoryWatcher::IsDead() const
{
    return m_fDead;
}
