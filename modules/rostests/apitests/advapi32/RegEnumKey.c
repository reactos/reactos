/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for the RegEnumKey API
 * PROGRAMMER:      Thomas Faber & Doug Lyons
 */

#include "precomp.h"

START_TEST(RegEnumKey)
{
    LONG ErrorCode;
    HKEY TestKey;
    HKEY hKey;
    WCHAR nameBuf[4];
    DWORD nameLen;

    /* Base key for our test */
    ErrorCode = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\ReactOS_apitest", 0, NULL, 0, KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS, NULL, &hKey, NULL);
    ok_dec(ErrorCode, ERROR_SUCCESS);

    /* Create 1 char subkey */
    ErrorCode = RegCreateKeyExW(hKey, L"1", 0, NULL, 0, READ_CONTROL, NULL, &TestKey, NULL);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    RegCloseKey(TestKey);

    /* Enumerate first key with space for 1 char */
    nameLen = 1;
    FillMemory(nameBuf, sizeof(nameBuf), 0x55);
    ErrorCode = RegEnumKeyExW(hKey, 0, nameBuf, &nameLen, NULL, NULL, NULL, NULL);
    ok_dec(ErrorCode, ERROR_MORE_DATA);
    ok_hex(nameBuf[0], 0x5555);
    ok_hex(nameBuf[1], 0x5555);
    ok_dec(nameLen, 1);

    /* Enumerate first key with space for 2 chars */
    nameLen = 2;
    FillMemory(nameBuf, sizeof(nameBuf), 0x55);
    ErrorCode = RegEnumKeyExW(hKey, 0, nameBuf, &nameLen, NULL, NULL, NULL, NULL);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok_hex(nameBuf[0], L'1');
    ok_hex(nameBuf[1], 0);
    ok_hex(nameBuf[2], 0x5555);
    ok_dec(nameLen, 1);

    /* Delete the subkey */
    ErrorCode = RegDeleteKeyW(hKey, L"1");
    ok_dec(ErrorCode, ERROR_SUCCESS);

    /* Create 2 char subkey */
    ErrorCode = RegCreateKeyExW(hKey, L"12", 0, NULL, 0, READ_CONTROL, NULL, &TestKey, NULL);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    RegCloseKey(TestKey);

    /* Enumerate first key with space for 2 chars */
    FillMemory(nameBuf, sizeof(nameBuf), 0x55);
    nameLen = 2;
    ErrorCode = RegEnumKeyExW(hKey, 0, nameBuf, &nameLen, NULL, NULL, NULL, NULL);
    ok_dec(ErrorCode, ERROR_MORE_DATA);
    ok_hex(nameBuf[0], 0x5555);
    ok_hex(nameBuf[1], 0x5555);
    ok(nameLen == 2, "nameLen = %ld, expected 2\n", nameLen);

    /* Enumerate first key with space for 3 chars */
    FillMemory(nameBuf, sizeof(nameBuf), 0x55);
    nameLen = 3;
    ErrorCode = RegEnumKeyExW(hKey, 0, nameBuf, &nameLen, NULL, NULL, NULL, NULL);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok_hex(nameBuf[0], L'1');
    ok_hex(nameBuf[1], L'2');
    ok_hex(nameBuf[2], 0);
    ok_hex(nameBuf[3], 0x5555);
    ok(nameLen == 2, "nameLen = %ld, expected 2\n", nameLen);

    /* Delete the subkey */
    ErrorCode = RegDeleteKeyW(hKey, L"12");
    ok_dec(ErrorCode, ERROR_SUCCESS);

    /* Delete our parent key */
    ErrorCode = RegDeleteKeyW(hKey, L"");
    ok_dec(ErrorCode, ERROR_SUCCESS);
    RegCloseKey(hKey);
}
