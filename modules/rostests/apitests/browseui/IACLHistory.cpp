/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for IACLHistory objects
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define _UNICODE
#define UNICODE
#include <apitest.h>
#include <shlobj.h>
#include <atlbase.h>
#include <atlcom.h>
#include <stdio.h>
#include <shellutils.h>

struct CCoInit
{
    CCoInit() { hres = CoInitialize(NULL); }
    ~CCoInit() { if (SUCCEEDED(hres)) { CoUninitialize(); } }
    HRESULT hres;
};

START_TEST(IACLHistory)
{
    CCoInit init;
    ok_hex(init.hres, S_OK);
    if (FAILED(init.hres))
    {
        skip("CoInitialize failed with 0x%08lX\n", init.hres);
        return;
    }

    HRESULT hr;
    CComPtr<IUnknown> pHistory;
    hr = CoCreateInstance(CLSID_ACLHistory, NULL, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARG(IUnknown, &pHistory));
    ok_long(hr, S_OK);
    ok_int(!!pHistory, TRUE);

    CComPtr<IEnumString> pEnum;
    hr = pHistory->QueryInterface(IID_PPV_ARG(IEnumString, &pEnum));
    ok_long(hr, S_OK);

    hr = pEnum->Reset();
    ok_long(hr, S_OK);
    hr = pEnum->Reset();
    ok_long(hr, S_OK);

    hr = pEnum->Skip(0);
    ok_long(hr, E_NOTIMPL);
    hr = pEnum->Skip(1);
    ok_long(hr, E_NOTIMPL);
    hr = pEnum->Skip(3);
    ok_long(hr, E_NOTIMPL);
}
