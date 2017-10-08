/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for the HKEY_CLASSES_ROOT key
 * PROGRAMMER:      Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <winreg.h>
#include <ndk/rtlfuncs.h>
#include <ndk/cmfuncs.h>
#include <ndk/cmtypes.h>

#define IS_HKCR(hk) (((UINT_PTR)hk & 3) == 2)

/* HKCU and HKLM system paths */
static UNICODE_STRING HKLM_ClassesPath = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Classes");
static UNICODE_STRING HKCU_ClassesPath;

static
LONG
DeleteSubKey(HKEY hKey, LPWSTR Root, LPWSTR SubKey)
{
    HKEY RootKey;
    LONG ErrorCode;

    ErrorCode = RegOpenKeyExW(
        hKey,
        Root,
        0,
        MAXIMUM_ALLOWED,
        &RootKey);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ErrorCode = RegDeleteKeyW(RootKey, SubKey);
    RegCloseKey(RootKey);

    return ErrorCode;
}

static
void
GetKeyName(HKEY hKey, PUNICODE_STRING KeyName)
{
    UNICODE_STRING InfoName;
    PKEY_NAME_INFORMATION NameInformation;
    ULONG InfoLength;
    NTSTATUS Status;

    /* Get info length */
    InfoLength = 0;
    Status = NtQueryKey(hKey, KeyNameInformation, NULL, 0, &InfoLength);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);

    /* Get it for real */
    NameInformation = RtlAllocateHeap(RtlGetProcessHeap(), 0, InfoLength);
    ok(NameInformation != NULL, "\n");

    Status = NtQueryKey(hKey, KeyNameInformation, NameInformation, InfoLength, &InfoLength);
    ok_ntstatus(Status, STATUS_SUCCESS);

    InfoName.Buffer = NameInformation->Name;
    InfoName.Length = NameInformation->NameLength;
    InfoName.MaximumLength = InfoName.Length;

    RtlDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE, &InfoName, KeyName);

    RtlFreeHeap(RtlGetProcessHeap(), 0, NameInformation);
}

