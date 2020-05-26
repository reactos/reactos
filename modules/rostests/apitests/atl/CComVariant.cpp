/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CComVariant
 * PROGRAMMER:      Mark Jansen
 */

/* In case we are building against the MS headers, we need to disable assertions. */
#undef ATLASSERT
#define ATLASSERT(x)
#define _ATL_NO_VARIANT_THROW

#include <apitest.h>
#include <atlbase.h>
#include <versionhelpers.h>

void expect_bool_imp(const CComVariant& ccv, bool value)
{
    winetest_ok(V_VT(&ccv) == VT_BOOL, "Expected .vt to be BOOL, was %u\n", V_VT(&ccv));
    VARIANT_BOOL expected = (value ? VARIANT_TRUE : VARIANT_FALSE);
    winetest_ok(V_BOOL(&ccv) == expected, "Expected value to be %u, was: %u\n", expected, V_BOOL(&ccv));
}

void expect_int_imp(const CComVariant& ccv, int value, unsigned short type)
{
    winetest_ok(V_VT(&ccv) == type, "Expected .vt to be %u, was %u\n", type, V_VT(&ccv));
    winetest_ok(V_I4(&ccv) == value, "Expected value to be %d, was: %ld\n", value, V_I4(&ccv));
}

void expect_uint_imp(const CComVariant& ccv, unsigned int value, unsigned short type)
{
    winetest_ok(V_VT(&ccv) == type, "Expected .vt to be %u, was %u\n", type, V_VT(&ccv));
    winetest_ok(V_UI4(&ccv) == value, "Expected value to be %u, was: %lu\n", value, V_UI4(&ccv));
}

void expect_double_imp(const CComVariant& ccv, double value, unsigned short type)
{
    winetest_ok(V_VT(&ccv) == type, "Expected .vt to be %u, was %u\n", type, V_VT(&ccv));
    winetest_ok(V_R8(&ccv) == value, "Expected value to be %f, was: %f\n", value, V_R8(&ccv));
}

void expect_error_imp(const CComVariant& ccv, SCODE value)
{
    winetest_ok(V_VT(&ccv) == VT_ERROR, "Expected .vt to be VT_ERROR, was %u\n", V_VT(&ccv));
    winetest_ok(V_ERROR(&ccv) == value, "Expected value to be %lx, was: %lx\n", value, V_ERROR(&ccv));
}

void expect_empty_imp(const CComVariant& ccv)
{
    winetest_ok(V_VT(&ccv) == VT_EMPTY, "Expected .vt to be VT_EMPTY, was %u\n", V_VT(&ccv));
    if (IsWindows8OrGreater())
        winetest_ok(V_I8(&ccv) == 0ll, "Expected value to be 0, was: %I64d\n", V_I8(&ccv));
}

#define expect_bool         (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : expect_bool_imp
#define expect_int          (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : expect_int_imp
#define expect_uint         (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : expect_uint_imp
#define expect_double       (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : expect_double_imp
#define expect_error        (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : expect_error_imp
#define expect_empty        (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : expect_empty_imp


