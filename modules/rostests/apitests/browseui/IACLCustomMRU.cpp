/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for IACLCustomMRU objects
 * COPYRIGHT:   Copyright 2017-2020 Mark Jansen (mark.jansen@reactos.org)
 */

#define _UNICODE
#define UNICODE
#include <apitest.h>
#include <shlobj.h>
#include <atlbase.h>
#include <atlstr.h>
#include <atlcom.h>
#include <atlwin.h>

// Yes, gcc at it again, let's validate everything found inside unused templates!
ULONG DbgPrint(PCH Format,...);

#include <shellutils.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <initguid.h>

#define ok_hex2(file, line, key, expression, result) \
    do { \
        int _value = (expression); \
        ok_(file, line)(_value == (result), "Wrong value for '%s', expected: " #result " (0x%x), got: 0x%x for %c\n", \
           #expression, (int)(result), _value, (char)key); \
    } while (0)

#define ok_wstri(x, y) \
    ok(lstrcmpiW(x, y) == 0, "Wrong string. Expected '%S', got '%S'\n", y, x)


DEFINE_GUID(IID_IACLCustomMRU,             0xf729fc5e, 0x8769, 0x4f3e, 0xbd, 0xb2, 0xd7, 0xb5, 0x0f, 0xd2, 0x27, 0x5b);
static const WCHAR szTestPath[] = L"TESTPATH_BROWSEUI_APITEST";

#undef INTERFACE
#define INTERFACE IACLCustomMRU

/* based on https://learn.microsoft.com/en-us/previous-versions/windows/desktop/legacy/bb776380(v=vs.85) */
DECLARE_INTERFACE_IID_(IACLCustomMRU, IUnknown, "F729FC5E-8769-4F3E-BDB2-D7B50FD2275B")
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS)PURE;
    STDMETHOD_(ULONG, Release) (THIS)PURE;

    // *** IACLCustomMRU methods ***
    STDMETHOD(Initialize) (THIS_ LPCWSTR pwszMRURegKey, DWORD dwMax) PURE;
    STDMETHOD(AddMRUString) (THIS_ LPCWSTR pwszEntry) PURE;
};


static void Cleanup_Testdata()
{
    CRegKey tmp;
    if (!tmp.Open(HKEY_CURRENT_USER, NULL, KEY_READ | KEY_WRITE))
        tmp.DeleteSubKey(szTestPath);
}

#define verify_mru(mru, ...)     verify_mru_(__FILE__, __LINE__, mru, __VA_ARGS__, NULL)
static void verify_mru_(const char* file, int line, IACLCustomMRU* mru, PCWSTR MRUString, ...)
{

    CRegKey key;
    key.Open(HKEY_CURRENT_USER, szTestPath);

    va_list args;
    va_start(args, MRUString);
    PCWSTR Entry;
    WCHAR Key = L'a';
    while ((Entry = va_arg(args, PCWSTR)))
    {
        WCHAR Value[MAX_PATH];
        ULONG nChars = _countof(Value);
        CStringW tmp;
        tmp += Key;
        LSTATUS Status = key.QueryStringValue(tmp, Value, &nChars);
        ok_hex2(file, line, Key, Status, ERROR_SUCCESS);
        if (Status == ERROR_SUCCESS)
        {
            ok_(file, line)(!wcscmp(Value, Entry), "Expected value %c to be %S, was %S\n", (char)Key, Entry, Value);
        }
        Key++;
    }
    va_end(args);

    if (Key != L'a')
    {
        WCHAR Value[MAX_PATH];
        ULONG nChars = _countof(Value);
        LSTATUS Status = key.QueryStringValue(L"MRUList", Value, &nChars);
        ok_hex2(file, line, Key, Status, ERROR_SUCCESS);
        if (Status == ERROR_SUCCESS)
        {
            ok_(file, line)(!wcscmp(Value, MRUString), "Expected MRUList to be %S, was %S\n", MRUString, Value);
        }
    }
}


