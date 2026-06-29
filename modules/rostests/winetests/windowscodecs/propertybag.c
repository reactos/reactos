/*
 * Copyright 2013 Ludger Sprenker
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

#include <stdarg.h>
#include <math.h>

#define COBJMACROS
#define CONST_VTABLE

#include "windef.h"
#include "objbase.h"
#include "wincodec.h"
#include "wincodecsdk.h"
#include "wine/test.h"

static const WCHAR wszTestProperty1[] = {'P','r','o','p','e','r','t','y','1',0};
static const WCHAR wszTestProperty2[] = {'P','r','o','p','e','r','t','y','2',0};
static const WCHAR wszTestInvalidProperty[] = {'I','n','v','a','l','i','d',0};

static void test_propertybag_getpropertyinfo(IPropertyBag2 *property, ULONG expected_count)
{
    HRESULT hr;
    PROPBAG2 pb[2];
    ULONG out_count;

    /* iProperty: Out of bounce */
    hr = IPropertyBag2_GetPropertyInfo(property, expected_count, 1, pb, &out_count);
    ok(hr == WINCODEC_ERR_VALUEOUTOFRANGE,
       "GetPropertyInfo handled iProperty out of bounce wrong, hr=%lx\n", hr);

    /* cProperty: Out of bounce */
    hr = IPropertyBag2_GetPropertyInfo(property, 0, expected_count+1, pb, &out_count);
    ok(hr == WINCODEC_ERR_VALUEOUTOFRANGE,
       "GetPropertyInfo handled cProperty out of bounce wrong, hr=%lx\n", hr);

    /* GetPropertyInfo can be called for zero items on Windows 8 but not on Windows 7 (wine behaves like Win8) */
    if (expected_count == 0)
        return;

    hr = IPropertyBag2_GetPropertyInfo(property, 0, expected_count, pb, &out_count);
    ok(hr == S_OK, "GetPropertyInfo failed, hr=%lx\n", hr);
    if (FAILED(hr))
        return;

    ok(expected_count == out_count,
       "GetPropertyInfo returned unexpected property count, %li != %li)\n",
       expected_count, out_count);

    if(expected_count != 2)
        return;

    ok(pb[0].vt == VT_UI1, "Invalid variant type, pb[0].vt=%x\n", pb[0].vt);
    ok(pb[0].dwType == PROPBAG2_TYPE_DATA, "Invalid variant type, pb[0].dwType=%lx\n", pb[0].dwType);
    ok(lstrcmpW(pb[0].pstrName, wszTestProperty1) == 0, "Invalid property name, pb[0].pstrName=%s\n", wine_dbgstr_w(pb[0].pstrName));
    CoTaskMemFree(pb[0].pstrName);

    ok(pb[1].vt == VT_R4, "Invalid variant type, pb[1].vt=%x\n", pb[1].vt);
    ok(pb[1].dwType == PROPBAG2_TYPE_DATA, "Invalid variant type, pb[1].dwType=%lx\n", pb[1].dwType);
    ok(lstrcmpW(pb[1].pstrName, wszTestProperty2) == 0, "Invalid property name, pb[1].pstrName=%s\n", wine_dbgstr_w(pb[1].pstrName));
    CoTaskMemFree(pb[1].pstrName);
}

static void test_propertybag_countproperties(IPropertyBag2 *property, ULONG expected_count)
{
    ULONG count = (ULONG)-1;
    HRESULT hr;

    hr = IPropertyBag2_CountProperties(property, NULL);
    ok(hr == E_INVALIDARG, "CountProperties returned unexpected result, hr=%lx\n", hr);

    hr = IPropertyBag2_CountProperties(property, &count);
    ok(hr == S_OK, "CountProperties failed, hr=%lx\n", hr);

    if (FAILED(hr))
        return;

    ok(count == expected_count, "CountProperties returned invalid value, count=%li\n", count);
}

