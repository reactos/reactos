/* Unit test suite for SHReg* functions
 *
 * Copyright 2002 Juergen Schmied
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "winuser.h"
#include "shlwapi.h"

/* Keys used for testing */
#define REG_TEST_KEY        "Software\\Wine\\Test"
#define REG_CURRENT_VERSION "Software\\Microsoft\\Windows\\CurrentVersion"

static HMODULE hshlwapi;
typedef DWORD (WINAPI *SHCopyKeyA_func)(HKEY,LPCSTR,HKEY,DWORD);
static SHCopyKeyA_func pSHCopyKeyA;
typedef DWORD (WINAPI *SHRegGetPathA_func)(HKEY,LPCSTR,LPCSTR,LPSTR,DWORD);
static SHRegGetPathA_func pSHRegGetPathA;

static const char * sTestpath1 = "%LONGSYSTEMVAR%\\subdir1";
static const char * sTestpath2 = "%FOO%\\subdir1";

static const char * sEnvvar1 = "bar";
static const char * sEnvvar2 = "ImARatherLongButIndeedNeededString";

static char sExpTestpath1[MAX_PATH];
static char sExpTestpath2[MAX_PATH];
static unsigned sExpLen1;
static unsigned sExpLen2;

static const char * sEmptyBuffer ="0123456789";

/* delete key and all its subkeys */
static DWORD delete_key( HKEY hkey, LPSTR parent, LPSTR keyname )
{
    HKEY parentKey;
    DWORD ret;

    RegCloseKey(hkey);

    /* open the parent of the key to close */
    ret = RegOpenKeyExA( HKEY_CURRENT_USER, parent, 0, KEY_ALL_ACCESS, &parentKey);
    if (ret != ERROR_SUCCESS)
        return ret;

    ret = SHDeleteKeyA( parentKey, keyname );
    RegCloseKey(parentKey);

    return ret;
}

static HKEY create_test_entries(void)
{
	HKEY hKey;
        DWORD ret;

        SetEnvironmentVariableA("LONGSYSTEMVAR", sEnvvar1);
        SetEnvironmentVariableA("FOO", sEnvvar2);

        ret = RegCreateKeyA(HKEY_CURRENT_USER, REG_TEST_KEY, &hKey);
	ok( ERROR_SUCCESS == ret, "RegCreateKeyA failed, ret=%lu\n", ret);

	if (hKey)
	{
           ok(!RegSetValueExA(hKey,"Test1",0,REG_EXPAND_SZ, (LPBYTE) sTestpath1, strlen(sTestpath1)+1), "RegSetValueExA failed\n");
           ok(!RegSetValueExA(hKey,"Test2",0,REG_SZ, (LPBYTE) sTestpath1, strlen(sTestpath1)+1), "RegSetValueExA failed\n");
           ok(!RegSetValueExA(hKey,"Test3",0,REG_EXPAND_SZ, (LPBYTE) sTestpath2, strlen(sTestpath2)+1), "RegSetValueExA failed\n");
	}

	sExpLen1 = ExpandEnvironmentStringsA(sTestpath1, sExpTestpath1, sizeof(sExpTestpath1));
	sExpLen2 = ExpandEnvironmentStringsA(sTestpath2, sExpTestpath2, sizeof(sExpTestpath2));

        ok(sExpLen1 > 0, "Couldn't expand %s\n", sTestpath1);
        trace("sExplen1 = (%d)\n", sExpLen1);
        ok(sExpLen2 > 0, "Couldn't expand %s\n", sTestpath2);
        trace("sExplen2 = (%d)\n", sExpLen2);

        return hKey;
}

