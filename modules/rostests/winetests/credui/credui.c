/*
 * Credentials User Interface Tests
 *
 * Copyright 2007 Robert Shearman for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "wincred.h"
#include "sspi.h"

#include "wine/test.h"

static SECURITY_STATUS (SEC_ENTRY *pSspiEncodeAuthIdentityAsStrings)
    (PSEC_WINNT_AUTH_IDENTITY_OPAQUE,PCWSTR*,PCWSTR*,PCWSTR*);
static void (SEC_ENTRY *pSspiFreeAuthIdentity)
    (PSEC_WINNT_AUTH_IDENTITY_OPAQUE);
static ULONG (SEC_ENTRY *pSspiPromptForCredentialsW)
    (PCWSTR,void*,ULONG,PCWSTR,PSEC_WINNT_AUTH_IDENTITY_OPAQUE,PSEC_WINNT_AUTH_IDENTITY_OPAQUE*,int*,ULONG);

static void test_CredUIPromptForCredentials(void)
{
    static const WCHAR wszServerName[] = L"WineTest";
    DWORD ret;
    WCHAR username[256];
    WCHAR password[256];
    CREDUI_INFOW credui_info;
    BOOL save = FALSE;

    credui_info.cbSize = sizeof(credui_info);
    credui_info.hwndParent = NULL;
    credui_info.pszMessageText = NULL;
    credui_info.hbmBanner = NULL;

    ret = CredUIConfirmCredentialsW(NULL, TRUE);
    ok(ret == ERROR_INVALID_PARAMETER /* 2003 + */ || ret == ERROR_NOT_FOUND /* XP */,
        "CredUIConfirmCredentials should have returned ERROR_INVALID_PARAMETER or ERROR_NOT_FOUND instead of %ld\n", ret);

    ret = CredUIConfirmCredentialsW(wszServerName, TRUE);
    ok(ret == ERROR_NOT_FOUND, "CredUIConfirmCredentials should have returned ERROR_NOT_FOUND instead of %ld\n", ret);

    username[0] = '\0';
    password[0] = '\0';
    ret = CredUIPromptForCredentialsW(NULL, NULL, NULL, 0, username,
                                      ARRAY_SIZE(username),
                                      password, ARRAY_SIZE(password),
                                      NULL, CREDUI_FLAGS_ALWAYS_SHOW_UI);
    ok(ret == ERROR_INVALID_FLAGS, "CredUIPromptForCredentials should have returned ERROR_INVALID_FLAGS instead of %ld\n", ret);

    ret = CredUIPromptForCredentialsW(NULL, NULL, NULL, 0, username,
                                      ARRAY_SIZE(username),
                                      password, ARRAY_SIZE(password),
                                      NULL, CREDUI_FLAGS_ALWAYS_SHOW_UI | CREDUI_FLAGS_GENERIC_CREDENTIALS);
    ok(ret == ERROR_INVALID_PARAMETER, "CredUIPromptForCredentials should have returned ERROR_INVALID_PARAMETER instead of %ld\n", ret);

    ret = CredUIPromptForCredentialsW(NULL, wszServerName, NULL, 0, username,
                                      ARRAY_SIZE(username),
                                      password, ARRAY_SIZE(password),
                                      NULL, CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX);
    ok(ret == ERROR_INVALID_PARAMETER, "CredUIPromptForCredentials should have returned ERROR_INVALID_PARAMETER instead of %ld\n", ret);

    if (winetest_interactive)
    {
        ret = CredUIPromptForCredentialsW(NULL, wszServerName, NULL, 0, username,
                                          ARRAY_SIZE(username),
                                          password, ARRAY_SIZE(password),
                                          &save, CREDUI_FLAGS_EXPECT_CONFIRMATION);
        ok(ret == ERROR_SUCCESS || ret == ERROR_CANCELLED, "CredUIPromptForCredentials failed with error %ld\n", ret);
        if (ret == ERROR_SUCCESS)
        {
            ret = CredUIConfirmCredentialsW(wszServerName, FALSE);
            ok(ret == ERROR_SUCCESS, "CredUIConfirmCredentials failed with error %ld\n", ret);
        }

        credui_info.pszCaptionText = L"CREDUI_FLAGS_EXPECT_CONFIRMATION";
        ret = CredUIPromptForCredentialsW(&credui_info, wszServerName, NULL, ERROR_ACCESS_DENIED,
                                          username, ARRAY_SIZE(username),
                                          password, ARRAY_SIZE(password),
                                          &save, CREDUI_FLAGS_EXPECT_CONFIRMATION);
        ok(ret == ERROR_SUCCESS || ret == ERROR_CANCELLED, "CredUIPromptForCredentials failed with error %ld\n", ret);
        if (ret == ERROR_SUCCESS)
        {
            ret = CredUIConfirmCredentialsW(wszServerName, FALSE);
            ok(ret == ERROR_SUCCESS, "CredUIConfirmCredentials failed with error %ld\n", ret);
        }

        credui_info.pszCaptionText = L"CREDUI_FLAGS_INCORRECT_PASSWORD|CREDUI_FLAGS_EXPECT_CONFIRMATION";
        ret = CredUIPromptForCredentialsW(&credui_info, wszServerName, NULL, 0,
                                          username, ARRAY_SIZE(username),
                                          password, ARRAY_SIZE(password),
                                          NULL, CREDUI_FLAGS_INCORRECT_PASSWORD|CREDUI_FLAGS_EXPECT_CONFIRMATION);
        ok(ret == ERROR_SUCCESS || ret == ERROR_CANCELLED, "CredUIPromptForCredentials failed with error %ld\n", ret);
        if (ret == ERROR_SUCCESS)
        {
            ret = CredUIConfirmCredentialsW(wszServerName, FALSE);
            ok(ret == ERROR_SUCCESS, "CredUIConfirmCredentials failed with error %ld\n", ret);
        }


        save = TRUE;
        credui_info.pszCaptionText = L"CREDUI_FLAGS_DO_NOT_PERSIST|CREDUI_FLAGS_EXPECT_CONFIRMATION";
        ret = CredUIPromptForCredentialsW(&credui_info, wszServerName, NULL, 0,
                                          username, ARRAY_SIZE(username),
                                          password, ARRAY_SIZE(password),
                                          &save, CREDUI_FLAGS_DO_NOT_PERSIST|CREDUI_FLAGS_EXPECT_CONFIRMATION);
        ok(ret == ERROR_SUCCESS || ret == ERROR_CANCELLED, "CredUIPromptForCredentials failed with error %ld\n", ret);
        ok(save, "save flag should have been untouched\n");

        save = FALSE;
        credui_info.pszCaptionText = L"CREDUI_FLAGS_PERSIST|CREDUI_FLAGS_EXPECT_CONFIRMATION";
        ret = CredUIPromptForCredentialsW(&credui_info, wszServerName, NULL, 0,
                                          username, ARRAY_SIZE(username),
                                          password, ARRAY_SIZE(password),
                                          &save, CREDUI_FLAGS_PERSIST|CREDUI_FLAGS_EXPECT_CONFIRMATION);
        ok(ret == ERROR_SUCCESS || ret == ERROR_CANCELLED, "CredUIPromptForCredentials failed with error %ld\n", ret);
        ok(!save, "save flag should have been untouched\n");
        if (ret == ERROR_SUCCESS)
        {
            ret = CredUIConfirmCredentialsW(wszServerName, FALSE);
            ok(ret == ERROR_SUCCESS, "CredUIConfirmCredentials failed with error %ld\n", ret);
        }

    }
}

