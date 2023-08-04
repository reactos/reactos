/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for SHPropertyBag Read/Write
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shlwapi_undoc.h>
#include <versionhelpers.h>

#include <pseh/pseh2.h>

static LPCWSTR s_pszPropNames[4] = { NULL, NULL, NULL, NULL };
static VARTYPE s_vt;
static INT s_cRead = 0;
static INT s_cWrite = 0;

static void ResetTest(VARTYPE vt,
                  LPCWSTR pszName0 = NULL, LPCWSTR pszName1 = NULL,
                  LPCWSTR pszName2 = NULL, LPCWSTR pszName3 = NULL)
{
    s_vt = vt;
    s_cRead = s_cWrite = 0;
    s_pszPropNames[0] = pszName0;
    s_pszPropNames[1] = pszName1;
    s_pszPropNames[2] = pszName2;
    s_pszPropNames[3] = pszName3;
}

static SAFEARRAY* CreateByteArray(LPCVOID pvSrc, DWORD cbSize)
{
    SAFEARRAYBOUND Bound;
    Bound.lLbound = 0;
    Bound.cElements = cbSize;

    SAFEARRAY* pArray = SafeArrayCreate(VT_UI1, 1, &Bound);
    if (!pArray)
        return NULL;

    void HUGEP *pvData;
    HRESULT hr = SafeArrayAccessData(pArray, &pvData);
    if (FAILED(hr))
    {
        SafeArrayDestroy(pArray);
        return NULL;
    }

    CopyMemory(pvData, pvSrc, cbSize);
    SafeArrayUnaccessData(pArray);

    return pArray;
}

class CDummyPropertyBag : public IPropertyBag
{
public:
    CDummyPropertyBag()
    {
    }

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override
    {
        ok(FALSE, "Unexpected call\n");
        return S_OK;
    }
    STDMETHODIMP_(ULONG) AddRef() override
    {
        ok(FALSE, "Unexpected call\n");
        return S_OK;
    }
    STDMETHODIMP_(ULONG) Release() override
    {
        ok(FALSE, "Unexpected call\n");
        return S_OK;
    }

    // IPropertyBag
    STDMETHODIMP Read(LPCWSTR pszPropName, VARIANT *pvari, IErrorLog *pErrorLog) override
    {
        ++s_cRead;
        ok_int(s_vt, V_VT(pvari));
        for (size_t i = 0; i < _countof(s_pszPropNames); ++i)
        {
            if (s_pszPropNames[i])
            {
                ok_wstr(pszPropName, s_pszPropNames[i]);
                s_pszPropNames[i] = NULL;

                if (lstrcmpiW(pszPropName, L"RECTL2.top") == 0)
                    return E_FAIL;

                if (lstrcmpiW(pszPropName, L"GUID1") == 0)
                {
                    V_VT(pvari) = (VT_UI1 | VT_ARRAY);
                    V_ARRAY(pvari) = CreateByteArray(&IID_IShellLink, sizeof(IID));
                    return S_OK;
                }

                if (lstrcmpiW(pszPropName, L"GUID2") == 0)
                {
                    WCHAR szText[50];
                    StringFromGUID2(IID_IUnknown, szText, _countof(szText));

                    V_VT(pvari) = VT_BSTR;
                    V_BSTR(pvari) = SysAllocString(szText);
                    return S_OK;
                }

                if (lstrcmpiW(pszPropName, L"GUID3") == 0)
                {
                    V_VT(pvari) = VT_EMPTY;
                    V_UI4(pvari) = 0xDEADFACE;
                    return S_OK;
                }

                goto Skip1;
            }
        }
        ok(FALSE, "Unexpected call\n");
Skip1:
        return S_OK;
    }

