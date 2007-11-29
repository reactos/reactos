/*
 * Copyright 2002 Andriy Palamarchuk
 *
 * Conformance test of the access functions.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <lmapibuf.h>

#include "wine/test.h"

WCHAR user_name[UNLEN + 1];
WCHAR computer_name[MAX_COMPUTERNAME_LENGTH + 1];

static const WCHAR sNonexistentUser[] = {'N','o','n','e','x','i','s','t','e','n','t',' ',
                                'U','s','e','r',0};
static WCHAR sTooLongName[] = {'T','h','i','s',' ','i','s',' ','a',' ','b','a','d',
    ' ','u','s','e','r','n','a','m','e',0};
static WCHAR sTooLongPassword[] = {'a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h',
    'a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h',
    'a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h',
    'a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h',
    'a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h',
    'a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h',
    'a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h',
    'a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h',
    'a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h',
    'a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h',
    'a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h','a','b','c','d','e','f','g','h',
    'a', 0};

static WCHAR sTestUserName[] = {'t', 'e', 's', 't', 'u', 's', 'e', 'r', 0};
static WCHAR sTestUserOldPass[] = {'o', 'l', 'd', 'p', 'a', 's', 's', 0};
static WCHAR sTestUserNewPass[] = {'n', 'e', 'w', 'p', 'a', 's', 's', 0};
static const WCHAR sBadNetPath[] = {'\\','\\','B','a',' ',' ','p','a','t','h',0};
static const WCHAR sInvalidName[] = {'\\',0};
static const WCHAR sInvalidName2[] = {'\\','\\',0};
static const WCHAR sEmptyStr[] = { 0 };

static NET_API_STATUS (WINAPI *pNetApiBufferFree)(LPVOID)=NULL;
static NET_API_STATUS (WINAPI *pNetApiBufferSize)(LPVOID,LPDWORD)=NULL;
static NET_API_STATUS (WINAPI *pNetQueryDisplayInformation)(LPWSTR,DWORD,DWORD,DWORD,DWORD,LPDWORD,PVOID*)=NULL;
static NET_API_STATUS (WINAPI *pNetUserGetInfo)(LPCWSTR,LPCWSTR,DWORD,LPBYTE*)=NULL;
static NET_API_STATUS (WINAPI *pNetUserModalsGet)(LPCWSTR,DWORD,LPBYTE*)=NULL;
static NET_API_STATUS (WINAPI *pNetUserAdd)(LPCWSTR,DWORD,LPBYTE,LPDWORD)=NULL;
static NET_API_STATUS (WINAPI *pNetUserChangePassword)(LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR)=NULL;
static NET_API_STATUS (WINAPI *pNetUserDel)(LPCWSTR,LPCWSTR)=NULL;

static int init_access_tests(void)
{
    DWORD dwSize;
    BOOL rc;

    user_name[0] = 0;
    dwSize = sizeof(user_name);
    rc=GetUserNameW(user_name, &dwSize);
    if (rc==FALSE && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
    {
        skip("GetUserNameW is not available.\n");
        return 0;
    }
    ok(rc, "User Name Retrieved\n");

    computer_name[0] = 0;
    dwSize = sizeof(computer_name);
    ok(GetComputerNameW(computer_name, &dwSize), "Computer Name Retrieved\n");
    return 1;
}

static NET_API_STATUS create_test_user(void)
{
    USER_INFO_1 usri;

    usri.usri1_name = sTestUserName;
    usri.usri1_password = sTestUserOldPass;
    usri.usri1_priv = USER_PRIV_USER;
    usri.usri1_home_dir = NULL;
    usri.usri1_comment = NULL;
    usri.usri1_flags = UF_SCRIPT;
    usri.usri1_script_path = NULL;

    return pNetUserAdd(NULL, 1, (LPBYTE)&usri, NULL);
}

static NET_API_STATUS delete_test_user(void)
{
    return pNetUserDel(NULL, sTestUserName);
}

static void run_usergetinfo_tests(void)
{
    NET_API_STATUS rc;
    PUSER_INFO_0 ui0 = NULL;
    PUSER_INFO_10 ui10 = NULL;
    DWORD dwSize;

    if((rc = create_test_user()) != NERR_Success )
    {
        skip("Skipping usergetinfo_tests, create_test_user failed: 0x%08x\n", rc);
        return;
    }

    /* Level 0 */
    rc=pNetUserGetInfo(NULL, sTestUserName, 0, (LPBYTE *)&ui0);
    ok(rc == NERR_Success, "NetUserGetInfo level 0 failed: 0x%08x.\n", rc);
    ok(!lstrcmpW(sTestUserName, ui0->usri0_name),"Username mismatch for level 0.\n");
    pNetApiBufferSize(ui0, &dwSize);
    ok(dwSize >= (sizeof(USER_INFO_0) +
                  (lstrlenW(ui0->usri0_name) + 1) * sizeof(WCHAR)),
       "Is allocated with NetApiBufferAllocate\n");

    /* Level 10 */
    rc=pNetUserGetInfo(NULL, sTestUserName, 10, (LPBYTE *)&ui10);
    ok(rc == NERR_Success, "NetUserGetInfo level 10 failed: 0x%08x.\n", rc);
    ok(!lstrcmpW(sTestUserName, ui10->usri10_name), "Username mismatch for level 10.\n");
    pNetApiBufferSize(ui10, &dwSize);
    ok(dwSize >= (sizeof(USER_INFO_10) +
                  (lstrlenW(ui10->usri10_name) + 1 +
                   lstrlenW(ui10->usri10_comment) + 1 +
                   lstrlenW(ui10->usri10_usr_comment) + 1 +
                   lstrlenW(ui10->usri10_full_name) + 1) * sizeof(WCHAR)),
       "Is allocated with NetApiBufferAllocate\n");

    pNetApiBufferFree(ui0);
    pNetApiBufferFree(ui10);

    /* errors handling */
    rc=pNetUserGetInfo(NULL, sTestUserName, 10000, (LPBYTE *)&ui0);
    ok(rc == ERROR_INVALID_LEVEL,"Invalid Level: rc=%d\n",rc);
    rc=pNetUserGetInfo(NULL, sNonexistentUser, 0, (LPBYTE *)&ui0);
    ok(rc == NERR_UserNotFound,"Invalid User Name: rc=%d\n",rc);
    todo_wine {
        /* FIXME - Currently Wine can't verify whether the network path is good or bad */
        rc=pNetUserGetInfo(sBadNetPath, sTestUserName, 0, (LPBYTE *)&ui0);
        ok(rc == ERROR_BAD_NETPATH || rc == ERROR_NETWORK_UNREACHABLE,
           "Bad Network Path: rc=%d\n",rc);
    }
    rc=pNetUserGetInfo(sEmptyStr, sTestUserName, 0, (LPBYTE *)&ui0);
    ok(rc == ERROR_BAD_NETPATH || rc == NERR_Success,
       "Bad Network Path: rc=%d\n",rc);
    rc=pNetUserGetInfo(sInvalidName, sTestUserName, 0, (LPBYTE *)&ui0);
    ok(rc == ERROR_INVALID_NAME,"Invalid Server Name: rc=%d\n",rc);
    rc=pNetUserGetInfo(sInvalidName2, sTestUserName, 0, (LPBYTE *)&ui0);
    ok(rc == ERROR_INVALID_NAME,"Invalid Server Name: rc=%d\n",rc);

    if(delete_test_user() != NERR_Success)
        trace("Deleting the test user failed. You might have to manually delete it.\n");
}