static void
test_IACLCustomMRU_Basics()
{
    CComPtr<IACLCustomMRU> CustomMRU;
    HRESULT hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL, CLSCTX_ALL, IID_PPV_ARG(IACLCustomMRU, &CustomMRU));
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    Cleanup_Testdata();

    /* Initialize with a NULL name will cause an AV */
    //hr = CustomMRU->Initialize(NULL, 0);

    hr = CustomMRU->Initialize(szTestPath, 0);
    ok_hex(hr, S_OK);
    /* Adding an entry with a dwMax of 0 will cause an AV */

    /* Calling it again will resize */
    hr = CustomMRU->Initialize(szTestPath, 3);
    ok_hex(hr, S_OK);
    verify_mru(CustomMRU, L"");

    hr = CustomMRU->AddMRUString(L"FIRST_ENTRY");
    ok_hex(hr, S_OK);
    verify_mru(CustomMRU, L"a", L"FIRST_ENTRY");

    hr = CustomMRU->AddMRUString(L"SECOND_ENTRY");
    ok_hex(hr, S_OK);
    verify_mru(CustomMRU, L"ba", L"FIRST_ENTRY", L"SECOND_ENTRY");

    hr = CustomMRU->AddMRUString(L"THIRD_ENTRY");
    ok_hex(hr, S_OK);
    verify_mru(CustomMRU, L"cba", L"FIRST_ENTRY", L"SECOND_ENTRY", L"THIRD_ENTRY");

    /* First entry is replaced */
    hr = CustomMRU->AddMRUString(L"FOURTH_ENTRY");
    ok_hex(hr, S_OK);
    verify_mru(CustomMRU, L"acb", L"FOURTH_ENTRY", L"SECOND_ENTRY", L"THIRD_ENTRY");

    /* Second entry is replaced */
    hr = CustomMRU->AddMRUString(L"FIFTH_ENTRY");
    ok_hex(hr, S_OK);
    verify_mru(CustomMRU, L"bac", L"FOURTH_ENTRY", L"FIFTH_ENTRY", L"THIRD_ENTRY");
}


static void FillDefault(IACLCustomMRU* CustomMRU)
{
    Cleanup_Testdata();
    HRESULT hr = CustomMRU->Initialize(szTestPath, 3);
    ok_hex(hr, S_OK);
    hr = CustomMRU->AddMRUString(L"FIRST_ENTRY");
    ok_hex(hr, S_OK);
    hr = CustomMRU->AddMRUString(L"SECOND_ENTRY");
    ok_hex(hr, S_OK);
    hr = CustomMRU->AddMRUString(L"THIRD_ENTRY");
    ok_hex(hr, S_OK);
}

static void
test_IACLCustomMRU_UpdateOrder()
{
    CComPtr<IACLCustomMRU> CustomMRU;
    HRESULT hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL, CLSCTX_ALL, IID_PPV_ARG(IACLCustomMRU, &CustomMRU));
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    Cleanup_Testdata();
    FillDefault(CustomMRU);
    verify_mru(CustomMRU, L"cba", L"FIRST_ENTRY", L"SECOND_ENTRY", L"THIRD_ENTRY");

    /* Add the first entry again */
    hr = CustomMRU->AddMRUString(L"FIRST_ENTRY");
    ok_hex(hr, S_OK);
    /* No change */
    verify_mru(CustomMRU, L"cba", L"FIRST_ENTRY", L"SECOND_ENTRY", L"THIRD_ENTRY");

    CustomMRU.Release();
    /* Now the order is updated */
    verify_mru(NULL, L"acb", L"FIRST_ENTRY", L"SECOND_ENTRY", L"THIRD_ENTRY");


    hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL, CLSCTX_ALL, IID_PPV_ARG(IACLCustomMRU, &CustomMRU));
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    Cleanup_Testdata();
    FillDefault(CustomMRU);
    verify_mru(CustomMRU, L"cba", L"FIRST_ENTRY", L"SECOND_ENTRY", L"THIRD_ENTRY");


    /* Add the first entry again */
    hr = CustomMRU->AddMRUString(L"FIRST_ENTRY");
    ok_hex(hr, S_OK);
    /* No change */
    verify_mru(CustomMRU, L"cba", L"FIRST_ENTRY", L"SECOND_ENTRY", L"THIRD_ENTRY");

    hr = CustomMRU->AddMRUString(L"SOMETHING_ELSE");
    ok_hex(hr, S_OK);
    /* Now all changes are persisted */
    verify_mru(CustomMRU, L"bac", L"FIRST_ENTRY", L"SOMETHING_ELSE", L"THIRD_ENTRY");
}

