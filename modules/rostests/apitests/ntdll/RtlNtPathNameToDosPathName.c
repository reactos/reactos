/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for RtlNtPathNameToDosPathName
 * COPYRIGHT:   Copyright 2017-2020 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

NTSTATUS (NTAPI *pRtlNtPathNameToDosPathName)(ULONG Flags, PRTL_UNICODE_STRING_BUFFER Path, PULONG Type, PULONG Unknown4);

#define ok_hex2(expression, result) \
    do { \
        int _value = (expression); \
        winetest_ok(_value == (result), "Wrong value for '%s', expected: " #result " (0x%x), got: 0x%x\n", \
           #expression, (int)(result), _value); \
    } while (0)


#define ok_ptr2(expression, result) \
    do { \
        void *_value = (expression); \
        winetest_ok(_value == (result), "Wrong value for '%s', expected: " #result " (%p), got: %p\n", \
           #expression, (void*)(result), _value); \
    } while (0)

#define ok_wstr2(x, y) \
    winetest_ok(wcscmp(x, y) == 0, "Wrong string. Expected '%S', got '%S'\n", y, x)



struct test_entry
{
    WCHAR* InputPath;
    WCHAR* OutputPath;
    ULONG Type;

    const char* File;
    int Line;
};


static struct test_entry test_data[] =
{
    /* Originally from RtlGetFullPathName_*.c (edited) */
    { L"",                      L"",                            RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L".\\test",               L".\\test",                     RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"/test",                 L"/test",                       RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"??\\",                  L"??\\",                        RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"??\\C:",                L"??\\C:",                      RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"??\\C:\\",              L"??\\C:\\",                    RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"??\\C:\\test",          L"??\\C:\\test",                RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"??\\C:\\test\\",        L"??\\C:\\test\\",              RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"C:",                    L"C:",                          RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"C:/test/",              L"C:/test/",                    RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"C:\\",                  L"C:\\",                        RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"C:\\",                  L"C:\\",                        RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"C:\\\\test",            L"C:\\\\test",                  RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"C:\\test",              L"C:\\test",                    RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"C:\\test",              L"C:\\test",                    RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"C:\\test",              L"C:\\test",                    RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"C:\\test\\",            L"C:\\test\\",                  RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"C:\\test\\",            L"C:\\test\\",                  RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"C:\\test\\",            L"C:\\test\\",                  RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\.",                   L"\\.",                         RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\.\\",                 L"\\.\\",                       RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\??\\",                L"",                            RTL_CONVERTED_NT_PATH,          __FILE__, __LINE__ },
    { L"\\??\\C:",              L"C:",                          RTL_CONVERTED_NT_PATH,          __FILE__, __LINE__ },
    { L"\\??\\C:\\",            L"C:\\",                        RTL_CONVERTED_NT_PATH,          __FILE__, __LINE__ },
    { L"\\??\\C:\\test",        L"C:\\test",                    RTL_CONVERTED_NT_PATH,          __FILE__, __LINE__ },
    { L"\\??\\C:\\test\\",      L"C:\\test\\",                  RTL_CONVERTED_NT_PATH,          __FILE__, __LINE__ },
    { L"\\\\.",                 L"\\\\.",                       RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\.\\",               L"\\\\.\\",                     RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\.\\",               L"\\\\.\\",                     RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\.\\",               L"\\\\.\\",                     RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\.\\Something\\",    L"\\\\.\\Something\\",          RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\.\\Something\\",    L"\\\\.\\Something\\",          RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\??\\",              L"\\\\??\\",                    RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\??\\",              L"\\\\??\\",                    RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\??\\C:",            L"\\\\??\\C:",                  RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\??\\C:",            L"\\\\??\\C:",                  RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\??\\C:\\",          L"\\\\??\\C:\\",                RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\??\\C:\\",          L"\\\\??\\C:\\",                RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\??\\C:\\test",      L"\\\\??\\C:\\test",            RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\??\\C:\\test",      L"\\\\??\\C:\\test",            RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\??\\C:\\test\\",    L"\\\\??\\C:\\test\\",          RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\??\\C:\\test\\",    L"\\\\??\\C:\\test\\",          RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\test",                L"\\test",                      RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\test",                L"\\test",                      RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\test",                L"\\test",                      RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"test",                  L"test",                        RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"test",                  L"test",                        RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"test",                  L"test",                        RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },

    /* Originally from RtlDetermineDosPathNameType.c (edited) */
    { L"",                      L"",                            RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L" ",                     L" ",                           RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"xyz",                   L"xyz",                         RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"CON",                   L"CON",                         RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"NUL",                   L"NUL",                         RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L":",                     L":",                           RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"::",                    L"::",                          RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L":::",                   L":::",                         RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"::::",                  L"::::",                        RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"::\\",                  L"::\\",                        RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\",                    L"\\",                          RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\:",                   L"\\:",                         RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\C:",                  L"\\C:",                        RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\C:\\",                L"\\C:\\",                      RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"/",                     L"/",                           RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"/:",                    L"/:",                          RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"/C:",                   L"/C:",                         RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"/C:/",                  L"/C:/",                        RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"C",                     L"C",                           RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"C:",                    L"C:",                          RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"C:a",                   L"C:a",                         RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"C:a\\",                 L"C:a\\",                       RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"C:\\",                  L"C:\\",                        RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"C:/",                   L"C:/",                         RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"C:\\a",                 L"C:\\a",                       RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"C:/a",                  L"C:/a",                        RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"C:\\\\",                L"C:\\\\",                      RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\",                  L"\\\\",                        RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\\\",                L"\\\\\\",                      RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\;",                 L"\\\\;",                       RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\f\\b\\",            L"\\\\f\\b\\",                  RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\f\\b",              L"\\\\f\\b",                    RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\f\\",               L"\\\\f\\",                     RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\f",                 L"\\\\f",                       RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\??\\UNC",             L"UNC",                         RTL_CONVERTED_NT_PATH,          __FILE__, __LINE__ },
    { L"\\??\\UNC\\",           L"\\\\",                        RTL_CONVERTED_UNC_PATH,         __FILE__, __LINE__ },
    { L"\\??\\UNC\\pth1\\pth2", L"\\\\pth1\\pth2",              RTL_CONVERTED_UNC_PATH,         __FILE__, __LINE__ },
    { L"\\??\\UNC\\path1",      L"\\\\path1",                   RTL_CONVERTED_UNC_PATH,         __FILE__, __LINE__ },
    { L"\\?",                   L"\\?",                         RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\?\\",                 L"\\?\\",                       RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\?\\UNC",              L"\\?\\UNC",                    RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\?\\UNC\\",            L"\\?\\UNC\\",                  RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\\\?\\UNC\\",          L"\\\\?\\UNC\\",                RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\??\\unc",             L"unc",                         RTL_CONVERTED_NT_PATH,          __FILE__, __LINE__ },
    { L"\\??\\unc\\",           L"\\\\",                        RTL_CONVERTED_UNC_PATH,         __FILE__, __LINE__ },
    { L"\\??\\unc\\pth1\\pth2", L"\\\\pth1\\pth2",              RTL_CONVERTED_UNC_PATH,         __FILE__, __LINE__ },
    { L"\\??\\unc\\path1",      L"\\\\path1",                   RTL_CONVERTED_UNC_PATH,         __FILE__, __LINE__ },
    { L"\\?",                   L"\\?",                         RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\?\\",                 L"\\?\\",                       RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\?\\unc",              L"\\?\\unc",                    RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\?\\unc\\",            L"\\?\\unc\\",                  RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\\\?\\unc\\",          L"\\\\?\\unc\\",                RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\?",                 L"\\\\?",                       RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\??",                L"\\\\??",                      RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\??\\",              L"\\\\??\\",                    RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\??\\C:\\",          L"\\\\??\\C:\\",                RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\.",                 L"\\\\.",                       RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\.\\",               L"\\\\.\\",                     RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\\\.\\C:\\",           L"\\\\.\\C:\\",                 RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\/",                   L"\\/",                         RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"/\\",                   L"/\\",                         RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"//",                    L"//",                          RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"///",                   L"///",                         RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"//;",                   L"//;",                         RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"//?",                   L"//?",                         RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"/\\?",                  L"/\\?",                        RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\/?",                  L"\\/?",                        RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"//??",                  L"//??",                        RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"//?" L"?/",             L"//?" L"?/",                   RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"//?" L"?/C:/",          L"//?" L"?/C:/",                RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"//.",                   L"//.",                         RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"\\/.",                  L"\\/.",                        RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"/\\.",                  L"/\\.",                        RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"//./",                  L"//./",                        RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"//./C:/",               L"//./C:/",                     RTL_UNCHANGED_DOS_PATH,         __FILE__, __LINE__ },
    { L"%SystemRoot%",          L"%SystemRoot%",                RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },


    /* Tests from RtlGetLengthWithoutTrailingPathSeperators.c */
    { L"",                      L"",                            RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"T",                     L"T",                           RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"Te",                    L"Te",                          RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"Tes",                   L"Tes",                         RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"Test",                  L"Test",                        RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },

    /* Separators tests */
    { L"\\.",                   L"\\.",                         RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\.",                   L"\\.",                         RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\.\\",                 L"\\.\\",                       RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\.\\T",                L"\\.\\T",                      RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\.\\Te",               L"\\.\\Te",                     RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\.\\Tes",              L"\\.\\Tes",                    RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\.\\Test",             L"\\.\\Test",                   RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\.\\Test\\",           L"\\.\\Test\\",                 RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\.\\Test\\s",          L"\\.\\Test\\s",                RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\.\\T\\est",           L"\\.\\T\\est",                 RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\.\\T\\e\\st",         L"\\.\\T\\e\\st",               RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\.\\T\\e\\s\\t",       L"\\.\\T\\e\\s\\t",             RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\.\\T\\e\\s\\t\\",     L"\\.\\T\\e\\s\\t\\",           RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\Tests\\String\\",     L"\\Tests\\String\\",           RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\.\\Test\\String\\",   L"\\.\\Test\\String\\",         RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\.\\Tests\\String\\",  L"\\.\\Tests\\String\\",        RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\.\\Tests\\String\\s", L"\\.\\Tests\\String\\s",       RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },

    /* Separator-only tests */
    { L"\\",                    L"\\",                          RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"/",                     L"/",                           RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },

    /* Mixed separators tests */
    { L"/Test/String",          L"/Test/String",                RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\Test/String",         L"\\Test/String",               RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"/Test\\String",         L"/Test\\String",               RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\Test/String",         L"\\Test/String",               RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"/Test/String\\",        L"/Test/String\\",              RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\Test/String\\",       L"\\Test/String\\",             RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"/Test\\String\\",       L"/Test\\String\\",             RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\Test/String\\",       L"\\Test/String\\",             RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"/Test/String/",         L"/Test/String/",               RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\Test/String/",        L"\\Test/String/",              RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"/Test\\String/",        L"/Test\\String/",              RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\Test/String/",        L"\\Test/String/",              RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\Test/String/",        L"\\Test/String/",              RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\Test\\\\String/",     L"\\Test\\\\String/",           RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },

    /* Common path formats tests */
    { L"Test\\String",          L"Test\\String",                RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\Test\\String",        L"\\Test\\String",              RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L".\\Test\\String",       L".\\Test\\String",             RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\.\\Test\\String",     L"\\.\\Test\\String",           RTL_UNCHANGED_UNK_PATH,         __FILE__, __LINE__ },
    { L"\\??\\Test\\String",    L"Test\\String",                RTL_CONVERTED_NT_PATH,          __FILE__, __LINE__ },

    /* Redundant trailing tests */
    { L"\\??\\Test\\String\\",  L"Test\\String\\",              RTL_CONVERTED_NT_PATH,          __FILE__, __LINE__ },
    { L"\\??\\Test\\String\\\\",L"Test\\String\\\\",            RTL_CONVERTED_NT_PATH,          __FILE__, __LINE__ },
    { L"\\??\\Test\\String\\\\\\\\\\", L"Test\\String\\\\\\\\\\",RTL_CONVERTED_NT_PATH,         __FILE__, __LINE__ },
};


static void test_specialhandling()
{
    RTL_UNICODE_STRING_BUFFER Buffer;
    const WCHAR TestString[] = L"\\??\\C:\\Test";
    WCHAR StaticBuffer[_countof(TestString)];
    ULONG Type;
    //PUCHAR Ptr;

    /* Just initializing the ByteBuffer does not work */
    memset(&Buffer, 0, sizeof(Buffer));
    RtlInitBuffer(&Buffer.ByteBuffer, (PUCHAR)StaticBuffer, sizeof(StaticBuffer));
    memcpy(StaticBuffer, TestString, sizeof(TestString));
    Type = 0x12345;

    ok_hex(pRtlNtPathNameToDosPathName(0, &Buffer, &Type, NULL), STATUS_SUCCESS);
    ok_hex(Type, RTL_UNCHANGED_UNK_PATH);
    ok_ptr(Buffer.String.Buffer, NULL);
    ok_int(Buffer.String.Length, 0);
    ok_int(Buffer.String.MaximumLength, 0);
    ok_ptr(Buffer.ByteBuffer.Buffer, Buffer.ByteBuffer.StaticBuffer);
    ok_int(Buffer.ByteBuffer.Size, Buffer.ByteBuffer.StaticSize);
    RtlFreeBuffer(&Buffer.ByteBuffer);

    /* Different strings in the String and ByteBuffer part */
    memset(&Buffer, 0, sizeof(Buffer));
    RtlInitBuffer(&Buffer.ByteBuffer, (PUCHAR)StaticBuffer, sizeof(StaticBuffer));
    memcpy(StaticBuffer, TestString, sizeof(TestString));
    Type = 0x12345;

    RtlInitUnicodeString(&Buffer.String, L"\\??\\D:\\1234");

    ok_hex(pRtlNtPathNameToDosPathName(0, &Buffer, &Type, NULL), STATUS_SUCCESS);
    ok_hex(Type, RTL_CONVERTED_NT_PATH);
    ok_wstr(Buffer.String.Buffer, L"C:\\Test");
    ok_int(Buffer.String.Length, 14);
    ok_int(Buffer.String.MaximumLength, 24);
    ok_ptr(Buffer.ByteBuffer.Buffer, Buffer.ByteBuffer.StaticBuffer);
    ok_int(Buffer.ByteBuffer.Size, Buffer.ByteBuffer.StaticSize);
    RtlFreeBuffer(&Buffer.ByteBuffer);


    /* Different strings, Buffer.String is not prefixed with \??\ */
    memset(&Buffer, 0, sizeof(Buffer));
    RtlInitBuffer(&Buffer.ByteBuffer, (PUCHAR)StaticBuffer, sizeof(StaticBuffer));
    memcpy(StaticBuffer, TestString, sizeof(TestString));
    Type = 0x12345;

    RtlInitUnicodeString(&Buffer.String, L"D:\\1234");

    ok_hex(pRtlNtPathNameToDosPathName(0, &Buffer, &Type, NULL), STATUS_SUCCESS);
    ok_hex(Type, RTL_UNCHANGED_DOS_PATH);
    ok_wstr(Buffer.String.Buffer, L"D:\\1234");
    ok_int(Buffer.String.Length, 14);
    ok_int(Buffer.String.MaximumLength, 16);
    ok_ptr(Buffer.ByteBuffer.Buffer, Buffer.ByteBuffer.StaticBuffer);
    ok_int(Buffer.ByteBuffer.Size, Buffer.ByteBuffer.StaticSize);
    RtlFreeBuffer(&Buffer.ByteBuffer);


    /* Different strings, smaller ByteBuffer */
    memset(&Buffer, 0, sizeof(Buffer));
    RtlInitBuffer(&Buffer.ByteBuffer, (PUCHAR)StaticBuffer, sizeof(StaticBuffer));
    memcpy(StaticBuffer, TestString, sizeof(TestString));
    Type = 0x12345;

    RtlInitUnicodeString(&Buffer.String, L"\\??\\D:\\1234");
    Buffer.ByteBuffer.Size -= 4 * sizeof(WCHAR);

    ok_hex(pRtlNtPathNameToDosPathName(0, &Buffer, &Type, NULL), STATUS_SUCCESS);
    ok_hex(Type, RTL_CONVERTED_NT_PATH);
    ok_wstr(Buffer.String.Buffer, L"C:\\Test");
    ok_int(Buffer.String.Length, 14);
    ok_ptr(Buffer.ByteBuffer.Buffer, Buffer.ByteBuffer.StaticBuffer);
    ok_int(Buffer.ByteBuffer.Size, Buffer.ByteBuffer.StaticSize - 4 * sizeof(WCHAR));
    RtlFreeBuffer(&Buffer.ByteBuffer);


    /* These tests show that the size of the ByteBuffer should
        at least equal the size of the string (minus 4, in case of \??\)!
        The results are all over the place, and are most likely the result of implementation details.. */

#if 0
    /* Different strings, too small ByteBuffer
        --> corrupt buffer, but none of the output params suggests so? */
    memset(&Buffer, 0, sizeof(Buffer));
    Buffer.ByteBuffer.Size = wcslen(TestString) * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    Buffer.ByteBuffer.Buffer = Ptr = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, Buffer.ByteBuffer.Size);
    memcpy(Buffer.ByteBuffer.Buffer, TestString, Buffer.ByteBuffer.Size);
    Type = 0x12345;

    RtlInitUnicodeString(&Buffer.String, L"\\??\\D:\\1234");
    Buffer.ByteBuffer.Size -= 5 * sizeof(WCHAR);

    ok_hex(pRtlNtPathNameToDosPathName(0, &Buffer, &Type, NULL), STATUS_SUCCESS);
    ok_hex(Type, RTL_CONVERTED_NT_PATH);
    //ok_wstr(Buffer.String.Buffer, L"C:\\");
    ok_int(Buffer.String.Length, 14);
    ok_int(Buffer.String.MaximumLength, 16);
    //ok_ptr(Buffer.ByteBuffer.Buffer, Ptr);    // An attempt is made at allocating a buffer, but the move fails because the size of ByteBuffer seems to be used??
    ok_int(Buffer.ByteBuffer.Size, 16);

    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer.ByteBuffer.Buffer);

    /* Different strings, too small ByteBuffer, different path separators
        --> corrupt buffer, but none of the output params suggests so? */
    memset(&Buffer, 0, sizeof(Buffer));
    Buffer.ByteBuffer.Size = wcslen(L"\\??\\C://Test") * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    Buffer.ByteBuffer.Buffer = Ptr = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, Buffer.ByteBuffer.Size);
    memcpy(Buffer.ByteBuffer.Buffer, L"\\??\\C://Test", Buffer.ByteBuffer.Size);
    Type = 0x12345;

    RtlInitUnicodeString(&Buffer.String, L"\\??\\D:\\1234");
    Buffer.ByteBuffer.Size -= 5 * sizeof(WCHAR);

    ok_hex(pRtlNtPathNameToDosPathName(0, &Buffer, &Type, NULL), STATUS_SUCCESS);
    ok_hex(Type, RTL_CONVERTED_NT_PATH);
    //ok_wstr(Buffer.String.Buffer, L"C:\\");
    ok_int(Buffer.String.Length, 14);
    ok_int(Buffer.String.MaximumLength, 16);
    ok_ptr(Buffer.ByteBuffer.Buffer, Ptr);
    ok_int(Buffer.ByteBuffer.Size, 16);

    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer.ByteBuffer.Buffer);
#endif
}

static void test_table(struct test_entry* Entry)
{
    RTL_UNICODE_STRING_BUFFER Buffer = { { 0 } };
    WCHAR StaticBuffer[MAX_PATH];
    ULONG Type = 0x12345;

    RtlInitBuffer(&Buffer.ByteBuffer, (PUCHAR)StaticBuffer, sizeof(StaticBuffer));

    RtlInitUnicodeString(&Buffer.String, Entry->InputPath);
    RtlEnsureBufferSize(RTL_SKIP_BUFFER_COPY, &Buffer.ByteBuffer, Buffer.String.MaximumLength);
    memcpy(Buffer.ByteBuffer.Buffer, Buffer.String.Buffer, Buffer.String.MaximumLength);

    ok_hex2(pRtlNtPathNameToDosPathName(0, &Buffer, &Type, NULL), STATUS_SUCCESS);

    ok_hex2(Type, Entry->Type);
    ok_wstr2(Buffer.String.Buffer, Entry->OutputPath);
    /* If there is no change in the path, the pointer is unchanged */
    if (!wcscmp(Entry->InputPath, Entry->OutputPath))
    {
        ok_ptr2(Buffer.String.Buffer, Entry->InputPath);
    }
    else
    {
        /* If there is a change in the path, the 'ByteBuffer' is used */
        winetest_ok((PUCHAR)Buffer.String.Buffer >= Buffer.ByteBuffer.StaticBuffer &&
                    (PUCHAR)Buffer.String.Buffer <= (Buffer.ByteBuffer.StaticBuffer + Buffer.ByteBuffer.StaticSize),
                    "Expected Buffer to point inside StaticBuffer\n");
    }
    ok_wstr2((const WCHAR *)Buffer.ByteBuffer.Buffer, Entry->OutputPath);

    ok_hex2(Buffer.MinimumStaticBufferForTerminalNul, 0);

    /* For none of our tests should we exceed the StaticBuffer size! */
    ok_ptr2(Buffer.ByteBuffer.Buffer, Buffer.ByteBuffer.StaticBuffer);
    ok_hex2(Buffer.ByteBuffer.Size, Buffer.ByteBuffer.StaticSize);

    ok_hex2(Buffer.ByteBuffer.ReservedForAllocatedSize, 0);
    ok_ptr2(Buffer.ByteBuffer.ReservedForIMalloc, NULL);

    RtlFreeBuffer(&Buffer.ByteBuffer);
}


START_TEST(RtlNtPathNameToDosPathName)
{
    RTL_UNICODE_STRING_BUFFER Buffer = { { 0 } };
    ULONG Type;
    size_t n;

    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    pRtlNtPathNameToDosPathName = (void *)GetProcAddress(ntdll, "RtlNtPathNameToDosPathName");

    if (!pRtlNtPathNameToDosPathName)
    {
        skip("RtlNtPathNameToDosPathName not found?\n");
        return;
    }

    ok_ntstatus(pRtlNtPathNameToDosPathName(0, NULL, NULL, NULL), STATUS_INVALID_PARAMETER);
    ok_ntstatus(pRtlNtPathNameToDosPathName(0, &Buffer, NULL, NULL), STATUS_SUCCESS);
    ok_ntstatus(pRtlNtPathNameToDosPathName(1, &Buffer, NULL, NULL), STATUS_INVALID_PARAMETER);

    Type = 0x12345;
    ok_ntstatus(pRtlNtPathNameToDosPathName(0, NULL, &Type, NULL), STATUS_INVALID_PARAMETER);
    ok_int(Type, 0);
    Type = 0x12345;
    ok_ntstatus(pRtlNtPathNameToDosPathName(0, &Buffer, &Type, NULL), STATUS_SUCCESS);
    ok_int(Type, RTL_UNCHANGED_UNK_PATH);
    Type = 0x12345;
    ok_ntstatus(pRtlNtPathNameToDosPathName(1, &Buffer, &Type, NULL), STATUS_INVALID_PARAMETER);
    ok_int(Type, 0);

    test_specialhandling();

    for (n = 0; n < _countof(test_data); ++n)
    {
        winetest_set_location(test_data[n].File, test_data[n].Line);
        test_table(test_data + n);
    }
}
