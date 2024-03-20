/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for SHPropertyBag Read/Write
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <stdio.h>
#include <shlwapi_undoc.h>
#include <versionhelpers.h>
#include <strsafe.h>

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
                    V_ARRAY(pvari) = CreateByteArray(&IID_IShellLinkW, sizeof(IID));
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
    SHORT sValue = 0xDEADu;
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
    ok_int(IsEqualGUID(guid, IID_IShellLinkW), TRUE);

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

static void SHPropertyBag_OnMemory(void)
{
    HRESULT hr;
    VARIANT vari;

    IPropertyBag *pPropBag = NULL;
    hr = SHCreatePropertyBagOnMemory(STGM_READWRITE, IID_IPropertyBag, (void**)&pPropBag);
    ok_long(hr, S_OK);
    if (pPropBag == NULL)
    {
        skip("pPropBag was NULL\n");
        return;
    }

    VariantInit(&vari);
    hr = pPropBag->Read(L"InvalidName", &vari, NULL);
    ok_long(hr, E_FAIL);
    VariantClear(&vari);

    VariantInit(&vari);
    V_VT(&vari) = VT_UI4;
    V_UI4(&vari) = 0xDEADFACE;
    hr = pPropBag->Write(L"Name1", &vari);
    ok_long(hr, S_OK);
    VariantClear(&vari);

    VariantInit(&vari);
    hr = pPropBag->Read(L"Name1", &vari, NULL);
    ok_long(hr, S_OK);
    ok_long(V_VT(&vari), VT_UI4);
    ok_long(V_UI4(&vari), 0xDEADFACE);
    VariantClear(&vari);

    pPropBag->Release();
    pPropBag = NULL;

    hr = SHCreatePropertyBagOnMemory(STGM_READ, IID_IPropertyBag, (void**)&pPropBag);
    ok_long(hr, S_OK);

    VariantInit(&vari);
    V_VT(&vari) = VT_UI4;
    V_UI4(&vari) = 0xDEADFACE;
    hr = pPropBag->Write(L"Name1", &vari);
    ok_long(hr, (IsWindowsVistaOrGreater() ? S_OK : E_ACCESSDENIED));
    VariantClear(&vari);

    VariantInit(&vari);
    V_VT(&vari) = VT_UI4;
    V_UI4(&vari) = 0xFEEDF00D;
    hr = pPropBag->Read(L"Name1", &vari, NULL);
    if (IsWindowsVistaOrGreater())
    {
        ok_long(hr, S_OK);
        ok_int(V_VT(&vari), VT_UI4);
        ok_long(V_UI4(&vari), 0xDEADFACE);
    }
    else
    {
        ok_long(hr, E_FAIL);
        ok_int(V_VT(&vari), VT_EMPTY);
        ok_long(V_UI4(&vari), 0xFEEDF00D);
    }
    VariantClear(&vari);

    pPropBag->Release();
    pPropBag = NULL;

    hr = SHCreatePropertyBagOnMemory(STGM_WRITE, IID_IPropertyBag, (void**)&pPropBag);
    ok_long(hr, S_OK);

    VariantInit(&vari);
    V_VT(&vari) = VT_UI4;
    V_UI4(&vari) = 0xDEADFACE;
    hr = pPropBag->Write(L"Name1", &vari);
    ok_long(hr, S_OK);
    VariantClear(&vari);

    VariantInit(&vari);
    V_VT(&vari) = VT_UI4;
    V_UI4(&vari) = 0xFEEDF00D;
    hr = pPropBag->Read(L"Name1", &vari, NULL);
    if (IsWindowsVistaOrGreater())
    {
        ok_long(hr, S_OK);
        ok_int(V_VT(&vari), VT_UI4);
        ok_long(V_UI4(&vari), 0xDEADFACE);
    }
    else
    {
        ok_long(hr, E_ACCESSDENIED);
        ok_int(V_VT(&vari), VT_EMPTY);
        ok_long(V_UI4(&vari), 0xFEEDF00D);
    }
    VariantClear(&vari);

    pPropBag->Release();

    hr = SHCreatePropertyBagOnMemory(STGM_READWRITE, IID_IPropertyBag2, (void**)&pPropBag);
    if (IsWindowsVistaOrGreater())
    {
        ok_long(hr, E_NOINTERFACE);
    }
    else
    {
        ok_long(hr, S_OK);
        pPropBag->Release();
    }

    hr = SHCreatePropertyBagOnMemory(STGM_READ, IID_IPropertyBag2, (void**)&pPropBag);
    if (IsWindowsVistaOrGreater())
    {
        ok_long(hr, E_NOINTERFACE);
    }
    else
    {
        ok_long(hr, S_OK);
        pPropBag->Release();
    }

    hr = SHCreatePropertyBagOnMemory(STGM_WRITE, IID_IPropertyBag2, (void**)&pPropBag);
    if (IsWindowsVistaOrGreater())
    {
        ok_long(hr, E_NOINTERFACE);
    }
    else
    {
        ok_long(hr, S_OK);
        pPropBag->Release();
    }
}

