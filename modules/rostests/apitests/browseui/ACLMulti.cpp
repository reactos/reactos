/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for CLSID_ACLMulti
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#define _UNICODE
#define UNICODE
#include <apitest.h>
#include <shlobj.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlstr.h>
#include <shlwapi_undoc.h>

CComModule gModule;
static CStringA *s_pstrTest = NULL;

class CEnumString1 :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumString,
    public IACList2
{
public:
    INT m_i, m_c;

    CEnumString1() : m_i(0), m_c(2)
    {
    }
    virtual ~CEnumString1()
    {
    }

    BEGIN_COM_MAP(CEnumString1)
        COM_INTERFACE_ENTRY_IID(IID_IEnumString, IEnumString)
        COM_INTERFACE_ENTRY_IID(IID_IACList2, IACList2)
        COM_INTERFACE_ENTRY_IID(IID_IACList, IACList)
    END_COM_MAP()

    // IEnumString
    STDMETHODIMP Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
    {
        *s_pstrTest += 'n';

        *rgelt = NULL;
        *pceltFetched = 0;

        if (m_i < m_c)
        {
            ++m_i;
            *rgelt = (LPOLESTR)CoTaskMemAlloc((4 + 1) * sizeof(WCHAR));
            lstrcpyW(*rgelt, L"TEST");
            *pceltFetched = 1;
            return S_OK;
        }

        return S_FALSE;
    }
    STDMETHODIMP Skip(ULONG celt)
    {
        *s_pstrTest += 's';
        if (m_i < m_c)
        {
            ++m_i;
            return S_OK;
        }
        return S_FALSE;
    }
    STDMETHODIMP Reset()
    {
        *s_pstrTest += 'r';
        m_i = 0;
        return S_OK;
    }
    STDMETHODIMP Clone(IEnumString **ppenum)
    {
        *s_pstrTest += 'c';
        return E_NOTIMPL;
    }

    // IACList
    STDMETHODIMP Expand(LPCOLESTR pszExpand)
    {
        CHAR szText[64];
        SHUnicodeToAnsi(pszExpand, szText, _countof(szText));
        trace("CEnumString1::Expand: %s\n", szText);
        *s_pstrTest += 'e';
        return S_OK;
    }

    // IACList2
    STDMETHODIMP SetOptions(DWORD dwFlags)
    {
        *s_pstrTest += 'o';
        return S_OK;
    }
    STDMETHODIMP GetOptions(DWORD* pdwFlags)
    {
        *s_pstrTest += 'g';
        *pdwFlags = 0;
        return S_OK;
    }
};

class CEnumString2 :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumString,
    public IACList2
{
public:
    INT m_i, m_c;

    CEnumString2() : m_i(0), m_c(2)
    {
    }
    virtual ~CEnumString2()
    {
    }

    BEGIN_COM_MAP(CEnumString1)
        COM_INTERFACE_ENTRY_IID(IID_IEnumString, IEnumString)
        COM_INTERFACE_ENTRY_IID(IID_IACList2, IACList2)
        COM_INTERFACE_ENTRY_IID(IID_IACList, IACList)
    END_COM_MAP()

    // IEnumString
    STDMETHODIMP Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
    {
        *s_pstrTest += 'N';

        *rgelt = NULL;
        *pceltFetched = 0;

        if (m_i < m_c)
        {
            ++m_i;
            *rgelt = (LPOLESTR)CoTaskMemAlloc((4 + 1) * sizeof(WCHAR));
            lstrcpyW(*rgelt, L"TEST");
            *pceltFetched = 1;
            return S_OK;
        }

        return S_FALSE;
    }
    STDMETHODIMP Skip(ULONG celt)
    {
        *s_pstrTest += 'S';
        if (m_i < m_c)
        {
            ++m_i;
            return S_OK;
        }
        return S_FALSE;
    }
    STDMETHODIMP Reset()
    {
        *s_pstrTest += 'R';
        m_i = 0;
        return S_OK;
    }
    STDMETHODIMP Clone(IEnumString **ppenum)
    {
        *s_pstrTest += 'C';
        return E_NOTIMPL;
    }

    // IACList
    STDMETHODIMP Expand(LPCOLESTR pszExpand)
    {
        CHAR szText[64];
        SHUnicodeToAnsi(pszExpand, szText, _countof(szText));
        trace("CEnumString2::Expand: %s\n", szText);
        *s_pstrTest += 'E';
        return S_OK;
    }

    // IACList2
    STDMETHODIMP SetOptions(DWORD dwFlags)
    {
        *s_pstrTest += 'O';
        return S_OK;
    }
    STDMETHODIMP GetOptions(DWORD* pdwFlags)
    {
        *s_pstrTest += 'G';
        *pdwFlags = 0;
        return S_OK;
    }
};