static void test_SspiPromptForCredentials(void)
{
    ULONG ret;
    SECURITY_STATUS status;
    CREDUI_INFOW info;
    PSEC_WINNT_AUTH_IDENTITY_OPAQUE id;
    const WCHAR *username, *domain, *creds;
    int save;

    if (!pSspiPromptForCredentialsW || !pSspiFreeAuthIdentity)
    {
        win_skip( "SspiPromptForCredentialsW is missing\n" );
        return;
    }

    info.cbSize         = sizeof(info);
    info.hwndParent     = NULL;
    info.pszMessageText = L"SspiTest";
    info.pszCaptionText = L"basic";
    info.hbmBanner      = NULL;
    ret = pSspiPromptForCredentialsW( NULL, &info, 0, L"basic", NULL, &id, &save, 0 );
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

    ret = pSspiPromptForCredentialsW( L"SspiTest", &info, 0, NULL, NULL, &id, &save, 0 );
    ok( ret == ERROR_NO_SUCH_PACKAGE, "got %lu\n", ret );

    if (winetest_interactive)
    {
        id = NULL;
        save = -1;
        ret = pSspiPromptForCredentialsW( L"SspiTest", &info, 0, L"basic", NULL, &id, &save, 0 );
        ok( ret == ERROR_SUCCESS || ret == ERROR_CANCELLED, "got %lu\n", ret );
        if (ret == ERROR_SUCCESS)
        {
            ok( id != NULL, "id not set\n" );
            ok( save == TRUE || save == FALSE, "got %d\n", save );

            username = creds = NULL;
            domain = (const WCHAR *)0xdeadbeef;
            status = pSspiEncodeAuthIdentityAsStrings( id, &username, &domain, &creds );
            ok( status == SEC_E_OK, "got %lu\n", status );
            ok( username != NULL, "username not set\n" );
            ok( domain == NULL, "domain not set\n" );
            ok( creds != NULL, "creds not set\n" );
            pSspiFreeAuthIdentity( id );
        }
    }
}

START_TEST(credui)
{
    HMODULE hcredui = GetModuleHandleA( "credui.dll" ), hsecur32 = LoadLibraryA( "secur32.dll" );

    if (hcredui)
        pSspiPromptForCredentialsW = (void *)GetProcAddress( hcredui, "SspiPromptForCredentialsW" );
    if (hsecur32)
    {
        pSspiEncodeAuthIdentityAsStrings = (void *)GetProcAddress( hsecur32, "SspiEncodeAuthIdentityAsStrings" );
        pSspiFreeAuthIdentity = (void *)GetProcAddress( hsecur32, "SspiFreeAuthIdentity" );
    }

    test_CredUIPromptForCredentials();
    test_SspiPromptForCredentials();

    FreeLibrary( hsecur32 );
}
