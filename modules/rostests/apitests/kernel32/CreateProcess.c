/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test spoiling of StaticUnicodeString by CreateProcessA
 * PROGRAMMERS:     Mark Jansen
 */

#include "precomp.h"

#include <ndk/rtlfuncs.h>

START_TEST(CreateProcess)
{
    PUNICODE_STRING StaticString;
    UNICODE_STRING CompareString;
    BOOL Process;
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    LONG Result;

    StaticString = &NtCurrentTeb()->StaticUnicodeString;
    RtlInitUnicodeString(&CompareString, L"--sentinel--");
    RtlCopyUnicodeString(StaticString, &CompareString);

    si.cb = sizeof(si);
    Process = CreateProcessA("ApplicationName", "CommandLine", NULL, NULL, FALSE, 0, NULL, "CurrentDir", &si, &pi);
    ok_int(Process, 0);

    Result = RtlCompareUnicodeString(StaticString, &CompareString, TRUE);
    ok(!Result, "Expected %s to equal %s\n",
       wine_dbgstr_wn(StaticString->Buffer, StaticString->Length / sizeof(WCHAR)),
       wine_dbgstr_wn(CompareString.Buffer, CompareString.Length / sizeof(WCHAR)));
}