static void
test_IACLCustomMRU_ExtraChars()
{
    CComPtr<IACLCustomMRU> CustomMRU;
    HRESULT hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL, CLSCTX_ALL, IID_PPV_ARG(IACLCustomMRU, &CustomMRU));
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    Cleanup_Testdata();

    /* Still returnes success */
    hr = CustomMRU->Initialize(szTestPath, 30);
    ok_hex(hr, S_OK);

    for (int n = 0; n < 30; ++n)
    {
        CStringW tmp;
        tmp.Format(L"%d", n);

        hr = CustomMRU->AddMRUString(tmp);
        ok_hex(hr, S_OK);
    }
    /* But is starting to wrap around */
    verify_mru(CustomMRU, L"a}|{zyxwvutsrqponmlkjihgfedcb", L"29",
               L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9",
               L"10", L"11", L"12", L"13", L"14", L"15", L"16", L"17", L"18", L"19",
               L"20", L"21", L"22", L"23", L"24", L"25", L"26", L"27", L"28");

    Cleanup_Testdata();
}

static void
test_IACLCustomMRU_Continue()
{
    CComPtr<IACLCustomMRU> CustomMRU;
    HRESULT hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL, CLSCTX_ALL, IID_PPV_ARG(IACLCustomMRU, &CustomMRU));
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    Cleanup_Testdata();
    FillDefault(CustomMRU);
    verify_mru(CustomMRU, L"cba", L"FIRST_ENTRY", L"SECOND_ENTRY", L"THIRD_ENTRY");

    CustomMRU.Release();

    hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL, CLSCTX_ALL, IID_PPV_ARG(IACLCustomMRU, &CustomMRU));
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    hr = CustomMRU->Initialize(szTestPath, 3);
    ok_hex(hr, S_OK);

    /* First entry is replaced */
    hr = CustomMRU->AddMRUString(L"FOURTH_ENTRY");
    ok_hex(hr, S_OK);
    verify_mru(CustomMRU, L"acb", L"FOURTH_ENTRY", L"SECOND_ENTRY", L"THIRD_ENTRY");

    CustomMRU.Release();

    hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL, CLSCTX_ALL, IID_PPV_ARG(IACLCustomMRU, &CustomMRU));
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    hr = CustomMRU->Initialize(szTestPath, 3);
    ok_hex(hr, S_OK);

    /* Second entry is replaced */
    hr = CustomMRU->AddMRUString(L"FIFTH_ENTRY");
    ok_hex(hr, S_OK);
    verify_mru(CustomMRU, L"bac", L"FOURTH_ENTRY", L"FIFTH_ENTRY", L"THIRD_ENTRY");

    CustomMRU.Release();

    hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL, CLSCTX_ALL, IID_PPV_ARG(IACLCustomMRU, &CustomMRU));
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;


    /* Save some garbage */
    CRegKey key;
    key.Open(HKEY_CURRENT_USER, szTestPath);
    key.SetStringValue(L"MRUList", L"b**");
    key.Close();

    hr = CustomMRU->Initialize(szTestPath, 3);
    ok_hex(hr, S_OK);

    CustomMRU.Release();

    /* Not cleaned up */
    verify_mru(CustomMRU, L"b**", L"FOURTH_ENTRY", L"FIFTH_ENTRY", L"THIRD_ENTRY");

    hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL, CLSCTX_ALL, IID_PPV_ARG(IACLCustomMRU, &CustomMRU));
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    hr = CustomMRU->Initialize(szTestPath, 3);
    ok_hex(hr, S_OK);

    /* Now it's just cleaned up */
    hr = CustomMRU->AddMRUString(L"SIXTH_ENTRY");
    ok_hex(hr, S_OK);
    verify_mru(CustomMRU, L"ab", L"SIXTH_ENTRY");

    CustomMRU.Release();

    hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL, CLSCTX_ALL, IID_PPV_ARG(IACLCustomMRU, &CustomMRU));
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    Cleanup_Testdata();
    FillDefault(CustomMRU);
    verify_mru(CustomMRU, L"cba", L"FIRST_ENTRY", L"SECOND_ENTRY", L"THIRD_ENTRY");

    CustomMRU.Release();

    hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL, CLSCTX_ALL, IID_PPV_ARG(IACLCustomMRU, &CustomMRU));
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    key.Open(HKEY_CURRENT_USER, szTestPath);
    key.SetStringValue(L"MRUList", L"baccccc");
    key.Close();

    hr = CustomMRU->Initialize(szTestPath, 3);
    ok_hex(hr, S_OK);
    CustomMRU.Release();

    verify_mru(CustomMRU, L"baccccc", L"FIRST_ENTRY", L"SECOND_ENTRY", L"THIRD_ENTRY");

    hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL, CLSCTX_ALL, IID_PPV_ARG(IACLCustomMRU, &CustomMRU));
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    hr = CustomMRU->Initialize(szTestPath, 3);
    ok_hex(hr, S_OK);

    hr = CustomMRU->AddMRUString(L"FOURTH_ENTRY");
    ok_hex(hr, S_OK);
    verify_mru(CustomMRU, L"a", L"FOURTH_ENTRY", L"SECOND_ENTRY", L"THIRD_ENTRY");

    CustomMRU.Release();
    Cleanup_Testdata();

    hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL, CLSCTX_ALL, IID_PPV_ARG(IACLCustomMRU, &CustomMRU));
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    hr = CustomMRU->Initialize(szTestPath, 3);
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    hr = CustomMRU->AddMRUString(L"FIRST_ENTRY");
    ok_hex(hr, S_OK);
    verify_mru(CustomMRU, L"a", L"FIRST_ENTRY");

    CustomMRU.Release();

    key.Open(HKEY_CURRENT_USER, szTestPath);
    key.SetStringValue(L"MRUList", L"aaa");
    key.Close();

    hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL, CLSCTX_ALL, IID_PPV_ARG(IACLCustomMRU, &CustomMRU));
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    hr = CustomMRU->Initialize(szTestPath, 3);
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    hr = CustomMRU->AddMRUString(L"SECOND_ENTRY");
    ok_hex(hr, S_OK);
    verify_mru(CustomMRU, L"ba", L"FIRST_ENTRY", L"SECOND_ENTRY");
}