static void test_SHGetValue(void)
{
	DWORD dwSize;
	DWORD dwType;
        DWORD dwRet;
	char buf[MAX_PATH];

	strcpy(buf, sEmptyBuffer);
	dwSize = MAX_PATH;
	dwType = -1;
        dwRet = SHGetValueA(HKEY_CURRENT_USER, REG_TEST_KEY, "Test1", &dwType, buf, &dwSize);
	ok( ERROR_SUCCESS == dwRet, "SHGetValueA failed, ret=%lu\n", dwRet);
	ok( 0 == strcmp(sExpTestpath1, buf), "Comparing of (%s) with (%s) failed\n", buf, sExpTestpath1);
	ok( REG_SZ == dwType, "Expected REG_SZ, got (%lu)\n", dwType);

	strcpy(buf, sEmptyBuffer);
	dwSize = MAX_PATH;
	dwType = -1;
        dwRet = SHGetValueA(HKEY_CURRENT_USER, REG_TEST_KEY, "Test2", &dwType, buf, &dwSize);
	ok( ERROR_SUCCESS == dwRet, "SHGetValueA failed, ret=%lu\n", dwRet);
	ok( 0 == strcmp(sTestpath1, buf) , "Comparing of (%s) with (%s) failed\n", buf, sTestpath1);
	ok( REG_SZ == dwType , "Expected REG_SZ, got (%lu)\n", dwType);
}

static void test_SHGetRegPath(void)
{
	char buf[MAX_PATH];
        DWORD dwRet;

	if (!pSHRegGetPathA)
		return;

	strcpy(buf, sEmptyBuffer);
        dwRet = (*pSHRegGetPathA)(HKEY_CURRENT_USER, REG_TEST_KEY, "Test1", buf, 0);
	ok( ERROR_SUCCESS == dwRet, "SHRegGetPathA failed, ret=%lu\n", dwRet);
	ok( 0 == strcmp(sExpTestpath1, buf) , "Comparing (%s) with (%s) failed\n", buf, sExpTestpath1);
}

