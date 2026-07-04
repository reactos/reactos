/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for CShellDispatch
 * COPYRIGHT:   Copyright 2026 Whindmar Saksit <whindsaks@proton.me>
 */

#include "shelltest.h"
#include <oleauto.h>
#include <shellutils.h>

static int GetFileSize(PCWSTR Path)
{
    WIN32_FIND_DATA wfd;
    HANDLE hFind = FindFirstFileW(Path, &wfd);
    if (hFind == INVALID_HANDLE_VALUE)
        return -1;
    FindClose(hFind);
    return wfd.nFileSizeLow;
}

static void VariantClearAndInvalidate(VARIANT *pV)
{
    if (!pV)
        return;
    VariantClear(pV);
    V_BSTR(pV) = (BSTR)(SIZE_T)0xBAADF00Dul;
}

template<class T> static void SafeRelease(T &p)
{
    if (!p)
        return;
    p->Release();
    p = NULL;
}

template<class T> static HRESULT CreateFolderFromNameSpace(T Dir, Folder **ppsdf)
{
    *ppsdf = NULL;
    CComPtr<IShellDispatch> pSD;
    HRESULT hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_ALL, IID_PPV_ARG(IShellDispatch, &pSD));
    if (FAILED(hr))
        return -1; // Special return value so we know to skip

    VARIANT vDir = { VT_I4 };
    V_I4(&vDir) = (UINT)(SIZE_T)Dir;
    if (!IS_INTRESOURCE((SIZE_T)Dir))
    {
        BSTR bs = SysAllocString((PCWSTR)(SIZE_T)Dir);
        if (!bs)
            return E_OUTOFMEMORY;
        V_VT(&vDir) = VT_BSTR;
        V_BSTR(&vDir) = bs;
    }
    hr = pSD->NameSpace(vDir, ppsdf);
    VariantClear(&vDir);
    return hr;
}

#define ok_NameSpaceFolderParseIsPath(ns, parse, path, mustexist, hrparse) do \
{ \
    if ((mustexist) && GetFileAttributesW((path)) == INVALID_FILE_ATTRIBUTES) \
    { \
        skip("Can't ParseName because \"%ls\" does not exist\n", (path)); \
        break; \
    } \
    hr = CreateFolderFromNameSpace((ns), &psdf); \
    ok_long(hr, S_OK); \
    if (SUCCEEDED(hr) && psdf) \
    { \
        FolderItem *pfi = NULL; \
        hr = psdf->ParseName((BSTR)(parse), &pfi); \
        ok_long(hr, (hrparse)); \
        if (SUCCEEDED(hr) && pfi) \
        { \
            BSTR bs = NULL; \
            hr = pfi->get_Path(&bs); \
            ok_long(hr, S_OK); \
            ok(SUCCEEDED(hr) && !lstrcmpiW(bs, (path)), "ParseName path \"%ls\" is not \"%ls\"\n", bs, (path)); \
            SysFreeString(SUCCEEDED(hr) ? bs : NULL); \
            SafeRelease(pfi); \
        } \
    } \
    SafeRelease(psdf); \
} while (0)

static void TestNameSpaceFolder()
{
    HRESULT hr;
    Folder *psdf = NULL;
    FolderItem *psdfi = NULL;
    WCHAR buf1[MAX_PATH], buf2[MAX_PATH];

    hr = CreateFolderFromNameSpace(CSIDL_DESKTOP, &psdf); // Root folder
    ok_long(hr, S_OK);
    if (hr == -1)
    {
        skip("Unable to initialize test\n");
        return;
    }
    SafeRelease(psdf);

    hr = CreateFolderFromNameSpace(CSIDL_CONTROLS, &psdf); // Virtual folder
    ok_long(hr, S_OK);
    SafeRelease(psdf);

    hr = CreateFolderFromNameSpace(CSIDL_DESKTOP, &psdf);
    ok_long(hr, S_OK);
    if (SUCCEEDED(hr))
    {
        hr = psdf->ParseName((BSTR)L"X:\\DoesNotExist.xyz", (psdfi = NULL, &psdfi));
        ok(hr != S_OK, "Must fail parsing\n");
        SafeRelease(psdfi);
    }
    SafeRelease(psdf);

    hr = CreateFolderFromNameSpace(CSIDL_DESKTOP, &psdf);
    ok_long(hr, S_OK);
    if (SUCCEEDED(hr))
    {
        hr = psdf->ParseName((BSTR)L"", (psdfi = NULL, &psdfi));
        ok_long(hr, S_OK);
        SafeRelease(psdfi);
    }
    SafeRelease(psdf);

    hr = CreateFolderFromNameSpace(L"", &psdf);
    ok(hr == S_OK || hr == S_FALSE, "Empty NS"); // S_FALSE on XP
    SafeRelease(psdf);

    ok_NameSpaceFolderParseIsPath(CSIDL_DESKTOP, L"", L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}", false, S_OK); // My Computer

    SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, buf1);
    ok_NameSpaceFolderParseIsPath(CSIDL_DESKTOP, buf1, buf1, true, S_OK);

    GetShortPathNameW(buf1, buf2, _countof(buf2)); // c:\progra~1
    ok_NameSpaceFolderParseIsPath(CSIDL_DESKTOP, buf2, buf1, true, S_OK); // ParseName+get_Path returns the long path
}

