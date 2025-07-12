/*
 * PROJECT:     ReactOS CTF
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     ITfInputProcessorProfiles implementation
 * COPYRIGHT:   Copyright 2009 Aric Stewart, CodeWeavers
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msctf);

////////////////////////////////////////////////////////////////////////////

class CInputProcessorProfiles
    : public ITfInputProcessorProfiles
    , public ITfSource
    , public ITfInputProcessorProfileMgr
    // , public ITfInputProcessorProfilesEx
    // , public ITfInputProcessorProfileSubstituteLayout
{
public:
    CInputProcessorProfiles();
    virtual ~CInputProcessorProfiles();

    static HRESULT CreateInstance(IUnknown *pUnkOuter, CInputProcessorProfiles **ppOut);

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID iid, LPVOID *ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** ITfInputProcessorProfiles methods **
    STDMETHODIMP Register(_In_ REFCLSID rclsid) override;
    STDMETHODIMP Unregister(_In_ REFCLSID rclsid) override;
    STDMETHODIMP AddLanguageProfile(
        _In_ REFCLSID rclsid,
        _In_ LANGID langid,
        _In_ REFGUID guidProfile,
        _In_ const WCHAR *pchDesc,
        _In_ ULONG cchDesc,
        _In_ const WCHAR *pchIconFile,
        _In_ ULONG cchFile,
        _In_ ULONG uIconIndex) override;
    STDMETHODIMP RemoveLanguageProfile(
        _In_ REFCLSID rclsid,
        _In_ LANGID langid,
        _In_ REFGUID guidProfile) override;
    STDMETHODIMP EnumInputProcessorInfo(_Out_ IEnumGUID **ppEnum) override;
    STDMETHODIMP GetDefaultLanguageProfile(
        _In_ LANGID langid,
        _In_ REFGUID catid,
        _Out_ CLSID *pclsid,
        _Out_ GUID *pguidProfile) override;
    STDMETHODIMP SetDefaultLanguageProfile(
        _In_ LANGID langid,
        _In_ REFCLSID rclsid,
        _In_ REFGUID guidProfiles) override;
    STDMETHODIMP ActivateLanguageProfile(
        _In_ REFCLSID rclsid,
        _In_ LANGID langid,
        _In_ REFGUID guidProfiles) override;
    STDMETHODIMP GetActiveLanguageProfile(
        _In_ REFCLSID rclsid,
        _Out_ LANGID *plangid,
        _Out_ GUID *pguidProfile) override;
    STDMETHODIMP GetLanguageProfileDescription(
        _In_ REFCLSID rclsid,
        _In_ LANGID langid,
        _In_ REFGUID guidProfile,
        _Out_ BSTR *pbstrProfile) override;
    STDMETHODIMP GetCurrentLanguage(_Out_ LANGID *plangid) override;
    STDMETHODIMP ChangeCurrentLanguage(_In_ LANGID langid) override;
    STDMETHODIMP GetLanguageList(
        _Out_ LANGID **ppLangId,
        _Out_ ULONG *pulCount) override;
    STDMETHODIMP EnumLanguageProfiles(
        _In_ LANGID langid,
        _Out_ IEnumTfLanguageProfiles **ppEnum) override;
    STDMETHODIMP EnableLanguageProfile(
        _In_ REFCLSID rclsid,
        _In_ LANGID langid,
        _In_ REFGUID guidProfile,
        _In_ BOOL fEnable) override;
    STDMETHODIMP IsEnabledLanguageProfile(
        _In_ REFCLSID rclsid,
        _In_ LANGID langid,
        _In_ REFGUID guidProfile,
        _Out_ BOOL *pfEnable) override;
    STDMETHODIMP EnableLanguageProfileByDefault(
        _In_ REFCLSID rclsid,
        _In_ LANGID langid,
        _In_ REFGUID guidProfile,
        _In_ BOOL fEnable) override;
    STDMETHODIMP SubstituteKeyboardLayout(
        _In_ REFCLSID rclsid,
        _In_ LANGID langid,
        _In_ REFGUID guidProfile,
        _In_ HKL hKL) override;

    // ** ITfSource methods **
    STDMETHODIMP AdviseSink(
        _In_ REFIID riid,
        _In_ IUnknown *punk,
        _Out_ DWORD *pdwCookie) override;
    STDMETHODIMP UnadviseSink(_In_ DWORD dwCookie) override;

    // ** ITfInputProcessorProfileMgr methods **
    STDMETHODIMP ActivateProfile(
        _In_ DWORD dwProfileType,
        _In_ LANGID langid,
        _In_ REFCLSID clsid,
        _In_ REFGUID guidProfile,
        _In_ HKL hkl,
        _In_ DWORD dwFlags) override;
    STDMETHODIMP DeactivateProfile(
        _In_ DWORD dwProfileType,
        _In_ LANGID langid,
        _In_ REFCLSID clsid,
        _In_ REFGUID guidProfile,
        _In_ HKL hkl,
        _In_ DWORD dwFlags) override;
    STDMETHODIMP GetProfile(
        _In_ DWORD dwProfileType,
        _In_ LANGID langid,
        _In_ REFCLSID clsid,
        _In_ REFGUID guidProfile,
        _In_ HKL hkl,
        _Out_ TF_INPUTPROCESSORPROFILE *pProfile) override;
    STDMETHODIMP EnumProfiles(
        _In_ LANGID langid,
        _Out_ IEnumTfInputProcessorProfiles **ppEnum) override;
    STDMETHODIMP ReleaseInputProcessor(
        _In_ REFCLSID rclsid,
        _In_ DWORD dwFlags) override;
    STDMETHODIMP RegisterProfile(
        _In_ REFCLSID rclsid,
        _In_ LANGID langid,
        _In_ REFGUID guidProfile,
        _In_ const WCHAR *pchDesc,
        _In_ ULONG cchDesc,
        _In_ const WCHAR *pchIconFile,
        _In_ ULONG cchFile,
        _In_ ULONG uIconIndex,
        _In_ HKL hklsubstitute,
        _In_ DWORD dwPreferredLayout,
        _In_ BOOL bEnabledByDefault,
        _In_ DWORD dwFlags) override;
    STDMETHODIMP UnregisterProfile(
        _In_ REFCLSID rclsid,
        _In_ LANGID langid,
        _In_ REFGUID guidProfile,
        _In_ DWORD dwFlags) override;
    STDMETHODIMP GetActiveProfile(
        _In_ REFGUID catid,
        _Out_ TF_INPUTPROCESSORPROFILE *pProfile) override;

protected:
    LONG m_cRefs;
    LANGID m_currentLanguage;
    struct list m_LanguageProfileNotifySink;
};

////////////////////////////////////////////////////////////////////////////

class CProfilesEnumGuid
    : public IEnumGUID
{
public:
    CProfilesEnumGuid();
    virtual ~CProfilesEnumGuid();

    static HRESULT CreateInstance(CProfilesEnumGuid **ppOut);

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID iid, LPVOID *ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** IEnumGUID methods **
    STDMETHODIMP Next(
        _In_ ULONG celt,
        _Out_ GUID *rgelt,
        _Out_ ULONG *pceltFetched) override;
    STDMETHODIMP Skip(_In_ ULONG celt) override;
    STDMETHODIMP Reset() override;
    STDMETHODIMP Clone(_Out_ IEnumGUID **ppenum) override;

protected:
    LONG m_cRefs;
    HKEY m_key;
    DWORD m_next_index;
};

////////////////////////////////////////////////////////////////////////////

class CEnumTfLanguageProfiles
    : public IEnumTfLanguageProfiles
{
public:
    CEnumTfLanguageProfiles(LANGID langid);
    virtual ~CEnumTfLanguageProfiles();

    static HRESULT CreateInstance(LANGID langid, CEnumTfLanguageProfiles **out);

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID iid, LPVOID *ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** IEnumTfLanguageProfiles methods **
    STDMETHODIMP Clone(_Out_ IEnumTfLanguageProfiles **ppEnum) override;
    STDMETHODIMP Next(
        _In_ ULONG ulCount,
        _Out_ TF_LANGUAGEPROFILE *pProfile,
        _Out_ ULONG *pcFetch) override;
    STDMETHODIMP Reset() override;
    STDMETHODIMP Skip(_In_ ULONG ulCount) override;

protected:
    LONG m_cRefs;
    HKEY m_tipkey;
    DWORD m_tip_index;
    WCHAR m_szwCurrentClsid[39];
    HKEY m_langkey;
    DWORD m_lang_index;
    LANGID m_langid;
    ITfCategoryMgr *m_catmgr;

    INT next_LanguageProfile(CLSID clsid, TF_LANGUAGEPROFILE *tflp);
};

////////////////////////////////////////////////////////////////////////////

class CEnumTfInputProcessorProfiles
    : public IEnumTfInputProcessorProfiles
{
public:
    CEnumTfInputProcessorProfiles();
    virtual ~CEnumTfInputProcessorProfiles();

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID iid, LPVOID *ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** IEnumTfInputProcessorProfiles methods **
    STDMETHODIMP Clone(_Out_ IEnumTfInputProcessorProfiles **ppEnum) override;
    STDMETHODIMP Next(
        _In_ ULONG ulCount,
        _Out_ TF_INPUTPROCESSORPROFILE *pProfile,
        _Out_ ULONG *pcFetch) override;
    STDMETHODIMP Reset() override;
    STDMETHODIMP Skip(_In_ ULONG ulCount) override;

protected:
    LONG m_cRefs;
};

////////////////////////////////////////////////////////////////////////////

static void
add_userkey(_In_ REFCLSID rclsid, _In_ LANGID langid, _In_ REFGUID guidProfile)
{
    HKEY key;
    WCHAR buf[39], buf2[39], fullkey[168];
    DWORD disposition = 0;
    LSTATUS error;

    TRACE("\n");

    StringFromGUID2(rclsid, buf, _countof(buf));
    StringFromGUID2(guidProfile, buf2, _countof(buf2));
    StringCchPrintfW(fullkey, _countof(fullkey), L"%s\\%s\\%s\\0x%08x\\%s",
                     szwSystemTIPKey, buf, L"LanguageProfile", langid, buf2);

    error = RegCreateKeyExW(HKEY_CURRENT_USER, fullkey, 0, NULL, 0,
                            KEY_READ | KEY_WRITE, NULL, &key, &disposition);

    if (error == ERROR_SUCCESS && disposition == REG_CREATED_NEW_KEY)
    {
        DWORD zero = 0;
        RegSetValueExW(key, L"Enable", 0, REG_DWORD, (PBYTE)&zero, sizeof(DWORD));
    }

    if (error == ERROR_SUCCESS)
        RegCloseKey(key);
}

////////////////////////////////////////////////////////////////////////////

CInputProcessorProfiles::CInputProcessorProfiles()
    : m_cRefs(1)
    , m_currentLanguage(::GetUserDefaultLCID())
{
    list_init(&m_LanguageProfileNotifySink);
}

CInputProcessorProfiles::~CInputProcessorProfiles()
{
    TRACE("destroying %p\n", this);
    free_sinks(&m_LanguageProfileNotifySink);
}

STDMETHODIMP CInputProcessorProfiles::QueryInterface(REFIID iid, LPVOID *ppvObj)
{
    *ppvObj = NULL;

    if (iid == IID_IUnknown || iid == IID_ITfInputProcessorProfiles)
        *ppvObj = static_cast<ITfInputProcessorProfiles *>(this);
    else if (iid == IID_ITfInputProcessorProfileMgr)
        *ppvObj = static_cast<ITfInputProcessorProfileMgr *>(this);
    else if (iid == IID_ITfSource)
        *ppvObj = static_cast<ITfSource *>(this);

    if (!*ppvObj)
    {
        WARN("unsupported interface: %s\n", debugstr_guid(&iid));
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CInputProcessorProfiles::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CInputProcessorProfiles::Release()
{
    ULONG ret = ::InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP CInputProcessorProfiles::Register(_In_ REFCLSID rclsid)
{
    HKEY tipkey;
    WCHAR buf[39], fullkey[68];

    TRACE("(%p) %s\n", this, debugstr_guid(&rclsid));

    StringFromGUID2(rclsid, buf, _countof(buf));
    StringCchPrintfW(fullkey, _countof(fullkey), L"%s\\%s", szwSystemTIPKey, buf);

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, fullkey, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL,
                        &tipkey, NULL) != ERROR_SUCCESS)
    {
        return E_FAIL;
    }

    RegCloseKey(tipkey);
    return S_OK;
}

STDMETHODIMP CInputProcessorProfiles::Unregister(_In_ REFCLSID rclsid)
{
    WCHAR buf[39], fullkey[68];

    TRACE("(%p) %s\n", this, debugstr_guid(&rclsid));

    StringFromGUID2(rclsid, buf, _countof(buf));
    StringCchPrintfW(fullkey, _countof(fullkey), L"%s\\%s", szwSystemTIPKey, buf);

    RegDeleteTreeW(HKEY_LOCAL_MACHINE, fullkey);
    RegDeleteTreeW(HKEY_CURRENT_USER, fullkey);
    return S_OK;
}

STDMETHODIMP CInputProcessorProfiles::AddLanguageProfile(
    _In_ REFCLSID rclsid,
    _In_ LANGID langid,
    _In_ REFGUID guidProfile,
    _In_ const WCHAR *pchDesc,
    _In_ ULONG cchDesc,
    _In_ const WCHAR *pchIconFile,
    _In_ ULONG cchFile,
    _In_ ULONG uIconIndex)
{
    HKEY tipkey, fmtkey;
    WCHAR buf[39], fullkey[100];
    DWORD disposition = 0;
    LSTATUS error;

    TRACE("(%p) %s %x %s %s %s %i\n", this, debugstr_guid(&rclsid), langid,
          debugstr_guid(&guidProfile), debugstr_wn(pchDesc, cchDesc),
          debugstr_wn(pchIconFile, cchFile), uIconIndex);

    StringFromGUID2(rclsid, buf, _countof(buf));
    StringCchPrintfW(fullkey, _countof(fullkey), L"%s\\%s", szwSystemTIPKey, buf);

    error = RegOpenKeyExW(HKEY_LOCAL_MACHINE, fullkey, 0, KEY_READ | KEY_WRITE, &tipkey);
    if (error != ERROR_SUCCESS)
        return E_FAIL;

    StringFromGUID2(guidProfile, buf, _countof(buf));
    StringCchPrintfW(fullkey, _countof(fullkey), L"%s\\0x%08x\\%s", L"LanguageProfile", langid, buf);

    error = RegCreateKeyExW(tipkey, fullkey, 0, NULL, 0, KEY_READ | KEY_WRITE,
                            NULL, &fmtkey, &disposition);
    if (error == ERROR_SUCCESS)
    {
        DWORD zero = 0x0;
        RegSetValueExW(fmtkey, L"Description", 0, REG_SZ, (PBYTE)pchDesc, cchDesc * sizeof(WCHAR));
        RegSetValueExW(fmtkey, L"IconFile", 0, REG_SZ, (PBYTE)pchIconFile, cchFile * sizeof(WCHAR));
        RegSetValueExW(fmtkey, L"IconIndex", 0, REG_DWORD, (PBYTE)&uIconIndex, sizeof(uIconIndex));
        if (disposition == REG_CREATED_NEW_KEY)
            RegSetValueExW(fmtkey, L"Enable", 0, REG_DWORD, (PBYTE)&zero, sizeof(DWORD));
        RegCloseKey(fmtkey);
        add_userkey(rclsid, langid, guidProfile);
    }
    RegCloseKey(tipkey);

    return (error == ERROR_SUCCESS) ? S_OK : E_FAIL;
}

STDMETHODIMP CInputProcessorProfiles::RemoveLanguageProfile(
    _In_ REFCLSID rclsid,
    _In_ LANGID langid,
    _In_ REFGUID guidProfile)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CInputProcessorProfiles::EnumInputProcessorInfo(_Out_ IEnumGUID **ppEnum)
{
    TRACE("(%p) %p\n", this, ppEnum);

    return CProfilesEnumGuid::CreateInstance((CProfilesEnumGuid **)ppEnum);
}

STDMETHODIMP CInputProcessorProfiles::GetDefaultLanguageProfile(
    _In_ LANGID langid,
    _In_ REFGUID catid,
    _Out_ CLSID *pclsid,
    _Out_ GUID *pguidProfile)
{
    WCHAR fullkey[168], buf[39];
    HKEY hKey;
    DWORD count;
    LSTATUS error;

    TRACE("%p) %x %s %p %p\n", this, langid, debugstr_guid(&catid), pclsid, pguidProfile);

    if (cicIsNullPtr(&catid) || !pclsid || !pguidProfile)
        return E_INVALIDARG;

    StringFromGUID2(catid, buf, _countof(buf));
    StringCchPrintfW(fullkey, _countof(fullkey), L"%s\\%s\\0x%08x\\%s", szwSystemCTFKey,
                     L"Assemblies", langid, buf);

    error = RegOpenKeyExW(HKEY_CURRENT_USER, fullkey, 0, KEY_READ | KEY_WRITE, &hKey);
    if (error != ERROR_SUCCESS)
        return S_FALSE;

    count = sizeof(buf);
    error = RegQueryValueExW(hKey, L"Default", 0, NULL, (PBYTE)buf, &count);
    if (error != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return S_FALSE;
    }
    CLSIDFromString(buf, pclsid);

    error = RegQueryValueExW(hKey, L"Profile", 0, NULL, (PBYTE)buf, &count);
    if (error == ERROR_SUCCESS)
        CLSIDFromString(buf, pguidProfile);

    RegCloseKey(hKey);

    return S_OK;
}

STDMETHODIMP CInputProcessorProfiles::SetDefaultLanguageProfile(
    _In_ LANGID langid,
    _In_ REFCLSID rclsid,
    _In_ REFGUID guidProfiles)
{
    WCHAR fullkey[168], buf[39];
    HKEY hKey;
    GUID catid;
    HRESULT hr;
    ITfCategoryMgr *catmgr;
    LSTATUS error;
    static const GUID * tipcats[3] = { &GUID_TFCAT_TIP_KEYBOARD,
                                       &GUID_TFCAT_TIP_SPEECH,
                                       &GUID_TFCAT_TIP_HANDWRITING };

    TRACE("(%p) %x %s %s\n", this, langid, debugstr_guid(&rclsid), debugstr_guid(&guidProfiles));

    if (cicIsNullPtr(&rclsid) || cicIsNullPtr(&guidProfiles))
        return E_INVALIDARG;

    hr = CategoryMgr_Constructor(NULL, (IUnknown**)&catmgr);
    if (FAILED(hr))
        return hr;

    if (catmgr->FindClosestCategory(rclsid, &catid, tipcats, _countof(tipcats)) != S_OK)
        hr = catmgr->FindClosestCategory(rclsid, &catid, NULL, 0);
    catmgr->Release();
    if (FAILED(hr))
        return E_FAIL;

    StringFromGUID2(catid, buf, _countof(buf));
    StringCchPrintfW(fullkey, _countof(fullkey), L"%s\\%s\\0x%08x\\%s", szwSystemCTFKey, L"Assemblies", langid, buf);

    error = RegCreateKeyExW(HKEY_CURRENT_USER, fullkey, 0, NULL, 0, KEY_READ | KEY_WRITE,
                            NULL, &hKey, NULL);
    if (error != ERROR_SUCCESS)
        return E_FAIL;

    StringFromGUID2(rclsid, buf, _countof(buf));
    RegSetValueExW(hKey, L"Default", 0, REG_SZ, (PBYTE)buf, sizeof(buf));
    StringFromGUID2(guidProfiles, buf, _countof(buf));
    RegSetValueExW(hKey, L"Profile", 0, REG_SZ, (PBYTE)buf, sizeof(buf));
    RegCloseKey(hKey);

    return S_OK;
}

STDMETHODIMP CInputProcessorProfiles::ActivateLanguageProfile(
    _In_ REFCLSID rclsid,
    _In_ LANGID langid,
    _In_ REFGUID guidProfiles)
{
    HRESULT hr;
    BOOL enabled;
    TF_LANGUAGEPROFILE LanguageProfile;

    TRACE("(%p) %s %x %s\n", this, debugstr_guid(&rclsid), langid, debugstr_guid(&guidProfiles));

    if (langid != m_currentLanguage)
        return E_INVALIDARG;

    if (get_active_textservice(rclsid, NULL))
    {
        TRACE("Already Active\n");
        return E_FAIL;
    }

    hr = IsEnabledLanguageProfile(rclsid, langid, guidProfiles, &enabled);
    if (FAILED(hr) || !enabled)
    {
        TRACE("Not Enabled\n");
        return E_FAIL;
    }

    LanguageProfile.clsid = rclsid;
    LanguageProfile.langid = langid;
    LanguageProfile.guidProfile = guidProfiles;
    LanguageProfile.fActive = TRUE;
    return add_active_textservice(&LanguageProfile);
}

STDMETHODIMP CInputProcessorProfiles::GetActiveLanguageProfile(
    _In_ REFCLSID rclsid,
    _Out_ LANGID *plangid,
    _Out_ GUID *pguidProfile)
{
    TF_LANGUAGEPROFILE profile;

    TRACE("(%p) %s %p %p\n", this, debugstr_guid(&rclsid), plangid, pguidProfile);

    if (cicIsNullPtr(&rclsid) || !plangid || !pguidProfile)
        return E_INVALIDARG;

    if (get_active_textservice(rclsid, &profile))
    {
        *plangid = profile.langid;
        *pguidProfile = profile.guidProfile;
        return S_OK;
    }
    else
    {
        *pguidProfile = GUID_NULL;
        return S_FALSE;
    }
}

STDMETHODIMP CInputProcessorProfiles::GetLanguageProfileDescription(
    _In_ REFCLSID rclsid,
    _In_ LANGID langid,
    _In_ REFGUID guidProfile,
    _Out_ BSTR *pbstrProfile)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CInputProcessorProfiles::GetCurrentLanguage(_Out_ LANGID *plangid)
{
    TRACE("(%p) 0x%x\n", this, m_currentLanguage);

    if (!plangid)
        return E_INVALIDARG;

    *plangid = m_currentLanguage;
    return S_OK;
}

STDMETHODIMP CInputProcessorProfiles::ChangeCurrentLanguage(_In_ LANGID langid)
{
    ITfLanguageProfileNotifySink *sink;
    struct list *cursor;
    BOOL accept;

    FIXME("STUB:(%p)\n", this);

    SINK_FOR_EACH(cursor, &m_LanguageProfileNotifySink, ITfLanguageProfileNotifySink, sink)
    {
        accept = TRUE;
        sink->OnLanguageChange(langid, &accept);
        if (!accept)
            return E_FAIL;
    }

    /* TODO:  On successful language change call OnLanguageChanged sink */
    return E_NOTIMPL;
}