static void test_SHQUeryValueEx(void)
{
	HKEY hKey;
	DWORD dwSize;
	DWORD dwType;
	char buf[MAX_PATH];
	DWORD dwRet;
	const char * sTestedFunction = "";
	DWORD nUsedBuffer1,nUsedBuffer2;

        sTestedFunction = "RegOpenKeyExA";
        dwRet = RegOpenKeyExA(HKEY_CURRENT_USER, REG_TEST_KEY, 0,  KEY_QUERY_VALUE, &hKey);
	ok( ERROR_SUCCESS == dwRet, "%s failed, ret=%lu\n", sTestedFunction, dwRet);

	/****** SHQueryValueExA ******/

	sTestedFunction = "SHQueryValueExA";
	nUsedBuffer1 = max(strlen(sExpTestpath1)+1, strlen(sTestpath1)+1);
	nUsedBuffer2 = max(strlen(sExpTestpath2)+1, strlen(sTestpath2)+1);
	/*
	 * Case 1.1 All arguments are NULL
	 */
        dwRet = SHQueryValueExA( hKey, "Test1", NULL, NULL, NULL, NULL);
	ok( ERROR_SUCCESS == dwRet, "%s failed, ret=%lu\n", sTestedFunction, dwRet);

	/*
	 * Case 1.2 dwType is set
	 */
	dwType = -1;
        dwRet = SHQueryValueExA( hKey, "Test1", NULL, &dwType, NULL, NULL);
	ok( ERROR_SUCCESS == dwRet, "%s failed, ret=%lu\n", sTestedFunction, dwRet);
	ok( REG_SZ == dwType , "Expected REG_SZ, got (%lu)\n", dwType);

	/*
	 * dwSize is set
         * dwExpanded < dwUnExpanded
	 */
	dwSize = 6;
        dwRet = SHQueryValueExA( hKey, "Test1", NULL, NULL, NULL, &dwSize);
	ok( ERROR_SUCCESS == dwRet, "%s failed, ret=%lu\n", sTestedFunction, dwRet);
	ok( dwSize == nUsedBuffer1, "Buffer sizes (%lu) and (%lu) are not equal\n", dwSize, nUsedBuffer1);

	/*
         * dwExpanded > dwUnExpanded
	 */
	dwSize = 6;
        dwRet = SHQueryValueExA( hKey, "Test3", NULL, NULL, NULL, &dwSize);
	ok( ERROR_SUCCESS == dwRet, "%s failed, ret=%lu\n", sTestedFunction, dwRet);
	ok( dwSize >= nUsedBuffer2, "Buffer size (%lu) should be >= (%lu)\n", dwSize, nUsedBuffer2);

	/*
	 * Case 1 string shrinks during expanding
	 */
	strcpy(buf, sEmptyBuffer);
	dwSize = 6;
	dwType = -1;
	dwRet = SHQueryValueExA( hKey, "Test1", NULL, &dwType, buf, &dwSize);
	ok( ERROR_MORE_DATA == dwRet, "Expected ERROR_MORE_DATA, got (%lu)\n", dwRet);
	ok( 0 == strcmp(sEmptyBuffer, buf) , "Comparing (%s) with (%s) failed\n", buf, sEmptyBuffer);
	ok( dwSize == nUsedBuffer1, "Buffer sizes (%lu) and (%lu) are not equal\n", dwSize, nUsedBuffer1);
	ok( REG_SZ == dwType , "Expected REG_SZ, got (%lu)\n", dwType);

	/*
	 * string grows during expanding
         * dwSize is smaller then the size of the unexpanded string
	 */
	strcpy(buf, sEmptyBuffer);
	dwSize = 6;
	dwType = -1;
	dwRet = SHQueryValueExA( hKey, "Test3", NULL, &dwType, buf, &dwSize);
	ok( ERROR_MORE_DATA == dwRet, "Expected ERROR_MORE_DATA, got (%lu)\n", dwRet);
	ok( 0 == strcmp(sEmptyBuffer, buf) , "Comparing (%s) with (%s) failed\n", buf, sEmptyBuffer);
	ok( dwSize >= nUsedBuffer2, "Buffer size (%lu) should be >= (%lu)\n", dwSize, nUsedBuffer2);
	ok( REG_SZ == dwType , "Expected REG_SZ, got (%lu)\n", dwType);

        /*
         * string grows during expanding
         * dwSize is larger then the size of the unexpanded string but smaller than the part before the backslash
         * if the unexpanded string fits into the buffer it can get cut when expanded
         */
        strcpy(buf, sEmptyBuffer);
        dwSize = strlen(sEnvvar2) - 2;
        dwType = -1;
        dwRet = SHQueryValueExA( hKey, "Test3", NULL, &dwType, buf, &dwSize);
        ok( ERROR_MORE_DATA == dwRet, "Expected ERROR_MORE_DATA, got (%lu)\n", dwRet);

        todo_wine
        {
                ok( (0 == strcmp("", buf)) | (0 == strcmp(sTestpath2, buf)),
                    "Expected empty or unexpanded string (win98), got (%s)\n", buf); 
        }

        ok( dwSize >= nUsedBuffer2, "Buffer size (%lu) should be >= (%lu)\n", dwSize, nUsedBuffer2);
        ok( REG_SZ == dwType , "Expected REG_SZ, got (%lu)\n", dwType);

	/*
         * string grows during expanding
         * dwSize is larger then the size of the part before the backslash but smaller then the expanded string
	 * if the unexpanded string fits into the buffer it can get cut when expanded
	 */
	strcpy(buf, sEmptyBuffer);
	dwSize = sExpLen2 - 4;
	dwType = -1;
        dwRet = SHQueryValueExA( hKey, "Test3", NULL, &dwType, buf, &dwSize);
	ok( ERROR_MORE_DATA == dwRet, "Expected ERROR_MORE_DATA, got (%lu)\n", dwRet);

        todo_wine
        {
                ok( (0 == strcmp("", buf)) | (0 == strcmp(sEnvvar2, buf)),
                    "Expected empty or first part of the string \"%s\", got \"%s\"\n", sEnvvar2, buf);
        }

	ok( dwSize >= nUsedBuffer2, "Buffer size (%lu) should be >= (%lu)\n", dwSize, nUsedBuffer2);
	ok( REG_SZ == dwType , "Expected REG_SZ, got (%lu)\n", dwType);

	/*
	 * The buffer is NULL but the size is set
	 */
	strcpy(buf, sEmptyBuffer);
	dwSize = 6;
	dwType = -1;
	dwRet = SHQueryValueExA( hKey, "Test3", NULL, &dwType, NULL, &dwSize);
	ok( ERROR_SUCCESS == dwRet, "%s failed, ret=%lu\n", sTestedFunction, dwRet);
	ok( dwSize >= nUsedBuffer2, "Buffer size (%lu) should be >= (%lu)\n", dwSize, nUsedBuffer2);
	ok( REG_SZ == dwType , "Expected REG_SZ, got (%lu)\n", dwType);

	RegCloseKey(hKey);
}

