/*
 * PROJECT:     ReactOS MSC Shell Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Shell extension implementation
 * COPYRIGHT:   Copyright 2026 Whindmar Saksit <whindsaks@proton.me>
 */

#include <windows.h>
#include <atlbase.h>
#include <atlcom.h>
#include <strsafe.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <initguid.h>
#include <msxml2.h>

DEFINE_GUID(CLSID_MscExtractIcon, 0x7A80E4A8,0x8005,0x11D2,0xBC,0xF8,0x00,0xC0,0x4F,0x72,0xC7,0x17);

CComModule g_Module;

class CMscExtractIcon :
    public CComCoClass<CMscExtractIcon, &CLSID_MscExtractIcon>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IExtractIconW,
    public IPersistFile
{
protected:
    PWSTR m_File = NULL;

public:
    CMscExtractIcon()
    {
    }

    ~CMscExtractIcon()
    {
        SHFree(m_File);
    }

    HRESULT GetIconLocationFromMsc(_In_ PWSTR pszIconFile, _In_ UINT cchMax, _Out_ int *piIndex);

    HRESULT GetDefaultIconLocation(_In_ PWSTR pszIconFile, _In_ UINT cchMax, _Out_ int *piIndex)
    {
        *piIndex = 0;
        WCHAR szDir[MAX_PATH + 1 + 8 + 3];
        GetSystemDirectory(szDir, MAX_PATH);
        PathAddBackslashW(szDir);
        return StringCchPrintfW(pszIconFile, cchMax, L"%smmc.exe", szDir) == S_OK ? S_OK : S_FALSE;
    }

    // IExtractIconW
    IFACEMETHODIMP Extract(_In_ PCWSTR pszFile, _In_ UINT nIconIndex, _Out_ HICON *phiconLarge, _Out_ HICON *phiconSmall, _In_ UINT nIconSize) override
    {
        return S_FALSE; // All my paths are real files, call SHExtractIconsW/SHDefExtractIconW for me.
    }

    IFACEMETHODIMP GetIconLocation(_In_ UINT GilIn, _In_ PWSTR pszIconFile, _In_ UINT cchMax, _Out_ int *piIndex, _Out_ UINT *GilOut) override
    {
        *GilOut = GIL_PERINSTANCE; // Every .msc file has a unique icon

        if (GilIn & GIL_ASYNC)
            return E_PENDING;

        if ((GilIn & GIL_DEFAULTICON) || FAILED(GetIconLocationFromMsc(pszIconFile, cchMax, piIndex)))
            return GetDefaultIconLocation(pszIconFile, cchMax, piIndex);
        return S_OK;
    }

    // IPersistFile
    IFACEMETHODIMP GetClassID(_Out_ CLSID *pClassID) override
    {
        return E_NOTIMPL;
    }

    IFACEMETHODIMP IsDirty() override
    {
        return E_NOTIMPL;
    }

    IFACEMETHODIMP Load(_In_ LPCOLESTR pszFileName, _In_ DWORD dwMode) override
    {
        SHFree(m_File);
        m_File = NULL;
        return SHStrDupW(pszFileName, &m_File);
    }

    IFACEMETHODIMP Save(_In_ LPCOLESTR pszFileName, _In_ BOOL fRemember) override
    {
        return E_NOTIMPL;
    }

    IFACEMETHODIMP SaveCompleted(_In_ LPCOLESTR pszFileName) override
    {
        return E_NOTIMPL;
    }

    IFACEMETHODIMP GetCurFile(_In_ LPOLESTR *ppszFileName) override
    {
        return E_NOTIMPL;
    }

    DECLARE_NO_REGISTRY()
    DECLARE_NOT_AGGREGATABLE(CMscExtractIcon)

    BEGIN_COM_MAP(CMscExtractIcon)
        COM_INTERFACE_ENTRY_IID(IID_IExtractIconW, IExtractIconW)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFile, IPersistFile)
    END_COM_MAP()
};

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_MscExtractIcon, CMscExtractIcon)
END_OBJECT_MAP()