STDMETHODIMP CInputProcessorProfiles::GetLanguageList(
    _Out_ LANGID **ppLangId,
    _Out_ ULONG *pulCount)
{
    FIXME("Semi-STUB:(%p)\n", this);
    *ppLangId = (LANGID *)CoTaskMemAlloc(sizeof(LANGID));
    **ppLangId = m_currentLanguage;
    *pulCount = 1;
    return S_OK;
}

STDMETHODIMP CInputProcessorProfiles::EnumLanguageProfiles(
    _In_ LANGID langid,
    _Out_ IEnumTfLanguageProfiles **ppEnum)
{
    TRACE("(%p) %x %p\n", this, langid, ppEnum);

    if (!ppEnum)
        return E_INVALIDARG;

    CEnumTfLanguageProfiles *profenum;
    HRESULT hr = CEnumTfLanguageProfiles::CreateInstance(langid, &profenum);
    *ppEnum = static_cast<IEnumTfLanguageProfiles *>(profenum);
    return hr;
}

STDMETHODIMP CInputProcessorProfiles::EnableLanguageProfile(
    _In_ REFCLSID rclsid,
    _In_ LANGID langid,
    _In_ REFGUID guidProfile,
    _In_ BOOL fEnable)
{
    HKEY key;
    WCHAR buf[39], buf2[39], fullkey[168];
    LSTATUS error;

    TRACE("(%p) %s %x %s %i\n", this, debugstr_guid(&rclsid), langid, debugstr_guid(&guidProfile), fEnable);

    StringFromGUID2(rclsid, buf, _countof(buf));
    StringFromGUID2(guidProfile, buf2, _countof(buf2));
    StringCchPrintfW(fullkey, _countof(fullkey), L"%s\\%s\\%s\\0x%08x\\%s", szwSystemTIPKey, buf,
                     L"LanguageProfile", langid, buf2);

    error = RegOpenKeyExW(HKEY_CURRENT_USER, fullkey, 0, KEY_READ | KEY_WRITE, &key);
    if (error != ERROR_SUCCESS)
        return E_FAIL;

    RegSetValueExW(key, L"Enable", 0, REG_DWORD, (LPBYTE)&fEnable, sizeof(fEnable));
    RegCloseKey(key);
    return S_OK;
}

