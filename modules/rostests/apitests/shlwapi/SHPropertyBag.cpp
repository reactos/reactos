/*
 * Copyright 2023 Katayama Hirofumi MZ
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <apitest.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shlwapi_undoc.h>

#include <pseh/pseh2.h>

static LPCWSTR s_pszPropName0 = NULL;
static LPCWSTR s_pszPropName1 = NULL;
static VARTYPE s_vt;
static INT s_cWrite = 0;

class CDummyWritePropertyBag : public IPropertyBag
{
public:
    CDummyWritePropertyBag()
    {
    }

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override
    {
        ok_int(0, 1);
        return S_OK;
    }
    STDMETHODIMP_(ULONG) AddRef() override
    {
        ok_int(0, 1);
        return S_OK;
    }
    STDMETHODIMP_(ULONG) Release() override
    {
        ok_int(0, 1);
        return S_OK;
    }

    // IPropertyBag
    STDMETHODIMP Read(LPCWSTR pszPropName, VARIANT *pvari, IErrorLog *pErrorLog) override
    {
        ok_int(0, 1);
        return S_OK;
    }

    STDMETHODIMP Write(LPCWSTR pszPropName, VARIANT *pvari) override
    {
        if (s_pszPropName0)
        {
            ok_wstr(pszPropName, s_pszPropName0);
            s_pszPropName0 = NULL;
        }
        else if (s_pszPropName1)
        {
            ok_wstr(pszPropName, s_pszPropName1);
            s_pszPropName1 = NULL;
        }
        ok_int(s_vt, V_VT(pvari));
        ++s_cWrite;
        return S_OK;
    }
};

static void SHPropertyBag_WriteTest(void)
{
    HRESULT hr;
    CDummyWritePropertyBag dummy;

    s_cWrite = 0;
    s_pszPropName0 = L"BOOL1";
    s_vt = VT_BOOL;
    hr = SHPropertyBag_WriteBOOL(&dummy, s_pszPropName0, TRUE);
    ok_long(hr, S_OK);
    ok_int(s_cWrite, 1);

    s_cWrite = 0;
    s_pszPropName0 = L"SHORT1";
    s_vt = VT_UI2;
    hr = SHPropertyBag_WriteSHORT(&dummy, s_pszPropName0, 1);
    ok_long(hr, S_OK);
    ok_int(s_cWrite, 1);

    s_cWrite = 0;
    s_pszPropName0 = L"LONG1";
    s_vt = VT_I4;
    hr = SHPropertyBag_WriteLONG(&dummy, s_pszPropName0, 1);
    ok_long(hr, S_OK);
    ok_int(s_cWrite, 1);

    s_cWrite = 0;
    s_pszPropName0 = L"DWORD1";
    s_vt = VT_UI4;
    hr = SHPropertyBag_WriteDWORD(&dummy, s_pszPropName0, 1);
    ok_long(hr, S_OK);
    ok_int(s_cWrite, 1);

    s_cWrite = 0;
    s_pszPropName0 = L"Str1";
    s_vt = VT_BSTR;
    hr = SHPropertyBag_WriteStr(&dummy, s_pszPropName0, L"1");
    ok_long(hr, S_OK);
    ok_int(s_cWrite, 1);

    s_cWrite = 0;
    s_pszPropName0 = L"POINTL1.x";
    s_pszPropName1 = L"POINTL1.y";
    s_vt = VT_I4;
    POINTL ptl = { 0xEEEE, 0xDDDD };
    hr = SHPropertyBag_WritePOINTL(&dummy, L"POINTL1", &ptl);
    ok_long(hr, S_OK);
    ok_int(s_cWrite, 2);

    s_cWrite = 0;
    s_pszPropName0 = L"POINTS1.x";
    s_pszPropName1 = L"POINTS1.y";
    s_vt = VT_I4;
    POINTS pts = { 0x2222, 0x3333 };
    hr = SHPropertyBag_WritePOINTS(&dummy, L"POINTS1", &pts);
    ok_long(hr, S_OK);
    ok_int(s_cWrite, 2);

    s_cWrite = 0;
    s_pszPropName0 = L"RECTL1.left";
    s_pszPropName1 = L"RECTL1.top";
    s_vt = VT_I4;
    RECTL rcl = { 123, 456, 789, 101112 };
    hr = SHPropertyBag_WriteRECTL(&dummy, L"RECTL1", &rcl);
    ok_long(hr, S_OK);
    ok_int(s_cWrite, 4);

    s_cWrite = 0;
    GUID guid;
    ZeroMemory(&guid, sizeof(guid));
    s_pszPropName0 = L"GUID1";
    s_pszPropName1 = NULL;
    s_vt = VT_BSTR;
    hr = SHPropertyBag_WriteGUID(&dummy, s_pszPropName0, &guid);
    ok_long(hr, S_OK);
    ok_int(s_cWrite, 1);
}

START_TEST(SHPropertyBag)
{
    SHPropertyBag_WriteTest();
}
