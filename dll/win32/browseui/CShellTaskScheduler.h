/*
 * PROJECT:     ReactOS browseui
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     IShellTaskScheduler implementation
 * COPYRIGHT:   Copyright 2020 Oleg Dubinskiy (oleg.dubinskij2013@yandex.ua)
 */

#pragma once

class CShellTaskScheduler :
    public CComCoClass<CShellTaskScheduler, &CLSID_ShellTaskScheduler>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellTaskScheduler
{
private:
public:
    CShellTaskScheduler();
    ~CShellTaskScheduler();

    // *** IShellTaskScheduler methods ***
    STDMETHODIMP AddTask(IRunnableTask *pTask, REFGUID rtoid, DWORD_PTR lParam, DWORD dwPriority);
    STDMETHODIMP RemoveTasks(REFGUID rtoid, DWORD_PTR lParam, BOOL fWaitIfRunning);
    virtual UINT STDMETHODCALLTYPE CountTasks(REFGUID rtoid);
    STDMETHODIMP Status(DWORD dwReleaseStatus, DWORD dwThreadTimeout);

public:

    DECLARE_REGISTRY_RESOURCEID(IDR_SHELLTASKSCHEDULER)
    DECLARE_NOT_AGGREGATABLE(CShellTaskScheduler)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CShellTaskScheduler)
        COM_INTERFACE_ENTRY_IID(IID_IShellTaskScheduler, IShellTaskScheduler)
    END_COM_MAP()
};
