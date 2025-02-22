/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for the RegGetvalueW API
 * PROGRAMMER:      Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include "precomp.h"

START_TEST(RegEnumValueW)
{
    LONG ErrorCode;
    HKEY TestKey;
    ULONG Data;
    DWORD NameLength, DataLength;
    DWORD DataType;
    WCHAR NameBuffer[7];

    /* Create our Test Key */
    ErrorCode = RegCreateKeyW( HKEY_CURRENT_USER, L"Software\\ReactOS_apitest", &TestKey );
    ok_dec(ErrorCode, ERROR_SUCCESS);

    /* All NULL is invalid */
    ErrorCode = RegEnumValueW(TestKey, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    ok_dec(ErrorCode, ERROR_INVALID_PARAMETER);

    /* Asking for the buffer length is not enough. */
    NameLength = 8;
    ErrorCode = RegEnumValueW(TestKey, 0, NULL, &NameLength, NULL, NULL, NULL, NULL);
    ok_dec(ErrorCode, ERROR_INVALID_PARAMETER);

    /* Or maybe it is, you just have to nicely ask */
    NameLength = 0;
    ErrorCode = RegEnumValueW(TestKey, 0, NULL, &NameLength, NULL, NULL, NULL, NULL);
    ok_dec(ErrorCode, ERROR_INVALID_PARAMETER);

    /* Name buffer alone is also prohibited */
    ErrorCode = RegEnumValueW(TestKey, 0, NameBuffer, NULL, NULL, NULL, NULL, NULL);
    ok_dec(ErrorCode, ERROR_INVALID_PARAMETER);

    /* You need to ask for both a minima */
    ErrorCode = RegEnumValueW(TestKey, 0, NameBuffer, &NameLength, NULL, NULL, NULL, NULL);
    ok_dec(ErrorCode, ERROR_NO_MORE_ITEMS);

    /* What if I only want to know the value type ? */
    ErrorCode = RegEnumValueW(TestKey, 0, NULL, NULL, &DataType, NULL, NULL, NULL);
    ok_dec(ErrorCode, ERROR_INVALID_PARAMETER);

    /* Set a value */
    Data = 0xF00DF00D;
    ErrorCode = RegSetValueExW(TestKey, L"Value1", 0, REG_BINARY, (LPBYTE)&Data, sizeof(Data));
    ok_dec(ErrorCode, ERROR_SUCCESS);

    /* Try various combinations of Arguments */
    NameLength = 7;
    ErrorCode = RegEnumValueW(TestKey, 0, NULL, &NameLength, NULL, NULL, NULL, NULL);
    ok_dec(ErrorCode, ERROR_INVALID_PARAMETER);
    /* Provide a buffer this time */
    ErrorCode = RegEnumValueW(TestKey, 0, NameBuffer, &NameLength, NULL, NULL, NULL, NULL);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    /* This doesn't include the NULL terminator */
    ok_dec(NameLength, 6);
    /* But the string is NULL terminated */
    ok_hex(NameBuffer[6], L'\0');
    ok(wcscmp(NameBuffer, L"Value1") == 0, "%S\n", NameBuffer);

    /* See if skipping the NULL value is a problem */
    NameLength = 6;
    memset(NameBuffer, 0xBA, sizeof(NameBuffer));
    ErrorCode = RegEnumValueW(TestKey, 0, NameBuffer, &NameLength, NULL, NULL, NULL, NULL);
    ok_dec(ErrorCode, ERROR_MORE_DATA);
    /* In fact, this is unchanged */
    ok_dec(NameLength, 6);
    /* And the string is untouched */
    ok_hex(NameBuffer[6], 0xBABA);
    ok_hex(NameBuffer[5], 0xBABA);
    ok_hex(NameBuffer[4], 0xBABA);
    ok_hex(NameBuffer[3], 0xBABA);
    ok_hex(NameBuffer[2], 0xBABA);
    ok_hex(NameBuffer[1], 0xBABA);
    ok_hex(NameBuffer[0], 0xBABA);

    /* Of course, anything smaller is an outrage */
    NameLength = 5;
    memset(NameBuffer, 0xBA, sizeof(NameBuffer));
    ErrorCode = RegEnumValueW(TestKey, 0, NameBuffer, &NameLength, NULL, NULL, NULL, NULL);
    ok_dec(ErrorCode, ERROR_MORE_DATA);
    /* Untouched... */
    ok_dec(NameLength, 5);
    /* And the string is untouched */
    ok_hex(NameBuffer[6], 0xBABA);
    ok_hex(NameBuffer[5], 0xBABA);
    ok_hex(NameBuffer[4], 0xBABA);
    ok_hex(NameBuffer[3], 0xBABA);
    ok_hex(NameBuffer[2], 0xBABA);
    ok_hex(NameBuffer[1], 0xBABA);
    ok_hex(NameBuffer[0], 0xBABA);

    /* Of course, asking for data without caring for the name would be strange... */
    DataLength = 0;
    ErrorCode = RegEnumValueW(TestKey, 0, NULL, NULL, NULL, NULL, NULL, &DataLength);
    ok_dec(ErrorCode, ERROR_INVALID_PARAMETER);

    DataLength = 0;
    NameLength = 7;
    ErrorCode = RegEnumValueW(TestKey, 0, NameBuffer, &NameLength, NULL, &DataType, NULL, &DataLength);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok_dec(DataLength, sizeof(Data));
    ok_hex(DataType, REG_BINARY);

    /* Same, but this time with NULL data buffer and non-zero data size */
    DataLength = 2;
    NameLength = 7;
    DataType = 0x01234567;
    ErrorCode = RegEnumValueW(TestKey, 0, NameBuffer, &NameLength, NULL, &DataType, NULL, &DataLength);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok_dec(DataLength, sizeof(Data));
    ok_hex(DataType, REG_BINARY);

    /* Same, but this time with data buffer and shrunk data size */
    DataLength = 2;
    Data = 0;
    DataType = 0x01234567;
    NameLength = 7;
    ErrorCode = RegEnumValueW(TestKey, 0, NameBuffer, &NameLength, NULL, &DataType, (LPBYTE)&Data, &DataLength);
    ok_dec(ErrorCode, ERROR_MORE_DATA);
    ok_dec(DataLength, sizeof(Data));
    ok_hex(Data, 0);
    ok_hex(DataType, REG_BINARY);

    /* Put the right parameters */
    DataLength = sizeof(Data);
    NameLength = 7;
    Data = 0;
    DataType = 0x01234567;
    ErrorCode = RegEnumValueW(TestKey, 0, NameBuffer, &NameLength, NULL, &DataType, (LPBYTE)&Data, &DataLength);
    ok_dec(ErrorCode, ERROR_SUCCESS);
    ok_dec(DataLength, sizeof(Data));
    ok_hex(Data, 0xF00DF00D);
    ok_hex(DataType, REG_BINARY);

    /* Delete the key */
    ErrorCode = RegDeleteKeyW(TestKey, L"");
    ok_dec(ErrorCode, ERROR_SUCCESS);
    RegCloseKey(TestKey);
}