STDMETHODIMP CInputProcessorProfiles::IsEnabledLanguageProfile(
    _In_ REFCLSID rclsid,
    _In_ LANGID langid,
    _In_ REFGUID guidProfile,
    _Out_ BOOL *pfEnable)
{
    HKEY key;
    WCHAR buf[39], buf2[39], fullkey[168];
    LSTATUS error;

    TRACE("(%p) %s, %i, %s, %p\n", this, debugstr_guid(&rclsid), langid, debugstr_guid(&guidProfile), pfEnable);

    if (!pfEnable)
        return E_INVALIDARG;

    StringFromGUID2(rclsid, buf, _countof(buf));
    StringFromGUID2(guidProfile, buf2, _countof(buf2));
    StringCchPrintfW(fullkey, _countof(fullkey), L"%s\\%s\\%s\\0x%08x\\%s", szwSystemTIPKey,
                     buf, L"LanguageProfile", langid, buf2);

    error = RegOpenKeyExW(HKEY_CURRENT_USER, fullkey, 0, KEY_READ | KEY_WRITE, &key);
    if (error == ERROR_SUCCESS)
    {
        DWORD count = sizeof(DWORD);
        error = RegQueryValueExW(key, L"Enable", 0, NULL, (LPBYTE)pfEnable, &count);
        RegCloseKey(key);
    }

    if (error != ERROR_SUCCESS) /* Try Default */
    {
        error = RegOpenKeyExW(HKEY_LOCAL_MACHINE, fullkey, 0, KEY_READ | KEY_WRITE, &key);
        if (error == ERROR_SUCCESS)
        {
            DWORD count = sizeof(DWORD);
            error = RegQueryValueExW(key, L"Enable", 0, NULL, (LPBYTE)pfEnable, &count);
            RegCloseKey(key);
        }
    }

    return (error == ERROR_SUCCESS) ? S_OK : E_FAIL;
}

