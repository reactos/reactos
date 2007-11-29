/*
 * PropVariant Tests
 *
 * Copyright 2004 Robert Shearman
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

#include "windows.h"

#include "wine/test.h"

/* invalid in all versions */
#define PROP_INV 255
/* valid in v0 and above (NT4+) */
#define PROP_V0  0
/* valid in v1 and above (Win2k+) */
#define PROP_V1  1
/* valid in v1a and above (WinXP+) */
#define PROP_V1A 2

struct valid_mapping
{
    BYTE simple;
    BYTE with_array;
    BYTE with_vector;
    BYTE byref;
} valid_types[] =
{
    { PROP_V0 , PROP_INV, PROP_INV, PROP_INV }, /* VT_EMPTY */
    { PROP_V0 , PROP_INV, PROP_INV, PROP_INV }, /* VT_NULL */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1  }, /* VT_I2 */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1  }, /* VT_I4 */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1  }, /* VT_R4 */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1  }, /* VT_R8 */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1  }, /* VT_CY */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1  }, /* VT_DATE */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1  }, /* VT_BSTR */
    { PROP_V1 , PROP_V1 , PROP_INV, PROP_V1  }, /* VT_DISPATCH */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1  }, /* VT_ERROR */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1  }, /* VT_BOOL */
    { PROP_V1 , PROP_V1 , PROP_V0 , PROP_V1  }, /* VT_VARIANT */
    { PROP_V1 , PROP_V1 , PROP_INV, PROP_V1  }, /* VT_UNKNOWN */
    { PROP_V1 , PROP_V1 , PROP_INV, PROP_V1  }, /* VT_DECIMAL */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 15 */
    { PROP_V1 , PROP_V1 , PROP_V1 , PROP_V1  }, /* VT_I1 */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1  }, /* VT_UI1 */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1  }, /* VT_UI2 */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1  }, /* VT_UI4 */
    { PROP_V0 , PROP_V1A, PROP_V0 , PROP_V1A }, /* VT_I8 */
    { PROP_V0 , PROP_V1A, PROP_V0 , PROP_V1A }, /* VT_UI8 */
    { PROP_V1 , PROP_V1 , PROP_INV, PROP_V1  }, /* VT_INT */
    { PROP_V1 , PROP_V1 , PROP_INV, PROP_V1  }, /* VT_UINT */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* VT_VOID */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* VT_HRESULT */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* VT_PTR */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* VT_SAFEARRAY */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* VT_CARRAY */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* VT_USERDEFINED */
    { PROP_V0 , PROP_INV, PROP_V0 , PROP_INV }, /* VT_LPSTR */
    { PROP_V0 , PROP_INV, PROP_V0 , PROP_INV }, /* VT_LPWSTR */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 32 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 33 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 34 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 35 */
    { PROP_V1 , PROP_V1 , PROP_INV, PROP_V1  }, /* VT_RECORD */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* VT_INT_PTR */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* VT_UINT_PTR */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 39 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 40 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 41 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 42 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 43 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 44 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 45 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 46 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 47 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 48 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 49 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 50 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 51 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 52 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 53 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 54 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 55 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 56 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 57 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 58 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 59 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 60 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 61 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 62 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 63 */
    { PROP_V0 , PROP_INV, PROP_V0 , PROP_INV }, /* VT_FILETIME */
    { PROP_V0 , PROP_INV, PROP_INV, PROP_INV }, /* VT_BLOB */
    { PROP_V0 , PROP_INV, PROP_INV, PROP_INV }, /* VT_STREAM */
    { PROP_V0 , PROP_INV, PROP_INV, PROP_INV }, /* VT_STORAGE */
    { PROP_V0 , PROP_INV, PROP_INV, PROP_INV }, /* VT_STREAMED_OBJECT */
    { PROP_V0 , PROP_INV, PROP_INV, PROP_INV }, /* VT_STORED_OBJECT */
    { PROP_V0 , PROP_INV, PROP_INV, PROP_INV }, /* VT_BLOB_OBJECT */
    { PROP_V0 , PROP_INV, PROP_V0 , PROP_INV }  /* VT_CF */
};

static const char* wine_vtypes[VT_CLSID+1] =
{
  "VT_EMPTY","VT_NULL","VT_I2","VT_I4","VT_R4","VT_R8","VT_CY","VT_DATE",
  "VT_BSTR","VT_DISPATCH","VT_ERROR","VT_BOOL","VT_VARIANT","VT_UNKNOWN",
  "VT_DECIMAL","15","VT_I1","VT_UI1","VT_UI2","VT_UI4","VT_I8","VT_UI8",
  "VT_INT","VT_UINT","VT_VOID","VT_HRESULT","VT_PTR","VT_SAFEARRAY",
  "VT_CARRAY","VT_USERDEFINED","VT_LPSTR","VT_LPWSTR","32","33","34","35",
  "VT_RECORD","VT_INT_PTR","VT_UINT_PTR","39","40","41","42","43","44","45",
  "46","47","48","49","50","51","52","53","54","55","56","57","58","59","60",
  "61","62","63","VT_FILETIME","VT_BLOB","VT_STREAM","VT_STORAGE",
  "VT_STREAMED_OBJECT","VT_STORED_OBJECT","VT_BLOB_OBJECT","VT_CF","VT_CLSID"
};

