/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for CreateService
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <winreg.h>
#include <ndk/rtlfuncs.h>
#include <ndk/cmfuncs.h>
#include <ndk/cmtypes.h>

#define IS_HKCR(hk) ((UINT_PTR)hk > 0 && ((UINT_PTR)hk & 3) == 2)

static
void
Test_KeyInformation(void)
{
    HKEY KeyHandle;
    DWORD ErrorCode;
    NTSTATUS Status;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Classes\\CLSID");
    UNICODE_STRING InfoName;
    CHAR Buffer[FIELD_OFFSET(KEY_NAME_INFORMATION, Name[512])];
    PKEY_NAME_INFORMATION NameInfo = (PKEY_NAME_INFORMATION)Buffer;
    ULONG ResultLength;


    ErrorCode = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"CLSID", 0, KEY_READ, &KeyHandle);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(IS_HKCR(KeyHandle), "\n");

    Status = NtQueryKey(KeyHandle, KeyNameInformation, NameInfo, sizeof(Buffer), &ResultLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    RtlInitUnicodeString(&InfoName, NameInfo->Name);
    InfoName.Length = NameInfo->NameLength;
    ok(RtlCompareUnicodeString(&KeyName, &InfoName, TRUE) == 0, "%.*S\n", InfoName.Length, InfoName.Buffer);

    RegCloseKey(KeyHandle);
}

static
void
Test_DuplicateHandle(void)
{
    HKEY KeyHandle, DupHandle;
    DWORD ErrorCode;
    BOOL Duplicated;
    NTSTATUS Status;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Classes\\CLSID");
    UNICODE_STRING InfoName;
    CHAR Buffer[FIELD_OFFSET(KEY_NAME_INFORMATION, Name[512])];
    PKEY_NAME_INFORMATION NameInfo = (PKEY_NAME_INFORMATION)Buffer;
    ULONG ResultLength;


    ErrorCode = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"CLSID", 0, KEY_READ, &KeyHandle);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(IS_HKCR(KeyHandle), "\n");

    Duplicated = DuplicateHandle(GetCurrentProcess(), KeyHandle, GetCurrentProcess(), (PHANDLE)&DupHandle, 0, 0, DUPLICATE_SAME_ACCESS);
    ok(Duplicated, "\n");
    ok(DupHandle != NULL, "\n");
    ok(!IS_HKCR(DupHandle), "\n");

    Status = NtQueryKey(DupHandle, KeyNameInformation, NameInfo, sizeof(Buffer), &ResultLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    RtlInitUnicodeString(&InfoName, NameInfo->Name);
    InfoName.Length = NameInfo->NameLength;
    ok(RtlCompareUnicodeString(&KeyName, &InfoName, TRUE) == 0, "%S\n", NameInfo->Name);

    RegCloseKey(KeyHandle);
    RegCloseKey(DupHandle);
}

START_TEST(HKEY_CLASSES_ROOT)
{
    Test_KeyInformation();
    Test_DuplicateHandle();
}