    STDMETHODIMP Write(LPCWSTR pszPropName, VARIANT *pvari) override
    {
        ++s_cWrite;
        ok_int(s_vt, V_VT(pvari));
        for (size_t i = 0; i < _countof(s_pszPropNames); ++i)
        {
            if (s_pszPropNames[i])
            {
                ok_wstr(pszPropName, s_pszPropNames[i]);
                s_pszPropNames[i] = NULL;
                if (lstrcmpiW(pszPropName, L"RECTL2.bottom") == 0)
                {
                    s_vt = VT_EMPTY;
                    ZeroMemory(&s_pszPropNames, sizeof(s_pszPropNames));
                    s_pszPropNames[0] = L"RECTL2.right";
                    return E_FAIL;
                }
                goto Skip2;
            }
        }
        ok(FALSE, "Unexpected call\n");
Skip2:
        return S_OK;
    }
};

static void SHPropertyBag_ReadTest(void)
{
    HRESULT hr;
    CDummyPropertyBag dummy;
    BOOL bValue = 0xDEADFACE;
    SHORT sValue = 0xDEAD;
    LONG lValue = 0xDEADDEAD;
    DWORD dwValue = 0xFEEDF00D;
    BSTR bstr = NULL;
    POINTL ptl = { 0xEEEE, 0xDDDD };
    POINTS pts = { 0x2222, 0x3333 };
    RECTL rcl = { 123, 456, 789, 101112 };
    GUID guid = { 0 };

    ResetTest(VT_BOOL, L"BOOL1");
    hr = SHPropertyBag_ReadBOOL(&dummy, s_pszPropNames[0], &bValue);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 1);
    ok_int(s_cWrite, 0);

    ResetTest(VT_UI2, L"SHORT1");
    hr = SHPropertyBag_ReadSHORT(&dummy, s_pszPropNames[0], &sValue);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 1);
    ok_int(s_cWrite, 0);

    ResetTest(VT_I4, L"LONG1");
    hr = SHPropertyBag_ReadLONG(&dummy, s_pszPropNames[0], &lValue);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 1);
    ok_int(s_cWrite, 0);

    ResetTest(VT_UI4, L"DWORD1");
    hr = SHPropertyBag_ReadDWORD(&dummy, s_pszPropNames[0], &dwValue);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 1);
    ok_int(s_cWrite, 0);

    ResetTest(VT_BSTR, L"Str1");
    hr = SHPropertyBag_ReadBSTR(&dummy, s_pszPropNames[0], &bstr);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 1);
    ok_int(s_cWrite, 0);
    SysFreeString(bstr);

    ResetTest(VT_I4, L"POINTL1.x", L"POINTL1.y");
    hr = SHPropertyBag_ReadPOINTL(&dummy, L"POINTL1", &ptl);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 2);
    ok_int(s_cWrite, 0);

    ResetTest(VT_I4, L"POINTS1.x", L"POINTS1.y");
    hr = SHPropertyBag_ReadPOINTS(&dummy, L"POINTS1", &pts);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 2);
    ok_int(s_cWrite, 0);

    ResetTest(VT_I4, L"RECTL1.left", L"RECTL1.top", L"RECTL1.right", L"RECTL1.bottom");
    hr = SHPropertyBag_ReadRECTL(&dummy, L"RECTL1", &rcl);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 4);
    ok_int(s_cWrite, 0);

    ResetTest(VT_I4, L"RECTL2.left", L"RECTL2.top", L"RECTL2.right", L"RECTL2.bottom");
    hr = SHPropertyBag_ReadRECTL(&dummy, L"RECTL2", &rcl);
    ok_long(hr, E_FAIL);
    ok_int(s_cRead, 2);
    ok_int(s_cWrite, 0);

    ResetTest(VT_EMPTY, L"GUID1");
    hr = SHPropertyBag_ReadGUID(&dummy, L"GUID1", &guid);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 1);
    ok_int(s_cWrite, 0);
    ok_int(IsEqualGUID(guid, IID_IShellLink), TRUE);

    ResetTest(VT_EMPTY, L"GUID2");
    hr = SHPropertyBag_ReadGUID(&dummy, L"GUID2", &guid);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 1);
    ok_int(s_cWrite, 0);
    ok_int(IsEqualGUID(guid, IID_IUnknown), TRUE);

    ResetTest(VT_EMPTY, L"GUID3");
    guid = IID_IExtractIcon;
    hr = SHPropertyBag_ReadGUID(&dummy, L"GUID3", &guid);

    if (IsWindowsVistaOrGreater())
        ok_long(hr, E_INVALIDARG);
    else
        ok_long(hr, S_OK);

    ok_int(s_cRead, 1);
    ok_int(s_cWrite, 0);
    ok_int(IsEqualGUID(guid, IID_IExtractIcon), TRUE);
}