struct CCoInit
{
    CCoInit() { hres = CoInitialize(NULL); }
    ~CCoInit() { if (SUCCEEDED(hres)) { CoUninitialize(); } }
    HRESULT hres;
};

START_TEST(ACLMulti)
{
    CCoInit init;
    ok_hr(init.hres, S_OK);
    if (!SUCCEEDED(init.hres))
    {
        skip("CCoInit failed\n");
        return;
    }

    s_pstrTest = new CStringA();

    CComPtr<IObjMgr> pManager;
    HRESULT hr = CoCreateInstance(CLSID_ACLMulti, NULL, CLSCTX_ALL,
                                  IID_IObjMgr, (LPVOID *)&pManager);
    ok_hr(hr, S_OK);
    if (!pManager)
    {
        skip("CoCreateInstance failed\n");
        return;
    }

    CComPtr<IEnumString> pEnum;
    hr = pManager->QueryInterface(IID_IEnumString, (LPVOID *)&pEnum);
    ok_hr(hr, S_OK);
    if (!pEnum)
    {
        skip("QueryInterface failed\n");
        return;
    }

    CComPtr<IACList> pACL;
    hr = pManager->QueryInterface(IID_IACList, (LPVOID *)&pACL);
    ok_hr(hr, S_OK);
    if (!pEnum)
    {
        skip("QueryInterface failed\n");
        return;
    }

    CComPtr<IEnumString> p1(new CComObject<CEnumString1>());
    hr = pManager->Append(p1);
    ok_hr(hr, S_OK);

    CComPtr<IEnumString> p2(new CComObject<CEnumString2>());
    hr = pManager->Append(p2);
    ok_hr(hr, S_OK);

    hr = pEnum->Reset();
    ok_hr(hr, S_OK);

    hr = pACL->Expand(L"C:\\");
    ok_hr(hr, S_OK);

    LPOLESTR psz;
    ULONG c;

    psz = NULL;
    hr = pEnum->Next(1, &psz, &c);
    ok_hr(hr, S_OK);
    ok_int(c, 1);
    CoTaskMemFree(psz);

    hr = pEnum->Skip(1);
    ok_hr(hr, E_NOTIMPL);

    psz = NULL;
    hr = pEnum->Next(1, &psz, &c);
    ok_hr(hr, S_OK);
    ok_int(c, 1);
    CoTaskMemFree(psz);

    psz = NULL;
    hr = pEnum->Next(1, &psz, &c);
    ok_hr(hr, S_OK);
    ok_int(c, 1);
    CoTaskMemFree(psz);

    psz = NULL;
    hr = pEnum->Next(1, &psz, &c);
    ok_hr(hr, S_OK);
    ok_int(c, 1);
    CoTaskMemFree(psz);

    psz = NULL;
    hr = pEnum->Next(1, &psz, &c);
    ok_hr(hr, S_FALSE);
    ok_int(c, 0);
    CoTaskMemFree(psz);

    pEnum->Reset();

    psz = NULL;
    hr = pEnum->Next(2, &psz, &c);
    ok_hr(hr, S_OK);
    ok_int(c, 1);
    CoTaskMemFree(psz);

    hr = pEnum->Skip(2);
    ok_hr(hr, E_NOTIMPL);

    psz = NULL;
    hr = pEnum->Next(1, &psz, &c);
    ok_hr(hr, S_OK);
    ok_int(c, 1);
    CoTaskMemFree(psz);

    hr = pEnum->Skip(1);
    ok_hr(hr, E_NOTIMPL);

    psz = NULL;
    hr = pEnum->Next(1, &psz, &c);
    ok_hr(hr, S_OK);
    ok_int(c, 1);
    CoTaskMemFree(psz);

    ok_str(s_pstrTest->GetString(), "rReEnnnNNNrRnnnN");
    delete s_pstrTest;
}
