/*
 * PROJECT:     ReactOS browseui
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     IShellTaskScheduler implementation
 * COPYRIGHT:   Copyright 2020 Oleg Dubinskiy (oleg.dubinskij30@gmail.com)
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
    STDMETHODIMP AddTask(IRunnableTask *pTask, REFGUID rtoid, DWORD_PTR lParam, DWORD dwPriority) override;
    STDMETHODIMP RemoveTasks(REFGUID rtoid, DWORD_PTR lParam, BOOL fWaitIfRunning) override;
    STDMETHODIMP_(UINT) CountTasks(REFGUID rtoid) override;
    STDMETHODIMP Status(DWORD dwReleaseStatus, DWORD dwThreadTimeout) override;

public:

    DECLARE_REGISTRY_RESOURCEID(IDR_SHELLTASKSCHEDULER)
    DECLARE_NOT_AGGREGATABLE(CShellTaskScheduler)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CShellTaskScheduler)
        COM_INTERFACE_ENTRY_IID(IID_IShellTaskScheduler, IShellTaskScheduler)
    END_COM_MAP()
};