static HRESULT SetXPathSelectionLanguage(_In_ IXMLDOMDocument2 *pDoc)
{
    VARIANT v;
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = const_cast<PWSTR>(L"XPath"); // Use XPath, not the old XSLPattern
    return pDoc->setProperty(const_cast<PWSTR>(L"SelectionLanguage"), v);
}

static HRESULT GetAttributeValue(_In_ IXMLDOMNode *pNode, _In_ LPCWSTR pszName, _Out_ BSTR *pOut)
{
    CComPtr<IXMLDOMNamedNodeMap> pMap;
    HRESULT hr;
    *pOut = NULL;
    if (SUCCEEDED(hr = pNode->get_attributes(&pMap)))
    {
        if (SUCCEEDED(hr = pMap->getNamedItem(const_cast<PWSTR>(pszName), &pNode)))
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            if (pNode)
            {
                hr = pNode->get_text(pOut);
                if (SUCCEEDED(hr) && !*pOut)
                    hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
                pNode->Release();
            }
        }
    }
    return hr;
}

static HRESULT LoadXmlFromVariant(_In_ VARIANT *pVar, _Out_ IXMLDOMDocument2 **ppDoc)
{
    VARIANT_BOOL succ = VARIANT_FALSE;
    HRESULT hr = CoCreateInstance(CLSID_DOMDocument30, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IXMLDOMDocument, (void**)ppDoc);
    if (FAILED(hr))
        return hr;
    else if (SUCCEEDED(hr = (*ppDoc)->load(*pVar, &succ)) && succ)
        return hr;
    (*ppDoc)->Release();
    return SUCCEEDED(hr) ? E_FAIL : hr;
}

HRESULT CMscExtractIcon::GetIconLocationFromMsc(_In_ PWSTR pszIconFile, _In_ UINT cchMax, _Out_ int *piIndex)
{
    if (!m_File)
        return E_UNEXPECTED;

    CComPtr<IStream> pStream;
    HRESULT hr = SHCreateStreamOnFileW(m_File, STGM_READ | STGM_SHARE_DENY_WRITE, &pStream);
    if (FAILED(hr))
        return hr;
    VARIANT v;
    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = pStream;
    CComPtr<IXMLDOMDocument2> pDoc;
    if (FAILED(hr = LoadXmlFromVariant(&v, &pDoc)))
        return hr;
    SetXPathSelectionLanguage(pDoc);

    CComPtr<IXMLDOMNode> pDomNode;
    hr = pDoc->selectSingleNode(const_cast<PWSTR>(L"/MMC_ConsoleFile/VisualAttributes/Icon"), &pDomNode);
    if (FAILED(hr))
        return hr;

    BSTR bstr;
    *piIndex = 0;
    if (SUCCEEDED(GetAttributeValue(pDomNode, L"Index", &bstr)))
    {
        *piIndex = StrToIntW(bstr);
        SysFreeString(bstr);
    }

    if (SUCCEEDED(hr = GetAttributeValue(pDomNode, L"File", &bstr)))
    {
        hr = StringCchCopyW(pszIconFile, cchMax, bstr);
        SysFreeString(bstr);
    }
    return hr;
}

STDAPI DllCanUnloadNow()
{
    return g_Module.DllCanUnloadNow();
}

STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Out_ LPVOID *ppv)
{
    return g_Module.DllGetClassObject(rclsid, riid, ppv);
}

static HRESULT DllServerRegistration(_In_ BOOL Install)
{
    const BOOL HasTlb = FALSE;
    HRESULT hr1 = Install ? g_Module.DllRegisterServer(HasTlb) : g_Module.DllUnregisterServer(HasTlb);
    if (FAILED(hr1) && Install)
        return hr1;
    HRESULT hr2 = g_Module.UpdateRegistryFromResource(IDR_EXTRACTICON, Install, NULL);
    if (FAILED(hr2))
        return hr2;
    return hr1;
}

STDAPI DllRegisterServer()
{
    return DllServerRegistration(TRUE);
}

STDAPI DllUnregisterServer()
{
    return DllServerRegistration(FALSE);
}

EXTERN_C BOOL WINAPI DllMain(_In_ HINSTANCE hInstance, _In_ DWORD dwReason, _In_ LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstance);
            g_Module.Init(ObjectMap, hInstance, NULL);
            break;
    }
    return TRUE;
}