STDMETHODIMP CInputProcessorProfiles::EnableLanguageProfileByDefault(
    _In_ REFCLSID rclsid,
    _In_ LANGID langid,
    _In_ REFGUID guidProfile,
    _In_ BOOL fEnable)
{
    HKEY key;
    WCHAR buf[39], buf2[39], fullkey[168];
    LSTATUS error;

    TRACE("(%p) %s %x %s %i\n", this, debugstr_guid(&rclsid), langid, debugstr_guid(&guidProfile), fEnable);

    StringFromGUID2(rclsid, buf, _countof(buf));
    StringFromGUID2(guidProfile, buf2, _countof(buf2));
    StringCchPrintfW(fullkey, _countof(fullkey), L"%s\\%s\\%s\\0x%08x\\%s", szwSystemTIPKey,
                     buf, L"LanguageProfile", langid, buf2);

    error = RegOpenKeyExW(HKEY_LOCAL_MACHINE, fullkey, 0, KEY_READ | KEY_WRITE, &key);
    if (error != ERROR_SUCCESS)
        return E_FAIL;

    RegSetValueExW(key, L"Enable", 0, REG_DWORD, (PBYTE)&fEnable, sizeof(fEnable));
    RegCloseKey(key);
    return S_OK;
}

STDMETHODIMP CInputProcessorProfiles::SubstituteKeyboardLayout(
    _In_ REFCLSID rclsid,
    _In_ LANGID langid,
    _In_ REFGUID guidProfile,
    _In_ HKL hKL)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CInputProcessorProfiles::ActivateProfile(
    _In_ DWORD dwProfileType,
    _In_ LANGID langid,
    _In_ REFCLSID clsid,
    _In_ REFGUID guidProfile,
    _In_ HKL hkl,
    _In_ DWORD dwFlags)
{
    FIXME("(%p)->(%d %x %s %s %p %x)\n", this, dwProfileType, langid, debugstr_guid(&clsid),
          debugstr_guid(&guidProfile), hkl, dwFlags);
    return E_NOTIMPL;
}

