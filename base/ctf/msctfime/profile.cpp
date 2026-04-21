/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Profile of msctfime.ime
 * COPYRIGHT:   Copyright 2024-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "msctfime.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctfime);

// The callback function type
typedef HRESULT (CALLBACK *FN_LanguageProfilesCallback)(TF_LANGUAGEPROFILE profile, LPARAM lParam);

/// @implemented
HRESULT
CicProfile::LanguageProfilesCallback(
    _In_ TF_LANGUAGEPROFILE profile,
    _Inout_opt_ LPARAM lParam)
{
    if (!profile.fActive || !memcmp(&profile.clsid, &GUID_NULL, sizeof(GUID_NULL)))
        return S_FALSE;
    PLANG_PROF_ENUM_ARG pArg = (PLANG_PROF_ENUM_ARG)lParam;
    if (!pArg)
        return S_OK;
    if (memcmp(&profile.catid, &pArg->catid, sizeof(profile.catid)) != 0)
        return S_FALSE;
    pArg->profile = profile;
    return S_OK;
}

template <typename T_IFACE, typename T_DATA>
class CicEnumValue
{
    T_IFACE* m_iface;
    FN_LanguageProfilesCallback m_callback;
    LPARAM m_lParam;

public:
    CicEnumValue(T_IFACE* iface, FN_LanguageProfilesCallback callback, LPARAM lParam = 0)
        : m_iface(iface)
        , m_callback(callback)
        , m_lParam(lParam)
    { }

    DWORD DoEnum()
    {
        HRESULT hr;
        T_DATA data;

        for (;;)
        {
            hr = m_iface->Next(1, &data, NULL);
            if (hr != S_OK)
                break;

            hr = m_callback(data, m_lParam);
            if (hr == S_OK)
                return ERROR_SUCCESS;
        }

        return ERROR_FILE_NOT_FOUND;
    }
};

/// @implemented
CicProfile::CicProfile()
{
    m_dwFlags = 0;
    m_cRefs = 1;
    m_pIPProfiles = NULL;
    m_pActiveLanguageProfileNotifySink = NULL;
    m_LangID1 = 0;
    m_nCodePage = CP_ACP;
    m_LangID2 = 0;
    m_dwUnknown1 = 0;
}

/// @implemented
CicProfile::~CicProfile()
{
    if (m_pIPProfiles)
    {
        if (m_LangID1)
            m_pIPProfiles->ChangeCurrentLanguage(m_LangID1);

        m_pIPProfiles->Release();
        m_pIPProfiles = NULL;
    }

    if (m_pActiveLanguageProfileNotifySink)
    {
        m_pActiveLanguageProfileNotifySink->_Unadvise();
        m_pActiveLanguageProfileNotifySink->Release();
        m_pActiveLanguageProfileNotifySink = NULL;
    }
}

/// @implemented
STDMETHODIMP CicProfile::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

/// @implemented
STDMETHODIMP_(ULONG) CicProfile::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

/// @implemented
STDMETHODIMP_(ULONG) CicProfile::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

/// @implemented
INT CALLBACK
CicProfile::ActiveLanguageProfileNotifySinkCallback(
    REFGUID rguid1,
    REFGUID rguid2,
    BOOL fActivated,
    LPVOID pUserData)
{
    CicProfile *pThis = (CicProfile *)pUserData;
    pThis->m_dwFlags &= ~0xE;
    return 0;
}

/// @implemented
HRESULT CicProfile::GetCodePageA(_Out_ UINT *puCodePage)
{
    if (!puCodePage)
        return E_INVALIDARG;

    if (m_dwFlags & 2)
    {
        *puCodePage = m_nCodePage;
        return S_OK;
    }

    *puCodePage = 0;

    LANGID LangID;
    HRESULT hr = GetLangId(&LangID);
    if (FAILED(hr))
        return E_FAIL;

    WCHAR szBuff[12];
    INT cch = ::GetLocaleInfoW(LangID, LOCALE_IDEFAULTANSICODEPAGE, szBuff, _countof(szBuff));
    if (cch)
    {
        szBuff[cch] = 0;
        m_nCodePage = *puCodePage = wcstoul(szBuff, NULL, 10);
        m_dwFlags |= 2;
    }

    return S_OK;
}

/// @implemented
HRESULT CicProfile::GetLangId(_Out_ LANGID *pLangID)
{
    *pLangID = 0;

    if (!m_pIPProfiles)
        return E_FAIL;

    if (m_dwFlags & 4)
    {
        *pLangID = m_LangID2;
        return S_OK;
    }

    HRESULT hr = m_pIPProfiles->GetCurrentLanguage(pLangID);
    if (SUCCEEDED(hr))
    {
        m_dwFlags |= 4;
        m_LangID2 = *pLangID;
    }

    return hr;
}

/// @implemented
HRESULT
CicProfile::InitProfileInstance(_Inout_ TLS *pTLS)
{
    HRESULT hr = TF_CreateInputProcessorProfiles(&m_pIPProfiles);
    if (FAILED(hr))
        return hr;

    if (!m_pActiveLanguageProfileNotifySink)
    {
        CActiveLanguageProfileNotifySink *pSink =
            new(cicNoThrow) CActiveLanguageProfileNotifySink(
                CicProfile::ActiveLanguageProfileNotifySinkCallback, this);
        if (!pSink)
        {
            m_pIPProfiles->Release();
            m_pIPProfiles = NULL;
            return E_FAIL;
        }
        m_pActiveLanguageProfileNotifySink = pSink;
    }

    if (pTLS->m_pThreadMgr)
        m_pActiveLanguageProfileNotifySink->_Advise(pTLS->m_pThreadMgr);

    return hr;
}

/// @implemented
HRESULT
CicProfile::GetActiveLanguageProfile(
    _In_ HKL hKL,
    _In_ REFGUID catid,
    _Out_ TF_LANGUAGEPROFILE* pProfile)
{
    CicInterface_RefCnt<IEnumTfLanguageProfiles> pEnum;
    HRESULT hr = m_pIPProfiles->EnumLanguageProfiles(LOWORD(hKL), &pEnum);
    if (FAILED(hr))
        return S_FALSE;

    LANG_PROF_ENUM_ARG arg;
    arg.catid = catid;

    CicEnumValue<IEnumTfLanguageProfiles, TF_LANGUAGEPROFILE>
        enumValue(pEnum, LanguageProfilesCallback, (LPARAM)&arg);
    DWORD ret = enumValue.DoEnum();
    if (ret != ERROR_SUCCESS || !pProfile)
        return S_FALSE;
    *pProfile = arg.profile;
    return S_OK;
}

/// @implemented
HRESULT CicProfile::IsIME(HKL hKL)
{
    CicInterface_RefCnt<IEnumTfLanguageProfiles> pEnum;
    HRESULT hr = m_pIPProfiles->EnumLanguageProfiles(LOWORD(hKL), &pEnum);
    if (FAILED(hr))
        return S_FALSE;

    CicEnumValue<IEnumTfLanguageProfiles, TF_LANGUAGEPROFILE>
        enumValue(pEnum, LanguageProfilesCallback);
    DWORD ret = enumValue.DoEnum();
    return (ret == ERROR_SUCCESS) ? S_OK : S_FALSE;
}
