/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for RtlComputePrivatizedDllName_U
 * COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"


static WCHAR ProcessDir[MAX_PATH];
static WCHAR LocalDir[MAX_PATH + 10];

static BOOL InitTestData()
{
    static const UNICODE_STRING PathDividerFind = RTL_CONSTANT_STRING(L"\\/");
    UNICODE_STRING StrU;
    USHORT PathDivider;

    GetModuleFileNameW(NULL, ProcessDir, RTL_NUMBER_OF(ProcessDir));
    GetModuleFileNameW(NULL, LocalDir, RTL_NUMBER_OF(LocalDir));

    RtlInitUnicodeString(&StrU, ProcessDir);

    if (!NT_SUCCESS(RtlFindCharInUnicodeString(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END,
                                               &StrU, &PathDividerFind, &PathDivider)))
    {
        skip("Failed to find path divider\n");
        return FALSE;
    }
    ProcessDir[PathDivider / sizeof(WCHAR) + 1] = UNICODE_NULL;

    RtlInitUnicodeString(&StrU, LocalDir);
    StrU.MaximumLength = sizeof(LocalDir);

    if (!NT_SUCCESS(RtlAppendUnicodeToString(&StrU, L".Local\\")))
    {
        skip("Failed to append .Local\\\n");
        return FALSE;
    }
    return TRUE;
}

static void ok_strings_(PCUNICODE_STRING RealName, PCUNICODE_STRING LocalName, LPCWSTR DllName, int line)
{
    WCHAR ExpectReal[MAX_PATH*2];
    WCHAR ExpectLocal[MAX_PATH*2];
    int RealLen;
    int ExpectLen;

    RealLen = swprintf(ExpectReal, L"%s%s", ProcessDir, DllName) * sizeof(WCHAR);
    ExpectLen = swprintf(ExpectLocal, L"%s%s", LocalDir, DllName) * sizeof(WCHAR);

    ok_(__FILE__, line)(RealLen == RealName->Length, "Expected Real %u, got %u\n",
                        (UINT)RealLen, (UINT)RealName->Length);
    ok_(__FILE__, line)(ExpectLen == LocalName->Length, "Expected Local %u, got %u\n",
                        (UINT)ExpectLen, (UINT)LocalName->Length);

    ok_(__FILE__, line)(!wcscmp(RealName->Buffer, ExpectReal), "Expected Real %s, got %s\n",
                        wine_dbgstr_w(ExpectReal), wine_dbgstr_w(RealName->Buffer));
    ok_(__FILE__, line)(!wcscmp(LocalName->Buffer, ExpectLocal), "Expected Local %s, got %s\n",
                        wine_dbgstr_w(ExpectLocal), wine_dbgstr_w(LocalName->Buffer));
}
#define ok_strings(Real, Local, Dll)    ok_strings_(Real, Local, Dll, __LINE__)


static void cleanup(PUNICODE_STRING String, WCHAR* Buffer, USHORT BufferSize)
{
    if (String->Buffer != Buffer)
    {
        RtlFreeUnicodeString(String);
        RtlInitEmptyUnicodeString(String, Buffer, BufferSize);
    }
}


