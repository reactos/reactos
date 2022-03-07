/*
* PROJECT:     ReactOS ATL
* LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
* PURPOSE:     ATL Synchronization
* COPYRIGHT:   Copyright 2022 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
*/

#pragma once

#ifndef __ATLSYNC_H__
#define __ATLSYNC_H__

#include "atlbase.h"

namespace ATL
{

class CCriticalSection : public CRITICAL_SECTION
{
    CCriticalSection()
    {
        ::InitializeCriticalSection(this);
    }

    ~CCriticalSection()
    {
        ::DeleteCriticalSection(this);
    }

    void Enter()
    {
        ::EnterCriticalSection(this);
    }

    BOOL TryEnter()
    {
        return ::TryEnterCriticalSection(this);
    }

    void Leave()
    {
        ::LeaveCriticalSection(this);
    }
};

class CEvent : public CHandle
{
    CEvent()
    {
    }

    CEvent(CEvent& hEvent) : CHandle(hEvent)
    {
    }

    CEvent(BOOL bManualReset, BOOL bInitialState)
    {
        Create(NULL, bManualReset, bInitialState, NULL);
    }

    CEvent(LPSECURITY_ATTRIBUTES pSecurity, BOOL bManualReset, BOOL bInitialState, LPCTSTR pszName)
    {
        Create(pSecurity, bManualReset, bInitialState, pszName);
    }

    explicit CEvent(HANDLE hEvent) : CHandle(hEvent)
    {
    }

    BOOL Create(LPSECURITY_ATTRIBUTES pSecurity, BOOL bManualReset, BOOL bInitialState, LPCTSTR pszName)
    {
        HANDLE hEvent = ::CreateEvent(pSecurity, bManualReset, bInitialState, pszName);
        ATLASSERT(hEvent != NULL);
        Attach(hEvent);
        return hEvent != NULL;
    }

    BOOL Open(DWORD dwAccess, BOOL bInheritHandle, LPCTSTR pszName)
    {
        HANDLE hEvent = ::OpenEvent(dwAccess, bInheritHandle, pszName);
        ATLASSERT(hEvent != NULL);
        Attach(hEvent);
        return hEvent != NULL;
    }

    BOOL Reset()
    {
        ATLASSERT(*this);
        return ::ResetEvent(*this);
    }

    BOOL Set()
    {
        ATLASSERT(*this);
        return ::SetEvent(*this);
    }

    BOOL Pulse()
    {
        ATLASSERT(*this);
        return ::PulseEvent(*this);
    }
};

class CMutex : public CHandle
{
    CMutex()
    {
    }

    CMutex(CMutex& hMutex) : CHandle(hMutex)
    {
    }

    explicit CMutex(BOOL bInitialOwner)
    {
        Create(NULL, bInitialOwner, NULL);
    }

    CMutex(LPSECURITY_ATTRIBUTES pSecurity, BOOL bInitialOwner, LPCTSTR pszName)
    {
        Create(pSecurity, bInitialOwner, pszName);
    }

    explicit CMutex(HANDLE hMutex) : CHandle(hMutex)
    {
    }

    BOOL Create(LPSECURITY_ATTRIBUTES pSecurity, BOOL bInitialOwner, LPCTSTR pszName)
    {
        HANDLE hMutex = ::CreateMutex(pSecurity, bInitialOwner, pszName);
        ATLASSERT(hMutex != NULL);
        Attach(hMutex);
        return hMutex != NULL;
    }

    BOOL Open(DWORD dwAccess, BOOL bInheritHandle, LPCTSTR pszName)
    {
        HANDLE hMutex = ::OpenMutex(dwAccess, bInheritHandle, pszName);
        ATLASSERT(hMutex != NULL);
        Attach(hMutex);
        return hMutex != NULL;
    }

    BOOL Release()
    {
        ATLASSERT(*this);
        return ::ReleaseMutex(*this);
    }
};

class CSemaphore : public CHandle
{
    CSemaphore()
    {
    }

    CSemaphore(CSemaphore& hSemaphore) : CHandle(hSemaphore)
    {
    }

    CSemaphore(LONG nInitialCount, LONG nMaxCount)
    {
        Create(NULL, nInitialCount, nMaxCount, NULL);
    }

    CSemaphore(LPSECURITY_ATTRIBUTES pSecurity, LONG nInitialCount, LONG nMaxCount, LPCTSTR pszName)
    {
        Create(pSecurity, nInitialCount, nMaxCount, pszName);
    }

    explicit CSemaphore(HANDLE hSemaphore) : CHandle(hSemaphore)
    {
    }

    BOOL Create(LPSECURITY_ATTRIBUTES pSecurity, LONG nInitialCount, LONG nMaxCount, LPCTSTR pszName)
    {
        HANDLE hSemaphore = ::CreateSemaphore(pSecurity, nInitialCount, nMaxCount, pszName);
        ATLASSERT(hSemaphore != NULL);
        Attach(hSemaphore);
        return hSemaphore != NULL;
    }

    BOOL Open(DWORD dwAccess, BOOL bInheritHandle, LPCTSTR pszName)
    {
        HANDLE hSemaphore = ::OpenSemaphore(dwAccess, bInheritHandle, pszName);
        ATLASSERT(hSemaphore != NULL);
        Attach(hSemaphore);
        return hSemaphore != NULL;
    }

    BOOL Release(LONG nReleaseCount = 1, LPLONG pnOldCount = NULL)
    {
        ATLASSERT(*this);
        return ::ReleaseSemaphore(*this, nReleaseCount, pnOldCount);
    }
};

} // namespace ATL

#endif // __ATLSYNC_H__
