/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Miscellaneous of msctfime.ime
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

BOOLEAN DllShutdownInProgress(VOID);
BOOL IsEALang(_In_opt_ LANGID LangID);
BOOL IsInteractiveUserLogon(VOID);
BYTE GetCharsetFromLangId(_In_ DWORD dwValue);
HIMC GetActiveContext(VOID);
BOOL MsimtfIsGuidMapEnable(_In_ HIMC hIMC, _Out_opt_ LPBOOL pbValue);
BOOL IsVKDBEKey(_In_ UINT uVirtKey);

ITfCategoryMgr *GetUIMCat(PCIC_LIBTHREAD pLibThread);
HRESULT InitDisplayAttrbuteLib(PCIC_LIBTHREAD pLibThread);
HRESULT UninitDisplayAttrbuteLib(PCIC_LIBTHREAD pLibThread);

/***********************************************************************/

HRESULT
GetCompartment(
    IUnknown *pUnknown,
    REFGUID rguid,
    ITfCompartment **ppComp,
    BOOL bThread);

HRESULT
SetCompartmentDWORD(
    TfEditCookie cookie,
    IUnknown *pUnknown,
    REFGUID rguid,
    DWORD dwValue,
    BOOL bThread);

HRESULT
GetCompartmentDWORD(
    IUnknown *pUnknown,
    REFGUID rguid,
    LPDWORD pdwValue,
    BOOL bThread);

HRESULT
SetCompartmentUnknown(
    TfEditCookie cookie,
    IUnknown *pUnknown,
    REFGUID rguid,
    IUnknown *punkValue);

HRESULT
ClearCompartment(
    TfClientId tid,
    IUnknown *pUnknown,
    REFGUID rguid,
    BOOL bThread);

/***********************************************************************/

class CModeBias
{
public:
    GUID m_guid;

    CModeBias() : m_guid(GUID_NULL) { }

    GUID ConvertModeBias(LONG bias);
    LONG ConvertModeBias(REFGUID guid);
    void SetModeBias(REFGUID rguid);
};

/***********************************************************************/

class CFunctionProviderBase : public ITfFunctionProvider
{
protected:
    TfClientId m_clientId;
    GUID m_guid;
    BSTR m_bstr;
    LONG m_cRefs;

public:
    CFunctionProviderBase(_In_ TfClientId clientId);
    virtual ~CFunctionProviderBase();

    // IUnknown interface
    STDMETHODIMP QueryInterface(_In_ REFIID riid, _Out_ LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfFunctionProvider interface
    STDMETHODIMP GetType(_Out_ GUID *guid) override;
    STDMETHODIMP GetDescription(_Out_ BSTR *desc) override;
    //STDMETHODIMP GetFunction(_In_ REFGUID guid, _In_ REFIID riid, _Out_ IUnknown **func) = 0;

    BOOL Init(_In_ REFGUID rguid, _In_ LPCWSTR psz);
};

/***********************************************************************/

class CFunctionProvider : public CFunctionProviderBase
{
public:
    CFunctionProvider(_In_ TfClientId clientId);

    STDMETHODIMP GetFunction(_In_ REFGUID guid, _In_ REFIID riid, _Out_ IUnknown **func) override;
};

/***********************************************************************/

class CFnDocFeed : public IAImmFnDocFeed
{
    LONG m_cRefs;

public:
    CFnDocFeed();
    virtual ~CFnDocFeed();

    // IUnknown interface
    STDMETHODIMP QueryInterface(_In_ REFIID riid, _Out_ LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IAImmFnDocFeed interface
    STDMETHODIMP DocFeed() override;
    STDMETHODIMP ClearDocFeedBuffer() override;
    STDMETHODIMP StartReconvert() override;
    STDMETHODIMP StartUndoCompositionString() override;
};