#define TYPED_URLS_KEY L"Software\\Microsoft\\Internet Explorer\\TypedURLs"

static void
RestoreTypedURLs(const CStringW& url1, const CStringW& url2)
{
    CRegKey key;
    key.Open(HKEY_CURRENT_USER, TYPED_URLS_KEY, KEY_WRITE);
    if (url1 != L"")
        key.SetStringValue(L"url1", url1);
    else
        key.DeleteValue(L"url1");
    if (url2 != L"")
        key.SetStringValue(L"url2", url2);
    else
        key.DeleteValue(L"url2");
}

static void
test_IACLCustomMRU_TypedURLs() // TypedURLs is special case
{
    CStringW url1, url2; // Save values
    {
        CRegKey key;
        key.Create(HKEY_CURRENT_USER, TYPED_URLS_KEY);

        WCHAR Value[MAX_PATH];
        ULONG cch = _countof(Value);
        LSTATUS Status = key.QueryStringValue(L"url1", Value, &cch);
        if (!Status)
            url1 = Value;

        cch = _countof(Value);
        Status = key.QueryStringValue(L"url2", Value, &cch);
        if (!Status)
            url2 = Value;

        // Set values
        key.SetStringValue(L"url1", L"aaa");
        key.SetStringValue(L"url2", L"bbb");
    }

    CComPtr<IACLCustomMRU> CustomMRU;
    HRESULT hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL, CLSCTX_ALL,
                                  IID_PPV_ARG(IACLCustomMRU, &CustomMRU));
    ok_hex(hr, S_OK);
    if (FAILED(hr))
    {
        skip("IACLCustomMRU was NULL\n");
        RestoreTypedURLs(url1, url2);
        return;
    }

    CComPtr<IACList> ACList;
    hr = CustomMRU->QueryInterface(IID_PPV_ARG(IACList, &ACList));
    ok_hex(hr, S_OK);
    if (SUCCEEDED(hr))
    {
        hr = ACList->Expand(L"C:");
        ok_hex(hr, E_NOTIMPL);
        hr = ACList->Expand(L"C:\\");
        ok_hex(hr, E_NOTIMPL);
        hr = ACList->Expand(L"C:\\Program Files");
        ok_hex(hr, E_NOTIMPL);
        hr = ACList->Expand(L"C:\\Program Files\\");
        ok_hex(hr, E_NOTIMPL);
        hr = ACList->Expand(L"http://");
        ok_hex(hr, E_NOTIMPL);
        hr = ACList->Expand(L"https://");
        ok_hex(hr, E_NOTIMPL);
        hr = ACList->Expand(L"https://google.co.jp/");
        ok_hex(hr, E_NOTIMPL);
    }

    hr = CustomMRU->Initialize(TYPED_URLS_KEY, 64);
    ok_hex(hr, S_OK);

    if (ACList)
    {
        hr = ACList->Expand(L"C:");
        ok_hex(hr, E_NOTIMPL);
        hr = ACList->Expand(L"C:\\");
        ok_hex(hr, E_NOTIMPL);
        hr = ACList->Expand(L"C:\\Program Files");
        ok_hex(hr, E_NOTIMPL);
        hr = ACList->Expand(L"C:\\Program Files\\");
        ok_hex(hr, E_NOTIMPL);
        hr = ACList->Expand(L"http://");
        ok_hex(hr, E_NOTIMPL);
        hr = ACList->Expand(L"https://");
        ok_hex(hr, E_NOTIMPL);
        hr = ACList->Expand(L"https://google.co.jp/");
        ok_hex(hr, E_NOTIMPL);
    }

    CComPtr<IEnumString> pEnum;
    hr = CustomMRU->QueryInterface(IID_PPV_ARG(IEnumString, &pEnum));
    ok_hex(hr, S_OK);
    if (FAILED(hr))
    {
        skip("IEnumString was NULL\n");
        RestoreTypedURLs(url1, url2);
        return;
    }

    CComPtr<IEnumString> pEnumClone;
    hr = pEnum->Clone(&pEnumClone);
    ok_hex(hr, E_NOTIMPL);

    hr = pEnum->Skip(1);
    ok_hex(hr, E_NOTIMPL);

