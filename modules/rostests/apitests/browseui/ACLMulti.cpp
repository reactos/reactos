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
#include <shlguid_undoc.h>

CComModule gModule;
static CStringA s_strTest;

class CEnumString :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumString,
    public IACList2
{
public:
    INT m_i, m_c;
    char m_ch;

    CEnumString()
    {
    }
    virtual ~CEnumString()
    {
    }
    void Initialize(INT c, char ch)
    {
        m_i = 0;
        m_c = c;
        m_ch = ch;
    }
    void Log(char ch)
    {
        s_strTest += ch;
        s_strTest += m_ch;
    }

    BEGIN_COM_MAP(CEnumString)
        COM_INTERFACE_ENTRY_IID(IID_IEnumString, IEnumString)
        COM_INTERFACE_ENTRY_IID(IID_IACList2, IACList2)
        COM_INTERFACE_ENTRY_IID(IID_IACList, IACList)
    END_COM_MAP()

    // IEnumString
    STDMETHODIMP Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
    {
        Log('N');

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
        Log('S');
        if (m_i < m_c)
        {
            ++m_i;
            return S_OK;
        }
        return S_FALSE;
    }
    STDMETHODIMP Reset()
    {
        Log('R');
        m_i = 0;
        return S_OK;
    }
    STDMETHODIMP Clone(IEnumString **ppenum)
    {
        Log('C');
        return E_NOTIMPL;
    }

    // IACList
    STDMETHODIMP Expand(LPCOLESTR pszExpand)
    {
        Log('E');
        CHAR szText[64];
        SHUnicodeToAnsi(pszExpand, szText, _countof(szText));
        trace("CEnumString::Expand: %s\n", szText);
        return S_OK;
    }

    // IACList2
    STDMETHODIMP SetOptions(DWORD dwFlags)
    {
        Log('O');
        return S_OK;
    }
    STDMETHODIMP GetOptions(DWORD* pdwFlags)
    {
        Log('G');
        *pdwFlags = 0;
        return S_OK;
    }

    IUnknown *GetUnknown()
    {
        return static_cast<IACList *>(this);
    }
};

template <typename T_BASE>
struct CRefWatch : public T_BASE
{
    STDMETHODIMP QueryInterface(REFIID iid, void **ppv)
    {
        if (iid == IID_IEnumString)
            T_BASE::Log('m');
        else if (iid == IID_IACList2)
            T_BASE::Log('i');
        else if (iid == IID_IACList)
            T_BASE::Log('s');
        else if (iid == IID_IUnknown)
            T_BASE::Log('u');
        else if (iid == IID_IACLCustomMRU)
            T_BASE::Log('r');
        else if (iid == IID_IEnumACString)
            T_BASE::Log('a');
        else
            T_BASE::Log('*');
        return T_BASE::QueryInterface(iid, ppv);
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

    CComPtr<CEnumString> p1(new CRefWatch<CComObject<CEnumString> >());
    p1->Initialize(2, '1');
    hr = pManager->Append(p1->GetUnknown());
    ok_hr(hr, S_OK);

    s_strTest += '|';

    CComPtr<CEnumString> p2(new CRefWatch<CComObject<CEnumString> >());
    p2->Initialize(3, '2');
    hr = pManager->Append(p2->GetUnknown());
    ok_hr(hr, S_OK);

    s_strTest += '|';

    hr = pEnum->Reset();
    ok_hr(hr, S_OK);

    s_strTest += '|';

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
    ok_hr(hr, S_OK);
    ok_int(c, 1);
    CoTaskMemFree(psz);

    s_strTest += '|';
    pEnum->Reset();
    s_strTest += '|';

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

    ok_str((LPCSTR)s_strTest, "m1a1s1|m2a2s2|R1R2|N1N1N1N2N2N2|R1R2|N1N1N1N2");
}
