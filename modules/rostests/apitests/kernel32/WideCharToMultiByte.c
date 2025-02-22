/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for WideCharToMultiByte
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

#define ntv6(x) (LOBYTE(LOWORD(GetVersion())) >= 6 ? (x) : 0)

static
VOID
Utf8Convert_(
    _In_ PCWSTR WideString,
    _In_ PCSTR ExpectedUtf8_2003,
    _In_ PCSTR ExpectedUtf8_Vista,
    _In_ BOOL IsInvalid,
    _In_ PCSTR File,
    _In_ INT Line)
{
    int WideLen;
    int Utf8Len;
    char Buffer[32];
    int Ret;
    ULONG i;
    ULONG Error;
    PCSTR ExpectedUtf8;

    ExpectedUtf8 = ntv6(1) ? ExpectedUtf8_Vista : ExpectedUtf8_2003;
    WideLen = lstrlenW(WideString);
    Utf8Len = lstrlenA(ExpectedUtf8);

    /* Get length only */
    Ret = WideCharToMultiByte(CP_UTF8, 0, WideString, WideLen, NULL, 0, NULL, NULL);
    ok_(File, Line)(Ret == Utf8Len, "Length check: Ret = %d\n", Ret);

    /* Get length including nul */
    Ret = WideCharToMultiByte(CP_UTF8, 0, WideString, WideLen + 1, NULL, 0, NULL, NULL);
    ok_(File, Line)(Ret == Utf8Len + 1, "Length check with null: Ret = %d\n", Ret);

    /* Convert, excluding null */
    FillMemory(Buffer, sizeof(Buffer), 0x55);
    Ret = WideCharToMultiByte(CP_UTF8, 0, WideString, WideLen, Buffer, sizeof(Buffer), NULL, NULL);
    ok_(File, Line)(Ret == Utf8Len, "Convert: Ret = %d\n", Ret);
    for (i = 0; i < Utf8Len; i++)
    {
        ok_(File, Line)(Buffer[i] == ExpectedUtf8[i], "Convert: Buffer[%lu] = 0x%x, expected 0x%x\n", i, (unsigned char)Buffer[i], (unsigned char)ExpectedUtf8[i]);
    }

    /* Convert, including null */
    FillMemory(Buffer, sizeof(Buffer), 0x55);
    Ret = WideCharToMultiByte(CP_UTF8, 0, WideString, WideLen + 1, Buffer, sizeof(Buffer), NULL, NULL);
    ok_(File, Line)(Ret == Utf8Len + 1, "Convert with null: Ret = %d\n", Ret);
    for (i = 0; i < Utf8Len + 1; i++)
    {
        ok_(File, Line)(Buffer[i] == ExpectedUtf8[i], "Convert with null: Buffer[%lu] = 0x%x, expected 0x%x\n", i, (unsigned char)Buffer[i], (unsigned char)ExpectedUtf8[i]);
    }

    /* Get length, reject invalid */
    SetLastError(0xfeedf00d);
    Ret = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, WideString, WideLen + 1, NULL, 0, NULL, NULL);
    Error = GetLastError();
    if (!ntv6(1))
    {
        ok_(File, Line)(Ret == 0, "Length check, reject invalid, NT5: Ret = %d\n", Ret);
        ok_(File, Line)(Error == ERROR_INVALID_FLAGS, "Length check, reject invalid, NT5: Error = %lu\n", Error);
    }
    else if (IsInvalid)
    {
        ok_(File, Line)(Ret == 0, "Length check, reject invalid: Ret = %d\n", Ret);
        ok_(File, Line)(Error == ERROR_NO_UNICODE_TRANSLATION, "Length check, reject invalid: Error = %lu\n", Error);
    }
    else
    {
        ok_(File, Line)(Ret == Utf8Len + 1, "Length check, reject invalid: Ret = %d\n", Ret);
    }

    /* Convert, reject invalid */
    FillMemory(Buffer, sizeof(Buffer), 0x55);
    SetLastError(0xfeedf00d);
    Ret = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, WideString, WideLen + 1, Buffer, sizeof(Buffer), NULL, NULL);
    Error = GetLastError();
    if (!ntv6(1))
    {
        ok_(File, Line)(Ret == 0, "Convert, reject invalid, NT5: Ret = %d\n", Ret);
        ok_(File, Line)(Error == ERROR_INVALID_FLAGS, "Convert, reject invalid, NT5: Error = %lu\n", Error);
    }
    else if (IsInvalid)
    {
        ok_(File, Line)(Ret == 0, "Convert, reject invalid: Ret = %d\n", Ret);
        ok_(File, Line)(Error == ERROR_NO_UNICODE_TRANSLATION, "Convert, reject invalid: Error = %lu\n", Error);
        for (i = 0; i < Utf8Len + 1; i++)
        {
            ok_(File, Line)(Buffer[i] == ExpectedUtf8[i], "Convert, reject invalid: Buffer[%lu] = 0x%x, expected 0x%x\n", i, (unsigned char)Buffer[i], (unsigned char)ExpectedUtf8[i]);
        }
    }
    else
    {
        ok_(File, Line)(Ret == Utf8Len + 1, "Convert, reject invalid: Ret = %d\n", Ret);
        for (i = 0; i < Utf8Len + 1; i++)
        {
            ok_(File, Line)(Buffer[i] == ExpectedUtf8[i], "Convert, reject invalid: Buffer[%lu] = 0x%x, expected 0x%x\n", i, (unsigned char)Buffer[i], (unsigned char)ExpectedUtf8[i]);
        }
    }
}
#define Utf8Convert(w, e, i) Utf8Convert_(w, e, e, i, __FILE__, __LINE__)
#define Utf8Convert_Vista(w, e, i, e2) Utf8Convert_(w, e, e2, i, __FILE__, __LINE__)

