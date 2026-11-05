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

/* Undocumented MRU functions */
typedef struct tagMRUINFOA
{
    DWORD   cbSize;
    UINT    uMax;
    UINT    fFlags;
    HKEY    hKey;
    LPCSTR  lpszSubKey;
    int (CALLBACK *lpfnCompare)(LPCSTR, LPCSTR);
} MRUINFOA;

typedef struct tagMRUINFOW
{
    DWORD   cbSize;
    UINT    uMax;
    UINT    fFlags;
    HKEY    hKey;
    LPCWSTR lpszSubKey;
    int (CALLBACK *lpfnCompare)(LPCWSTR, LPCWSTR);
} MRUINFOW;

#define MRU_STRING     0  /* this one's invented */
#define MRU_BINARY     1
#define MRU_CACHEWRITE 2

#define LIST_SIZE 3 /* Max entries for each mru */

static HMODULE hComctl32;
static HANDLE (WINAPI *pCreateMRUListA)(MRUINFOA*);
static void   (WINAPI *pFreeMRUList)(HANDLE);
static INT    (WINAPI *pAddMRUStringA)(HANDLE,LPCSTR);
static INT    (WINAPI *pEnumMRUListA)(HANDLE,INT,LPVOID,DWORD);
static INT    (WINAPI *pEnumMRUListW)(HANDLE,INT,LPVOID,DWORD);
static HANDLE (WINAPI *pCreateMRUListLazyA)(MRUINFOA*, DWORD, DWORD, DWORD);
static HANDLE (WINAPI *pCreateMRUListLazyW)(MRUINFOW*, DWORD, DWORD, DWORD);
static INT    (WINAPI *pFindMRUData)(HANDLE, LPCVOID, DWORD, LPINT);
static INT    (WINAPI *pAddMRUData)(HANDLE, LPCVOID, DWORD);
static HANDLE (WINAPI *pCreateMRUListW)(MRUINFOW*);

static void init_functions(void)
{
    hComctl32 = LoadLibraryA("comctl32.dll");

#define X2(f, ord) p##f = (void*)GetProcAddress(hComctl32, (const char *)ord);
    X2(CreateMRUListA, 151);
    X2(FreeMRUList, 152);
    X2(AddMRUStringA, 153);
    X2(EnumMRUListA, 154);
    X2(CreateMRUListLazyA, 157);
    X2(AddMRUData, 167);
    X2(FindMRUData, 169);
    X2(CreateMRUListW, 400);
    X2(EnumMRUListW, 403);
    X2(CreateMRUListLazyW, 404);
#undef X2
}

/* Based on RegDeleteTreeW from dlls/advapi32/registry.c */
static LSTATUS mru_RegDeleteTreeA(HKEY hKey, LPCSTR lpszSubKey)
{
    LONG ret;
    DWORD dwMaxSubkeyLen, dwMaxValueLen;
    DWORD dwMaxLen, dwSize;
    CHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;
    HKEY hSubKey = hKey;

    if(lpszSubKey)
    {
        ret = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
        if (ret) return ret;
    }

    /* Get highest length for keys, values */
    ret = RegQueryInfoKeyA(hSubKey, NULL, NULL, NULL, NULL,
            &dwMaxSubkeyLen, NULL, NULL, &dwMaxValueLen, NULL, NULL, NULL);
    if (ret) goto cleanup;

    dwMaxSubkeyLen++;
    dwMaxValueLen++;
    dwMaxLen = max(dwMaxSubkeyLen, dwMaxValueLen);
    if (dwMaxLen > ARRAY_SIZE(szNameBuf))
    {
        /* Name too big: alloc a buffer for it */
        if (!(lpszName = malloc(dwMaxLen * sizeof(CHAR))))
        {
            ret = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }


    /* Recursively delete all the subkeys */
    while (TRUE)
    {
        dwSize = dwMaxLen;
        if (RegEnumKeyExA(hSubKey, 0, lpszName, &dwSize, NULL,
                          NULL, NULL, NULL)) break;

        ret = mru_RegDeleteTreeA(hSubKey, lpszName);
        if (ret) goto cleanup;
    }

    if (lpszSubKey)
        ret = RegDeleteKeyA(hKey, lpszSubKey);
    else
        while (TRUE)
        {
            dwSize = dwMaxLen;
            if (RegEnumValueA(hKey, 0, lpszName, &dwSize,
                  NULL, NULL, NULL, NULL)) break;

            ret = RegDeleteValueA(hKey, lpszName);
            if (ret) goto cleanup;
        }

cleanup:
    /* Free buffer if allocated */
    if (lpszName != szNameBuf)
        free(lpszName);
    if(lpszSubKey)
        RegCloseKey(hSubKey);
    return ret;
}

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
    mru_RegDeleteTreeA(hKey, REG_TEST_BASESUBKEYA);
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

    ok(!ret && buff[0], "Checking MRU: got %ld from RegQueryValueExW\n", ret);
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
           "Checking MRU item %d ('%c'): got %ld from RegQueryValueExW\n",
           i, mrulist[i], ret);
        if(ret || !buff[0]) return;
        ok(!strcmp(buff, items[mrulist[i]-'a']),
           "Checking MRU item %d ('%c'): expected \"%s\", got \"%s\"\n",
           i, mrulist[i], buff, items[mrulist[i] - 'a']);
    }
}

