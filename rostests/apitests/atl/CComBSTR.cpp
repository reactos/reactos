/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CComBSTR
 * PROGRAMMER:      Mark Jansen
 */

#include <apitest.h>
#include <atlbase.h>
#include <atlcom.h>
#include "resource.h"

#define verify_str       (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : verify_str_imp
#define verify_str2      (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : verify_str_imp2


static void verify_str_imp2(const CComBSTR& comstr, PCWSTR expected, size_t ExpectedLength)
{
    BSTR str = (BSTR)comstr;
    if (expected || ExpectedLength)
    {
        winetest_ok(str != NULL, "Expected str to be a valid pointer\n");
        if (str)
        {
            if (expected)
            {
                winetest_ok(!wcscmp(str, expected), "Expected the string to be '%s', was '%s'\n", wine_dbgstr_w(expected), wine_dbgstr_w(str));
            }
            size_t Length = comstr.Length();
            winetest_ok(Length == ExpectedLength, "Expected Length to be %u, was: %u\n", ExpectedLength, Length);
            Length = comstr.ByteLength();
            ExpectedLength *= sizeof(WCHAR);
            winetest_ok(Length == ExpectedLength, "Expected ByteLength to be %u, was: %u\n", ExpectedLength, Length);
        }
    }
    else
    {
        winetest_ok(str == NULL || str[0] == '\0', "Expected str to be empty, was: '%s'\n", wine_dbgstr_w(str));
    }
}

static void verify_str_imp(const CComBSTR& comstr, PCWSTR expected)
{
    verify_str_imp2(comstr, expected, expected ? wcslen(expected) : 0);
}

void test_construction()
{
    CComBSTR empty1, empty2;
    CComBSTR happyW(L"I am a happy BSTR");
    CComBSTR happyA("I am a happy BSTR");
    CComBSTR happyW4(4, L"I am a happy BSTR");
    CComBSTR fromlen1(1), fromlen10(10);
    CComBSTR fromBSTRW(happyW), fromBSTRA(happyA), fromBSTRW4(happyW4);
    CComBSTR fromBSTRlen1(fromlen1), fromBSTRlen10(fromlen10);

    verify_str(empty1, NULL);
    verify_str(empty2, NULL);
    verify_str(happyW, L"I am a happy BSTR");
    verify_str(happyA, L"I am a happy BSTR");
    verify_str(happyW4, L"I am");
    verify_str2(fromlen1, NULL, 1);
    verify_str2(fromlen10, NULL, 10);
    verify_str(fromBSTRW, L"I am a happy BSTR");
    verify_str(fromBSTRA, L"I am a happy BSTR");
    verify_str(fromBSTRW4, L"I am");
    verify_str2(fromBSTRlen1, NULL, 1);
    verify_str2(fromBSTRlen10, NULL, 10);
}

void test_copyassignment()
{
    CComBSTR happy(L"I am a happy BSTR"), empty, odd;
    CComBSTR happyCopy1, happyCopy2, emptyCopy, oddCopy;

    odd = ::SysAllocStringByteLen("aaaaa", 3);

    happyCopy1 = happy.Copy();
    happyCopy2 = happy;    // Calls happyW.Copy()
    emptyCopy = empty.Copy();
    oddCopy = odd.Copy();

    verify_str(happy, L"I am a happy BSTR");
    verify_str(empty, NULL);
    verify_str2(odd, L"\u6161a", 2);
    verify_str(happyCopy1, L"I am a happy BSTR");
    verify_str(happyCopy2, L"I am a happy BSTR");
    verify_str(emptyCopy, NULL);
    verify_str2(oddCopy, L"\u6161a", 2);
    ok((BSTR)happy != (BSTR)happyCopy1, "Expected pointers to be different\n");
    ok((BSTR)happy != (BSTR)happyCopy2, "Expected pointers to be different\n");


    happyCopy1 = (LPCOLESTR)NULL;
    happyCopy2 = (LPCSTR)NULL;

    verify_str(happyCopy1, NULL);
    verify_str(happyCopy2, NULL);

    HRESULT hr = happy.CopyTo(&happyCopy1);
    ok(hr == S_OK, "Expected hr to be E_POINTER, was: %08lx\n", hr);

#if 0
    // This asserts
    hr = happy.CopyTo((BSTR*)NULL);
    ok(hr == E_POINTER, "Expected hr to be E_POINTER, was: %u\n");
#endif
}

void test_fromguid()
{
    GUID guid = { 0x12345678, 0x9abc, 0xdef0, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0} };
    CComBSTR fromGuid(guid), empty;
    verify_str(fromGuid, L"{12345678-9ABC-DEF0-1234-56789ABCDEF0}");
    verify_str(empty, NULL);
    empty = fromGuid;
    verify_str(empty, L"{12345678-9ABC-DEF0-1234-56789ABCDEF0}");
}

void test_loadres()
{
    CComBSTR test1, test2, test3;
    HMODULE mod = GetModuleHandle(NULL);

    ok(true == test1.LoadString(mod, IDS_TEST1), "Expected LoadString to succeed\n");
    ok(true == test2.LoadString(mod, IDS_TEST2), "Expected LoadString to succeed\n");
    ok(false == test3.LoadString(mod, IDS_TEST2 + 1), "Expected LoadString to fail\n");

    verify_str(test1, L"Test string one.");
    verify_str(test2, L"I am a happy BSTR");
    verify_str(test3, NULL);
}

START_TEST(CComBSTR)
{
    test_construction();
    test_copyassignment();
    test_fromguid();
    test_loadres();
}
