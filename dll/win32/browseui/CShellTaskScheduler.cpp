/*
 * PROJECT:     ReactOS browseui
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     IShellTaskScheduler implementation
 * COPYRIGHT:   Copyright 2020 Oleg Dubinskiy (oleg.dubinskij2013@yandex.ua)
 */

#include "precomp.h"

CShellTaskScheduler::CShellTaskScheduler()
{
}

CShellTaskScheduler::~CShellTaskScheduler()
{
}

// *** IShellTaskScheduler methods ***
STDMETHODIMP CShellTaskScheduler::AddTask(IRunnableTask *pTask, REFGUID rtoid, DWORD_PTR lParam, DWORD dwPriority)
{
    TRACE("(%p, %u, %d, %d)\n", this, pTask, rtoid, lParam, dwPriority);
    return E_NOTIMPL;
}

STDMETHODIMP CShellTaskScheduler::RemoveTasks(REFGUID rtoid, DWORD_PTR lParam, BOOL fWaitIfRunning)
{
    TRACE("(%u, %d, %d)\n", this, rtoid, lParam, fWaitIfRunning);
    return E_NOTIMPL;
}

UINT STDMETHODCALLTYPE CShellTaskScheduler::CountTasks(REFGUID rtoid)
{
    TRACE("(%u)\n", this, rtoid);
    return E_NOTIMPL;
}

STDMETHODIMP CShellTaskScheduler::Status(DWORD dwReleaseStatus, DWORD dwThreadTimeout)
{
    TRACE("(%d, %d)\n", this, dwReleaseStatus, dwThreadTimeout);
    return E_NOTIMPL;
}