static void TestFolderItem()
{
    WCHAR buf[MAX_PATH];
    VARIANT v = {};
    Folder *psdf = NULL;
    FolderItem *psdfi = NULL;
    FolderItem2 *psdfi2;
    HRESULT hr = CreateFolderFromNameSpace(CSIDL_DESKTOP, &psdf);
    if (hr == -1)
    {
        skip("Unable to initialize test\n");
        return;
    }
    GetModuleFileName(NULL, buf, _countof(buf));
    hr = psdf->ParseName(buf, &psdfi);
    if ((ok(SUCCEEDED(hr), "ParseName %ls failed\n", buf), SUCCEEDED(hr)))
    {
        hr = psdfi->get_IsFileSystem(&V_BOOL(&v));
        ok(hr == S_OK && V_BOOL(&v), "IsFileSystem");
        
        hr = psdfi->get_IsFolder(&V_BOOL(&v));
        ok(hr == S_OK && !V_BOOL(&v), "!IsFolder");
        
        hr = psdfi->get_Size(&V_I4(&v));
        ok(hr == S_OK && V_I4(&v) == GetFileSize(buf), "Size");

        hr = psdfi->QueryInterface(IID_PPV_ARG(FolderItem2, &psdfi2));
        if ((ok(SUCCEEDED(hr), "QI FolderItem2\n", buf), SUCCEEDED(hr)))
        {
            hr = psdfi2->ExtendedProperty((BSTR)L"Size", &v);
            if ((ok(SUCCEEDED(hr), "ExtendedProperty failed\n"), SUCCEEDED(hr)))
            {
                VariantChangeType(&v, &v, 0, VT_I4);
                ok(V_I4(&v) == GetFileSize(buf), "Wrong size from property\n");
                VariantClearAndInvalidate(&v);
            }
            SafeRelease(psdfi2);
        }
        SafeRelease(psdfi);
    }
    SafeRelease(psdf);
}

START_TEST(CShellDispatch)
{
    CCoInit ComInit;

    TestNameSpaceFolder();
    TestFolderItem();

    HRESULT hr;
    CComPtr<IShellDispatch2> pSD2;
    if (FAILED(hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_ALL, IID_PPV_ARG(IShellDispatch2, &pSD2))))
    {
        skip("Unable to initialize test\n");
        return;
    }

    IDispatch *pDisp = NULL;
    hr = pSD2->get_Application(&pDisp);
    ok_long(hr, S_OK);
    ok(!!pDisp, "get_Application\n");
    SafeRelease(pDisp);
    hr = pSD2->get_Application(NULL);
    ok(FAILED(hr), "get_Application NULL\n");

    LONG lVal;
    hr = pSD2->IsRestricted((BSTR)L"DoesNotExist", (BSTR)L"DoesNotExist", (lVal = 42, &lVal));
    ok_long(hr, S_OK);
    ok(lVal == 0, "IsRestricted\n");

    VARIANT v = {};
    V_I4(&v) = PROCESSOR_ARCHITECTURE_UNKNOWN;
    hr = pSD2->GetSystemInformation((BSTR)L"ProcessorArchitecture", &v);
    ok_long(hr, S_OK);
    ok(V_I4(&v) != PROCESSOR_ARCHITECTURE_UNKNOWN, "GetSystemInformation ProcessorArchitecture\n");
    VariantClearAndInvalidate(&v);

    hr = pSD2->IsServiceRunning((BSTR)L"DoesNotExist", &v);
    ok_long(hr, S_OK);
    ok_long(V_VT(&v), VT_BOOL);
    ok_long(V_BOOL(&v), VARIANT_FALSE);
    VariantClearAndInvalidate(&v);

    CComPtr<IShellDispatch4> pSD4;
    if (FAILED(hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_ALL, IID_PPV_ARG(IShellDispatch4, &pSD4))))
    {
        skip("Unable to initialize test\n");
        return;
    }
    hr = pSD4->GetSetting(SSF_NOCONFIRMRECYCLE, &V_BOOL(&v));
    ok_long(hr, S_OK);
    ok(V_BOOL(&v) == VARIANT_FALSE || V_BOOL(&v) == VARIANT_TRUE, "VARIANT_BOOL\n");
}