static void test_propertybag_read(IPropertyBag2 *property)
{
    HRESULT hr;
    PROPBAG2 options[3] = {{0}};
    VARIANT values[3];
    HRESULT itm_hr[3] = {S_OK, S_OK, S_OK};

    /* 1. One unknown property */
    options[0].pstrName = (LPOLESTR)wszTestInvalidProperty;
    hr = IPropertyBag2_Read(property, 1, options, NULL, values, itm_hr);
    ok(hr == E_FAIL,
       "Read for an unknown property did not fail with expected code, hr=%lx\n", hr);

    /* 2. One known property */
    options[0].pstrName = (LPOLESTR)wszTestProperty1;
    itm_hr[0] = E_FAIL;
    hr = IPropertyBag2_Read(property, 1, options, NULL, values, itm_hr);
    ok(hr == S_OK, "Read failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(itm_hr[0] == S_OK,
           "Read failed, itm_hr[0]=%lx\n", itm_hr[0]);
        ok(V_VT(&values[0]) == VT_UI1,
           "Read failed, V_VT(&values[0])=%x\n", V_VT(&values[0]));
        ok(V_UNION(&values[0], bVal) == 12,
           "Read failed, &values[0]=%i\n", V_UNION(&values[0], bVal));

        VariantClear(&values[0]);
    }

    /* 3. Two known properties */
    options[0].pstrName = (LPOLESTR)wszTestProperty1;
    options[1].pstrName = (LPOLESTR)wszTestProperty2;
    itm_hr[0] = E_FAIL;
    itm_hr[1] = E_FAIL;
    hr = IPropertyBag2_Read(property, 2, options, NULL, values, itm_hr);
    ok(hr == S_OK, "Read failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(itm_hr[0] == S_OK, "Read failed, itm_hr[0]=%lx\n", itm_hr[0]);
        ok(V_VT(&values[0]) == VT_UI1, "Read failed, V_VT(&values[0])=%x\n", V_VT(&values[0]));
        ok(V_UNION(&values[0], bVal) == 12, "Read failed, &values[0]=%i\n", V_UNION(&values[0], bVal));

        ok(itm_hr[1] == S_OK, "Read failed, itm_hr[1]=%lx\n", itm_hr[1]);
        ok(V_VT(&values[1]) == VT_R4, "Read failed, V_VT(&values[1])=%x\n", V_VT(&values[1]));
        ok(V_UNION(&values[1], fltVal) == (float)3.14, "Read failed, &values[1]=%f\n", V_UNION(&values[1], fltVal));

        VariantClear(&values[0]);
        VariantClear(&values[1]);
    }


    /* 4. One unknown property between two valid */

    /* Exotic initializations so we can detect what is unchanged */
    itm_hr[0] = -1; itm_hr[1] = -1; itm_hr[2] = -1;
    V_VT(&values[0]) = VT_NULL;
    V_VT(&values[1]) = VT_NULL;
    V_VT(&values[2]) = VT_NULL;
    V_UNION(&values[0], bVal) = 254;
    V_UNION(&values[1], bVal) = 254;
    V_UNION(&values[2], bVal) = 254;

    options[0].pstrName = (LPOLESTR)wszTestProperty1;
    options[1].pstrName = (LPOLESTR)wszTestInvalidProperty;
    options[2].pstrName = (LPOLESTR)wszTestProperty2;

    hr = IPropertyBag2_Read(property, 3, options, NULL, values, itm_hr);
    ok(hr == E_FAIL, "Read failed, hr=%lx\n", hr);
    if (hr == E_FAIL)
    {
        ok(itm_hr[0] == S_OK, "Read error code has unexpected value, itm_hr[0]=%lx\n", itm_hr[0]);
        ok(itm_hr[1] == -1,   "Read error code has unexpected value, itm_hr[1]=%lx\n", itm_hr[1]);
        ok(itm_hr[2] == -1,   "Read error code has unexpected value, itm_hr[2]=%lx\n", itm_hr[2]);

        ok(V_VT(&values[0]) == VT_UI1,  "Read variant has unexpected type, V_VT(&values[0])=%x\n", V_VT(&values[0]));
        ok(V_VT(&values[1]) == VT_NULL, "Read variant has unexpected type, V_VT(&values[1])=%x\n", V_VT(&values[1]));
        ok(V_VT(&values[2]) == VT_NULL, "Read variant has unexpected type, V_VT(&values[2])=%x\n", V_VT(&values[2]));

        ok(V_UNION(&values[0], bVal) == 12,  "Read variant has unexpected value, V_UNION(&values[0])=%i\n", V_UNION(&values[0], bVal));
        ok(V_UNION(&values[1], bVal) == 254, "Read variant has unexpected value, V_UNION(&values[1])=%i\n", V_UNION(&values[1], bVal));
        ok(V_UNION(&values[2], bVal) == 254, "Read variant has unexpected value, V_UNION(&values[2])=%i\n", V_UNION(&values[2], bVal));
    }
}

