/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implement CLSID_ACLHistory for auto-completion
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

class CACLHistory
    : public CComCoClass<CACLHistory, &CLSID_ACLHistory>
    , public CComObjectRootEx<CComMultiThreadModelNoCS>
    , public IEnumString
{
public:
    CACLHistory();
    virtual ~CACLHistory();

    // *** IEnumString methods ***
    STDMETHODIMP Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched) override;
    STDMETHODIMP Skip(ULONG celt) override;
    STDMETHODIMP Reset() override;
    STDMETHODIMP Clone(IEnumString **ppenum) override;

public:
    DECLARE_REGISTRY_RESOURCEID(IDR_ACLHISTORY)
    DECLARE_NOT_AGGREGATABLE(CACLHistory)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CACLHistory)
        COM_INTERFACE_ENTRY_IID(IID_IEnumString, IEnumString)
    END_COM_MAP()
};
