/*
 * PROJECT:     ReactOS browseui
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     IUserAssist implementation
 * COPYRIGHT:   Copyright 2020 Oleg Dubinskiy (oleg.dubinskij2013@yandex.ua)
 */
// See https://www.geoffchappell.com/studies/windows/ie/browseui/classes/userassist.htm

#pragma once

class CUserAssist :
    public CComCoClass<CUserAssist, &CLSID_UserAssist>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IUserAssist
{
private:
public:
    CUserAssist();
    ~CUserAssist();

    // *** IUserAssist methods ***
    STDMETHODIMP FireEvent(GUID const *guid, INT param1, ULONG param2, WPARAM wparam, LPARAM lparam);
    // FIXME: PVOID should point to undocumented UEMINFO structure.
    STDMETHODIMP QueryEvent(GUID const *guid, INT param, WPARAM wparam, LPARAM lparam, PVOID ptr);
    STDMETHODIMP SetEvent(GUID const *guid, INT param, WPARAM wparam, LPARAM lparam, PVOID ptr);
    STDMETHODIMP Enable(BOOL bEnable);

public:

    DECLARE_REGISTRY_RESOURCEID(IDR_USERASSIST)
    DECLARE_NOT_AGGREGATABLE(CUserAssist)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CUserAssist)
        COM_INTERFACE_ENTRY_IID(IID_IUserAssist, IUserAssist)
    END_COM_MAP()
};