STDMETHODIMP CInputProcessorProfiles::DeactivateProfile(
    _In_ DWORD dwProfileType,
    _In_ LANGID langid,
    _In_ REFCLSID clsid,
    _In_ REFGUID guidProfile,
    _In_ HKL hkl,
    _In_ DWORD dwFlags)
{
    FIXME("(%p)->(%d %x %s %s %p %x)\n", this, dwProfileType, langid, debugstr_guid(&clsid),
          debugstr_guid(&guidProfile), hkl, dwFlags);
    return E_NOTIMPL;
}

STDMETHODIMP CInputProcessorProfiles::GetProfile(
    _In_ DWORD dwProfileType,
    _In_ LANGID langid,
    _In_ REFCLSID clsid,
    _In_ REFGUID guidProfile,
    _In_ HKL hkl,
    _Out_ TF_INPUTPROCESSORPROFILE *pProfile)
{
    FIXME("(%p)->(%d %x %s %s %p %p)\n", this, dwProfileType, langid, debugstr_guid(&clsid),
          debugstr_guid(&guidProfile), hkl, pProfile);
    return E_NOTIMPL;
}

STDMETHODIMP CInputProcessorProfiles::EnumProfiles(
    _In_ LANGID langid,
    _Out_ IEnumTfInputProcessorProfiles **ppEnum)
{
    TRACE("(%p)->(%x %p)\n", this, langid, ppEnum);

    CEnumTfInputProcessorProfiles *enum_profiles = new(cicNoThrow) CEnumTfInputProcessorProfiles();
    if (!enum_profiles)
        return E_OUTOFMEMORY;

    *ppEnum = static_cast<IEnumTfInputProcessorProfiles *>(enum_profiles);
    return S_OK;
}