static int CALLBACK cmp_mru_strA(LPCSTR data1, LPCSTR data2)
{
    return lstrcmpiA(data1, data2);
}

static void test_MRUListA(void)
{
    const char *checks[LIST_SIZE+1];
    MRUINFOA infoA;
    HANDLE hMRU;
    HKEY hKey;
    INT iRet;

    if (!pCreateMRUListA || !pFreeMRUList || !pAddMRUStringA || !pEnumMRUListA)
    {
        win_skip("MRU entry points not found\n");
        return;
    }

    if (0)
    {
    /* Create (NULL) - crashes native */
    hMRU = pCreateMRUListA(NULL);
    }

    /* size too small */
    infoA.cbSize = sizeof(infoA) - 2;
    infoA.uMax = LIST_SIZE;
    infoA.fFlags = MRU_STRING;
    infoA.hKey = NULL;
    infoA.lpszSubKey = REG_TEST_SUBKEYA;
    infoA.lpfnCompare = cmp_mru_strA;

    SetLastError(0);
    hMRU = pCreateMRUListA(&infoA);
    ok (!hMRU && !GetLastError(),
        "CreateMRUListA(too small) expected NULL,0 got %p,%ld\n",
        hMRU, GetLastError());

    /* size too big */
    infoA.cbSize = sizeof(infoA) + 2;
    infoA.uMax = LIST_SIZE;
    infoA.fFlags = MRU_STRING;
    infoA.hKey = NULL;
    infoA.lpszSubKey = REG_TEST_SUBKEYA;
    infoA.lpfnCompare = cmp_mru_strA;

    SetLastError(0);
    hMRU = pCreateMRUListA(&infoA);
    ok (!hMRU && !GetLastError(),
        "CreateMRUListA(too big) expected NULL,0 got %p,%ld\n",
        hMRU, GetLastError());

    /* NULL hKey */
    infoA.cbSize = sizeof(infoA);
    infoA.uMax = LIST_SIZE;
    infoA.fFlags = MRU_STRING;
    infoA.hKey = NULL;
    infoA.lpszSubKey = REG_TEST_SUBKEYA;
    infoA.lpfnCompare = cmp_mru_strA;

    SetLastError(0);
    hMRU = pCreateMRUListA(&infoA);
    ok (!hMRU && !GetLastError(),
        "CreateMRUListA(NULL key) expected NULL,0 got %p,%ld\n",
        hMRU, GetLastError());

    /* NULL subkey name */
    infoA.cbSize = sizeof(infoA);
    infoA.uMax = LIST_SIZE;
    infoA.fFlags = MRU_STRING;
    infoA.hKey = NULL;
    infoA.lpszSubKey = NULL;
    infoA.lpfnCompare = cmp_mru_strA;

    SetLastError(0);
    hMRU = pCreateMRUListA(&infoA);
    ok (!hMRU && !GetLastError(),
        "CreateMRUListA(NULL name) expected NULL,0 got %p,%ld\n",
        hMRU, GetLastError());

    /* Create a string MRU */
    ok(!RegCreateKeyA(HKEY_CURRENT_USER, REG_TEST_KEYA, &hKey),
       "Couldn't create test key \"%s\"\n", REG_TEST_KEYA);
    if (!hKey) return;

    infoA.cbSize = sizeof(infoA);
    infoA.uMax = LIST_SIZE;
    infoA.fFlags = MRU_STRING;
    infoA.hKey = hKey;
    infoA.lpszSubKey = REG_TEST_SUBKEYA;
    infoA.lpfnCompare = cmp_mru_strA;

    hMRU = pCreateMRUListA(&infoA);
    ok(hMRU && !GetLastError(),
       "CreateMRUListA(string) expected non-NULL,0 got %p,%ld\n",
       hMRU, GetLastError());

    if (hMRU)
    {
        char buffer[255];
        checks[0] = "Test 1";
        checks[1] = "Test 2";
        checks[2] = "Test 3";
        checks[3] = "Test 4";

        /* Add (NULL list) */
        SetLastError(0);
        iRet = pAddMRUStringA(NULL, checks[0]);
        ok(iRet == -1 && !GetLastError(),
           "AddMRUStringA(NULL list) expected -1,0 got %d,%ld\n",
           iRet, GetLastError());

        /* Add (NULL string) */
        SetLastError(0);
        iRet = pAddMRUStringA(hMRU, NULL);
        todo_wine ok(iRet == 0 && !GetLastError(),
           "AddMRUStringA(NULL str) expected 0,0 got %d,%ld\n",
           iRet, GetLastError());

        /* Add 3 strings. Check the registry is correct after each add */
        SetLastError(0);
        iRet = pAddMRUStringA(hMRU, checks[0]);
        ok(iRet == 0 && !GetLastError(),
           "AddMRUStringA(1) expected 0,0 got %d,%ld\n",
           iRet, GetLastError());
        check_reg_entries("a", checks);

        SetLastError(0);
        iRet = pAddMRUStringA(hMRU, checks[1]);
        ok(iRet == 1 && !GetLastError(),
           "AddMRUStringA(2) expected 1,0 got %d,%ld\n",
           iRet, GetLastError());
        check_reg_entries("ba", checks);

        SetLastError(0);
        iRet = pAddMRUStringA(hMRU, checks[2]);
        ok(iRet == 2 && !GetLastError(),
           "AddMRUStringA(2) expected 2,0 got %d,%ld\n",
           iRet, GetLastError());
        check_reg_entries("cba", checks);

        /* Add a duplicate of the 2nd string - it should move to the front,
         * but keep the same index in the registry.
         */
        SetLastError(0);
        iRet = pAddMRUStringA(hMRU, checks[1]);
        ok(iRet == 1 && !GetLastError(),
           "AddMRUStringA(re-add 1) expected 1,0 got %d,%ld\n",
           iRet, GetLastError());
        check_reg_entries("bca", checks);

        /* Add a new string - replaces the oldest string + moves to the front */
        SetLastError(0);
        iRet = pAddMRUStringA(hMRU, checks[3]);
        ok(iRet == 0 && !GetLastError(),
           "AddMRUStringA(add new) expected 0,0 got %d,%ld\n",
           iRet, GetLastError());
        checks[0] = checks[3];
        check_reg_entries("abc", checks);

        /* NULL buffer = get list size */
        iRet = pEnumMRUListA(hMRU, 0, NULL, 0);
        ok(iRet == 3 || iRet == -1 /* Vista */, "EnumMRUList expected %d or -1, got %d\n", LIST_SIZE, iRet);

        /* negative item pos = get list size */
        iRet = pEnumMRUListA(hMRU, -1, NULL, 0);
        ok(iRet == 3 || iRet == -1 /* Vista */, "EnumMRUList expected %d or -1, got %d\n", LIST_SIZE, iRet);

        /* negative item pos = get list size */
        iRet = pEnumMRUListA(hMRU, -5, NULL, 0);
        ok(iRet == 3 || iRet == -1 /* Vista */, "EnumMRUList expected %d or -1, got %d\n", LIST_SIZE, iRet);

        /* negative item pos = get list size */
        iRet = pEnumMRUListA(hMRU, -1, buffer, 255);
        ok(iRet == 3, "EnumMRUList expected %d, got %d\n", LIST_SIZE, iRet);

        /* negative item pos = get list size */
        iRet = pEnumMRUListA(hMRU, -5, buffer, 255);
        ok(iRet == 3, "EnumMRUList expected %d, got %d\n", LIST_SIZE, iRet);

        /* check entry 0 */
        buffer[0] = 0;
        iRet = pEnumMRUListA(hMRU, 0, buffer, 255);
        ok(iRet == lstrlenA(checks[3]), "EnumMRUList expected %d, got %d\n", lstrlenA(checks[3]), iRet);
        ok(strcmp(buffer, checks[3]) == 0, "EnumMRUList expected %s, got %s\n", checks[3], buffer);

        /* check entry 0 with a too small buffer */
        buffer[0] = 0;   /* overwritten with 'T' */
        buffer[1] = 'A'; /* overwritten with 0   */
        buffer[2] = 'A'; /* unchanged */
        buffer[3] = 0;   /* unchanged */
        iRet = pEnumMRUListA(hMRU, 0, buffer, 2);
        ok(iRet == lstrlenA(checks[3]), "EnumMRUList expected %d, got %d\n", lstrlenA(checks[3]), iRet);
        ok(strcmp(buffer, "T") == 0, "EnumMRUList expected %s, got %s\n", "T", buffer);
        /* make sure space after buffer has old values */
        ok(buffer[2] == 'A', "EnumMRUList expected %02x, got %02x\n", 'A', buffer[2]);

        /* check entry 1 */
        buffer[0] = 0;
        iRet = pEnumMRUListA(hMRU, 1, buffer, 255);
        ok(iRet == lstrlenA(checks[1]), "EnumMRUList expected %d, got %d\n", lstrlenA(checks[1]), iRet);
        ok(strcmp(buffer, checks[1]) == 0, "EnumMRUList expected %s, got %s\n", checks[1], buffer);

        /* check entry 2 */
        buffer[0] = 0;
        iRet = pEnumMRUListA(hMRU, 2, buffer, 255);
        ok(iRet == lstrlenA(checks[2]), "EnumMRUList expected %d, got %d\n", lstrlenA(checks[2]), iRet);
        ok(strcmp(buffer, checks[2]) == 0, "EnumMRUList expected %s, got %s\n", checks[2], buffer);

        /* check out of bounds entry 3 */
        strcpy(buffer, "dummy");
        iRet = pEnumMRUListA(hMRU, 3, buffer, 255);
        ok(iRet == -1, "EnumMRUList expected %d, got %d\n", -1, iRet);
        ok(strcmp(buffer, "dummy") == 0, "EnumMRUList expected unchanged buffer %s, got %s\n", "dummy", buffer);

        /* Finished with this MRU */
        pFreeMRUList(hMRU);
    }

    pFreeMRUList(NULL); /* should not crash */
}

