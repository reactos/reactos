/*
 * comctl32 MRU unit tests
 *
 * Copyright (C) 2004 Jon Griffiths <jon_p_griffiths@yahoo.com>
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
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "commctrl.h"
#include "shlwapi.h"

#include "wine/test.h"

/* Keys for testing MRU functions */
#define REG_TEST_BASEKEYA    "Software\\Wine"
#define REG_TEST_BASESUBKEYA "Test"
#define REG_TEST_KEYA    REG_TEST_BASEKEYA "\\" REG_TEST_BASESUBKEYA
#define REG_TEST_SUBKEYA "MRUTest"
#define REG_TEST_FULLKEY REG_TEST_KEYA "\\" REG_TEST_SUBKEYA

/* Undocumented MRU structures & functions */
typedef struct tagCREATEMRULISTA
{
    DWORD   cbSize;
    DWORD   nMaxItems;
    DWORD   dwFlags;
    HKEY    hKey;
    LPCSTR  lpszSubKey;
    PROC    lpfnCompare;
} CREATEMRULISTA, *LPCREATEMRULISTA;

#define MRUF_STRING_LIST  0
#define MRUF_BINARY_LIST  1
#define MRUF_DELAYED_SAVE 2

#define LIST_SIZE 3 /* Max entries for each mru */

static CREATEMRULISTA mruA =
{
    sizeof(CREATEMRULISTA),
    LIST_SIZE,
    0,
    NULL,
    REG_TEST_SUBKEYA,
    NULL
};

static HMODULE hComctl32;
static HANDLE (WINAPI *pCreateMRUListA)(LPCREATEMRULISTA);
static void   (WINAPI *pFreeMRUList)(HANDLE);
static INT    (WINAPI *pAddMRUStringA)(HANDLE,LPCSTR);
/*
static INT    (WINAPI *pFindMRUStringA)(HANDLE,LPCSTR,LPINT);
static INT    (WINAPI *pEnumMRUList)(HANDLE,INT,LPVOID,DWORD);
*/

static BOOL create_reg_entries(void)
{
    HKEY hKey = NULL;

    ok(!RegCreateKeyA(HKEY_CURRENT_USER, REG_TEST_FULLKEY, &hKey),
       "Couldn't create test key \"%s\"\n", REG_TEST_KEYA);
    if (!hKey) return FALSE;
    RegCloseKey(hKey);
    return TRUE;
}

static void delete_reg_entries(void)
{
    HKEY hKey;

    if (RegOpenKeyExA(HKEY_CURRENT_USER, REG_TEST_BASEKEYA, 0, KEY_ALL_ACCESS,
                      &hKey))
        return;
    SHDeleteKeyA(hKey, REG_TEST_BASESUBKEYA);
    RegCloseKey(hKey);
}

static void check_reg_entries(const char *mrulist, const char**items)
{
    char buff[128];
    HKEY hKey = NULL;
    DWORD type, size, ret;
    unsigned int i;

    ok(!RegOpenKeyA(HKEY_CURRENT_USER, REG_TEST_FULLKEY, &hKey),
       "Couldn't open test key \"%s\"\n", REG_TEST_FULLKEY);
    if (!hKey) return;

    type = REG_SZ;
    size = sizeof(buff);
    buff[0] = '\0';
    ret = RegQueryValueExA(hKey, "MRUList", NULL, &type, (LPBYTE)buff, &size);

    ok(!ret && buff[0], "Checking MRU: got %d from RegQueryValueExW\n", ret);
    if(ret || !buff[0]) return;

    ok(strcmp(buff, mrulist) == 0, "Checking MRU: Expected list %s, got %s\n",
       mrulist, buff);
    if(strcmp(buff, mrulist)) return;

    for (i = 0; i < strlen(mrulist); i++)
    {
        char name[2];
        name[0] = mrulist[i];
        name[1] = '\0';
        type = REG_SZ;
        size = sizeof(buff);
        buff[0] = '\0';
        ret = RegQueryValueExA(hKey, name, NULL, &type, (LPBYTE)buff, &size);
        ok(!ret && buff[0],
           "Checking MRU item %d ('%c'): got %d from RegQueryValueExW\n",
           i, mrulist[i], ret);
        if(ret || !buff[0]) return;
        ok(!strcmp(buff, items[mrulist[i]-'a']),
           "Checking MRU item %d ('%c'): expected \"%s\", got \"%s\"\n",
           i, mrulist[i], buff, items[mrulist[i] - 'a']);
    }
}

static INT CALLBACK cmp_mru_strA(LPCVOID data1, LPCVOID data2)
{
    return lstrcmpiA(data1, data2);
}

static HANDLE create_mruA(HKEY hKey, DWORD flags, PROC cmp)
{
    mruA.dwFlags = flags;
    mruA.lpfnCompare = cmp;
    mruA.hKey = hKey;

    SetLastError(0);
    return pCreateMRUListA(&mruA);
}