static
VOID
TestUtf8(VOID)
{
    Utf8Convert(L"", "", FALSE);

    /* Various character ranges */
    Utf8Convert(L"A", "A", FALSE);
    Utf8Convert(L"\x007f", "\x7f", FALSE);
    Utf8Convert(L"\x0080", "\xc2\x80", FALSE);
    Utf8Convert(L"\x00ff", "\xc3\xbf", FALSE);
    Utf8Convert(L"\x0100", "\xc4\x80", FALSE);
    Utf8Convert(L"\x07ff", "\xdf\xbf", FALSE);
    Utf8Convert(L"\x0800", "\xe0\xa0\x80", FALSE);
    Utf8Convert(L"\xd7ff", "\xed\x9f\xbf", FALSE);
    Utf8Convert(L"\xe000", "\xee\x80\x80", FALSE);
    Utf8Convert(L"\xffff", "\xef\xbf\xbf", FALSE);

    /* surrogate pairs */
    Utf8Convert(L"\xd800\xdc00", "\xf0\x90\x80\x80", FALSE); /* U+10000 */
    Utf8Convert(L"\xd800\xdfff", "\xf0\x90\x8f\xbf", FALSE); /* U+103ff */
    Utf8Convert(L"\xd801\xdc00", "\xf0\x90\x90\x80", FALSE); /* U+10400 */
    Utf8Convert(L"\xdbff\xdfff", "\xf4\x8f\xbf\xbf", FALSE); /* U+10ffff */

    /* standalone lead surrogate becomes 0xfffd on Vista, goes through verbatim on 2003 */
    Utf8Convert_Vista(L"\xd800", "\xed\xa0\x80", TRUE,
                                 "\xef\xbf\xbd");
    Utf8Convert_Vista(L"\xd800-", "\xed\xa0\x80-", TRUE,
                                  "\xef\xbf\xbd-");
    Utf8Convert_Vista(L"\xdbff", "\xed\xaf\xbf", TRUE,
                                 "\xef\xbf\xbd");
    Utf8Convert_Vista(L"\xdbff-", "\xed\xaf\xbf-", TRUE,
                                  "\xef\xbf\xbd-");

    /* standalone trail surrogate becomes 0xfffd */
    Utf8Convert_Vista(L"\xdc00", "\xed\xb0\x80", TRUE,
                                 "\xef\xbf\xbd");
    Utf8Convert_Vista(L"\xdc00-", "\xed\xb0\x80-", TRUE,
                                  "\xef\xbf\xbd-");
    Utf8Convert_Vista(L"\xdfff", "\xed\xbf\xbf", TRUE,
                                 "\xef\xbf\xbd");
    Utf8Convert_Vista(L"\xdfff-", "\xed\xbf\xbf-", TRUE,
                                  "\xef\xbf\xbd-");

    /* Reverse surrogate pair */
    Utf8Convert_Vista(L"\xdfff\xdbff", "\xed\xbf\xbf\xed\xaf\xbf", TRUE,
                                       "\xef\xbf\xbd\xef\xbf\xbd");

    /* Byte order marks */
    Utf8Convert(L"\xfeff", "\xef\xbb\xbf", FALSE);
    Utf8Convert(L"\xfffe", "\xef\xbf\xbe", FALSE);

    /* canonically equivalent representations -- no normalization should happen */
    Utf8Convert(L"\x1e09", "\xe1\xb8\x89", FALSE);
    Utf8Convert(L"\x0107\x0327", "\xc4\x87\xcc\xa7", FALSE);
    Utf8Convert(L"\x00e7\x0301", "\xc3\xa7\xcc\x81", FALSE);
    Utf8Convert(L"\x0063\x0327\x0301", "\x63\xcc\xa7\xcc\x81", FALSE);
    Utf8Convert(L"\x0063\x0301\x0327", "\x63\xcc\x81\xcc\xa7", FALSE);
}

START_TEST(WideCharToMultiByte)
{
    TestUtf8();
}