/* checks Level 1 of NetQueryDisplayInformation
 * FIXME: Needs to be rewritten to not depend on the spelling of the users,
 * ideally based on the admin and guest user SIDs/RIDs.*/
static void run_querydisplayinformation1_tests(void)
{
    PNET_DISPLAY_USER Buffer, rec;
    DWORD Result, EntryCount;
    DWORD i = 0;
    BOOL hasAdmin = FALSE;
    BOOL hasGuest = FALSE;
    static const WCHAR sAdminUserName[] = {'A','d','m','i','n','i','s','t','r','a',
        't','o','r',0};
    static const WCHAR sGuestUserName[] = {'G','u','e','s','t',0};

    do
    {
        Result = pNetQueryDisplayInformation(
            NULL, 1, i, 1000, MAX_PREFERRED_LENGTH, &EntryCount,
            (PVOID *)&Buffer);

        ok((Result == ERROR_SUCCESS) || (Result == ERROR_MORE_DATA),
           "Information Retrieved\n");
        rec = Buffer;
        for(; EntryCount > 0; EntryCount--)
        {
            if (!lstrcmpW(rec->usri1_name, sAdminUserName))
            {
                ok(!hasAdmin, "One admin user\n");
                ok(rec->usri1_flags & UF_SCRIPT, "UF_SCRIPT flag is set\n");
                ok(rec->usri1_flags & UF_NORMAL_ACCOUNT, "UF_NORMAL_ACCOUNT flag is set\n");
                hasAdmin = TRUE;
            }
            else if (!lstrcmpW(rec->usri1_name, sGuestUserName))
            {
                ok(!hasGuest, "One guest record\n");
                ok(rec->usri1_flags & UF_SCRIPT, "UF_SCRIPT flag is set\n");
                ok(rec->usri1_flags & UF_NORMAL_ACCOUNT, "UF_NORMAL_ACCOUNT flag is set\n");
                hasGuest = TRUE;
            }

            i = rec->usri1_next_index;
            rec++;
        }

        pNetApiBufferFree(Buffer);
    } while (Result == ERROR_MORE_DATA);

    ok(hasAdmin, "Has Administrator account\n");
}

static void run_usermodalsget_tests(void)
{
    NET_API_STATUS rc;
    USER_MODALS_INFO_2 * umi2 = NULL;

    rc = pNetUserModalsGet(NULL, 2, (LPBYTE *)&umi2);
    ok(rc == ERROR_SUCCESS, "NetUserModalsGet failed, rc = %d\n", rc);

    if (umi2)
        pNetApiBufferFree(umi2);
}