static void _test_key_deleted(HKEY hKey, BOOL Deleted, ULONG LineNumber)
{
    DWORD ErrorCode = RegQueryInfoKeyW(hKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if (Deleted)
        ok_(__FILE__, LineNumber)(ErrorCode == ERROR_KEY_DELETED, "\n");
    else
        ok_(__FILE__, LineNumber)(ErrorCode == ERROR_SUCCESS, "\n");
}
#define ok_key_deleted(hKey) _test_key_deleted(hKey, TRUE, __LINE__)
#define ok_key_not_deleted(hKey) _test_key_deleted(hKey, FALSE, __LINE__)

static void _test_key_name(HKEY hKey, PUNICODE_STRING Prefix, LPCWSTR Name, ULONG LineNumber)
{
    UNICODE_STRING ExpectedName, KeyName;
    WCHAR Buffer[1024];

    ExpectedName.Length = 0;
    ExpectedName.MaximumLength = sizeof(Buffer);
    ExpectedName.Buffer = Buffer;

    RtlAppendUnicodeStringToString(&ExpectedName, Prefix);
    RtlAppendUnicodeToString(&ExpectedName, L"\\");
    RtlAppendUnicodeToString(&ExpectedName, Name);

    GetKeyName(hKey, &KeyName);

    ok_(__FILE__, LineNumber)(RtlCompareUnicodeString(&KeyName, &ExpectedName, TRUE) == 0,
            "Key name is %.*S, expected %.*S\n",
            KeyName.Length, KeyName.Buffer,
            ExpectedName.Length, ExpectedName.Buffer);

    RtlFreeUnicodeString(&KeyName);
}

#define ok_key_name(hKey, Prefix, Name) _test_key_name(hKey, Prefix, Name, __LINE__)

static
void
Test_CreateOpenKey(void)
{
    HKEY MachineKey, MachineSubKey;
    HKEY UserKey, UserSubKey;
    HKEY ClassesRootKey, ClassesRootSubKey;
    DWORD ErrorCode;
    DWORD Disposition;

    /* First create a subkey in HKLM */
    ErrorCode = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        L"Software\\Classes\\Apitest_HKLM",
        0,
        NULL,
        0,
        MAXIMUM_ALLOWED,
        NULL,
        &MachineKey,
        NULL);

    if (ErrorCode == ERROR_ACCESS_DENIED)
    {
        win_skip("Please run those tests with Administrator rights\n");
        return;
    }

    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(!IS_HKCR(MachineKey), "\n");

    /* Open it in HKCR */
    ErrorCode = RegOpenKeyExW(
        HKEY_CLASSES_ROOT,
        L"Apitest_HKLM",
        0,
        MAXIMUM_ALLOWED,
        &ClassesRootKey);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(IS_HKCR(ClassesRootKey), "\n");
    ok_key_name(ClassesRootKey, &HKLM_ClassesPath, L"Apitest_HKLM");

    /* Try opening it in HKCU */
    UserKey = (HKEY)0xCAFEF00D;
    ErrorCode = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Classes\\Apitest_HKLM",
        0,
        MAXIMUM_ALLOWED,
        &UserKey);
    ok_dec(ErrorCode, ERROR_FILE_NOT_FOUND);
    ok_hdl(UserKey, NULL);

    /* Cleanup */
    RegCloseKey(ClassesRootKey);
    RegCloseKey(MachineKey);
    ErrorCode = DeleteSubKey(HKEY_LOCAL_MACHINE, L"Software\\Classes", L"Apitest_HKLM");
    ok_dec(ErrorCode, ERROR_SUCCESS);

    /* Try creating in HKCR */
    ErrorCode = RegCreateKeyExW(
        HKEY_CLASSES_ROOT,
        L"Apitest_HKCR",
        0,
        NULL,
        0,
        MAXIMUM_ALLOWED,
        NULL,
        &ClassesRootKey,
        NULL);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(IS_HKCR(ClassesRootKey), "\n");
    ok_key_name(ClassesRootKey, &HKLM_ClassesPath, L"Apitest_HKCR");

    /* It should be present in HKLM */
    ErrorCode = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"Software\\Classes\\Apitest_HKCR",
        0,
        MAXIMUM_ALLOWED,
        &MachineKey);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(!IS_HKCR(MachineKey), "\n");

    /* But not in HKCU */
    UserKey = (HKEY)0xCAFEF00D;
    ErrorCode = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Classes\\Apitest_HKCR",
        0,
        MAXIMUM_ALLOWED,
        &UserKey);
    ok_dec(ErrorCode, ERROR_FILE_NOT_FOUND);
    ok_hdl(UserKey, NULL);

    /* This must delete the one in HKLM */
    ErrorCode = RegDeleteKeyW(HKEY_CLASSES_ROOT, L"Apitest_HKCR");
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok_key_deleted(ClassesRootKey);
    ok_key_deleted(MachineKey);
    RegCloseKey(ClassesRootKey);
    RegCloseKey(MachineKey);

    /* Test that it is really not present anymore */
    MachineKey = (HKEY)0xCAFEF00D;
    ErrorCode = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"Software\\Classes\\Apitest_HKCR",
        0,
        MAXIMUM_ALLOWED,
        &MachineKey);
    ok_dec(ErrorCode, ERROR_FILE_NOT_FOUND);
    ok_hdl(MachineKey, NULL);

    /* Try creating in HKCU */
    ErrorCode = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Classes\\Apitest_HKCU",
        0,
        NULL,
        0,
        MAXIMUM_ALLOWED,
        NULL,
        &UserKey,
        NULL);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(!IS_HKCR(UserKey), "\n");

    /* Try opening it in HKCR */
    ErrorCode = RegOpenKeyExW(
        HKEY_CLASSES_ROOT,
        L"Apitest_HKCU",
        0,
        MAXIMUM_ALLOWED,
        &ClassesRootKey);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(IS_HKCR(ClassesRootKey), "\n");
    ok_key_name(ClassesRootKey, &HKCU_ClassesPath, L"Apitest_HKCU");

    /* And in HKLM */
    ErrorCode = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"Software\\Classes\\Apitest_HKCU",
        0,
        MAXIMUM_ALLOWED,
        &MachineKey);
    ok_dec(ErrorCode, ERROR_FILE_NOT_FOUND);
    ok_hdl(MachineKey, NULL);

    ErrorCode = RegDeleteKeyW(HKEY_CLASSES_ROOT, L"Apitest_HKCU");
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok_key_deleted(ClassesRootKey);
    ok_key_deleted(UserKey);
    RegCloseKey(ClassesRootKey);
    RegCloseKey(UserKey);

    /* Test that it is really not present anymore */
    UserKey = (HKEY)0xCAFEF00D;
    ErrorCode = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Classes\\Apitest_HKCU",
        0,
        MAXIMUM_ALLOWED,
        &UserKey);
    ok_dec(ErrorCode, ERROR_FILE_NOT_FOUND);
    ok_hdl(UserKey, NULL);

    /* Try creating in both HKLM and HKCU */
    ErrorCode = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Classes\\Apitest_HKLM_HKCU",
        0,
        NULL,
        0,
        MAXIMUM_ALLOWED,
        NULL,
        &UserKey,
        NULL);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(!IS_HKCR(UserKey), "\n");

    ErrorCode = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        L"Software\\Classes\\Apitest_HKLM_HKCU",
        0,
        NULL,
        0,
        MAXIMUM_ALLOWED,
        NULL,
        &MachineKey,
        NULL);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(!IS_HKCR(MachineKey), "\n");

    /* Open it in HKCR */
    ErrorCode = RegOpenKeyExW(
        HKEY_CLASSES_ROOT,
        L"Apitest_HKLM_HKCU",
        0,
        MAXIMUM_ALLOWED,
        &ClassesRootKey);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(IS_HKCR(ClassesRootKey), "\n");
    /* Verify it has opened the HKCU one */
    ok_key_name(ClassesRootKey, &HKCU_ClassesPath, L"Apitest_HKLM_HKCU");

    /* Try the same thing, but this time with RegCreateKeyEx API */
    RegCloseKey(ClassesRootKey);
    ErrorCode = RegCreateKeyExW(
        HKEY_CLASSES_ROOT,
        L"Apitest_HKLM_HKCU",
        0,
        NULL,
        0,
        MAXIMUM_ALLOWED,
        NULL,
        &ClassesRootKey,
        &Disposition);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(IS_HKCR(ClassesRootKey), "\n");
    /* Verify it has opened the HKCU one */
    ok_key_name(ClassesRootKey, &HKCU_ClassesPath, L"Apitest_HKLM_HKCU");
    ok_hex(Disposition, REG_OPENED_EXISTING_KEY);

    /* Deleting it from HKCR first deletes the one in HKCU */
    ErrorCode = RegDeleteKeyW(HKEY_CLASSES_ROOT, L"Apitest_HKLM_HKCU");
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok_key_deleted(UserKey);
    ok_key_deleted(ClassesRootKey);
    ok_key_not_deleted(MachineKey);

    RegCloseKey(UserKey);
    RegCloseKey(ClassesRootKey);

    /* This deletes it from HKLM this time */
    ErrorCode = RegDeleteKeyW(HKEY_CLASSES_ROOT, L"Apitest_HKLM_HKCU");
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok_key_deleted(MachineKey);
    RegCloseKey(MachineKey);

    /* See what happens with subkeys */
    ErrorCode = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        L"Software\\Classes\\Apitest_HKLM",
        0,
        NULL,
        0,
        MAXIMUM_ALLOWED,
        NULL,
        &MachineKey,
        NULL);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(!IS_HKCR(MachineKey), "\n");

    /* Open it in HKCR */
    ErrorCode = RegOpenKeyExW(
        HKEY_CLASSES_ROOT,
        L"Apitest_HKLM",
        0,
        MAXIMUM_ALLOWED,
        &ClassesRootKey);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(IS_HKCR(ClassesRootKey), "\n");
    ok_key_name(ClassesRootKey, &HKLM_ClassesPath, L"Apitest_HKLM");

    /* Create a corresponding subkey in HKCU */
    ErrorCode = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Classes\\Apitest_HKLM\\HKCU_Subkey",
        0,
        NULL,
        0,
        MAXIMUM_ALLOWED,
        NULL,
        &UserSubKey,
        NULL);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(!IS_HKCR(UserSubKey), "\n");

    /* Open it as an HKCR subkey */
    ErrorCode = RegOpenKeyExW(
        ClassesRootKey,
        L"HKCU_Subkey",
        0,
        MAXIMUM_ALLOWED,
        &ClassesRootSubKey);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(IS_HKCR(ClassesRootSubKey), "\n");
    ok_key_name(ClassesRootSubKey, &HKCU_ClassesPath, L"Apitest_HKLM\\HKCU_Subkey");

    /* Try the same thing, but this time with RegCreateKeyEx API */
    RegCloseKey(ClassesRootSubKey);
    ErrorCode = RegCreateKeyExW(
        ClassesRootKey,
        L"HKCU_Subkey",
        0,
        NULL,
        0,
        MAXIMUM_ALLOWED,
        NULL,
        &ClassesRootSubKey,
        &Disposition);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(IS_HKCR(ClassesRootSubKey), "\n");
    /* Verify it has opened the HKCU one */
    ok_key_name(ClassesRootSubKey, &HKCU_ClassesPath, L"Apitest_HKLM\\HKCU_Subkey");
    ok_hex(Disposition, REG_OPENED_EXISTING_KEY);

    /* This one now exists */
    ErrorCode = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Classes\\Apitest_HKLM",
        0,
        MAXIMUM_ALLOWED,
        &UserKey);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(!IS_HKCR(UserKey), "\n");

    /* Delete */
    ErrorCode = RegDeleteKeyW(UserKey, L"HKCU_Subkey");
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok_key_deleted(UserSubKey);
    ok_key_deleted(ClassesRootSubKey);
    RegCloseKey(UserSubKey);
    RegCloseKey(ClassesRootSubKey);

    /* See what this deletes */
    RegDeleteKeyW(HKEY_CLASSES_ROOT, L"Apitest_HKLM");
    ok_key_deleted(UserKey);
    RegCloseKey(UserKey);
    ok_key_not_deleted(ClassesRootKey);
    ok_key_not_deleted(MachineKey);

    /* Once again */
    RegDeleteKeyW(HKEY_CLASSES_ROOT, L"Apitest_HKLM");
    ok_key_deleted(ClassesRootKey);
    ok_key_deleted(MachineKey);
    RegCloseKey(ClassesRootKey);
    RegCloseKey(MachineKey);

    /* Same, but with HKCU first */
    ErrorCode = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Classes\\Apitest_HKCU",
        0,
        NULL,
        0,
        MAXIMUM_ALLOWED,
        NULL,
        &UserKey,
        NULL);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(!IS_HKCR(UserKey), "\n");

    /* Open it in HKCR */
    ErrorCode = RegOpenKeyExW(
        HKEY_CLASSES_ROOT,
        L"Apitest_HKCU",
        0,
        MAXIMUM_ALLOWED,
        &ClassesRootKey);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(IS_HKCR(ClassesRootKey), "\n");
    ok_key_name(ClassesRootKey, &HKCU_ClassesPath, L"Apitest_HKCU");

    /* Try creating a subkey with this HKCR handle, which points to a subkey in HKCU. */
    ErrorCode = RegCreateKeyExW(
        ClassesRootKey,
        L"HKCR_Subkey",
        0,
        NULL,
        0,
        MAXIMUM_ALLOWED,
        NULL,
        &ClassesRootSubKey,
        NULL);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(IS_HKCR(ClassesRootSubKey), "\n");
    /* It is in fact created in HKLM */
    ok_key_name(ClassesRootSubKey, &HKLM_ClassesPath, L"Apitest_HKCU\\HKCR_Subkey");
    /* Let's see if we can delete it */
    RegDeleteKeyW(ClassesRootKey, L"HKCR_Subkey");
    ok_key_deleted(ClassesRootSubKey);
    RegCloseKey(ClassesRootSubKey);

    /* Create a corresponding subkey in HKLM */
    ErrorCode = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        L"Software\\Classes\\Apitest_HKCU\\HKLM_Subkey",
        0,
        NULL,
        0,
        MAXIMUM_ALLOWED,
        NULL,
        &MachineSubKey,
        NULL);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(!IS_HKCR(MachineSubKey), "\n");

    /* Open it from the HKCR handle (which is still pointing to HKCU) */
    ok_key_name(ClassesRootKey, &HKCU_ClassesPath, L"Apitest_HKCU");
    ErrorCode = RegOpenKeyExW(
        ClassesRootKey,
        L"HKLM_Subkey",
        0,
        MAXIMUM_ALLOWED,
        &ClassesRootSubKey);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(IS_HKCR(ClassesRootSubKey), "\n");
    ok_key_name(ClassesRootSubKey, &HKLM_ClassesPath, L"Apitest_HKCU\\HKLM_Subkey");

    /* This one now exists */
    ErrorCode = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"Software\\Classes\\Apitest_HKCU",
        0,
        MAXIMUM_ALLOWED,
        &MachineKey);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(!IS_HKCR(MachineKey), "\n");

    /* Delete this subkey */
    ErrorCode = RegDeleteKeyW(MachineKey, L"HKLM_Subkey");
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok_key_deleted(MachineSubKey);
    ok_key_deleted(ClassesRootSubKey);

    /* Create another subkey, this time from HKCU */
    ErrorCode = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Classes\\Apitest_HKCU\\HKCU_Subkey",
        0,
        NULL,
        0,
        MAXIMUM_ALLOWED,
        NULL,
        &UserSubKey,
        NULL);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(!IS_HKCR(UserSubKey), "\n");

    /* And try creating it again as a subkey of this HKCR handle (which points to HKCU). */
    ok_key_name(ClassesRootKey, &HKCU_ClassesPath, L"Apitest_HKCU");
    ErrorCode = RegCreateKeyExW(
        ClassesRootKey,
        L"HKCU_Subkey",
        0,
        NULL,
        0,
        MAXIMUM_ALLOWED,
        NULL,
        &ClassesRootSubKey,
        &Disposition);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(IS_HKCR(ClassesRootSubKey), "\n");
    /* This time the one in HKCU is opened */
    ok_key_name(ClassesRootSubKey, &HKCU_ClassesPath, L"Apitest_HKCU\\HKCU_Subkey");
    ok_hex(Disposition, REG_OPENED_EXISTING_KEY);
    /* Let's see if we can delete it */
    RegDeleteKeyW(ClassesRootKey, L"HKCU_Subkey");
    ok_key_deleted(ClassesRootSubKey);
    RegCloseKey(ClassesRootSubKey);
    ok_key_deleted(UserSubKey);
    RegCloseKey(UserSubKey);

    RegCloseKey(MachineSubKey);
    RegCloseKey(ClassesRootSubKey);

    /* See what this deletes */
    RegDeleteKeyW(HKEY_CLASSES_ROOT, L"Apitest_HKCU");
    ok_key_deleted(UserKey);
    RegCloseKey(UserKey);
    ok_key_deleted(ClassesRootKey);
    RegCloseKey(UserKey);
    ok_key_not_deleted(MachineKey);

    /* Once again */
    RegDeleteKeyW(HKEY_CLASSES_ROOT, L"Apitest_HKCU");
    ok_key_deleted(MachineKey);
    RegCloseKey(MachineKey);
}

static
void
Test_DuplicateHandle(void)
{
    HKEY KeyHandle, DupHandle;
    DWORD ErrorCode;
    BOOL Duplicated;

    ErrorCode = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"CLSID", 0, KEY_READ, &KeyHandle);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok(IS_HKCR(KeyHandle), "\n");

    Duplicated = DuplicateHandle(GetCurrentProcess(), KeyHandle, GetCurrentProcess(), (PHANDLE)&DupHandle, 0, 0, DUPLICATE_SAME_ACCESS);
    ok(Duplicated, "\n");
    ok(DupHandle != NULL, "\n");
    ok(!IS_HKCR(DupHandle), "\n");

    RegCloseKey(KeyHandle);
    RegCloseKey(DupHandle);
}

START_TEST(HKEY_CLASSES_ROOT)
{
    HKEY UserKey;
    LONG ErrorCode;

    /* Get HKCU real key name */
    ErrorCode = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Classes",
        0,
        KEY_READ,
        &UserKey);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    GetKeyName(UserKey, &HKCU_ClassesPath);
    RegCloseKey(UserKey);

    Test_CreateOpenKey();
    Test_DuplicateHandle();
}