static void SHPropertyBag_OnRegKey(void)
{
    HKEY hKey, hSubKey;
    LONG error;
    VARIANT vari;
    WCHAR szText[MAX_PATH];
    IStream *pStream;
    GUID guid;
    BYTE guid_and_extra[sizeof(GUID) + sizeof(GUID)];

    // Create HKCU\Software\ReactOS registry key
    error = RegCreateKeyW(HKEY_CURRENT_USER, L"Software\\ReactOS", &hKey);
    if (error)
    {
        skip("FAILED to create HKCU\\Software\\ReactOS\n");
        return;
    }

    IPropertyBag *pPropBag;
    HRESULT hr;

    // Try to create new registry key
    RegDeleteKeyW(hKey, L"PropBagTest");
    hr = SHCreatePropertyBagOnRegKey(hKey, L"PropBagTest", 0,
                                     IID_IPropertyBag, (void **)&pPropBag);
    ok_long(hr, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

    // Try to create new registry key
    RegDeleteKeyW(hKey, L"PropBagTest");
    hr = SHCreatePropertyBagOnRegKey(hKey, L"PropBagTest", STGM_READWRITE,
                                     IID_IPropertyBag, (void **)&pPropBag);
    ok_long(hr, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

    // Create new registry key
    RegDeleteKeyW(hKey, L"PropBagTest");
    hr = SHCreatePropertyBagOnRegKey(hKey, L"PropBagTest", STGM_CREATE | STGM_READWRITE,
                                     IID_IPropertyBag, (void **)&pPropBag);
    if (FAILED(hr))
    {
        skip("SHCreatePropertyBagOnRegKey FAILED\n");
        RegCloseKey(hKey);
        return;
    }

    // Write UI4
    VariantInit(&vari);
    V_VT(&vari) = VT_UI4;
    V_UI4(&vari) = 0xDEADFACE;
    hr = pPropBag->Write(L"Name1", &vari);
    ok_long(hr, S_OK);
    VariantClear(&vari);

    // Read UI4
    VariantInit(&vari);
    hr = pPropBag->Read(L"Name1", &vari, NULL);
    ok_long(hr, S_OK);
    ok_long(V_VT(&vari), VT_UI4);
    ok_long(V_UI4(&vari), 0xDEADFACE);
    VariantClear(&vari);

    // Write BSTR
    VariantInit(&vari);
    V_VT(&vari) = VT_BSTR;
    V_BSTR(&vari) = SysAllocString(L"StrValue");
    hr = pPropBag->Write(L"Name2", &vari);
    ok_long(hr, S_OK);
    VariantClear(&vari);

    // Read BSTR
    VariantInit(&vari);
    V_VT(&vari) = VT_BSTR;
    hr = pPropBag->Read(L"Name2", &vari, NULL);
    ok_long(hr, S_OK);
    ok_long(V_VT(&vari), VT_BSTR);
    ok_wstr(V_BSTR(&vari), L"StrValue");
    VariantClear(&vari);

    // Write GUID
    VariantInit(&vari);
    V_VT(&vari) = VT_UNKNOWN;
    V_UNKNOWN(&vari) = SHCreateMemStream((BYTE*)&IID_IShellLinkW, sizeof(IID_IShellLinkW));
    hr = pPropBag->Write(L"Name4", &vari);
    ok_long(hr, S_OK);
    VariantClear(&vari);

    // Read GUID
    VariantInit(&vari);
    V_VT(&vari) = VT_EMPTY;
    hr = pPropBag->Read(L"Name4", &vari, NULL);
    if (IsWindowsVistaOrGreater())
    {
        ok_long(hr, S_OK);
        ok_long(V_VT(&vari), VT_UNKNOWN);
        pStream = (IStream*)V_UNKNOWN(&vari);
        FillMemory(&guid, sizeof(guid), 0xEE);
        hr = pStream->Read(&guid, sizeof(guid), NULL);
        ok_long(hr, S_OK);
        ok_int(::IsEqualGUID(guid, IID_IShellLinkW), TRUE);
    }
    else // XP/2k3 Read is buggy
    {
        ok_long(hr, E_FAIL);
        ok_long(V_VT(&vari), VT_EMPTY);
    }
    VariantClear(&vari);

    pPropBag->Release();

    // Check registry
    error = RegOpenKeyExW(hKey, L"PropBagTest", 0, KEY_READ, &hSubKey);
    ok_long(error, ERROR_SUCCESS);
    DWORD dwType, dwValue, cbValue = sizeof(dwValue);
    error = RegQueryValueExW(hSubKey, L"Name1", NULL, &dwType, (BYTE*)&dwValue, &cbValue);
    ok_long(error, ERROR_SUCCESS);
    ok_long(dwType, REG_DWORD);
    ok_long(dwValue, 0xDEADFACE);
    ok_long(cbValue, sizeof(DWORD));
    cbValue = sizeof(szText);
    error = RegQueryValueExW(hSubKey, L"Name2", NULL, &dwType, (BYTE*)szText, &cbValue);
    ok_long(error, ERROR_SUCCESS);
    ok_long(dwType, REG_SZ);
    ok_wstr(szText, L"StrValue");
    cbValue = sizeof(guid_and_extra);
    error = RegQueryValueExW(hSubKey, L"Name4", NULL, &dwType, (BYTE*)&guid_and_extra, &cbValue);
    ok_long(error, ERROR_SUCCESS);
    ok_long(dwType, REG_BINARY);
    ok_int(memcmp(&guid_and_extra, &GUID_NULL, sizeof(GUID)), 0);
    ok_int(memcmp(&guid_and_extra[sizeof(GUID)], &IID_IShellLinkW, sizeof(GUID)), 0);
    RegCloseKey(hSubKey);

    // Create as read-only
    hr = SHCreatePropertyBagOnRegKey(hKey, L"PropBagTest", STGM_READ,
                                     IID_IPropertyBag, (void **)&pPropBag);
    ok_long(hr, S_OK);

    // Read UI4
    VariantInit(&vari);
    hr = pPropBag->Read(L"Name1", &vari, NULL);
    ok_long(hr, S_OK);
    ok_long(V_VT(&vari), VT_UI4);
    ok_long(V_UI4(&vari), 0xDEADFACE);
    VariantClear(&vari);

    // Write UI4
    VariantInit(&vari);
    V_VT(&vari) = VT_UI4;
    V_UI4(&vari) = 0xDEADFACE;
    hr = pPropBag->Write(L"Name1", &vari);
    ok_long(hr, E_ACCESSDENIED);
    VariantClear(&vari);

    pPropBag->Release();

    // Create as write-only IPropertyBag2
    hr = SHCreatePropertyBagOnRegKey(hKey, L"PropBagTest", STGM_WRITE,
                                     IID_IPropertyBag2, (void **)&pPropBag);
    ok_long(hr, S_OK);

    // Write UI4
    VariantInit(&vari);
    V_VT(&vari) = VT_UI4;
    V_UI4(&vari) = 0xDEADFACE;
    hr = pPropBag->Write(L"Name3", &vari);
    ok_long(hr, E_NOTIMPL);
    VariantClear(&vari);

    // Read UI4
    VariantInit(&vari);
    V_UI4(&vari) = 0xFEEDF00D;
    hr = pPropBag->Read(L"Name3", &vari, NULL);
    ok_long(hr, E_NOTIMPL);
    ok_int(V_VT(&vari), VT_EMPTY);
    ok_long(V_UI4(&vari), 0xFEEDF00D);
    VariantClear(&vari);

    pPropBag->Release();

    // Clean up
    RegDeleteKeyW(hKey, L"PropBagTest");
    RegCloseKey(hKey);
}

static void SHPropertyBag_SHSetIniStringW(void)
{
    WCHAR szIniFile[MAX_PATH];
    WCHAR szValue[MAX_PATH];
    BOOL bRet;
    DWORD dwRet;

    ExpandEnvironmentStringsW(L"%TEMP%\\SHSetIniString.ini", szIniFile, _countof(szIniFile));

    DeleteFileW(szIniFile);

    trace("%ls\n", szIniFile);

    bRet = SHSetIniStringW(L"TestSection", L"Key", L"Value", szIniFile);
    ok_int(bRet, TRUE);

    WritePrivateProfileStringW(NULL, NULL, NULL, szIniFile);

    dwRet = SHGetIniStringW(L"TestSection", L"Key", szValue, _countof(szValue), szIniFile);
    ok_long(dwRet, 5);
    ok_wstr(szValue, L"Value");

    bRet = SHSetIniStringW(L"TestSection", L"Key", NULL, szIniFile);
    ok_int(bRet, TRUE);

    WritePrivateProfileStringW(NULL, NULL, NULL, szIniFile);

    dwRet = SHGetIniStringW(L"TestSection", L"Key", szValue, _countof(szValue), szIniFile);
    ok_long(dwRet, 0);
    ok_wstr(szValue, L"");

    bRet = SHSetIniStringW(L"TestSection", L"Key", L"ABC\x3042\x3044\x3046\x2665", szIniFile);
    ok_int(bRet, TRUE);

    WritePrivateProfileStringW(NULL, NULL, NULL, szIniFile);

    dwRet = SHGetIniStringW(L"TestSection", L"Key", szValue, _countof(szValue), szIniFile);
    ok_long(dwRet, 7);
    ok_wstr(szValue, L"ABC\x3042\x3044\x3046\x2665");

    szValue[0] = 0x3000;
    szValue[1] = UNICODE_NULL;
    dwRet = SHGetIniStringW(L"TestSection", L"NotExistentKey", szValue, _countof(szValue), szIniFile);
    ok_long(dwRet, 0);
    ok_wstr(szValue, L"");

    DeleteFileW(szIniFile);
}

static void SHPropertyBag_OnIniFile(void)
{
    WCHAR szIniFile[MAX_PATH], szValue[MAX_PATH];
    HRESULT hr;
    IPropertyBag *pPropBag;
    VARIANT vari;
    DWORD dwRet;

    ExpandEnvironmentStringsW(L"%TEMP%\\SHPropertyBag.ini", szIniFile, _countof(szIniFile));

    DeleteFileW(szIniFile);
    fclose(_wfopen(szIniFile, L"w"));

    trace("%ls\n", szIniFile);

    // read-write
    hr = SHCreatePropertyBagOnProfileSection(
        szIniFile,
        L"TestSection",
        STGM_READWRITE,
        IID_IPropertyBag,
        (void**)&pPropBag);
    ok_long(hr, S_OK);
    ok_int(PathFileExistsW(szIniFile), TRUE);

    // Write UI4
    VariantInit(&vari);
    V_VT(&vari) = VT_UI4;
    V_UI4(&vari) = 0xDEADFACE;
    hr = pPropBag->Write(L"Name1", &vari);
    ok_long(hr, S_OK);
    VariantClear(&vari);

    // Write BSTR
    VariantInit(&vari);
    V_VT(&vari) = VT_BSTR;
    V_BSTR(&vari) = SysAllocString(L"StrValue");
    hr = pPropBag->Write(L"Name2", &vari);
    ok_long(hr, S_OK);
    VariantClear(&vari);

    // Write BSTR (dirty UTF-7)
    VariantInit(&vari);
    V_VT(&vari) = VT_BSTR;
    V_BSTR(&vari) = SysAllocString(L"ABC\x3042\x3044\x3046\x2665");
    hr = pPropBag->Write(L"@Name3", &vari);
    ok_long(hr, S_OK);
    VariantClear(&vari);

    // Write BSTR (clean UTF-7)
    VariantInit(&vari);
    V_VT(&vari) = VT_BSTR;
    V_BSTR(&vari) = SysAllocString(L"1234abc");
    hr = pPropBag->Write(L"@Name4", &vari);
    ok_long(hr, S_OK);
    VariantClear(&vari);

    pPropBag->Release();

    // Flush
    WritePrivateProfileStringW(NULL, NULL, NULL, szIniFile);

    // Check INI file
    dwRet = GetPrivateProfileStringW(L"TestSection", L"Name1", L"BAD", szValue, _countof(szValue), szIniFile);
    ok_long(dwRet, 10);
    ok_wstr(szValue, L"3735943886");

    dwRet = GetPrivateProfileStringW(L"TestSection", L"Name2", L"BAD", szValue, _countof(szValue), szIniFile);
    ok_long(dwRet, 8);
    ok_wstr(szValue, L"StrValue");

    GetPrivateProfileStringW(L"TestSection", L"Name3", L"NotFound", szValue, _countof(szValue), szIniFile);
    ok_int(memcmp(szValue, L"ABC", 3 * sizeof(WCHAR)), 0);

    GetPrivateProfileStringW(L"TestSection.A", L"Name3", L"NotFound", szValue, _countof(szValue), szIniFile);
    ok_int(memcmp(szValue, L"ABC", 3 * sizeof(WCHAR)), 0);

    GetPrivateProfileStringW(L"TestSection.W", L"Name3", L"NotFound", szValue, _countof(szValue), szIniFile);
    ok_wstr(szValue, L"ABC+MEIwRDBGJmU-"); // UTF-7

    GetPrivateProfileStringW(L"TestSection", L"Name4", L"NotFound", szValue, _countof(szValue), szIniFile);
    ok_wstr(szValue, L"1234abc");

    GetPrivateProfileStringW(L"TestSection.A", L"Name4", L"NotFound", szValue, _countof(szValue), szIniFile);
    ok_wstr(szValue, L"NotFound");

    GetPrivateProfileStringW(L"TestSection.W", L"Name4", L"NotFound", szValue, _countof(szValue), szIniFile);
    ok_wstr(szValue, L"NotFound");

    // read-only
    hr = SHCreatePropertyBagOnProfileSection(
        szIniFile,
        NULL,
        STGM_READ,
        IID_IPropertyBag,
        (void**)&pPropBag);
    ok_long(hr, S_OK);

    // Read UI4
    VariantInit(&vari);
    V_VT(&vari) = VT_UI4;
    hr = pPropBag->Read(L"TestSection\\Name1", &vari, NULL);
    ok_long(hr, S_OK);
    ok_long(V_UI4(&vari), 0xDEADFACE);
    VariantClear(&vari);

    // Read BSTR
    VariantInit(&vari);
    V_VT(&vari) = VT_BSTR;
    hr = pPropBag->Read(L"TestSection\\Name2", &vari, NULL);
    ok_long(hr, S_OK);
    ok_wstr(V_BSTR(&vari), L"StrValue");
    VariantClear(&vari);

    // Read BSTR (dirty UTF-7)
    VariantInit(&vari);
    V_VT(&vari) = VT_BSTR;
    hr = pPropBag->Read(L"TestSection\\@Name3", &vari, NULL);
    ok_long(hr, S_OK);
    ok_wstr(V_BSTR(&vari), L"ABC\x3042\x3044\x3046\x2665");
    VariantClear(&vari);

    // Read BSTR (clean UTF-7)
    VariantInit(&vari);
    V_VT(&vari) = VT_BSTR;
    hr = pPropBag->Read(L"TestSection\\@Name4", &vari, NULL);
    ok_long(hr, S_OK);
    ok_wstr(V_BSTR(&vari), L"1234abc");
    VariantClear(&vari);

    pPropBag->Release();

    DeleteFileW(szIniFile);
}

static void SHPropertyBag_PerScreenRes(void)
{
    HDC hDC = GetDC(NULL);
    INT cxWidth = GetDeviceCaps(hDC, HORZRES);
    INT cyHeight = GetDeviceCaps(hDC, VERTRES);
    INT cMonitors = GetSystemMetrics(SM_CMONITORS);
    ReleaseDC(NULL, hDC);

    WCHAR szBuff1[64], szBuff2[64];
    StringCchPrintfW(szBuff1, _countof(szBuff1), L"%dx%d(%d)", cxWidth, cyHeight, cMonitors);

    szBuff2[0] = UNICODE_NULL;
    SHGetPerScreenResName(szBuff2, _countof(szBuff2), 0);
    ok_wstr(szBuff1, szBuff2);
}

START_TEST(SHPropertyBag)
{
    SHPropertyBag_ReadTest();
    SHPropertyBag_WriteTest();
    SHPropertyBag_OnMemory();
    SHPropertyBag_OnRegKey();
    SHPropertyBag_SHSetIniStringW();
    SHPropertyBag_OnIniFile();
    SHPropertyBag_PerScreenRes();
}
