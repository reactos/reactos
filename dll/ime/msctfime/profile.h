/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Profile of msctfime.ime
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "sinks.h"

class CicProfile : public IUnknown
{
protected:
    ITfInputProcessorProfiles *m_pIPProfiles;
    CActiveLanguageProfileNotifySink *m_pActiveLanguageProfileNotifySink;
    LANGID  m_LangID1;
    WORD    m_padding1;
    DWORD   m_dwFlags;
    UINT    m_nCodePage;
    LANGID  m_LangID2;
    WORD    m_padding2;
    DWORD   m_dwUnknown1;
    LONG    m_cRefs;

    static INT CALLBACK
    ActiveLanguageProfileNotifySinkCallback(
        REFGUID rguid1,
        REFGUID rguid2,
        BOOL fActivated,
        LPVOID pUserData);

public:
    CicProfile();
    virtual ~CicProfile();

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    HRESULT
    GetActiveLanguageProfile(
        _In_ HKL hKL,
        _In_ REFGUID rguid,
        _Out_ TF_LANGUAGEPROFILE *pProfile);
    HRESULT GetLangId(_Out_ LANGID *pLangID);
    HRESULT GetCodePageA(_Out_ UINT *puCodePage);

    HRESULT InitProfileInstance(_Inout_ TLS *pTLS);

    BOOL IsIME(HKL hKL);
};