static void test_construction()
{
    {
        CComVariant empty;
        expect_empty(empty);
    }
    {
        CComBSTR bstr(L"TESTW");
        CComVariant olestr((LPCOLESTR)bstr), comstr(bstr);
        ok(V_VT(&olestr) == VT_BSTR, "Expected .vt to be VT_LPWSTR, was %u\n", V_VT(&olestr));
        ok(!wcscmp(V_BSTR(&olestr), L"TESTW"), "Expected value to be L\"TESTW\", was: %s\n", wine_dbgstr_w(V_BSTR(&olestr)));
        ok(V_VT(&comstr) == VT_BSTR, "Expected .vt to be VT_LPWSTR, was %u\n", V_VT(&comstr));
        ok(!wcscmp(V_BSTR(&comstr), L"TESTW"), "Expected value to be L\"TESTW\", was: %s\n", wine_dbgstr_w(V_BSTR(&comstr)));
    }
    {
        CComVariant cstr((LPCSTR)"TESTA");
        ok(V_VT(&cstr) == VT_BSTR, "Expected .vt to be VT_LPWSTR, was %u\n", V_VT(&cstr));
        ok(!wcscmp(V_BSTR(&cstr), L"TESTA"), "Expected value to be L\"TESTW\", was: %s\n", wine_dbgstr_w(V_BSTR(&cstr)));
    }
    {
        CComVariant trueVal(true), falseVal(false);
        ok(V_VT(&trueVal) == VT_BOOL, "Expected .vt to be BOOL, was %u\n", V_VT(&trueVal));
        ok(V_BOOL(&trueVal) == VARIANT_TRUE, "Expected value to be VARIANT_TRUE, was: %u\n", V_BOOL(&trueVal));
        ok(V_VT(&falseVal) == VT_BOOL, "Expected .vt to be BOOL, was %u\n", V_VT(&falseVal));
        ok(V_BOOL(&falseVal) == VARIANT_FALSE, "Expected value to be VARIANT_TRUE, was: %u\n", V_BOOL(&falseVal));
    }
    {
        CComVariant b1((BYTE)33);
        ok(V_VT(&b1) == VT_UI1, "Expected .vt to be VT_UI1, was %u\n", V_VT(&b1));
        ok(V_UI1(&b1) == (BYTE)33, "Expected value to be 33, was: %u\n", V_UI1(&b1));
    }
    {
        CComVariant c1((char)33);
        ok(V_VT(&c1) == VT_I1, "Expected .vt to be VT_I1, was %u\n", V_VT(&c1));
        ok(V_I1(&c1) == (char)33, "Expected value to be 33, was: %d\n", V_I1(&c1));
    }
    {
        CComVariant s1((short)12345);
        ok(V_VT(&s1) == VT_I2, "Expected .vt to be VT_I2, was %u\n", V_VT(&s1));
        ok(V_I2(&s1) == (short)12345, "Expected value to be 12345, was: %d\n", V_I1(&s1));
    }
    {
        CComVariant us1((unsigned short)12345);
        ok(V_VT(&us1) == VT_UI2, "Expected .vt to be VT_UI2, was %u\n", V_VT(&us1));
        ok(V_UI2(&us1) == (unsigned short)12345, "Expected value to be 12345, was: %u\n", V_UI2(&us1));
    }
    {
        CComVariant i1((int)4, VT_I4), i2((int)3, VT_INT), i3((int)2, VT_I2), i4((int)1);
        expect_int(i1, 4, VT_I4);
        expect_int(i2, 3, VT_INT);
        expect_error(i3, E_INVALIDARG);
        expect_int(i4, 1, VT_I4);
    }
    {
        CComVariant ui1((unsigned int)4, VT_UI4), ui2((unsigned int)3, VT_UINT), ui3((unsigned int)2, VT_UI2), ui4((unsigned int)1);
        expect_uint(ui1, 4, VT_UI4);
        expect_uint(ui2, 3, VT_UINT);
        expect_error(ui3, E_INVALIDARG);
        expect_uint(ui4, 1, VT_UI4);
    }
    {
        CComVariant l1((long)4, VT_I4), l2((long)3, VT_INT), l3((long)2, VT_ERROR), l4((long)1);
        expect_int(l1, 4, VT_I4);
        expect_error(l2, E_INVALIDARG);
        expect_error(l3, 2);
        expect_int(l4, 1, VT_I4);
    }
    {
        CComVariant ul1((unsigned long)33);
        expect_uint(ul1, 33, VT_UI4);
    }
    {
        CComVariant f1(3.4f);
        ok(V_VT(&f1) == VT_R4, "Expected .vt to be VT_R4, was %u\n", V_VT(&f1));
        ok(V_R4(&f1) == 3.4f, "Expected value to be 3.4f, was: %f\n", V_R4(&f1));
    }
    {
        CComVariant d1(3.4, VT_R8), d2(3.4, VT_DATE), d3(8.8, VT_I1), d4(1.9);
        expect_double(d1, 3.4, VT_R8);
        expect_double(d2, 3.4, VT_DATE);
        expect_error(d3, E_INVALIDARG);
        expect_double(d4, 1.9, VT_R8);
    }
    {
        LONGLONG lv = 12030912309123ll;
        CComVariant l1(lv);
        ok(V_VT(&l1) == VT_I8, "Expected .vt to be VT_I8, was %u\n", V_VT(&l1));
        ok(V_I8(&l1) == lv, "Expected value to be %s, was: %s\n", wine_dbgstr_longlong(lv), wine_dbgstr_longlong(V_I8(&l1)));
    }
    {
        ULONGLONG lv = 12030912309123ull;
        CComVariant l1(lv);
        ok(V_VT(&l1) == VT_UI8, "Expected .vt to be VT_UI8, was %u\n", V_VT(&l1));
        ok(V_UI8(&l1) == lv, "Expected value to be %s, was: %s\n", wine_dbgstr_longlong(lv), wine_dbgstr_longlong(V_UI8(&l1)));
    }
    {
        CY cy;
        cy.int64 = 12030912309123ll;
        CComVariant c1(cy);
        ok(V_VT(&c1) == VT_CY, "Expected .vt to be VT_CY, was %u\n", V_VT(&c1));
        ok(V_CY(&c1).int64 == cy.int64, "Expected value to be %s, was: %s\n", wine_dbgstr_longlong(cy.int64), wine_dbgstr_longlong(V_CY(&c1).int64));
    }
    // IDispatch
    // IUnknown
}


