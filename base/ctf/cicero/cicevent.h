/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero event object handling
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "cicbase.h"

class CicEvent
{
    HANDLE m_hEvent;
    LPCTSTR m_pszName;

public:
    CicEvent() : m_hEvent(NULL), m_pszName(NULL)
    {
    }
    ~CicEvent()
    {
        Close();
    }

    BOOL Create(LPSECURITY_ATTRIBUTES lpSA, LPCTSTR pszName)
    {
        if (pszName)
            m_pszName = pszName;
        if (!m_pszName)
            return FALSE;
        m_hEvent = ::CreateEvent(lpSA, FALSE, FALSE, m_pszName);
        return (m_hEvent != NULL);
    }
    BOOL Open(LPCTSTR pszName)
    {
        if (pszName)
            m_pszName = pszName;
        m_hEvent = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, m_pszName);
        return (m_hEvent != NULL);
    }
    void Close()
    {
        if (m_hEvent)
        {
            ::CloseHandle(m_hEvent);
            m_hEvent = NULL;
        }
    }

    BOOL Wait(DWORD dwMilliseconds)
    {
        return (::WaitForSingleObject(m_hEvent, dwMilliseconds) == WAIT_OBJECT_0);
    }
};