static void test_MRUListA(void)
{
    const char *checks[LIST_SIZE+1];
    HANDLE hMRU;
    HKEY hKey;
    INT iRet;

    pCreateMRUListA = (void*)GetProcAddress(hComctl32,(LPCSTR)151);
    pFreeMRUList = (void*)GetProcAddress(hComctl32,(LPCSTR)152);
    pAddMRUStringA = (void*)GetProcAddress(hComctl32,(LPCSTR)153);
    if (!pCreateMRUListA || !pFreeMRUList || !pAddMRUStringA)
        return;

    if (0)
    {
    /* Create (NULL) - crashes native */
    hMRU = pCreateMRUListA(NULL);
    }

    /* Create (size too small) */
    mruA.cbSize = sizeof(mruA) - 2;
    hMRU = create_mruA(NULL, MRUF_STRING_LIST, (PROC)cmp_mru_strA);
    ok (!hMRU && !GetLastError(),
        "CreateMRUListA(too small) expected NULL,0 got %p,%d\n",
        hMRU, GetLastError());
    mruA.cbSize = sizeof(mruA);

    /* Create (size too big) */
    mruA.cbSize = sizeof(mruA) + 2;
    hMRU = create_mruA(NULL, MRUF_STRING_LIST, (PROC)cmp_mru_strA);
    ok (!hMRU && !GetLastError(),
        "CreateMRUListA(too big) expected NULL,0 got %p,%d\n",
        hMRU, GetLastError());
    mruA.cbSize = sizeof(mruA);

    /* Create (NULL hKey) */
    hMRU = create_mruA(NULL, MRUF_STRING_LIST, (PROC)cmp_mru_strA);
    ok (!hMRU && !GetLastError(),
        "CreateMRUListA(NULL key) expected NULL,0 got %p,%d\n",
        hMRU, GetLastError());

    /* Create (NULL name) */
    mruA.lpszSubKey = NULL;
    hMRU = create_mruA(NULL, MRUF_STRING_LIST, (PROC)cmp_mru_strA);
    ok (!hMRU && !GetLastError(),
        "CreateMRUListA(NULL name) expected NULL,0 got %p,%d\n",
        hMRU, GetLastError());
    mruA.lpszSubKey = REG_TEST_SUBKEYA;

    /* Create a string MRU */
    ok(!RegCreateKeyA(HKEY_CURRENT_USER, REG_TEST_KEYA, &hKey),
       "Couldn't create test key \"%s\"\n", REG_TEST_KEYA);
    if (!hKey)
        return;
    hMRU = create_mruA(hKey, MRUF_STRING_LIST, (PROC)cmp_mru_strA);
    ok(hMRU && !GetLastError(),
       "CreateMRUListA(string) expected non-NULL,0 got %p,%d\n",
       hMRU, GetLastError());

    if (hMRU)
    {
        checks[0] = "Test 1";
        checks[1] = "Test 2";
        checks[2] = "Test 3";
        checks[3] = "Test 4";

        /* Add (NULL list) */
        SetLastError(0);
        iRet = pAddMRUStringA(NULL, checks[0]);
        ok(iRet == -1 && !GetLastError(),
           "AddMRUStringA(NULL list) expected -1,0 got %d,%d\n",
           iRet, GetLastError());

        /* Add (NULL string) */
        if (0)
        {
	/* Some native versions crash when passed NULL or fail to SetLastError()  */
        SetLastError(0);
        iRet = pAddMRUStringA(hMRU, NULL);
        ok(iRet == 0 && GetLastError() == ERROR_INVALID_PARAMETER,
           "AddMRUStringA(NULL str) expected 0,ERROR_INVALID_PARAMETER got %d,%d\n",
           iRet, GetLastError());
        }

        /* Add 3 strings. Check the registry is correct after each add */
        SetLastError(0);
        iRet = pAddMRUStringA(hMRU, checks[0]);
        ok(iRet == 0 && !GetLastError(),
           "AddMRUStringA(1) expected 0,0 got %d,%d\n",
           iRet, GetLastError());
        check_reg_entries("a", checks);

        SetLastError(0);
        iRet = pAddMRUStringA(hMRU, checks[1]);
        ok(iRet == 1 && !GetLastError(),
           "AddMRUStringA(2) expected 1,0 got %d,%d\n",
           iRet, GetLastError());
        check_reg_entries("ba", checks);

        SetLastError(0);
        iRet = pAddMRUStringA(hMRU, checks[2]);
        ok(iRet == 2 && !GetLastError(),
           "AddMRUStringA(2) expected 2,0 got %d,%d\n",
           iRet, GetLastError());
        check_reg_entries("cba", checks);

        /* Add a duplicate of the 2nd string - it should move to the front,
         * but keep the same index in the registry.
         */
        SetLastError(0);
        iRet = pAddMRUStringA(hMRU, checks[1]);
        ok(iRet == 1 && !GetLastError(),
           "AddMRUStringA(re-add 1) expected 1,0 got %d,%d\n",
           iRet, GetLastError());
        check_reg_entries("bca", checks);

        /* Add a new string - replaces the oldest string + moves to the front */
        SetLastError(0);
        iRet = pAddMRUStringA(hMRU, checks[3]);
        ok(iRet == 0 && !GetLastError(),
           "AddMRUStringA(add new) expected 0,0 got %d,%d\n",
           iRet, GetLastError());
        checks[0] = checks[3];
        check_reg_entries("abc", checks);

        /* Finished with this MRU */
        pFreeMRUList(hMRU);
    }

    /* Free (NULL list) - Doesn't crash */
    pFreeMRUList(NULL);
}

START_TEST(mru)
{
    hComctl32 = GetModuleHandleA("comctl32.dll");

    delete_reg_entries();
    if (!create_reg_entries())
        return;

    test_MRUListA();

    delete_reg_entries();
}
