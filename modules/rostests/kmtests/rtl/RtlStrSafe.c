/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for ntstrsafe.h functions
 * PROGRAMMER:      Hernán Di Pietro <hernan.di.pietro@gmail.com>
 */

#define KMT_EMULATE_KERNEL
#include <kmt_test.h>
#include <ntstrsafe.h>
#include <ntdef.h>
#include <ndk/rtlfuncs.h>

static void Test_RtlUnicodeStringPrintf()
{
    WCHAR Buffer[1024] = { 0 };
    UNICODE_STRING UsString;
    WCHAR FormatStringInts[] = L"%d %d %d";
    
    UsString.Buffer = Buffer;
    UsString.Length = 0;
    UsString.MaximumLength = sizeof(Buffer);
    
    const WCHAR Result[] = L"1 2 3";
    ok_eq_bool(RtlUnicodeStringPrintf(&UsString, FormatStringInts, 1, 2, 3), STATUS_SUCCESS);
    ok_eq_uint(UsString.Length, sizeof(Result));
    ok_eq_uint(UsString.MaximumLength, sizeof(Buffer));
    ok_eq_wstr(UsString.Buffer, Result);
    

}

static void Test_RtlUnicodeStringPrintfEx()
{

}

static void Test_RtlUnicodeStringValidate()
{
    
}

START_TEST(RtlStrSafe)
{
    Test_RtlUnicodeStringPrintf();
    Test_RtlUnicodeStringPrintfEx();
}