static void SHPropertyBag_WriteTest(void)
{
    HRESULT hr;
    CDummyPropertyBag dummy;

    ResetTest(VT_EMPTY, L"EMPTY1");
    hr = SHPropertyBag_Delete(&dummy, s_pszPropNames[0]);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 0);
    ok_int(s_cWrite, 1);

    ResetTest(VT_BOOL, L"BOOL1");
    hr = SHPropertyBag_WriteBOOL(&dummy, s_pszPropNames[0], TRUE);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 0);
    ok_int(s_cWrite, 1);

    ResetTest(VT_UI2, L"SHORT1");
    hr = SHPropertyBag_WriteSHORT(&dummy, s_pszPropNames[0], 1);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 0);
    ok_int(s_cWrite, 1);

    ResetTest(VT_I4, L"LONG1");
    hr = SHPropertyBag_WriteLONG(&dummy, s_pszPropNames[0], 1);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 0);
    ok_int(s_cWrite, 1);

    ResetTest(VT_UI4, L"DWORD1");
    hr = SHPropertyBag_WriteDWORD(&dummy, s_pszPropNames[0], 1);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 0);
    ok_int(s_cWrite, 1);

    ResetTest(VT_BSTR, L"Str1");
    hr = SHPropertyBag_WriteStr(&dummy, s_pszPropNames[0], L"1");
    ok_long(hr, S_OK);
    ok_int(s_cRead, 0);
    ok_int(s_cWrite, 1);

    ResetTest(VT_I4, L"POINTL1.x", L"POINTL1.y");
    POINTL ptl = { 0xEEEE, 0xDDDD };
    hr = SHPropertyBag_WritePOINTL(&dummy, L"POINTL1", &ptl);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 0);
    ok_int(s_cWrite, 2);

    ResetTest(VT_I4, L"POINTS1.x", L"POINTS1.y");
    POINTS pts = { 0x2222, 0x3333 };
    hr = SHPropertyBag_WritePOINTS(&dummy, L"POINTS1", &pts);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 0);
    ok_int(s_cWrite, 2);

    ResetTest(VT_I4, L"RECTL1.left", L"RECTL1.top", L"RECTL1.right", L"RECTL1.bottom");
    RECTL rcl = { 123, 456, 789, 101112 };
    hr = SHPropertyBag_WriteRECTL(&dummy, L"RECTL1", &rcl);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 0);
    ok_int(s_cWrite, 4);

    ResetTest(VT_I4, L"RECTL2.left", L"RECTL2.top", L"RECTL2.right", L"RECTL2.bottom");
    hr = SHPropertyBag_WriteRECTL(&dummy, L"RECTL2", &rcl);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 0);
    ok_int(s_cWrite, 5);

    GUID guid;
    ZeroMemory(&guid, sizeof(guid));
    ResetTest(VT_BSTR, L"GUID1");
    hr = SHPropertyBag_WriteGUID(&dummy, L"GUID1", &guid);
    ok_long(hr, S_OK);
    ok_int(s_cRead, 0);
    ok_int(s_cWrite, 1);
}

START_TEST(SHPropertyBag)
{
    SHPropertyBag_ReadTest();
    SHPropertyBag_WriteTest();
}