#define INVALID_LPOLESTR ((LPOLESTR)(LONG_PTR)0xDEADBEEF)
    LPOLESTR apsz[2] = { NULL, INVALID_LPOLESTR };
    ULONG c = 0;
    hr = pEnum->Next(2, apsz, &c);
    ok_hex(hr, S_OK);
    ok_wstri(apsz[0], L"aaa");
    ok_int(c, 1);
    ok(apsz[1] == INVALID_LPOLESTR, "apsz[1] was '%S'\n", apsz[1]);
    CoTaskMemFree(apsz[0]);

    LPOLESTR psz = INVALID_LPOLESTR;
    c = 0;
    hr = pEnum->Next(0, &psz, &c);
    ok_hex(hr, S_OK);
    ok(psz == INVALID_LPOLESTR, "psz was '%S'\n", psz);
    ok_int(c, 0);

    psz = NULL;
    c = 0;
    hr = pEnum->Next(1, &psz, &c);
    ok_hex(hr, S_OK);
    ok_wstri(psz, L"bbb");
    ok_int(c, 1);
    CoTaskMemFree(psz);

    hr = CustomMRU->AddMRUString(L"https://google.co.jp");
    ok_hex(hr, E_FAIL);
    hr = CustomMRU->AddMRUString(L"C:");
    ok_hex(hr, E_FAIL);
    hr = CustomMRU->AddMRUString(L"C:\\");
    ok_hex(hr, E_FAIL);

    RestoreTypedURLs(url1, url2);
}

START_TEST(IACLCustomMRU)
{
    CCoInit init;
    ok_hex(init.hr, S_OK);
    if (!SUCCEEDED(init.hr))
        return;

    test_IACLCustomMRU_Basics();
    test_IACLCustomMRU_UpdateOrder();
    test_IACLCustomMRU_ExtraChars();
    test_IACLCustomMRU_Continue();
    test_IACLCustomMRU_TypedURLs();

    Cleanup_Testdata();
}