STDMETHODIMP CInputProcessorProfiles::ReleaseInputProcessor(
    _In_ REFCLSID rclsid,
    _In_ DWORD dwFlags)
{
    FIXME("(%p)->(%s %x)\n", this, debugstr_guid(&rclsid), dwFlags);
    return E_NOTIMPL;
}

STDMETHODIMP CInputProcessorProfiles::RegisterProfile(
    _In_ REFCLSID rclsid,
    _In_ LANGID langid,
    _In_ REFGUID guidProfile,
    _In_ const WCHAR *pchDesc,
    _In_ ULONG cchDesc,
    _In_ const WCHAR *pchIconFile,
    _In_ ULONG cchFile,
    _In_ ULONG uIconIndex,
    _In_ HKL hklsubstitute,
    _In_ DWORD dwPreferredLayout,
    _In_ BOOL bEnabledByDefault,
    _In_ DWORD dwFlags)
{
    FIXME("(%p)->(%s %x %s %s %d %s %u %u %p %x %x %x)\n", this, debugstr_guid(&rclsid), langid,
          debugstr_guid(&guidProfile), debugstr_w(pchDesc), cchDesc, debugstr_w(pchIconFile),
          cchFile, uIconIndex, hklsubstitute, dwPreferredLayout, bEnabledByDefault, dwFlags);
    return E_NOTIMPL;
}

STDMETHODIMP CInputProcessorProfiles::UnregisterProfile(
    _In_ REFCLSID rclsid,
    _In_ LANGID langid,
    _In_ REFGUID guidProfile,
    _In_ DWORD dwFlags)
{
    FIXME("(%p)->(%s %x %s %x)\n", this, debugstr_guid(&rclsid), langid,
          debugstr_guid(&guidProfile), dwFlags);
    return E_NOTIMPL;
}

STDMETHODIMP CInputProcessorProfiles::GetActiveProfile(
    _In_ REFGUID catid,
    _Out_ TF_INPUTPROCESSORPROFILE *pProfile)
{
    FIXME("(%p)->(%s %p)\n", this, debugstr_guid(&catid), pProfile);
    return E_NOTIMPL;
}

STDMETHODIMP CInputProcessorProfiles::AdviseSink(
    _In_ REFIID riid,
    _In_ IUnknown *punk,
    _Out_ DWORD *pdwCookie)
{
    TRACE("(%p) %s %p %p\n", this, debugstr_guid(&riid), punk, pdwCookie);

    if (cicIsNullPtr(&riid) || !punk || !pdwCookie)
        return E_INVALIDARG;

    if (riid == IID_ITfLanguageProfileNotifySink)
        return advise_sink(&m_LanguageProfileNotifySink, IID_ITfLanguageProfileNotifySink,
                           COOKIE_MAGIC_IPPSINK, punk, pdwCookie);

    FIXME("(%p) Unhandled Sink: %s\n", this, debugstr_guid(&riid));
    return E_NOTIMPL;
}

STDMETHODIMP CInputProcessorProfiles::UnadviseSink(_In_ DWORD dwCookie)
{
    TRACE("(%p) %x\n", this, dwCookie);

    if (get_Cookie_magic(dwCookie) != COOKIE_MAGIC_IPPSINK)
        return E_INVALIDARG;

    return unadvise_sink(dwCookie);
}

