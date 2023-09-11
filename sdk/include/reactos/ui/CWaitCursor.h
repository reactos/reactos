/*
 * PROJECT:    ReactOS headers
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Wait cursor management
 * COPYRIGHT:  Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

class CWaitCursor
{
public:
    CWaitCursor()
    {
        if (s_nLock++ == 0)
        {
            if (!s_hWaitCursor)
                s_hWaitCursor = ::LoadCursor(NULL, IDC_WAIT);
            s_hOldCursor = ::SetCursor(s_hWaitCursor);
        }
        else
        {
            ::SetCursor(s_hWaitCursor);
        }
    }
    ~CWaitCursor()
    {
        if (--s_nLock == 0)
        {
            ::SetCursor(s_hOldCursor);
            s_hOldCursor = NULL;
        }
    }

    CWaitCursor(const CWaitCursor&) = delete;
    CWaitCursor& operator=(const CWaitCursor&) = delete;
    void *operator new(size_t) = delete;
    void operator delete(void*) = delete;

    static BOOL IsWaiting()
    {
        return s_nLock > 0;
    }

    static void Restore()
    {
        ::SetCursor(s_hWaitCursor);
    }

protected:
    static LONG s_nLock;
    static HCURSOR s_hOldCursor;
    static HCURSOR s_hWaitCursor;
};

DECLSPEC_SELECTANY LONG CWaitCursor::s_nLock = 0;
DECLSPEC_SELECTANY HCURSOR CWaitCursor::s_hOldCursor = NULL;
DECLSPEC_SELECTANY HCURSOR CWaitCursor::s_hWaitCursor = NULL;
