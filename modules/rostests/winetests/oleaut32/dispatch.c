/*
 * Dispatch test
 *
 * Copyright 2009 James Hawkins
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

#define COBJMACROS
#define CONST_VTABLE

#include <wine/test.h>
#include <windef.h>
#include <winbase.h>
#include <oaidl.h>

static const WCHAR szSunshine[] = {'S','u','n','s','h','i','n','e',0};

/* Temporary storage for ok_bstr. */
static CHAR temp_str[MAX_PATH];

#define ok_bstr(bstr, expected, format) \
    do { \
    WideCharToMultiByte(CP_ACP, 0, bstr, -1, temp_str, MAX_PATH, NULL, NULL); \
    if (lstrcmpA(temp_str, expected) != 0) \
        ok(0, format, expected, temp_str); \
    } while(0);

#define INIT_DISPPARAMS(dp, args, named_args, num_args, num_named_args) \
  dp.rgvarg = args; \
  dp.rgdispidNamedArgs = named_args; \
  dp.cArgs = num_args; \
  dp.cNamedArgs = num_named_args; \

/* Initializes vararg with three values:
 *  VT_I2 - 42
 *  VT_I4 - 1234567890
 *  VT_BSTR - "Sunshine"
 */
#define INIT_VARARG(vararg) \
  VariantInit(&vararg[0]); \
  V_VT(&vararg[0]) = VT_I2; \
  V_I2(&vararg[0]) = 42; \
  VariantInit(&vararg[1]); \
  V_VT(&vararg[1]) = VT_I4; \
  V_I4(&vararg[1]) = 1234567890; \
  VariantInit(&vararg[2]); \
  V_VT(&vararg[2]) = VT_BSTR; \
  V_BSTR(&vararg[2]) = SysAllocString(szSunshine);

/* Clears the vararg. */
#define CLEAR_VARARG(vararg) \
  VariantClear(&vararg[0]); \
  VariantClear(&vararg[1]); \
  VariantClear(&vararg[2]);

