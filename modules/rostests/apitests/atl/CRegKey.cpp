/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CRegKey
 * PROGRAMMERS:     Mark Jansen
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <atlbase.h>

#ifdef HAVE_APITEST
    #include <apitest.h>
#else
    #include "atltest.h"
#endif

START_TEST(CRegKey)
{
    CRegKey key;
    CRegKey key2(HKEY_CURRENT_USER);

    ok(key.m_hKey == NULL, "Expected m_hKey to be initialized to 0, was: %p\n", key.m_hKey);
    ok(key2.m_hKey == HKEY_CURRENT_USER, "Expected m_hKey to be initialized to HKEY_CURRENT_USER, was: %p\n", key2.m_hKey);
    ok(key2 == HKEY_CURRENT_USER, "Expected operator HKEY() to be implemented\n");

    // Take ownership
    CRegKey key3(key2);
    ok(key3.m_hKey == HKEY_CURRENT_USER, "Expected m_hKey to be initialized to HKEY_CURRENT_USER, was: %p\n", key3.m_hKey);
    ok(key2.m_hKey == NULL, "Expected m_hKey to be initialized to 0, was: %p\n", key2.m_hKey);

    LSTATUS lret;

    lret = key.Close();
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);
    lret = key2.Close();
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);
    lret = key3.Close();
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);

    // read/write
    lret = key.Open(HKEY_CURRENT_USER, _T("Environment"));
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);
    ok(key.m_hKey != NULL, "Expected m_hKey to not be NULL, was: %p\n", key.m_hKey);


    HKEY tmp = key.m_hKey;
    HKEY detached = key.Detach();
    ok(key.m_hKey == NULL, "Expected m_hKey to be 0, was: %p\n", key.m_hKey);
    ok(detached == tmp, "Expected detached to be %p, was: %p\n", tmp, detached);
    key.Attach(detached);
    ok(key.m_hKey == tmp, "Expected m_hKey to be %p, was: %p\n", tmp, key.m_hKey);

    lret = key2.Open(HKEY_CURRENT_USER, _T("Environment"), KEY_READ);
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);

    lret = key3.Open(HKEY_CURRENT_USER, _T("Environment"), KEY_WRITE);
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);

    TCHAR testdata[] = _T("XX-XX");
    lret = key.SetValue(_T("APITEST_VALUE_NAME"), REG_SZ, testdata, sizeof(testdata));
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);

    TCHAR buffer[100];
    ULONG buffer_size = sizeof(buffer);
    DWORD type = 0x12345;
    memset(buffer, 0, sizeof(buffer));

    lret = key.QueryValue(_T("APITEST_VALUE_NAME"), &type, buffer, &buffer_size);
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);
    ok(type == REG_SZ, "Expected type to be REG_SZ, was: %lu\n", type);
    ok(buffer_size == sizeof(testdata), "Expected buffer_size to be %u, was: %lu\n", sizeof(testdata), buffer_size);
    ok(!memcmp(buffer, testdata, sizeof(testdata)), "Expected to get the same input as what was written!\n");


    buffer_size = sizeof(buffer);
    type = 0x12345;
    memset(buffer, 0, sizeof(buffer));
    lret = key2.QueryValue(_T("APITEST_VALUE_NAME"), &type, buffer, &buffer_size);
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);
    ok(type == REG_SZ, "Expected type to be REG_SZ, was: %lu\n", type);
    ok(buffer_size == sizeof(testdata), "Expected buffer_size to be %u, was: %lu\n", sizeof(testdata), buffer_size);
    ok(!memcmp(buffer, testdata, sizeof(testdata)), "Expected to get the same input as what was written!\n");

    buffer_size = sizeof(buffer);
    type = 0x12345;
    memset(buffer, 0, sizeof(buffer));
    lret = key3.QueryValue(_T("APITEST_VALUE_NAME"), &type, buffer, &buffer_size);
    ok(lret == ERROR_ACCESS_DENIED, "Expected lret to be ERROR_ACCESS_DENIED, was: %lu\n", lret);
    ok(type == 0 || ((sizeof(void*) == 8) && broken(type == 1)) || broken(type > 200), "Expected type to be 0, was: %lu\n", type);
    ok(buffer_size == sizeof(buffer), "Expected buffer_size to be %u, was: %lu\n", sizeof(buffer), buffer_size);


    lret = key2.SetValue(_T("APITEST_VALUE_NAME"), REG_SZ, testdata, sizeof(testdata));
    ok(lret == ERROR_ACCESS_DENIED, "Expected lret to be ERROR_ACCESS_DENIED, was: %lu\n", lret);

    lret = key2.DeleteValue(_T("APITEST_VALUE_NAME"));
    ok(lret == ERROR_ACCESS_DENIED, "Expected lret to be ERROR_ACCESS_DENIED, was: %lu\n", lret);

    DWORD dword = 0x54321;
    lret = key2.QueryDWORDValue(_T("APITEST_VALUE_NAME"), dword);
    ok(lret == ERROR_MORE_DATA, "Expected lret to be ERROR_MORE_DATA, was: %lu\n", lret);
    ok(dword == 0x54321, "Expected dword to be 0x54321, was: %lu\n", dword);

    lret = key.SetValue(_T("APITEST_VALUE_NAME"), REG_SZ, testdata, sizeof(TCHAR));
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_ACCESS_DENIED, was: %lu\n", lret);

    dword = 0x54321;
    lret = key2.QueryDWORDValue(_T("APITEST_VALUE_NAME"), dword);
    ok(lret == ERROR_INVALID_DATA, "Expected lret to be ERROR_MORE_DATA, was: %lu\n", lret);
    ok(dword != 0x54321, "Expected dword to NOT be 0x54321, was: %lu\n", dword);

    lret = key3.SetDWORDValue(_T("APITEST_VALUE_NAME"), 0x12345);
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);

    dword = 0x54321;
    lret = key2.QueryDWORDValue(_T("APITEST_VALUE_NAME"), dword);
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);
    ok(dword == 0x12345, "Expected dword to be 0x12345, was: %lu\n", dword);


    lret = key3.DeleteValue(_T("APITEST_VALUE_NAME"));
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);


    lret = key.SetKeyValue(_T("APITEST_KEY_NAME"), _T("APITEST_VALUE"));
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);

    CRegKey qv;

    // COUNTOF, not SIZEOF!!!!
    lret = qv.Open(HKEY_CURRENT_USER, _T("Environment\\APITEST_KEY_NAME"));
    buffer_size = _countof(buffer);
    memset(buffer, 0, sizeof(buffer));
    lret = qv.QueryStringValue(NULL, buffer, &buffer_size);
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);
    ok(buffer_size == _countof("APITEST_VALUE"), "Expected buffer_size to be %u, was: %lu\n", _countof("APITEST_VALUE"), buffer_size);
    ok(!_tcscmp(buffer, _T("APITEST_VALUE")), "Expected to get the same input as what was written!\n");

    lret = key.SetKeyValue(_T("APITEST_KEY_NAME"), _T("APITEST_VALUE2"), _T("APITEST_VALUE_NAME"));
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);

    buffer_size = _countof(buffer);
    memset(buffer, 0, sizeof(buffer));
    lret = qv.QueryStringValue(_T("APITEST_VALUE_NAME"), buffer, &buffer_size);
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);
    ok(buffer_size == _countof("APITEST_VALUE2"), "Expected buffer_size to be %u, was: %lu\n", _countof("APITEST_VALUE2"), buffer_size);
    ok(!_tcscmp(buffer, _T("APITEST_VALUE2")), "Expected to get the same input as what was written!\n");

    lret = key.DeleteSubKey(_T("APITEST_KEY_NAME"));
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);

    GUID guid, guid2;
    memset(&guid, 56, sizeof(guid));

    lret = key.SetGUIDValue(_T("GUID_NAME"), guid);
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);

    lret = key.QueryGUIDValue(_T("GUID_NAME"), guid2);
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);
    ok(!memcmp(&guid, &guid2, sizeof(guid)), "Expected guid to equal guid2\n");

    buffer_size = _countof(buffer);
    memset(buffer, 0, sizeof(buffer));
    lret = key2.QueryStringValue(_T("GUID_NAME"), buffer, &buffer_size);
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);
    ok(buffer_size == _countof("{38383838-3838-3838-3838-383838383838}"),
        "Expected buffer_size to be %u, was: %lu\n", _countof("{38383838-3838-3838-3838-383838383838}"), buffer_size);
    ok(!_tcscmp(buffer, _T("{38383838-3838-3838-3838-383838383838}")), "Expected to get the same input as what was written!\n");

    memset(&guid, 33, 5);
    lret = key.SetBinaryValue(_T("BIN_NAME"), &guid, 5);
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);

    buffer_size = sizeof(buffer);
    memset(buffer, 0, sizeof(buffer));
    lret = key.QueryBinaryValue(_T("GUID_NAME"), buffer, &buffer_size);
    ok(lret == ERROR_INVALID_DATA, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);
    ok(buffer_size == sizeof(_T("{38383838-3838-3838-3838-383838383838}")),
        "Expected buffer_size to be %u, was: %lu\n", sizeof(_T("{38383838-3838-3838-3838-383838383838}")), buffer_size);
    ok(buffer[0] == '{', "Expected buffer[0] to be 123, was: %i\n", (int)buffer[0]);

    buffer_size = sizeof(buffer);
    memset(buffer, 0, sizeof(buffer));
    lret = key.QueryBinaryValue(_T("BIN_NAME"), buffer, &buffer_size);
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);
    ok(buffer_size == 5, "Expected buffer_size to be %i, was: %lu\n", 5, buffer_size);
    ok(!memcmp(buffer, &guid, 5), "Expected the first 5 bytes of buffer to equal the data in null_guid\n");

    lret = key.DeleteValue(_T("GUID_NAME"));
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);

    lret = key.DeleteValue(_T("BIN_NAME"));
    ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);

    {
        CRegKey test1;
        lret = test1.Create(HKEY_CURRENT_USER, _T("TEST1"));
        ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);

        CRegKey test2;
        lret = test2.Create(test1, _T("TEST2"));
        ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);

        CRegKey test3;
        lret = test3.Create(test2, _T("TEST3"));
        ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);
    }
    {
        CRegKey key(HKEY_CURRENT_USER);
        lret = key.RecurseDeleteKey(_T("TEST1"));
        ok(lret == ERROR_SUCCESS, "Expected lret to be ERROR_SUCCESS, was: %lu\n", lret);
    }
}