static void test_propertybag_write(IPropertyBag2 *property)
{
    HRESULT hr;
    PROPBAG2 options[2] = {{0}};
    VARIANT values[2];

    VariantInit(&values[0]);
    VariantInit(&values[1]);

    /* 1. One unknown property */
    options[0].pstrName = (LPOLESTR)wszTestInvalidProperty;
    hr = IPropertyBag2_Write(property, 1, options, values);
    ok(hr == E_FAIL, "Write for an unknown property did not fail with expected code, hr=%lx\n", hr);

    /* 2. One property without correct type */
    options[0].pstrName = (LPOLESTR)wszTestProperty1;
    V_VT(&values[0]) = VT_UI1;
    V_UNION(&values[0], bVal) = 1;
    hr = IPropertyBag2_Write(property, 1, options, values);
    ok(hr == S_OK, "Write for one property failed, hr=%lx\n", hr);

    /* 3. One property with mismatching type */
    options[0].pstrName = (LPOLESTR)wszTestProperty1;
    V_VT(&values[0]) = VT_I1;
    V_UNION(&values[0], bVal) = 2;
    hr = IPropertyBag2_Write(property, 1, options, values);
    ok(hr == WINCODEC_ERR_PROPERTYUNEXPECTEDTYPE,
       "Write with mismatching type did not fail with expected code hr=%lx\n", hr);

    /* 4. Reset one property to empty */
    options[0].pstrName = (LPOLESTR)wszTestProperty1;
    VariantClear(&values[0]);
    hr = IPropertyBag2_Write(property, 1, options, values);
    ok(hr == WINCODEC_ERR_PROPERTYUNEXPECTEDTYPE,
       "Write to reset to empty value does not fail with expected code, hr=%lx\n", hr);

    /* 5. Set two properties */
    options[0].pstrName = (LPOLESTR)wszTestProperty1;
    V_VT(&values[0]) = VT_UI1;
    V_UNION(&values[0], bVal) = 12;
    options[1].pstrName = (LPOLESTR)wszTestProperty2;
    V_VT(&values[1]) = VT_R4;
    V_UNION(&values[1], fltVal) = (float)3.14;
    hr = IPropertyBag2_Write(property, 2, options, values);
    ok(hr == S_OK, "Write for two properties failed, hr=%lx\n", hr);
}

static void test_empty_propertybag(void)
{
    HRESULT hr;
    IWICComponentFactory *factory;
    IPropertyBag2 *property;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICComponentFactory, (void**)&factory);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%lx\n", hr);

    hr = IWICComponentFactory_CreateEncoderPropertyBag(factory, NULL, 0, &property);

    ok(hr == S_OK, "Creating EncoderPropertyBag failed, hr=%lx\n", hr);
    if (FAILED(hr)) return;

    test_propertybag_countproperties(property, 0);

    test_propertybag_getpropertyinfo(property, 0);

    IPropertyBag2_Release(property);

    IWICComponentFactory_Release(factory);
}

static void test_filled_propertybag(void)
{
    HRESULT hr;
    IWICComponentFactory *factory;
    IPropertyBag2 *property;
    PROPBAG2 opts[2]= {
        {PROPBAG2_TYPE_DATA, VT_UI1, 0, 0, (LPOLESTR)wszTestProperty1, {0}},
        {PROPBAG2_TYPE_DATA, VT_R4, 0, 0, (LPOLESTR)wszTestProperty2, {0}}
    };

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICComponentFactory, (void**)&factory);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%lx\n", hr);

    hr = IWICComponentFactory_CreateEncoderPropertyBag(factory, opts, 2, &property);

    ok(hr == S_OK, "Creating EncoderPropertyBag failed, hr=%lx\n", hr);
    if (FAILED(hr)) return;

    test_propertybag_countproperties(property, 2);

    test_propertybag_getpropertyinfo(property, 2);

    test_propertybag_write(property);

    test_propertybag_read(property);

    IPropertyBag2_Release(property);

    IWICComponentFactory_Release(factory);
}

START_TEST(propertybag)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_empty_propertybag();

    test_filled_propertybag();

    CoUninitialize();
}