static void test_DispGetParam(void)
{
    HRESULT hr;
    DISPPARAMS dispparams;
    VARIANTARG vararg[3];
    VARIANT result;
    unsigned int err_index;

    VariantInit(&result);

    /* DispGetParam crashes on Windows if pdispparams is NULL. */

    /* pdispparams has zero parameters. */
    INIT_DISPPARAMS(dispparams, NULL, NULL, 0, 0);
    VariantInit(&result);
    err_index = 0xdeadbeef;
    hr = DispGetParam(&dispparams, 0, VT_I2, &result, &err_index);
    ok(hr == DISP_E_PARAMNOTFOUND,
       "Expected DISP_E_PARAMNOTFOUND, got %08lx\n", hr);
    ok(V_VT(&result) == VT_EMPTY,
       "Expected VT_EMPTY, got %08x\n", V_VT(&result));
    ok(err_index == 0xdeadbeef,
       "Expected err_index to be unchanged, got %d\n", err_index);

    /* pdispparams has zero parameters, position is invalid. */
    INIT_DISPPARAMS(dispparams, NULL, NULL, 0, 0);
    VariantInit(&result);
    err_index = 0xdeadbeef;
    hr = DispGetParam(&dispparams, 1, VT_I2, &result, &err_index);
    ok(hr == DISP_E_PARAMNOTFOUND,
       "Expected DISP_E_PARAMNOTFOUND, got %08lx\n", hr);
    ok(V_VT(&result) == VT_EMPTY,
       "Expected VT_EMPTY, got %08x\n", V_VT(&result));
    ok(err_index == 0xdeadbeef,
       "Expected err_index to be unchanged, got %d\n", err_index);

    /* pdispparams has zero parameters, pvarResult is NULL. */
    INIT_DISPPARAMS(dispparams, NULL, NULL, 0, 0);
    err_index = 0xdeadbeef;
    hr = DispGetParam(&dispparams, 0, VT_I2, NULL, &err_index);
    ok(hr == DISP_E_PARAMNOTFOUND,
       "Expected DISP_E_PARAMNOTFOUND, got %08lx\n", hr);
    ok(err_index == 0xdeadbeef,
       "Expected err_index to be unchanged, got %d\n", err_index);

    /* pdispparams has zero parameters, puArgErr is NULL. */
    INIT_DISPPARAMS(dispparams, NULL, NULL, 0, 0);
    VariantInit(&result);
    hr = DispGetParam(&dispparams, 0, VT_I2, &result, NULL);
    ok(hr == DISP_E_PARAMNOTFOUND,
       "Expected DISP_E_PARAMNOTFOUND, got %08lx\n", hr);
    ok(V_VT(&result) == VT_EMPTY,
       "Expected VT_EMPTY, got %08x\n", V_VT(&result));

    /* pdispparams.cArgs is 1, yet pdispparams.rgvarg is NULL. */
    INIT_DISPPARAMS(dispparams, NULL, NULL, 1, 0);
    VariantInit(&result);
    err_index = 0xdeadbeef;
    hr = DispGetParam(&dispparams, 0, VT_I2, &result, &err_index);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);
    ok(V_VT(&result) == VT_EMPTY,
       "Expected VT_EMPTY, got %08x\n", V_VT(&result));
    ok(err_index == 0, "Expected 0, got %d\n", err_index);

    /* pdispparams.cNamedArgs is 1, yet pdispparams.rgdispidNamedArgs is NULL.
     *
     * This crashes on Windows.
     */

    /* {42, 1234567890, "Sunshine"} */
    INIT_VARARG(vararg);

    /* Get the first param.  position is end-based, so 2 is the first parameter
     * of 3 parameters.
     */
    INIT_DISPPARAMS(dispparams, vararg, NULL, 3, 0);
    VariantInit(&result);
    err_index = 0xdeadbeef;
    hr = DispGetParam(&dispparams, 2, VT_I2, &result, &err_index);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(V_VT(&result) == VT_I2, "Expected VT_I2, got %08x\n", V_VT(&result));
    ok(V_I2(&result) == 42, "Expected 42, got %d\n", V_I2(&result));
    ok(err_index == 0xdeadbeef,
       "Expected err_index to be unchanged, got %d\n", err_index);

    /* Get the second param. */
    INIT_DISPPARAMS(dispparams, vararg, NULL, 3, 0);
    VariantInit(&result);
    err_index = 0xdeadbeef;
    hr = DispGetParam(&dispparams, 1, VT_I4, &result, &err_index);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(V_VT(&result) == VT_I4, "Expected VT_I4, got %08x\n", V_VT(&result));
    ok(V_I4(&result) == 1234567890,
       "Expected 1234567890, got %ld\n", V_I4(&result));
    ok(err_index == 0xdeadbeef,
       "Expected err_index to be unchanged, got %d\n", err_index);

    /* Get the third param. */
    INIT_DISPPARAMS(dispparams, vararg, NULL, 3, 0);
    VariantInit(&result);
    err_index = 0xdeadbeef;
    hr = DispGetParam(&dispparams, 0, VT_BSTR, &result, &err_index);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(V_VT(&result) == VT_BSTR, "Expected VT_BSTR, got %08x\n", V_VT(&result));
    ok_bstr(V_BSTR(&result), "Sunshine", "Expected %s, got %s\n");
    ok(err_index == 0xdeadbeef,
       "Expected err_index to be unchanged, got %d\n", err_index);
    VariantClear(&result);

    /* position is out of range. */
    INIT_DISPPARAMS(dispparams, vararg, NULL, 3, 0);
    VariantInit(&result);
    err_index = 0xdeadbeef;
    hr = DispGetParam(&dispparams, 3, VT_I2, &result, &err_index);
    ok(hr == DISP_E_PARAMNOTFOUND,
       "Expected DISP_E_PARAMNOTFOUND, got %08lx\n", hr);
    ok(V_VT(&result) == VT_EMPTY,
       "Expected VT_EMPTY, got %08x\n", V_VT(&result));
    ok(err_index == 0xdeadbeef,
       "Expected err_index to be unchanged, got %d\n", err_index);

    /* pvarResult is NULL. */
    INIT_DISPPARAMS(dispparams, vararg, NULL, 3, 0);
    err_index = 0xdeadbeef;
    hr = DispGetParam(&dispparams, 2, VT_I2, NULL, &err_index);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);
    ok(err_index == 0, "Expected 0, got %d\n", err_index);

    /* puArgErr is NULL. */
    INIT_DISPPARAMS(dispparams, vararg, NULL, 3, 0);
    VariantInit(&result);
    hr = DispGetParam(&dispparams, 2, VT_I2, &result, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(V_VT(&result) == VT_I2, "Expected VT_I2, got %08x\n", V_VT(&result));
    ok(V_I2(&result) == 42, "Expected 42, got %d\n", V_I2(&result));

    /* Coerce the first param to VT_I4. */
    INIT_DISPPARAMS(dispparams, vararg, NULL, 3, 0);
    VariantInit(&result);
    err_index = 0xdeadbeef;
    hr = DispGetParam(&dispparams, 2, VT_I4, &result, &err_index);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(V_VT(&result) == VT_I4, "Expected VT_I4, got %08x\n", V_VT(&result));
    ok(V_I4(&result) == 42, "Expected 42, got %ld\n", V_I4(&result));
    ok(err_index == 0xdeadbeef,
       "Expected err_index to be unchanged, got %d\n", err_index);

    /* Coerce the first param to VT_BSTR. */
    INIT_DISPPARAMS(dispparams, vararg, NULL, 3, 0);
    VariantInit(&result);
    err_index = 0xdeadbeef;
    hr = DispGetParam(&dispparams, 2, VT_BSTR, &result, &err_index);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(V_VT(&result) == VT_BSTR, "Expected VT_BSTR, got %08x\n", V_VT(&result));
    ok_bstr(V_BSTR(&result), "42", "Expected %s, got %s\n");
    ok(err_index == 0xdeadbeef,
       "Expected err_index to be unchanged, got %d\n", err_index);
    VariantClear(&result);

    /* Coerce the second (VT_I4) param to VT_I2. */
    INIT_DISPPARAMS(dispparams, vararg, NULL, 3, 0);
    VariantInit(&result);
    err_index = 0xdeadbeef;
    hr = DispGetParam(&dispparams, 1, VT_I2, &result, &err_index);
    ok(hr == DISP_E_OVERFLOW, "Expected DISP_E_OVERFLOW, got %08lx\n", hr);
    ok(V_VT(&result) == VT_EMPTY,
       "Expected VT_EMPTY, got %08x\n", V_VT(&result));
    ok(err_index == 1, "Expected 1, got %d\n", err_index);

    /* Coerce the third (VT_BSTR) param to VT_I2. */
    INIT_DISPPARAMS(dispparams, vararg, NULL, 3, 0);
    VariantInit(&result);
    err_index = 0xdeadbeef;
    hr = DispGetParam(&dispparams, 0, VT_I2, &result, &err_index);
    ok(hr == DISP_E_TYPEMISMATCH,
       "Expected DISP_E_TYPEMISMATCH, got %08lx\n", hr);
    ok(V_VT(&result) == VT_EMPTY,
       "Expected VT_EMPTY, got %08x\n", V_VT(&result));
    ok(err_index == 2, "Expected 2, got %d\n", err_index);

    /* Coerce the first parameter to an invalid type. */
    INIT_DISPPARAMS(dispparams, vararg, NULL, 3, 0);
    VariantInit(&result);
    err_index = 0xdeadbeef;
    hr = DispGetParam(&dispparams, 2, VT_ILLEGAL, &result, &err_index);
    ok(hr == DISP_E_BADVARTYPE, "Expected DISP_E_BADVARTYPE, got %08lx\n", hr);
    ok(V_VT(&result) == VT_EMPTY,
       "Expected VT_EMPTY, got %08x\n", V_VT(&result));
    ok(err_index == 0, "Expected 0, got %d\n", err_index);

    CLEAR_VARARG(vararg);

    /* Coerce the first parameter, which is of type VT_EMPTY, to VT_BSTR. */
    VariantInit(&vararg[0]);
    INIT_DISPPARAMS(dispparams, vararg, NULL, 1, 0);
    VariantInit(&result);
    err_index = 0xdeadbeef;
    hr = DispGetParam(&dispparams, 0, VT_BSTR, &result, &err_index);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(V_VT(&result) == VT_BSTR, "Expected VT_BSTR, got %08x\n", V_VT(&result));
    ok(err_index == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", err_index);
    VariantClear(&result);
}

static HRESULT WINAPI unk_QI(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        return S_OK;
    }
    else
    {
        *obj = NULL;
        return E_NOINTERFACE;
    }
}