typedef struct {
    MRUINFOA mruA;
    BOOL ret;
} create_lazya_t;

static const create_lazya_t create_lazyA[] = {
    {{ sizeof(MRUINFOA) + 1, 0, 0, HKEY_CURRENT_USER, NULL, NULL }, FALSE },
    {{ sizeof(MRUINFOA) - 1, 0, 0, HKEY_CURRENT_USER, NULL, NULL }, FALSE },
    {{ sizeof(MRUINFOA) + 1, 0, 0, HKEY_CURRENT_USER, "WineTest", NULL }, TRUE },
    {{ sizeof(MRUINFOA) - 1, 0, 0, HKEY_CURRENT_USER, "WineTest", NULL }, TRUE },
    {{ sizeof(MRUINFOA), 0, 0, HKEY_CURRENT_USER, "WineTest", NULL }, TRUE },
    {{ sizeof(MRUINFOA), 0, 0, HKEY_CURRENT_USER, NULL, NULL }, FALSE },
    {{ sizeof(MRUINFOA), 0, 0, NULL, "WineTest", NULL }, FALSE },
    {{ 0, 0, 0, NULL, "WineTest", NULL }, FALSE },
    {{ 0, 0, 0, HKEY_CURRENT_USER, "WineTest", NULL }, TRUE }
};