static void test_copyconstructor()
{
    {
        CComVariant empty;
        CComVariant empty2(empty);
        expect_empty(empty2);
    }
    {
        CComBSTR bstr(L"TESTW");
        CComVariant olestr((LPCOLESTR)bstr);
        CComVariant olestr2(olestr);
        ok(V_VT(&olestr2) == VT_BSTR, "Expected .vt to be VT_LPWSTR, was %u\n", V_VT(&olestr2));
        ok(!wcscmp(V_BSTR(&olestr2), L"TESTW"), "Expected value to be L\"TESTW\", was: %s\n", wine_dbgstr_w(V_BSTR(&olestr2)));
    }
    {
        CComVariant trueVal(true);
        CComVariant trueVal2(trueVal);
        ok(V_VT(&trueVal2) == VT_BOOL, "Expected .vt to be BOOL, was %u\n", V_VT(&trueVal2));
        ok(V_BOOL(&trueVal2) == VARIANT_TRUE, "Expected value to be VARIANT_TRUE, was: %u\n", V_BOOL(&trueVal2));
    }
    {
        CComVariant b1((BYTE)33);
        CComVariant b2(b1);
        ok(V_VT(&b2) == VT_UI1, "Expected .vt to be VT_UI1, was %u\n", V_VT(&b2));
        ok(V_UI1(&b2) == (BYTE)33, "Expected value to be 33, was: %u\n", V_UI1(&b2));
    }
    {
        CComVariant c1((char)33);
        CComVariant c2(c1);
        ok(V_VT(&c2) == VT_I1, "Expected .vt to be VT_I1, was %u\n", V_VT(&c2));
        ok(V_I1(&c2) == (char)33, "Expected value to be 33, was: %d\n", V_I1(&c2));
    }
    {
        CComVariant s1((short)12345);
        CComVariant s2(s1);
        ok(V_VT(&s2) == VT_I2, "Expected .vt to be VT_I2, was %u\n", V_VT(&s2));
        ok(V_I2(&s2) == (short)12345, "Expected value to be 12345, was: %d\n", V_I1(&s2));
    }
    {
        CComVariant us1((unsigned short)12345);
        CComVariant us2(us1);
        ok(V_VT(&us2) == VT_UI2, "Expected .vt to be VT_UI2, was %u\n", V_VT(&us2));
        ok(V_UI2(&us2) == (unsigned short)12345, "Expected value to be 12345, was: %u\n", V_UI2(&us2));
    }
    {
        CComVariant i1((int)4, VT_I4);
        CComVariant i2(i1);
        expect_int(i2, 4, VT_I4);
    }
    {
        CComVariant ui1((unsigned int)4, VT_UI4);
        CComVariant ui2(ui1);
        expect_uint(ui2, 4, VT_UI4);
    }
    {
        CComVariant l1((long)4, VT_I4);
        CComVariant l2(l1);
        expect_uint(l2, 4, VT_I4);
    }
    {
        CComVariant ul1((unsigned long)33);
        CComVariant ul2(ul1);
        expect_uint(ul2, 33, VT_UI4);
    }
    {
        CComVariant f1(3.4f);
        CComVariant f2(f1);
        ok(V_VT(&f2) == VT_R4, "Expected .vt to be VT_R4, was %u\n", V_VT(&f2));
        ok(V_R4(&f2) == 3.4f, "Expected value to be 3.4f, was: %f\n", V_R4(&f2));
    }
    {
        CComVariant d1(3.4, VT_R8);
        CComVariant d2(d1);
        expect_double(d2, 3.4, VT_R8);
    }
    {
        LONGLONG lv = 12030912309123ll;
        CComVariant l1(lv);
        CComVariant l2(l1);
        ok(V_VT(&l2) == VT_I8, "Expected .vt to be VT_I8, was %u\n", V_VT(&l2));
        ok(V_I8(&l2) == lv, "Expected value to be %s, was: %s\n", wine_dbgstr_longlong(lv), wine_dbgstr_longlong(V_I8(&l2)));
    }
    {
        ULONGLONG lv = 12030912309123ull;
        CComVariant l1(lv);
        CComVariant l2(l1);
        ok(V_VT(&l2) == VT_UI8, "Expected .vt to be VT_UI8, was %u\n", V_VT(&l2));
        ok(V_UI8(&l2) == lv, "Expected value to be %s, was: %s\n", wine_dbgstr_longlong(lv), wine_dbgstr_longlong(V_UI8(&l2)));
    }
    {
        CY cy;
        cy.int64 = 12030912309123ll;
        CComVariant c1(cy);
        CComVariant c2(c1);
        ok(V_VT(&c2) == VT_CY, "Expected .vt to be VT_I8, was %u\n", V_VT(&c2));
        ok(V_CY(&c2).int64 == cy.int64, "Expected value to be %s, was: %s\n", wine_dbgstr_longlong(cy.int64), wine_dbgstr_longlong(V_CY(&c2).int64));
    }
    // IDispatch
    // IUnknown
}

