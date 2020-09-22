/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell change notification
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#pragma once

#include "CDirectoryList.h"

// NOTE: Regard to asynchronous procedure call (APC), please see:
// https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-sleepex

class CDirectoryWatcher
{
public:
    HANDLE m_hDirectory;
    WCHAR m_szDirectoryPath[MAX_PATH];

    static CDirectoryWatcher *Create(LPCWSTR pszDirectoryPath, BOOL fSubTree);
    static void RequestAllWatchersTermination();
    ~CDirectoryWatcher();

    BOOL IsDead() const;
    BOOL RestartWatching();
    void QuitWatching();
    BOOL RequestAddWatcher();
    BOOL RequestTermination();
    void ReadCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered);

protected:
    BOOL m_fDead;
    BOOL m_fRecursive;
    CDirectoryList m_dir_list;
    OVERLAPPED m_overlapped;

    BOOL CreateAPCThread();
    void ProcessNotification();
    CDirectoryWatcher(LPCWSTR pszDirectoryPath, BOOL fSubTree);
};