static void test_dllnames(void)
{
    WCHAR Buf1[MAX_PATH];
    WCHAR Buf2[MAX_PATH];

    UNICODE_STRING Str1, Str2;
    UNICODE_STRING DllName;
    NTSTATUS Status;

    RtlInitEmptyUnicodeString(&Str1, Buf1, sizeof(Buf1));
    RtlInitEmptyUnicodeString(&Str2, Buf2, sizeof(Buf2));

    RtlInitUnicodeString(&DllName, L"kernel32.dll");
    Status = RtlComputePrivatizedDllName_U(&DllName, &Str1, &Str2);
    ok(Status == STATUS_SUCCESS, "0x%lx\n", Status);
    ok_strings(&Str1, &Str2, L"kernel32.dll");
    cleanup(&Str1, Buf1, sizeof(Buf1));
    cleanup(&Str2, Buf2, sizeof(Buf2));


    RtlInitUnicodeString(&DllName, L"kernel32");
    Status = RtlComputePrivatizedDllName_U(&DllName, &Str1, &Str2);
    ok(Status == STATUS_SUCCESS, "0x%lx\n", Status);
    ok_strings(&Str1, &Str2, L"kernel32.DLL");
    cleanup(&Str1, Buf1, sizeof(Buf1));
    cleanup(&Str2, Buf2, sizeof(Buf2));

    RtlInitUnicodeString(&DllName, L"kernel32.dll.dll");
    Status = RtlComputePrivatizedDllName_U(&DllName, &Str1, &Str2);
    ok(Status == STATUS_SUCCESS, "0x%lx\n", Status);
    ok_strings(&Str1, &Str2, L"kernel32.dll.dll");
    cleanup(&Str1, Buf1, sizeof(Buf1));
    cleanup(&Str2, Buf2, sizeof(Buf2));

    RtlInitUnicodeString(&DllName, L"kernel32.dll.exe");
    Status = RtlComputePrivatizedDllName_U(&DllName, &Str1, &Str2);
    ok(Status == STATUS_SUCCESS, "0x%lx\n", Status);
    ok_strings(&Str1, &Str2, L"kernel32.dll.exe");
    cleanup(&Str1, Buf1, sizeof(Buf1));
    cleanup(&Str2, Buf2, sizeof(Buf2));

    RtlInitUnicodeString(&DllName, L"kernel32.");
    Status = RtlComputePrivatizedDllName_U(&DllName, &Str1, &Str2);
    ok(Status == STATUS_SUCCESS, "0x%lx\n", Status);
    ok_strings(&Str1, &Str2, L"kernel32.");
    cleanup(&Str1, Buf1, sizeof(Buf1));
    cleanup(&Str2, Buf2, sizeof(Buf2));

    RtlInitUnicodeString(&DllName, L".kernel32");
    Status = RtlComputePrivatizedDllName_U(&DllName, &Str1, &Str2);
    ok(Status == STATUS_SUCCESS, "0x%lx\n", Status);
    ok_strings(&Str1, &Str2, L".kernel32.DLL");
    cleanup(&Str1, Buf1, sizeof(Buf1));
    cleanup(&Str2, Buf2, sizeof(Buf2));

    RtlInitUnicodeString(&DllName, L"..kernel32");
    Status = RtlComputePrivatizedDllName_U(&DllName, &Str1, &Str2);
    ok(Status == STATUS_SUCCESS, "0x%lx\n", Status);
    ok_strings(&Str1, &Str2, L"..kernel32");
    cleanup(&Str1, Buf1, sizeof(Buf1));
    cleanup(&Str2, Buf2, sizeof(Buf2));

    RtlInitUnicodeString(&DllName, L".kernel32.");
    Status = RtlComputePrivatizedDllName_U(&DllName, &Str1, &Str2);
    ok(Status == STATUS_SUCCESS, "0x%lx\n", Status);
    ok_strings(&Str1, &Str2, L".kernel32.");
    cleanup(&Str1, Buf1, sizeof(Buf1));
    cleanup(&Str2, Buf2, sizeof(Buf2));


    RtlInitUnicodeString(&DllName, L"test\\kernel32.dll");
    Status = RtlComputePrivatizedDllName_U(&DllName, &Str1, &Str2);
    ok(Status == STATUS_SUCCESS, "0x%lx\n", Status);
    ok_strings(&Str1, &Str2, L"kernel32.dll");
    cleanup(&Str1, Buf1, sizeof(Buf1));
    cleanup(&Str2, Buf2, sizeof(Buf2));

    RtlInitUnicodeString(&DllName, L"test/kernel32.dll");
    Status = RtlComputePrivatizedDllName_U(&DllName, &Str1, &Str2);
    ok(Status == STATUS_SUCCESS, "0x%lx\n", Status);
    ok_strings(&Str1, &Str2, L"kernel32.dll");
    cleanup(&Str1, Buf1, sizeof(Buf1));
    cleanup(&Str2, Buf2, sizeof(Buf2));

    RtlInitUnicodeString(&DllName, L"test.dll/kernel32");
    Status = RtlComputePrivatizedDllName_U(&DllName, &Str1, &Str2);
    ok(Status == STATUS_SUCCESS, "0x%lx\n", Status);
    ok_strings(&Str1, &Str2, L"kernel32.DLL");
    cleanup(&Str1, Buf1, sizeof(Buf1));
    cleanup(&Str2, Buf2, sizeof(Buf2));

    RtlInitUnicodeString(&DllName, L"test.dll\\kernel32");
    Status = RtlComputePrivatizedDllName_U(&DllName, &Str1, &Str2);
    ok(Status == STATUS_SUCCESS, "0x%lx\n", Status);
    ok_strings(&Str1, &Str2, L"kernel32.DLL");
    cleanup(&Str1, Buf1, sizeof(Buf1));
    cleanup(&Str2, Buf2, sizeof(Buf2));

    RtlInitUnicodeString(&DllName, L"//");
    Status = RtlComputePrivatizedDllName_U(&DllName, &Str1, &Str2);
    ok(Status == STATUS_SUCCESS, "0x%lx\n", Status);
    ok_strings(&Str1, &Str2, L".DLL");
    cleanup(&Str1, Buf1, sizeof(Buf1));
    cleanup(&Str2, Buf2, sizeof(Buf2));

    // Bug :)
    RtlInitUnicodeString(&DllName, L"\\");
    Status = RtlComputePrivatizedDllName_U(&DllName, &Str1, &Str2);
    ok(Status == STATUS_SUCCESS, "0x%lx\n", Status);
    if (wcsstr(Str1.Buffer, L"\\\\"))
    {
        trace("Allowing bug found in windows' implementation\n");
        ok_strings(&Str1, &Str2, L"\\.DLL");
    }
    else
    {
        ok_strings(&Str1, &Str2, L".DLL");
    }
    cleanup(&Str1, Buf1, sizeof(Buf1));
    cleanup(&Str2, Buf2, sizeof(Buf2));

    RtlInitUnicodeString(&DllName, L"");
    Status = RtlComputePrivatizedDllName_U(&DllName, &Str1, &Str2);
    ok(Status == STATUS_SUCCESS, "0x%lx\n", Status);
    ok_strings(&Str1, &Str2, L".DLL");
    cleanup(&Str1, Buf1, sizeof(Buf1));
    cleanup(&Str2, Buf2, sizeof(Buf2));
}

static void test_allocations(void)
{
    WCHAR Buf1[10];
    WCHAR Buf2[10];

    UNICODE_STRING Str1, Str2;
    UNICODE_STRING DllName;
    NTSTATUS Status;

    RtlInitEmptyUnicodeString(&Str1, Buf1, sizeof(Buf1));
    RtlInitEmptyUnicodeString(&Str2, Buf2, sizeof(Buf2));

    RtlInitUnicodeString(&DllName, L"kernel32.dll");
    Status = RtlComputePrivatizedDllName_U(&DllName, &Str1, &Str2);
    ok(Status == STATUS_SUCCESS, "0x%lx\n", Status);
    ok_strings(&Str1, &Str2, L"kernel32.dll");
    ok(Str1.Buffer != Buf1, "Expected a changed buffer\n");
    ok(Str2.Buffer != Buf2, "Expected a changed buffer\n");
    cleanup(&Str1, Buf1, sizeof(Buf1));
    cleanup(&Str2, Buf2, sizeof(Buf2));
}


START_TEST(RtlComputePrivatizedDllName_U)
{
    if (!InitTestData())
        return;

    test_dllnames();
    test_allocations();
}