static void test_validtypes(void)
{
    PROPVARIANT propvar;
    HRESULT hr;
    unsigned int i;
    BYTE version;

    memset(&propvar, 0, sizeof(propvar));

    /* detect version */
    propvar.vt = VT_I2 | VT_ARRAY;
    hr = PropVariantClear(&propvar);
    if (hr == S_OK)
    {
        propvar.vt = VT_I8 | VT_ARRAY;
        hr = PropVariantClear(&propvar);
        if (hr == S_OK)
        {
            version = PROP_V1A;
            trace("detected prop variant v1a\n");
        }
        else
        {
            version = PROP_V1;
            trace("detected prop variant v1\n");
        }
    }
    else
    {
        version = PROP_V0;
        trace("detected prop variant v0\n");
    }

    for (i = 0; i < sizeof(valid_types)/sizeof(valid_types[0]); i++)
    {
        BOOL expected_result;

        propvar.vt = i;
        hr = PropVariantClear(&propvar);
        expected_result = (valid_types[i].simple <= version ? TRUE : FALSE);
        ok(expected_result == !(hr == STG_E_INVALIDPARAMETER),
            "PropVariantClear(%s) should have returned 0x%08x, but returned 0x%08x\n",
            wine_vtypes[i],
            expected_result ? S_OK : STG_E_INVALIDPARAMETER, hr);

        propvar.vt = i | VT_ARRAY;
        hr = PropVariantClear(&propvar);
        expected_result = (valid_types[i].with_array <= version ? TRUE : FALSE);
        ok(expected_result == !(hr == STG_E_INVALIDPARAMETER),
            "PropVariantClear(%s|VT_ARRAY) should have returned 0x%08x, but returned 0x%08x\n",
            wine_vtypes[i],
            expected_result ? S_OK : STG_E_INVALIDPARAMETER, hr);

        propvar.vt = i | VT_VECTOR;
        hr = PropVariantClear(&propvar);
        expected_result = (valid_types[i].with_vector <= version ? TRUE : FALSE);
        ok(expected_result == !(hr == STG_E_INVALIDPARAMETER),
            "PropVariantClear(%s|VT_VECTOR) should have returned 0x%08x, but returned 0x%08x\n",
            wine_vtypes[i],
            expected_result ? S_OK : STG_E_INVALIDPARAMETER, hr);

        propvar.vt = i | VT_BYREF;
        hr = PropVariantClear(&propvar);
        expected_result = (valid_types[i].byref <= version ? TRUE : FALSE);
        ok(expected_result == !(hr == STG_E_INVALIDPARAMETER),
            "PropVariantClear(%s|VT_BYREF) should have returned 0x%08x, but returned 0x%08x\n",
            wine_vtypes[i],
            expected_result ? S_OK : STG_E_INVALIDPARAMETER, hr);
    }
}

static void test_copy(void)
{
    static char szTestString[] = "Test String";
    static WCHAR wszTestString[] = {'T','e','s','t',' ','S','t','r','i','n','g',0};
    PROPVARIANT propvarSrc;
    PROPVARIANT propvarDst;
    HRESULT hr;

    propvarSrc.vt = VT_BSTR;
    U(propvarSrc).bstrVal = SysAllocString(wszTestString);

    hr = PropVariantCopy(&propvarDst, &propvarSrc);
    ok(hr == S_OK, "PropVariantCopy(...VT_BSTR...) failed\n");
    ok(!lstrcmpW(U(propvarSrc).bstrVal, U(propvarDst).bstrVal), "BSTR not copied properly\n");
    hr = PropVariantClear(&propvarSrc);
    ok(hr == S_OK, "PropVariantClear(...VT_BSTR...) failed\n");
    hr = PropVariantClear(&propvarDst);
    ok(hr == S_OK, "PropVariantClear(...VT_BSTR...) failed\n");

    propvarSrc.vt = VT_LPWSTR;
    U(propvarSrc).pwszVal = wszTestString;
    hr = PropVariantCopy(&propvarDst, &propvarSrc);
    ok(hr == S_OK, "PropVariantCopy(...VT_LPWSTR...) failed\n");
    ok(!lstrcmpW(U(propvarSrc).pwszVal, U(propvarDst).pwszVal), "Wide string not copied properly\n");
    hr = PropVariantClear(&propvarDst);
    ok(hr == S_OK, "PropVariantClear(...VT_LPWSTR...) failed\n");
    memset(&propvarSrc, 0, sizeof(propvarSrc));

    propvarSrc.vt = VT_LPSTR;
    U(propvarSrc).pszVal = szTestString;
    hr = PropVariantCopy(&propvarDst, &propvarSrc);
    ok(hr == S_OK, "PropVariantCopy(...VT_LPSTR...) failed\n");
    ok(!strcmp(U(propvarSrc).pszVal, U(propvarDst).pszVal), "String not copied properly\n");
    hr = PropVariantClear(&propvarDst);
    ok(hr == S_OK, "PropVariantClear(...VT_LPSTR...) failed\n");
    memset(&propvarSrc, 0, sizeof(propvarSrc));
}

START_TEST(propvariant)
{
    test_validtypes();
    test_copy();
}