static void test_CreateMRUListLazyA(void)
{
    int i;

    if (!pCreateMRUListLazyA || !pFreeMRUList)
    {
        win_skip("CreateMRUListLazyA or FreeMRUList entry points not found\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(create_lazyA); i++)
    {
        const create_lazya_t *ptr = &create_lazyA[i];
        HANDLE hMRU;

        hMRU = pCreateMRUListLazyA((MRUINFOA*)&ptr->mruA, 0, 0, 0);
        if (ptr->ret)
        {
            ok(hMRU != NULL, "%d: got %p\n", i, hMRU);
            pFreeMRUList(hMRU);
        }
        else
            ok(hMRU == NULL, "%d: got %p\n", i, hMRU);
    }
}

static void test_EnumMRUList(void)
{
    if (!pEnumMRUListA || !pEnumMRUListW)
    {
        win_skip("EnumMRUListA/EnumMRUListW entry point not found\n");
        return;
    }

    /* NULL handle - should not crash */
    pEnumMRUListA(NULL, 0, NULL, 0);
    pEnumMRUListW(NULL, 0, NULL, 0);
}

static void test_FindMRUData(void)
{
    INT iRet;

    if (!pFindMRUData)
    {
        win_skip("FindMRUData entry point not found\n");
        return;
    }

    /* NULL handle */
    iRet = pFindMRUData(NULL, NULL, 0, NULL);
    ok(iRet == -1, "FindMRUData expected -1, got %d\n", iRet);
}

static void test_AddMRUData(void)
{
    INT iRet;

    if (!pAddMRUData)
    {
        win_skip("AddMRUData entry point not found\n");
        return;
    }

    /* NULL handle */
    iRet = pFindMRUData(NULL, NULL, 0, NULL);
    ok(iRet == -1, "AddMRUData expected -1, got %d\n", iRet);
}

static void test_CreateMRUListW(void)
{
    MRUINFOW infoW;
    void *named;
    HKEY hKey;
    HANDLE hMru;

    if (!pCreateMRUListW)
    {
        win_skip("CreateMRUListW entry point not found\n");
        return;
    }

    /* exported by name too on recent versions */
    named = GetProcAddress(hComctl32, "CreateMRUListW");
    if (named)
        ok(named == pCreateMRUListW, "got %p, expected %p\n", named, pCreateMRUListW);

    ok(!RegCreateKeyA(HKEY_CURRENT_USER, REG_TEST_KEYA, &hKey),
       "Couldn't create test key \"%s\"\n", REG_TEST_KEYA);

    infoW.cbSize = sizeof(infoW);
    infoW.uMax = 1;
    infoW.fFlags = 0;
    infoW.lpszSubKey = L"MRUTest";
    infoW.hKey = hKey;
    infoW.lpfnCompare = NULL;

    hMru = pCreateMRUListW(&infoW);
    ok(hMru != NULL, "got %p\n", hMru);
    pFreeMRUList(hMru);

    /* smaller size */
    infoW.cbSize = sizeof(infoW) - 1;
    infoW.uMax = 1;
    infoW.fFlags = 0;
    infoW.lpszSubKey = L"MRUTest";
    infoW.hKey = hKey;
    infoW.lpfnCompare = NULL;

    hMru = pCreateMRUListW(&infoW);
    ok(hMru != NULL, "got %p\n", hMru);
    pFreeMRUList(hMru);

    /* increased size */
    infoW.cbSize = sizeof(infoW) + 1;
    infoW.uMax = 1;
    infoW.fFlags = 0;
    infoW.lpszSubKey = L"MRUTest";
    infoW.hKey = hKey;
    infoW.lpfnCompare = NULL;

    hMru = pCreateMRUListW(&infoW);
    ok(hMru != NULL, "got %p\n", hMru);
    pFreeMRUList(hMru);

    /* zero size */
    infoW.cbSize = 0;
    infoW.uMax = 1;
    infoW.fFlags = 0;
    infoW.lpszSubKey = L"MRUTest";
    infoW.hKey = hKey;
    infoW.lpfnCompare = NULL;

    hMru = pCreateMRUListW(&infoW);
    ok(hMru != NULL, "got %p\n", hMru);
    pFreeMRUList(hMru);

    /* NULL hKey */
    infoW.cbSize = sizeof(infoW);
    infoW.uMax = 1;
    infoW.fFlags = 0;
    infoW.lpszSubKey = L"MRUTest";
    infoW.hKey = NULL;
    infoW.lpfnCompare = NULL;

    hMru = pCreateMRUListW(&infoW);
    ok(hMru == NULL, "got %p\n", hMru);

    RegCloseKey(hKey);
}

static void test_CreateMRUListLazyW(void)
{
    MRUINFOW infoW;
    void *named;
    HKEY hKey;
    HANDLE hMru;

    if (!pCreateMRUListLazyW)
    {
        win_skip("CreateMRUListLazyW entry point not found\n");
        return;
    }

    /* check that it's not exported by name */
    named = GetProcAddress(hComctl32, "CreateMRUListLazyW");
    ok(named == NULL, "got %p\n", named);

    ok(!RegCreateKeyA(HKEY_CURRENT_USER, REG_TEST_KEYA, &hKey),
       "Couldn't create test key \"%s\"\n", REG_TEST_KEYA);

    infoW.cbSize = sizeof(infoW);
    infoW.uMax = 1;
    infoW.fFlags = 0;
    infoW.lpszSubKey = L"MRUTest";
    infoW.hKey = hKey;
    infoW.lpfnCompare = NULL;

    hMru = pCreateMRUListLazyW(&infoW, 0, 0, 0);
    ok(hMru != NULL, "got %p\n", hMru);
    pFreeMRUList(hMru);

    /* smaller size */
    infoW.cbSize = sizeof(infoW) - 1;
    infoW.uMax = 1;
    infoW.fFlags = 0;
    infoW.lpszSubKey = L"MRUTest";
    infoW.hKey = hKey;
    infoW.lpfnCompare = NULL;

    hMru = pCreateMRUListLazyW(&infoW, 0, 0, 0);
    ok(hMru != NULL, "got %p\n", hMru);
    pFreeMRUList(hMru);

    /* increased size */
    infoW.cbSize = sizeof(infoW) + 1;
    infoW.uMax = 1;
    infoW.fFlags = 0;
    infoW.lpszSubKey = L"MRUTest";
    infoW.hKey = hKey;
    infoW.lpfnCompare = NULL;

    hMru = pCreateMRUListLazyW(&infoW, 0, 0, 0);
    ok(hMru != NULL, "got %p\n", hMru);
    pFreeMRUList(hMru);

    /* zero size */
    infoW.cbSize = 0;
    infoW.uMax = 1;
    infoW.fFlags = 0;
    infoW.lpszSubKey = L"MRUTest";
    infoW.hKey = hKey;
    infoW.lpfnCompare = NULL;

    hMru = pCreateMRUListLazyW(&infoW, 0, 0, 0);
    ok(hMru != NULL, "got %p\n", hMru);
    pFreeMRUList(hMru);

    /* NULL hKey */
    infoW.cbSize = sizeof(infoW);
    infoW.uMax = 1;
    infoW.fFlags = 0;
    infoW.lpszSubKey = L"MRUTest";
    infoW.hKey = NULL;
    infoW.lpfnCompare = NULL;

    hMru = pCreateMRUListLazyW(&infoW, 0, 0, 0);
    ok(hMru == NULL, "got %p\n", hMru);

    RegCloseKey(hKey);
}

START_TEST(mru)
{
    delete_reg_entries();
    if (!create_reg_entries())
        return;

    init_functions();

    test_MRUListA();
    test_CreateMRUListLazyA();
    test_CreateMRUListLazyW();
    test_EnumMRUList();
    test_FindMRUData();
    test_AddMRUData();
    test_CreateMRUListW();

    delete_reg_entries();
}