HRESULT CInputProcessorProfiles::CreateInstance(IUnknown *pUnkOuter, CInputProcessorProfiles **ppOut)
{
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CInputProcessorProfiles *This = new(cicNoThrow) CInputProcessorProfiles();
    if (!This)
        return E_OUTOFMEMORY;

    *ppOut = This;
    TRACE("returning %p\n", *ppOut);
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////

CProfilesEnumGuid::CProfilesEnumGuid()
    : m_cRefs(1)
    , m_key(NULL)
    , m_next_index(0)
{
}

CProfilesEnumGuid::~CProfilesEnumGuid()
{
    TRACE("destroying %p\n", this);
    RegCloseKey(m_key);
}

STDMETHODIMP CProfilesEnumGuid::QueryInterface(REFIID iid, LPVOID *ppvObj)
{
    *ppvObj = NULL;

    if (iid == IID_IUnknown || iid == IID_IEnumGUID)
        *ppvObj = static_cast<IEnumGUID *>(this);

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(&iid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CProfilesEnumGuid::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CProfilesEnumGuid::Release()
{
    ULONG ret = ::InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP CProfilesEnumGuid::Next(
    _In_ ULONG celt,
    _Out_ GUID *rgelt,
    _Out_ ULONG *pceltFetched)
{
    ULONG fetched = 0;

    TRACE("(%p)\n", this);

    if (!rgelt)
        return E_POINTER;

    if (m_key)
    {
        while (fetched < celt)
        {
            LSTATUS error;
            HRESULT hr;
            WCHAR catid[39];
            DWORD cName = _countof(catid);

            error = RegEnumKeyExW(m_key, m_next_index, catid, &cName, NULL, NULL, NULL, NULL);
            if (error != ERROR_SUCCESS && error != ERROR_MORE_DATA)
                break;

            ++m_next_index;

            hr = CLSIDFromString(catid, rgelt);
            if (FAILED(hr))
                continue;

            ++fetched;
            ++rgelt;
        }
    }

    if (pceltFetched)
        *pceltFetched = fetched;
    return fetched == celt ? S_OK : S_FALSE;
}

STDMETHODIMP CProfilesEnumGuid::Skip(_In_ ULONG celt)
{
    TRACE("(%p)\n", this);
    m_next_index += celt;
    return S_OK;
}

STDMETHODIMP CProfilesEnumGuid::Reset()
{
    TRACE("(%p)\n", this);
    m_next_index = 0;
    return S_OK;
}

STDMETHODIMP CProfilesEnumGuid::Clone(_Out_ IEnumGUID **ppenum)
{
    TRACE("(%p)\n", this);

    if (!ppenum)
        return E_POINTER;

    *ppenum = NULL;

    CProfilesEnumGuid *pEnum;
    HRESULT hr = CProfilesEnumGuid::CreateInstance(&pEnum);
    if (SUCCEEDED(hr))
    {
        pEnum->m_next_index = m_next_index;
        *ppenum = static_cast<IEnumGUID *>(pEnum);
    }

    return hr;
}

HRESULT CProfilesEnumGuid::CreateInstance(CProfilesEnumGuid **ppOut)
{
    CProfilesEnumGuid *This = new(cicNoThrow) CProfilesEnumGuid();
    if (!This)
        return E_OUTOFMEMORY;

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, szwSystemTIPKey, 0, NULL, 0,
                        KEY_READ | KEY_WRITE, NULL, &This->m_key, NULL) != ERROR_SUCCESS)
    {
        delete This;
        return E_FAIL;
    }

    *ppOut = This;
    TRACE("returning %p\n", *ppOut);
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////

CEnumTfLanguageProfiles::CEnumTfLanguageProfiles(LANGID langid)
    : m_cRefs(1)
    , m_langid(langid)
{
}

CEnumTfLanguageProfiles::~CEnumTfLanguageProfiles()
{
    TRACE("destroying %p\n", this);
    RegCloseKey(m_tipkey);
    if (m_langkey)
        RegCloseKey(m_langkey);
    m_catmgr->Release();
}

HRESULT CEnumTfLanguageProfiles::CreateInstance(LANGID langid, CEnumTfLanguageProfiles **out)
{
    HRESULT hr;
    CEnumTfLanguageProfiles *This = new(cicNoThrow) CEnumTfLanguageProfiles(langid);
    if (!This)
        return E_OUTOFMEMORY;

    hr = CategoryMgr_Constructor(NULL, (IUnknown**)&This->m_catmgr);
    if (FAILED(hr))
    {
        delete This;
        return hr;
    }

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, szwSystemTIPKey, 0, NULL, 0,
                        KEY_READ | KEY_WRITE, NULL, &This->m_tipkey, NULL) != ERROR_SUCCESS)
    {
        delete This;
        return E_FAIL;
    }

    *out = This;
    TRACE("returning %p\n", *out);
    return S_OK;
}

STDMETHODIMP CEnumTfLanguageProfiles::QueryInterface(REFIID iid, LPVOID *ppvObj)
{
    *ppvObj = NULL;

    if (iid == IID_IUnknown || iid == IID_IEnumTfLanguageProfiles)
        *ppvObj = static_cast<IEnumTfLanguageProfiles *>(this);

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(&iid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEnumTfLanguageProfiles::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CEnumTfLanguageProfiles::Release()
{
    ULONG ret = ::InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

INT CEnumTfLanguageProfiles::next_LanguageProfile(CLSID clsid, TF_LANGUAGEPROFILE *tflp)
{
    WCHAR fullkey[168], profileid[39];
    LSTATUS error;
    DWORD cName = _countof(profileid);
    GUID profile;

    if (!m_langkey)
    {
        StringCchPrintfW(fullkey, _countof(fullkey), L"%s\\%s\\0x%08x", m_szwCurrentClsid,
                         L"LanguageProfile", m_langid);
        error = RegOpenKeyExW(m_tipkey, fullkey, 0, KEY_READ | KEY_WRITE, &m_langkey);
        if (error != ERROR_SUCCESS)
        {
            m_langkey = NULL;
            return -1;
        }
        m_lang_index = 0;
    }

    error = RegEnumKeyExW(m_langkey, m_lang_index, profileid, &cName, NULL, NULL, NULL, NULL);
    if (error != ERROR_SUCCESS && error != ERROR_MORE_DATA)
    {
        RegCloseKey(m_langkey);
        m_langkey = NULL;
        return -1;
    }
    ++m_lang_index;

    if (tflp)
    {
        static const GUID * tipcats[3] = { &GUID_TFCAT_TIP_KEYBOARD,
                                           &GUID_TFCAT_TIP_SPEECH,
                                           &GUID_TFCAT_TIP_HANDWRITING };
        HRESULT hr = CLSIDFromString(profileid, &profile);
        if (FAILED(hr))
            return 0;

        tflp->clsid = clsid;
        tflp->langid = m_langid;
        tflp->fActive = get_active_textservice(clsid, NULL);
        tflp->guidProfile = profile;
        if (m_catmgr->FindClosestCategory(clsid, &tflp->catid, tipcats, 3) != S_OK)
            m_catmgr->FindClosestCategory(clsid, &tflp->catid, NULL, 0);
    }

    return 1;
}

STDMETHODIMP CEnumTfLanguageProfiles::Next(
    _In_ ULONG ulCount,
    _Out_ TF_LANGUAGEPROFILE *pProfile,
    _Out_ ULONG *pcFetch)
{
    ULONG fetched = 0;

    TRACE("(%p)\n", this);

    if (!pProfile)
        return E_POINTER;

    if (m_tipkey)
    {
        while (fetched < ulCount)
        {
            LSTATUS error;
            HRESULT hr;
            DWORD cName = _countof(m_szwCurrentClsid);
            CLSID clsid;

            error = RegEnumKeyExW(m_tipkey, m_tip_index, m_szwCurrentClsid, &cName, NULL, NULL,
                                  NULL, NULL);
            if (error != ERROR_SUCCESS && error != ERROR_MORE_DATA)
                break;

            ++m_tip_index;

            hr = CLSIDFromString(m_szwCurrentClsid, &clsid);
            if (FAILED(hr))
                continue;

            while (fetched < ulCount)
            {
                INT res = next_LanguageProfile(clsid, pProfile);
                if (res == 1)
                {
                    ++fetched;
                    ++pProfile;
                }
                else if (res == -1)
                {
                    break;
                }
            }
        }
    }

    if (pcFetch)
        *pcFetch = fetched;
    return (fetched == ulCount) ? S_OK : S_FALSE;
}

STDMETHODIMP CEnumTfLanguageProfiles::Skip(_In_ ULONG celt)
{
    FIXME("STUB (%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CEnumTfLanguageProfiles::Reset()
{
    TRACE("(%p)\n", this);
    m_tip_index = 0;
    if (m_langkey)
        RegCloseKey(m_langkey);
    m_langkey = NULL;
    m_lang_index = 0;
    return S_OK;
}

STDMETHODIMP CEnumTfLanguageProfiles::Clone(_Out_ IEnumTfLanguageProfiles **ppEnum)
{
    TRACE("(%p)\n", this);

    if (!ppEnum)
        return E_POINTER;

    *ppEnum = NULL;

    CEnumTfLanguageProfiles *new_This;
    HRESULT hr = CEnumTfLanguageProfiles::CreateInstance(m_langid, &new_This);
    if (FAILED(hr))
        return hr;

    new_This->m_tip_index = m_tip_index;
    lstrcpynW(new_This->m_szwCurrentClsid, m_szwCurrentClsid, _countof(new_This->m_szwCurrentClsid));

    if (m_langkey)
    {
        WCHAR fullkey[168];
        StringCchPrintfW(fullkey, _countof(fullkey), L"%s\\%s\\0x%08x", m_szwCurrentClsid, L"LanguageProfile", m_langid);
        RegOpenKeyExW(new_This->m_tipkey, fullkey, 0, KEY_READ | KEY_WRITE, &new_This->m_langkey);
        new_This->m_lang_index = m_lang_index;
    }

    *ppEnum = static_cast<IEnumTfLanguageProfiles *>(new_This);
    return hr;
}

////////////////////////////////////////////////////////////////////////////

CEnumTfInputProcessorProfiles::CEnumTfInputProcessorProfiles()
    : m_cRefs(1)
{
}

CEnumTfInputProcessorProfiles::~CEnumTfInputProcessorProfiles()
{
}

STDMETHODIMP CEnumTfInputProcessorProfiles::QueryInterface(REFIID iid, LPVOID *ppvObj)
{
    if (!ppvObj)
        return E_POINTER;

    *ppvObj = NULL;
    if (iid == IID_IUnknown || iid == IID_IEnumTfInputProcessorProfiles)
        *ppvObj = static_cast<IEnumTfInputProcessorProfiles *>(this);

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(&iid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEnumTfInputProcessorProfiles::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CEnumTfInputProcessorProfiles::Release()
{
    ULONG ret = ::InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP CEnumTfInputProcessorProfiles::Clone(_Out_ IEnumTfInputProcessorProfiles **ppEnum)
{
    FIXME("(%p)->(%p)\n", this, ppEnum);
    return E_NOTIMPL;
}

STDMETHODIMP CEnumTfInputProcessorProfiles::Next(
    _In_ ULONG ulCount,
    _Out_ TF_INPUTPROCESSORPROFILE *pProfile,
    _Out_ ULONG *pcFetch)
{
    FIXME("(%p)->(%u %p %p)\n", this, ulCount, pProfile, pcFetch);
    if (pcFetch)
        *pcFetch = 0;
    return S_FALSE;
}

STDMETHODIMP CEnumTfInputProcessorProfiles::Reset()
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CEnumTfInputProcessorProfiles::Skip(_In_ ULONG ulCount)
{
    FIXME("(%p)->(%u)\n", this, ulCount);
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////

EXTERN_C
HRESULT InputProcessorProfiles_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    return CInputProcessorProfiles::CreateInstance(pUnkOuter, (CInputProcessorProfiles **)ppOut);
}