static void test_assign()
{
    {
        CComVariant empty;
        CComVariant empty2 = empty;
        expect_empty(empty2);
    }
    {
        CComBSTR bstr(L"TESTW");
        CComVariant olestr = (LPCOLESTR)bstr;
        ok(V_VT(&olestr) == VT_BSTR, "Expected .vt to be VT_LPWSTR, was %u\n", V_VT(&olestr));
        ok(!wcscmp(V_BSTR(&olestr), L"TESTW"), "Expected value to be L\"TESTW\", was: %s\n", wine_dbgstr_w(V_BSTR(&olestr)));
        CComVariant olestr2 = olestr;
        ok(V_VT(&olestr2) == VT_BSTR, "Expected .vt to be VT_LPWSTR, was %u\n", V_VT(&olestr2));
        ok(!wcscmp(V_BSTR(&olestr2), L"TESTW"), "Expected value to be L\"TESTW\", was: %s\n", wine_dbgstr_w(V_BSTR(&olestr2)));
    }
    {
        CComVariant trueVal = true;
        ok(V_VT(&trueVal) == VT_BOOL, "Expected .vt to be BOOL, was %u\n", V_VT(&trueVal));
        ok(V_BOOL(&trueVal) == VARIANT_TRUE, "Expected value to be VARIANT_TRUE, was: %u\n", V_BOOL(&trueVal));
        CComVariant trueVal2 = trueVal;
        ok(V_VT(&trueVal2) == VT_BOOL, "Expected .vt to be BOOL, was %u\n", V_VT(&trueVal2));
        ok(V_BOOL(&trueVal2) == VARIANT_TRUE, "Expected value to be VARIANT_TRUE, was: %u\n", V_BOOL(&trueVal2));
    }
    {
        CComVariant b1 = (BYTE)33;
        ok(V_VT(&b1) == VT_UI1, "Expected .vt to be VT_UI1, was %u\n", V_VT(&b1));
        ok(V_UI1(&b1) == (BYTE)33, "Expected value to be 33, was: %u\n", V_UI1(&b1));
        CComVariant b2 = b1;
        ok(V_VT(&b2) == VT_UI1, "Expected .vt to be VT_UI1, was %u\n", V_VT(&b2));
        ok(V_UI1(&b2) == (BYTE)33, "Expected value to be 33, was: %u\n", V_UI1(&b2));
    }
    {
        CComVariant c1 = (char)33;
        ok(V_VT(&c1) == VT_I1, "Expected .vt to be VT_I1, was %u\n", V_VT(&c1));
        ok(V_I1(&c1) == (char)33, "Expected value to be 33, was: %d\n", V_I1(&c1));
        CComVariant c2 = c1;
        ok(V_VT(&c2) == VT_I1, "Expected .vt to be VT_I1, was %u\n", V_VT(&c2));
        ok(V_I1(&c2) == (char)33, "Expected value to be 33, was: %d\n", V_I1(&c2));
    }
    {
        CComVariant s1 = (short)12345;
        ok(V_VT(&s1) == VT_I2, "Expected .vt to be VT_I2, was %u\n", V_VT(&s1));
        ok(V_I2(&s1) == (short)12345, "Expected value to be 12345, was: %d\n", V_I1(&s1));
        CComVariant s2 = s1;
        ok(V_VT(&s2) == VT_I2, "Expected .vt to be VT_I2, was %u\n", V_VT(&s2));
        ok(V_I2(&s2) == (short)12345, "Expected value to be 12345, was: %d\n", V_I1(&s2));
    }
    {
        CComVariant us1 = (unsigned short)12345;
        ok(V_VT(&us1) == VT_UI2, "Expected .vt to be VT_UI2, was %u\n", V_VT(&us1));
        ok(V_UI2(&us1) == (unsigned short)12345, "Expected value to be 12345, was: %u\n", V_UI2(&us1));
        CComVariant us2 = us1;
        ok(V_VT(&us2) == VT_UI2, "Expected .vt to be VT_UI2, was %u\n", V_VT(&us2));
        ok(V_UI2(&us2) == (unsigned short)12345, "Expected value to be 12345, was: %u\n", V_UI2(&us2));
    }
    {
        CComVariant i1 = (int)4;
        expect_int(i1, 4, VT_I4);
        CComVariant i2 = i1;
        expect_int(i2, 4, VT_I4);
    }
    {
        CComVariant ui1 = (unsigned int)4;
        expect_uint(ui1, 4, VT_UI4);
        CComVariant ui2 = ui1;
        expect_uint(ui2, 4, VT_UI4);
    }
    {
        CComVariant l1 = (long)4;
        expect_uint(l1, 4, VT_I4);
        CComVariant l2 = l1;
        expect_uint(l2, 4, VT_I4);
    }
    {
        CComVariant ul1 = (unsigned long)33;
        expect_uint(ul1, 33, VT_UI4);
        CComVariant ul2 = ul1;
        expect_uint(ul2, 33, VT_UI4);
    }
    {
        CComVariant f1 = 3.4f;
        ok(V_VT(&f1) == VT_R4, "Expected .vt to be VT_R4, was %u\n", V_VT(&f1));
        ok(V_R4(&f1) == 3.4f, "Expected value to be 3.4f, was: %f\n", V_R4(&f1));
        CComVariant f2 = f1;
        ok(V_VT(&f2) == VT_R4, "Expected .vt to be VT_R4, was %u\n", V_VT(&f2));
        ok(V_R4(&f2) == 3.4f, "Expected value to be 3.4f, was: %f\n", V_R4(&f2));
    }
    {
        CComVariant d1 = 3.4;
        expect_double(d1, 3.4, VT_R8);
        CComVariant d2 = d1;
        expect_double(d2, 3.4, VT_R8);
    }
    {
        LONGLONG lv = 12030912309123ll;
        CComVariant l1 = lv;
        ok(V_VT(&l1) == VT_I8, "Expected .vt to be VT_I8, was %u\n", V_VT(&l1));
        ok(V_I8(&l1) == lv, "Expected value to be %s, was: %s\n", wine_dbgstr_longlong(lv), wine_dbgstr_longlong(V_I8(&l1)));
        CComVariant l2 = l1;
        ok(V_VT(&l2) == VT_I8, "Expected .vt to be VT_I8, was %u\n", V_VT(&l2));
        ok(V_I8(&l2) == lv, "Expected value to be %s, was: %s\n", wine_dbgstr_longlong(lv), wine_dbgstr_longlong(V_I8(&l2)));
    }
    {
        ULONGLONG lv = 12030912309123ull;
        CComVariant l1 = lv;
        ok(V_VT(&l1) == VT_UI8, "Expected .vt to be VT_UI8, was %u\n", V_VT(&l1));
        ok(V_UI8(&l1) == lv, "Expected value to be %s, was: %s\n", wine_dbgstr_longlong(lv), wine_dbgstr_longlong(V_UI8(&l1)));
        CComVariant l2 = l1;
        ok(V_VT(&l2) == VT_UI8, "Expected .vt to be VT_UI8, was %u\n", V_VT(&l2));
        ok(V_UI8(&l2) == lv, "Expected value to be %s, was: %s\n", wine_dbgstr_longlong(lv), wine_dbgstr_longlong(V_UI8(&l2)));
    }
    {
        CY cy;
        cy.int64 = 12030912309123ll;
        CComVariant c1 = cy;
        ok(V_VT(&c1) == VT_CY, "Expected .vt to be VT_I8, was %u\n", V_VT(&c1));
        ok(V_CY(&c1).int64 == cy.int64, "Expected value to be %s, was: %s\n", wine_dbgstr_longlong(cy.int64), wine_dbgstr_longlong(V_CY(&c1).int64));
        CComVariant c2 = c1;
        ok(V_VT(&c2) == VT_CY, "Expected .vt to be VT_I8, was %u\n", V_VT(&c2));
        ok(V_CY(&c2).int64 == cy.int64, "Expected value to be %s, was: %s\n", wine_dbgstr_longlong(cy.int64), wine_dbgstr_longlong(V_CY(&c2).int64));
    }
    // IDispatch
    // IUnknown
}