static void test_SHCopyKey(void)
{
	HKEY hKeySrc, hKeyDst;
        DWORD dwRet;

	/* Delete existing destination sub keys */
	hKeyDst = NULL;
	if (!RegOpenKeyA(HKEY_CURRENT_USER, REG_TEST_KEY "\\CopyDestination", &hKeyDst) && hKeyDst)
	{
		SHDeleteKeyA(hKeyDst, NULL);
		RegCloseKey(hKeyDst);
	}

	hKeyDst = NULL;
        dwRet = RegCreateKeyA(HKEY_CURRENT_USER, REG_TEST_KEY "\\CopyDestination", &hKeyDst);
        if (dwRet || !hKeyDst)
	{
                ok( 0, "Destination couldn't be created, RegCreateKeyA returned (%lu)\n", dwRet);
		return;
	}

	hKeySrc = NULL;
        dwRet = RegOpenKeyA(HKEY_LOCAL_MACHINE, REG_CURRENT_VERSION, &hKeySrc);
        if (dwRet || !hKeySrc)
	{
                ok( 0, "Source couldn't be opened, RegOpenKeyA returned (%lu)\n", dwRet);
		return;
	}


	if (pSHCopyKeyA)
        {
                dwRet = (*pSHCopyKeyA)(hKeySrc, NULL, hKeyDst, 0);
                ok ( ERROR_SUCCESS == dwRet, "Copy failed, ret=(%lu)\n", dwRet);
        }

	RegCloseKey(hKeySrc);
	RegCloseKey(hKeyDst);

        /* Check we copied the sub keys, i.e. something that's on every windows system (including Wine) */
	hKeyDst = NULL;
        dwRet = RegOpenKeyA(HKEY_CURRENT_USER, REG_TEST_KEY "\\CopyDestination\\Setup", &hKeyDst);
        if (dwRet || !hKeyDst)
	{
                ok ( 0, "Copy couldn't be opened, RegOpenKeyA returned (%lu)\n", dwRet);
		return;
	}

	/* And the we copied the values too */
	ok(!SHQueryValueExA(hKeyDst, "BootDir", NULL, NULL, NULL, NULL), "SHQueryValueExA failed\n");

	RegCloseKey(hKeyDst);
}

static void test_SHDeleteKey(void)
{
    HKEY hKeyTest, hKeyS;
    DWORD dwRet;
    int sysfail = 1;

    if (!RegOpenKeyA(HKEY_CURRENT_USER, REG_TEST_KEY, &hKeyTest))
    {
        if (!RegCreateKey(hKeyTest, "ODBC", &hKeyS))
        {
            HKEY hKeyO;

            if (!RegCreateKey(hKeyS, "ODBC.INI", &hKeyO))
            {
                RegCloseKey (hKeyO);

                if (!RegCreateKey(hKeyS, "ODBCINST.INI", &hKeyO))
                {
                    RegCloseKey (hKeyO);
                    sysfail = 0;
                }
            }
            RegCloseKey (hKeyS);
        }
        RegCloseKey (hKeyTest);
    }

    if (!sysfail)
    {

        dwRet = SHDeleteKeyA(HKEY_CURRENT_USER, REG_TEST_KEY "\\ODBC");
        ok ( ERROR_SUCCESS == dwRet, "SHDeleteKey failed, ret=(%lu)\n", dwRet);

        dwRet = RegOpenKeyA(HKEY_CURRENT_USER, REG_TEST_KEY "\\ODBC", &hKeyS);
        ok ( ERROR_FILE_NOT_FOUND == dwRet, "SHDeleteKey did not delete\n");

        if (dwRet == ERROR_SUCCESS)
            RegCloseKey (hKeyS);
    }
    else
        ok( 0, "Could not set up SHDeleteKey test\n");
}

START_TEST(shreg)
{
	HKEY hkey = create_test_entries();

        if (!hkey) return;

	hshlwapi = GetModuleHandleA("shlwapi.dll");
	if (hshlwapi)
	{
		pSHCopyKeyA=(SHCopyKeyA_func)GetProcAddress(hshlwapi,"SHCopyKeyA");
		pSHRegGetPathA=(SHRegGetPathA_func)GetProcAddress(hshlwapi,"SHRegGetPathA");
	}
	test_SHGetValue();
	test_SHQUeryValueEx();
	test_SHGetRegPath();
	test_SHCopyKey();
        test_SHDeleteKey();
        delete_key( hkey, "Software\\Wine", "Test" );
}
