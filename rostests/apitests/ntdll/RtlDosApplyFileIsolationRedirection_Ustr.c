/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlDosApplyFileIsolationRedirection_Ustr
 * PROGRAMMER:      Giannis Adamopoulos
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <ndk/rtlfuncs.h>

#define ok_eq_hex(value, expected) ok((value) == (expected), #value " = 0x%lx, expected 0x%lx\n", value, expected)
#define ok_eq_pointer(value, expected) ok((value) == (expected), #value " = %p, expected %p\n", value, expected)

UNICODE_STRING DotDll = RTL_CONSTANT_STRING(L".DLL");

void TestDefaultSxsRedirection(void)
{
    UNICODE_STRING GdiPlusSXS = RTL_CONSTANT_STRING(L"\\WinSxS\\x86_microsoft.windows.gdiplus_6595b64144ccf1df_1.");
    UNICODE_STRING Comctl32SXS = RTL_CONSTANT_STRING(L"\\WinSxS\\x86_microsoft.windows.common-controls_6595b64144ccf1df_5.82");
    UNICODE_STRING Comctl32 = RTL_CONSTANT_STRING(L"COMCTL32.DLL");
    UNICODE_STRING GdiPlus = RTL_CONSTANT_STRING(L"GDIPLUS.DLL");
    UNICODE_STRING CallerBuffer;
    UNICODE_STRING DynamicString;
    PUNICODE_STRING FullNameOut;
    USHORT Position;

    NTSTATUS Status;

    /* NOTE: in xp and 2k3 gdiplus does not exist in system32 */
    RtlInitUnicodeString(&CallerBuffer, NULL);
    RtlInitUnicodeString(&DynamicString, NULL);
    FullNameOut = NULL;
    Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                      &GdiPlus,
                                                      &DotDll,
                                                      &CallerBuffer,
                                                      &DynamicString,
                                                      &FullNameOut,
                                                      NULL,
                                                      NULL,
                                                      NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_pointer(CallerBuffer.Buffer, NULL);
    ok_eq_pointer(FullNameOut, &DynamicString);
    Status = RtlFindCharInUnicodeString(RTL_FIND_CHAR_IN_UNICODE_STRING_CASE_INSENSITIVE,
                                        &GdiPlusSXS,
                                        &DynamicString,
                                        &Position);
    ok_eq_hex(Status, STATUS_SUCCESS);
       
    
    RtlInitUnicodeString(&CallerBuffer, NULL);
    RtlInitUnicodeString(&DynamicString, NULL);
    FullNameOut = NULL;
    Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                      &Comctl32,
                                                      &DotDll,
                                                      &CallerBuffer,
                                                      &DynamicString,
                                                      &FullNameOut,
                                                      NULL,
                                                      NULL,
                                                      NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_pointer(CallerBuffer.Buffer, NULL);
    ok_eq_pointer(FullNameOut, &DynamicString);
    Status = RtlFindCharInUnicodeString(RTL_FIND_CHAR_IN_UNICODE_STRING_CASE_INSENSITIVE,
                                        &Comctl32SXS,
                                        &DynamicString,
                                        &Position);
    ok_eq_hex(Status, STATUS_SUCCESS);
}

void TestDotLocal(void)
{
}

START_TEST(RtlDosApplyFileIsolationRedirection_Ustr)
{
    TestDotLocal();
    TestDefaultSxsRedirection();
}