static void run_userhandling_tests(void)
{
    NET_API_STATUS ret;
    USER_INFO_1 usri;

    usri.usri1_priv = USER_PRIV_USER;
    usri.usri1_home_dir = NULL;
    usri.usri1_comment = NULL;
    usri.usri1_flags = UF_SCRIPT;
    usri.usri1_script_path = NULL;

    usri.usri1_name = sTooLongName;
    usri.usri1_password = sTestUserOldPass;

    ret = pNetUserAdd(NULL, 1, (LPBYTE)&usri, NULL);
    ok(ret == NERR_BadUsername, "Adding user with too long username returned 0x%08x\n", ret);

    usri.usri1_name = sTestUserName;
    usri.usri1_password = sTooLongPassword;

    ret = pNetUserAdd(NULL, 1, (LPBYTE)&usri, NULL);
    ok(ret == NERR_PasswordTooShort, "Adding user with too long password returned 0x%08x\n", ret);

    usri.usri1_name = sTooLongName;
    usri.usri1_password = sTooLongPassword;

    ret = pNetUserAdd(NULL, 1, (LPBYTE)&usri, NULL);
    ok(ret == NERR_BadUsername,
            "Adding user with too long username/password returned 0x%08x\n", ret);

    usri.usri1_name = sTestUserName;
    usri.usri1_password = sTestUserOldPass;

    ret = pNetUserAdd(NULL, 5, (LPBYTE)&usri, NULL);
    ok(ret == ERROR_INVALID_LEVEL, "Adding user with level 5 returned 0x%08x\n", ret);

    ret = pNetUserAdd(NULL, 1, (LPBYTE)&usri, NULL);
    if(ret == ERROR_ACCESS_DENIED)
    {
        skip("Insufficient permissions to add users. Skipping test.\n");
        return;
    }
    if(ret == NERR_UserExists)
    {
        skip("User already exists, skipping test to not mess up the system\n");
        return;
    }

    ok(ret == NERR_Success, "Adding user failed with error 0x%08x\n", ret);
    if(ret != NERR_Success)
        return;

    ret = pNetUserChangePassword(NULL, sNonexistentUser, sTestUserOldPass,
            sTestUserNewPass);
    ok(ret == NERR_UserNotFound,
            "Changing password for nonexistent user returned 0x%08x.\n", ret);

    ret = pNetUserChangePassword(NULL, sTestUserName, sTestUserOldPass,
            sTestUserOldPass);
    ok(ret == NERR_Success,
            "Changing old password to old password returned 0x%08x.\n", ret);

    ret = pNetUserChangePassword(NULL, sTestUserName, sTestUserNewPass,
            sTestUserOldPass);
    ok(ret == ERROR_INVALID_PASSWORD,
            "Trying to change password giving an invalid password returned 0x%08x.\n", ret);

    ret = pNetUserChangePassword(NULL, sTestUserName, sTestUserOldPass,
            sTooLongPassword);
    ok(ret == ERROR_PASSWORD_RESTRICTION,
            "Changing to a password that's too long returned 0x%08x.\n", ret);

    ret = pNetUserChangePassword(NULL, sTestUserName, sTestUserOldPass,
            sTestUserNewPass);
    ok(ret == NERR_Success, "Changing the password correctly returned 0x%08x.\n", ret);

    ret = pNetUserDel(NULL, sTestUserName);
    ok(ret == NERR_Success, "Deleting the user failed.\n");

    ret = pNetUserDel(NULL, sTestUserName);
    ok(ret == NERR_UserNotFound, "Deleting a nonexistent user returned 0x%08x\n",ret);
}

START_TEST(access)
{
    HMODULE hnetapi32=LoadLibraryA("netapi32.dll");

    pNetApiBufferFree=(void*)GetProcAddress(hnetapi32,"NetApiBufferFree");
    pNetApiBufferSize=(void*)GetProcAddress(hnetapi32,"NetApiBufferSize");
    pNetQueryDisplayInformation=(void*)GetProcAddress(hnetapi32,"NetQueryDisplayInformation");
    pNetUserGetInfo=(void*)GetProcAddress(hnetapi32,"NetUserGetInfo");
    pNetUserModalsGet=(void*)GetProcAddress(hnetapi32,"NetUserModalsGet");
    pNetUserAdd=(void*)GetProcAddress(hnetapi32, "NetUserAdd");
    pNetUserChangePassword=(void*)GetProcAddress(hnetapi32, "NetUserChangePassword");
    pNetUserDel=(void*)GetProcAddress(hnetapi32, "NetUserDel");

    /* These functions were introduced with NT. It's safe to assume that
     * if one is not available, none are.
     */
    if (!pNetApiBufferFree) {
        skip("Needed functions are not available\n");
        FreeLibrary(hnetapi32);
        return;
    }

    if (init_access_tests()) {
        run_userhandling_tests();
        run_usergetinfo_tests();
        run_querydisplayinformation1_tests();
        run_usermodalsget_tests();
    }

    FreeLibrary(hnetapi32);
}
