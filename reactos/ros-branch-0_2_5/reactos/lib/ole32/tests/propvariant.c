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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "windows.h"

HRESULT WINAPI PropVariantCopy(PROPVARIANT*,const PROPVARIANT*);
HRESULT WINAPI PropVariantClear(PROPVARIANT*);

#include "wine/test.h"

#ifdef NONAMELESSUNION
# define U(x) (x).u
#else
# define U(x) (x)
#endif

struct valid_mapping
{
    BOOL simple;
    BOOL with_array;
    BOOL with_vector;
    BOOL byref;
} valid_types[] =
{
    { TRUE , FALSE, FALSE, FALSE }, /* VT_EMPTY */
    { TRUE , FALSE, FALSE, FALSE }, /* VT_NULL */
    { TRUE , FALSE, TRUE , FALSE }, /* VT_I2 */
    { TRUE , FALSE, TRUE , FALSE }, /* VT_I4 */
    { TRUE , FALSE, TRUE , FALSE }, /* VT_R4 */
    { TRUE , FALSE, TRUE , FALSE }, /* VT_R8 */
    { TRUE , FALSE, TRUE , FALSE }, /* VT_CY */
    { TRUE , FALSE, TRUE , FALSE }, /* VT_DATE */
    { TRUE , FALSE, TRUE , FALSE }, /* VT_BSTR */
    { FALSE, FALSE, FALSE, FALSE }, /* VT_DISPATCH */
    { TRUE , FALSE, TRUE , FALSE }, /* VT_ERROR */
    { TRUE , FALSE, TRUE , FALSE }, /* VT_BOOL */
    { FALSE, FALSE, TRUE , FALSE }, /* VT_VARIANT */
    { FALSE, FALSE, FALSE, FALSE }, /* VT_UNKNOWN */
    { FALSE, FALSE, FALSE, FALSE }, /* VT_DECIMAL */
    { FALSE, FALSE, FALSE, FALSE }, /* 15 */
    { FALSE, FALSE, FALSE, FALSE }, /* VT_I1 */
    { TRUE , FALSE, TRUE , FALSE }, /* VT_UI1 */
    { TRUE , FALSE, TRUE , FALSE }, /* VT_UI2 */
    { TRUE , FALSE, TRUE , FALSE }, /* VT_UI4 */
    { TRUE , FALSE, TRUE , FALSE }, /* VT_I8 */
    { TRUE , FALSE, TRUE , FALSE }, /* VT_UI8 */
    { FALSE, FALSE, FALSE, FALSE }, /* VT_INT */
    { FALSE, FALSE, FALSE, FALSE }, /* VT_UINT */
    { FALSE, FALSE, FALSE, FALSE }, /* VT_VOID */
    { FALSE, FALSE, FALSE, FALSE }, /* VT_HRESULT */
    { FALSE, FALSE, FALSE, FALSE }, /* VT_PTR */
    { FALSE, FALSE, FALSE, FALSE }, /* VT_SAFEARRAY */
    { FALSE, FALSE, FALSE, FALSE }, /* VT_CARRAY */
    { FALSE, FALSE, FALSE, FALSE }, /* VT_USERDEFINED */
    { TRUE , FALSE, TRUE , FALSE }, /* VT_LPSTR */
    { TRUE , FALSE, TRUE , FALSE }, /* VT_LPWSTR */
    { FALSE, FALSE, FALSE, FALSE }, /* 32 */
    { FALSE, FALSE, FALSE, FALSE }, /* 33 */
    { FALSE, FALSE, FALSE, FALSE }, /* 34 */
    { FALSE, FALSE, FALSE, FALSE }, /* 35 */
    { FALSE, FALSE, FALSE, FALSE }, /* VT_RECORD */
    { FALSE, FALSE, FALSE, FALSE }, /* VT_INT_PTR */
    { FALSE, FALSE, FALSE, FALSE }, /* VT_UINT_PTR */
    { FALSE, FALSE, FALSE, FALSE }, /* 39 */
    { FALSE, FALSE, FALSE, FALSE }, /* 40 */
    { FALSE, FALSE, FALSE, FALSE }, /* 41 */
    { FALSE, FALSE, FALSE, FALSE }, /* 42 */
    { FALSE, FALSE, FALSE, FALSE }, /* 43 */
    { FALSE, FALSE, FALSE, FALSE }, /* 44 */
    { FALSE, FALSE, FALSE, FALSE }, /* 45 */
    { FALSE, FALSE, FALSE, FALSE }, /* 46 */
    { FALSE, FALSE, FALSE, FALSE }, /* 47 */
    { FALSE, FALSE, FALSE, FALSE }, /* 48 */
    { FALSE, FALSE, FALSE, FALSE }, /* 49 */
    { FALSE, FALSE, FALSE, FALSE }, /* 50 */
    { FALSE, FALSE, FALSE, FALSE }, /* 51 */
    { FALSE, FALSE, FALSE, FALSE }, /* 52 */
    { FALSE, FALSE, FALSE, FALSE }, /* 53 */
    { FALSE, FALSE, FALSE, FALSE }, /* 54 */
    { FALSE, FALSE, FALSE, FALSE }, /* 55 */
    { FALSE, FALSE, FALSE, FALSE }, /* 56 */
    { FALSE, FALSE, FALSE, FALSE }, /* 57 */
    { FALSE, FALSE, FALSE, FALSE }, /* 58 */
    { FALSE, FALSE, FALSE, FALSE }, /* 59 */
    { FALSE, FALSE, FALSE, FALSE }, /* 60 */
    { FALSE, FALSE, FALSE, FALSE }, /* 61 */
    { FALSE, FALSE, FALSE, FALSE }, /* 62 */
    { FALSE, FALSE, FALSE, FALSE }, /* 63 */
    { TRUE , FALSE, TRUE , FALSE }, /* VT_FILETIME */
    { TRUE , FALSE, FALSE, FALSE }, /* VT_BLOB */
    { TRUE , FALSE, FALSE, FALSE }, /* VT_STREAM */
    { TRUE , FALSE, FALSE, FALSE }, /* VT_STORAGE */
    { TRUE , FALSE, FALSE, FALSE }, /* VT_STREAMED_OBJECT */
    { TRUE , FALSE, FALSE, FALSE }, /* VT_STORED_OBJECT */
    { TRUE , FALSE, FALSE, FALSE }, /* VT_BLOB_OBJECT */
    { TRUE , FALSE, TRUE , FALSE }  /* VT_CF */
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

static void test_validtypes()
{
    PROPVARIANT propvar;
    HRESULT hr;
    unsigned int i;

    memset(&propvar, 0, sizeof(propvar));

    for (i = 0; i < sizeof(valid_types)/sizeof(valid_types[0]); i++)
    {
        propvar.vt = i;
        hr = PropVariantClear(&propvar);
        ok(valid_types[i].simple == !(hr == STG_E_INVALIDPARAMETER),
            "PropVariantClear(%s) should have returned 0x%08lx, but returned 0x%08lx\n",
            wine_vtypes[i],
            valid_types[i].simple ? S_OK : STG_E_INVALIDPARAMETER, hr);

        propvar.vt = i | VT_ARRAY;
        hr = PropVariantClear(&propvar);
        ok(valid_types[i].with_array == !(hr == STG_E_INVALIDPARAMETER),
            "PropVariantClear(%s|VT_ARRAY) should have returned 0x%08lx, but returned 0x%08lx\n",
            wine_vtypes[i],
            valid_types[i].with_array ? S_OK : STG_E_INVALIDPARAMETER, hr);

        propvar.vt = i | VT_VECTOR;
        hr = PropVariantClear(&propvar);
        ok(valid_types[i].with_vector == !(hr == STG_E_INVALIDPARAMETER),
            "PropVariantClear(%s|VT_VECTOR) should have returned 0x%08lx, but returned 0x%08lx\n",
            wine_vtypes[i],
            valid_types[i].with_vector ? S_OK : STG_E_INVALIDPARAMETER, hr);

        propvar.vt = i | VT_BYREF;
        hr = PropVariantClear(&propvar);
        ok(valid_types[i].byref == !(hr == STG_E_INVALIDPARAMETER),
            "PropVariantClear(%s|VT_BYREF) should have returned 0x%08lx, but returned 0x%08lx\n",
            wine_vtypes[i],
            valid_types[i].byref ? S_OK : STG_E_INVALIDPARAMETER, hr);
    }
}

static void test_copy()
{
    static const char szTestString[] = "Test String";
    static const WCHAR wszTestString[] = {'T','e','s','t',' ','S','t','r','i','n','g',0};
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
    U(propvarSrc).pwszVal = (LPWSTR)wszTestString;
    hr = PropVariantCopy(&propvarDst, &propvarSrc);
    ok(hr == S_OK, "PropVariantCopy(...VT_LPWSTR...) failed\n");
    ok(!lstrcmpW(U(propvarSrc).pwszVal, U(propvarDst).pwszVal), "Wide string not copied properly\n");
    hr = PropVariantClear(&propvarDst);
    ok(hr == S_OK, "PropVariantClear(...VT_LPWSTR...) failed\n");
    memset(&propvarSrc, 0, sizeof(propvarSrc));

    propvarSrc.vt = VT_LPSTR;
    U(propvarSrc).pszVal = (LPSTR)szTestString;
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
