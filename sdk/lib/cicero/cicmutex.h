/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero mutex handling
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "cicbase.h"

class CicMutex
{
    HANDLE m_hMutex;
    BOOL m_bInit;

public:
    CicMutex() : m_hMutex(NULL), m_bInit(FALSE)
    {
    }
    ~CicMutex()
    {
        Uninit();
    }

    void Init(LPSECURITY_ATTRIBUTES lpSA, LPCTSTR pszMutexName)
    {
        m_hMutex = ::CreateMutex(lpSA, FALSE, pszMutexName);
        m_bInit = TRUE;
    }
    void Uninit()
    {
        if (m_hMutex)
        {
            ::CloseHandle(m_hMutex);
            m_hMutex = NULL;
        }
        m_bInit = FALSE;
    }

    BOOL Enter()
    {
        DWORD dwWait = ::WaitForSingleObject(m_hMutex, 5000);
        return (dwWait == WAIT_OBJECT_0) || (dwWait == WAIT_ABANDONED);
    }
    void Leave()
    {
        ::ReleaseMutex(m_hMutex);
    }
};
