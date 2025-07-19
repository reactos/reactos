/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for GUIDFromStringA/W
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <shelltest.h>
#include <initguid.h>
#include <undocshell.h>
#include <versionhelpers.h>

DEFINE_GUID(invalid_guid, 0xDEADDEAD, 0xDEAD, 0xDEAD, 0xED, 0xED, 0xED, 0xED,
            0xED, 0xED, 0xED, 0xED);

//DEFINE_GUID(IID_IShellLinkW, 0x000214F9, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00,
//            0x00, 0x00, 0x00, 0x46);
//DEFINE_GUID(IID_IShellLinkW_Invalid, 0x000214F9, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00,
//            0x00, 0x00, 0x00, 0xED);

static void TEST_GUIDFromStringA(void)
{
    GUID guid;
    BOOL ret;

    guid = invalid_guid;
    _SEH2_TRY
    {
        ret = GUIDFromStringA(NULL, &guid);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = 0xDEADBEEF;
    }
    _SEH2_END;

    ok(ret == FALSE ||             // Win8+
       ret == (int)(0xDEADBEEF),   // Win2k3-Win7
       "Wrong value for ret (0x%X)\n", ret);
    ok_int(memcmp(&guid, &invalid_guid, sizeof(guid)) == 0, TRUE);

    guid = invalid_guid;
    ok_int(GUIDFromStringA("", &guid), FALSE);
    ok_int(memcmp(&guid, &invalid_guid, sizeof(guid)) == 0, TRUE);

    guid = invalid_guid;
    ok_int(GUIDFromStringA("{", &guid), FALSE);
    ok_int(memcmp(&guid, &invalid_guid, sizeof(guid)) == 0, TRUE);

    guid = invalid_guid;
    ok_int(GUIDFromStringA("{000214F9-0000-0000-C000-000000000046", &guid), FALSE);
    //ok_int(memcmp(&guid, &IID_IShellLinkW_Invalid, sizeof(guid)) == 0, TRUE); // Ignorable corner case

    guid = invalid_guid;
    ok_int(GUIDFromStringA("{000214F9-0000-0000-C000-000000000046}", &guid), TRUE);
    ok_int(memcmp(&guid, &IID_IShellLinkW, sizeof(guid)) == 0, TRUE);

    guid = invalid_guid;
    ok_int(GUIDFromStringA("{000214F9-0000-0000-C000-000000000046}g", &guid), TRUE);
    ok_int(memcmp(&guid, &IID_IShellLinkW, sizeof(guid)) == 0, TRUE);

    guid = invalid_guid;
    ok_int(GUIDFromStringA(" {000214F9-0000-0000-C000-000000000046}", &guid), FALSE);
    ok_int(memcmp(&guid, &invalid_guid, sizeof(guid)) == 0, TRUE);
}

static void TEST_GUIDFromStringW(void)
{
    GUID guid;
    BOOL ret;

    guid = invalid_guid;
    _SEH2_TRY
    {
        ret = GUIDFromStringW(NULL, &guid);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = 0xDEADBEEF;
    }
    _SEH2_END;

    ok(ret == (int)(0xDEADBEEF) || // Win8+
       ret == FALSE,               // Win2k3-Win7
       "Wrong value for ret (0x%X)\n", ret);
    ok_int(memcmp(&guid, &invalid_guid, sizeof(guid)) == 0, TRUE);

    guid = invalid_guid;
    ok_int(GUIDFromStringW(L"", &guid), FALSE);
    ok_int(memcmp(&guid, &invalid_guid, sizeof(guid)) == 0, TRUE);

    guid = invalid_guid;
    ok_int(GUIDFromStringW(L"{", &guid), FALSE);
    ok_int(memcmp(&guid, &invalid_guid, sizeof(guid)) == 0, TRUE);

    guid = invalid_guid;
    ok_int(GUIDFromStringW(L"{000214F9-0000-0000-C000-000000000046", &guid), FALSE);
    //ok_int(memcmp(&guid, &IID_IShellLinkW_Invalid, sizeof(guid)) == 0, TRUE); // Ignorable corner case

    guid = invalid_guid;
    ok_int(GUIDFromStringW(L"{000214F9-0000-0000-C000-000000000046}", &guid), TRUE);
    ok_int(memcmp(&guid, &IID_IShellLinkW, sizeof(guid)) == 0, TRUE);

    guid = invalid_guid;
    ok_int(GUIDFromStringW(L"{000214F9-0000-0000-C000-000000000046}g", &guid), TRUE);
    ok_int(memcmp(&guid, &IID_IShellLinkW, sizeof(guid)) == 0, TRUE);

    guid = invalid_guid;
    ok_int(GUIDFromStringW(L" {000214F9-0000-0000-C000-000000000046}", &guid), FALSE);
    ok_int(memcmp(&guid, &invalid_guid, sizeof(guid)) == 0, TRUE);
}

START_TEST(GUIDFromString)
{
    TEST_GUIDFromStringA();
    TEST_GUIDFromStringW();
}