static ULONG WINAPI unk_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI unk_Release(IUnknown *iface)
{
    return 1;
}

static const IUnknownVtbl unkvtbl =
{
    unk_QI,
    unk_AddRef,
    unk_Release
};

static IUnknown test_unk = { &unkvtbl };

static void test_CreateStdDispatch(void)
{
    static const WCHAR stdole2W[] = {'s','t','d','o','l','e','2','.','t','l','b',0};
    ITypeLib *tl;
    ITypeInfo *ti;
    IUnknown *unk;
    HRESULT hr;

    hr = CreateStdDispatch(NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = CreateStdDispatch(NULL, NULL, NULL, &unk);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = LoadTypeLib(stdole2W, &tl);
    ok(hr == S_OK, "got %08lx\n", hr);
    hr = ITypeLib_GetTypeInfoOfGuid(tl, &IID_IUnknown, &ti);
    ok(hr == S_OK, "got %08lx\n", hr);
    ITypeLib_Release(tl);

    hr = CreateStdDispatch(NULL, &test_unk, NULL, &unk);
    ok(hr == E_INVALIDARG, "got %08lx\n", hr);

    hr = CreateStdDispatch(NULL, NULL, ti, &unk);
    ok(hr == E_INVALIDARG, "got %08lx\n", hr);

    hr = CreateStdDispatch(NULL, &test_unk, ti, &unk);
    ok(hr == S_OK, "got %08lx\n", hr);
    IUnknown_Release(unk);

    ITypeInfo_Release(ti);
}

START_TEST(dispatch)
{
    test_DispGetParam();
    test_CreateStdDispatch();
}