static void test_misc()
{
    HRESULT hr;
    {
        CComVariant empty;
        hr = empty.Clear();
        ok(SUCCEEDED(hr), "Expected .Clear() to succeed, but it failed: 0x%lx\n", hr);
        expect_empty(empty);
    }

    {
        CComBSTR bstr(L"TESTW");
        CComVariant olestr((LPCOLESTR)bstr), empty;

        hr = empty.Copy(&olestr);   // VARIANT*
        ok(SUCCEEDED(hr), "Expected .Copy() to succeed, but it failed: 0x%lx\n", hr);
        ok(V_VT(&empty) == VT_BSTR, "Expected .vt to be VT_LPWSTR, was %u\n", V_VT(&empty));
        ok(!wcscmp(V_BSTR(&empty), L"TESTW"), "Expected value to be L\"TESTW\", was: %s\n", wine_dbgstr_w(V_BSTR(&empty)));

        /* Clear does not null out the rest, it just sets .vt! */
        hr = olestr.Clear();
        ok(SUCCEEDED(hr), "Expected .Clear() to succeed, but it failed: 0x%lx\n", hr);
        ok(V_VT(&olestr) == VT_EMPTY, "Expected .vt to be VT_EMPTY, was %u\n", V_VT(&olestr));
    }

    {
        CComVariant d1(3.4, VT_R8), empty;
        hr = empty.Copy(&d1);
        ok(SUCCEEDED(hr), "Expected .Copy() to succeed, but it failed: 0x%lx\n", hr);
        expect_double(empty, 3.4, VT_R8);
    }

    {
        LONGLONG lv = 12030912309123ll;
        CComVariant l1(lv);
        CComVariant empty;
        hr = empty.Copy(&l1);
        ok(SUCCEEDED(hr), "Expected .Copy() to succeed, but it failed: 0x%lx\n", hr);
        ok(V_VT(&empty) == VT_I8, "Expected .vt to be VT_I8, was %u\n", V_VT(&empty));
        ok(V_I8(&empty) == lv, "Expected value to be %s, was: %s\n", wine_dbgstr_longlong(lv), wine_dbgstr_longlong(V_I8(&empty)));
    }

    {
        CY cy;
        cy.int64 = 12030912309123ll;
        CComVariant c1(cy);
        CComVariant empty;
        hr = empty.Copy(&c1);
        ok(SUCCEEDED(hr), "Expected .Copy() to succeed, but it failed: 0x%lx\n", hr);
        ok(V_VT(&empty) == VT_CY, "Expected .vt to be VT_I8, was %u\n", V_VT(&empty));
        ok(V_CY(&empty).int64 == cy.int64, "Expected value to be %s, was: %s\n", wine_dbgstr_longlong(cy.int64), wine_dbgstr_longlong(V_CY(&empty).int64));
    }
    {
        CComVariant var = (int)333;
        CComVariant var2;
        // var2 = var changed to bstr
        HRESULT hr = var2.ChangeType(VT_BSTR, &var);
        ok(SUCCEEDED(hr), "Expected .ChangeType() to succeed, but it failed: 0x%lx\n", hr);
        expect_int(var, 333, VT_I4);
        ok(V_VT(&var2) == VT_BSTR, "Expected .vt to be VT_LPWSTR, was %u\n", V_VT(&var2));
        ok(!wcscmp(V_BSTR(&var2), L"333"), "Expected value to be L\"TESTW\", was: %s\n", wine_dbgstr_w(V_BSTR(&var2)));

        // change in place
        hr = var.ChangeType(VT_BSTR);
        ok(SUCCEEDED(hr), "Expected .ChangeType() to succeed, but it failed: 0x%lx\n", hr);
        ok(V_VT(&var) == VT_BSTR, "Expected .vt to be VT_LPWSTR, was %u\n", V_VT(&var));
        ok(!wcscmp(V_BSTR(&var), L"333"), "Expected value to be L\"TESTW\", was: %s\n", wine_dbgstr_w(V_BSTR(&var)));
    }
}


START_TEST(CComVariant)
{
    test_construction();
    test_copyconstructor();
    test_assign();
    test_misc();
}
