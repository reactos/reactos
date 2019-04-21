/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for Button window class v6
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include "wine/test.h"
#include <windows.h>

typedef INT (CALLBACK *MRUStringCmpFnW)(LPCWSTR lhs, LPCWSTR rhs);
typedef INT (CALLBACK *MRUBinaryCmpFn)(LPCVOID lhs, LPCVOID rhs, DWORD length);

/* https://docs.microsoft.com/en-us/windows/desktop/shell/mruinfo */
typedef struct tagMRUINFOW
{
    DWORD   cbSize;
    UINT    uMax;
    UINT    fFlags;
    HKEY    hKey;
    LPWSTR  lpszSubKey;
    union
    {
        MRUStringCmpFnW string_cmpfn;
        MRUBinaryCmpFn  binary_cmpfn;
    } u;
} MRUINFOW, *LPMRUINFOW;

/* flags for MRUINFOW.fFlags */
#define MRU_BINARY 0x0001
#define MRU_CACHEWRITE 0x0002

static WCHAR s_szMRUTest[] = L"MRUTest";
static const WCHAR s_szA[] = L"1st test data";
static const WCHAR s_szB[] = L"2nd test data";
static const WCHAR s_szC[] = L"3rd test data";
static const WCHAR s_szD[] = L"4th test data";
static const WCHAR s_szNewItem[] = L"New Item";
static const WCHAR s_szCADB[] = L"cadb";

/* comctl32.400: CreateMRUListW */
typedef HANDLE (WINAPI *CREATEMRULISTW)(const MRUINFOW *);
static CREATEMRULISTW s_pCreateMRUListW = NULL;

/* comctl32.401: AddMRUStringW */
typedef INT (WINAPI *ADDMRUSTRINGW)(HANDLE, LPCWSTR);
static ADDMRUSTRINGW s_pAddMRUStringW = NULL;

/* comctl32.402: FindMRUStringW */
typedef INT (WINAPI *FINDMRUSTRINGW)(HANDLE, LPCWSTR, LPINT);
static FINDMRUSTRINGW s_pFindMRUStringW = NULL;

/* comctl32.403: EnumMRUListW */
typedef INT (WINAPI *ENUMMRULISTW)(HANDLE, INT, LPVOID, DWORD);
static ENUMMRULISTW s_pEnumMRUListW = NULL;

/* comctl32.152: FreeMRUList */
typedef void (WINAPI *FREEMRULIST)(HANDLE);
static FREEMRULIST s_pFreeMRUList = NULL;

static void SetupRegistryforMRUListTest(BOOL bCreate)
{
    HKEY hKey = NULL;
    LONG result;
    DWORD cb;
    static const WCHAR s_szMRUList[] = L"MRUList";

    if (bCreate)
    {
        result = RegCreateKeyExW(HKEY_CURRENT_USER, s_szMRUTest, 0, NULL, 0,
                                 KEY_ALL_ACCESS | KEY_WOW64_32KEY,
                                 NULL, &hKey, NULL);
        ok_long(result, ERROR_SUCCESS);
        ok(hKey != NULL, "hKey was NULL\n");
        {
            cb = (lstrlenW(s_szCADB) + 1) * sizeof(WCHAR);
            result = RegSetValueExW(hKey, s_szMRUList, 0, REG_SZ, (LPBYTE)s_szCADB, cb);
            ok_long(result, ERROR_SUCCESS);

            cb = (lstrlenW(s_szA) + 1) * sizeof(WCHAR);
            result = RegSetValueExW(hKey, L"a", 0, REG_SZ, (LPBYTE)s_szA, cb);
            ok_long(result, ERROR_SUCCESS);

            cb = (lstrlenW(s_szB) + 1) * sizeof(WCHAR);
            result = RegSetValueExW(hKey, L"b", 0, REG_SZ, (LPBYTE)s_szB, cb);
            ok_long(result, ERROR_SUCCESS);

            cb = (lstrlenW(s_szC) + 1) * sizeof(WCHAR);
            result = RegSetValueExW(hKey, L"c", 0, REG_SZ, (LPBYTE)s_szC, cb);
            ok_long(result, ERROR_SUCCESS);

            cb = (lstrlenW(s_szD) + 1) * sizeof(WCHAR);
            result = RegSetValueExW(hKey, L"d", 0, REG_SZ, (LPBYTE)s_szD, cb);
            ok_long(result, ERROR_SUCCESS);
        }
        result = RegCloseKey(hKey);
        ok_long(result, ERROR_SUCCESS);
    }
    else
    {
        result = RegOpenKeyExW(HKEY_CURRENT_USER, s_szMRUTest, 0,
                               KEY_ALL_ACCESS | KEY_WOW64_32KEY,
                               &hKey);
        ok_long(result, ERROR_SUCCESS);
        ok(hKey != NULL, "hKey was NULL\n");
        {
            result = RegDeleteValueW(hKey, s_szMRUList);
            ok_long(result, ERROR_SUCCESS);

            result = RegDeleteValueW(hKey, L"a");
            ok_long(result, ERROR_SUCCESS);

            result = RegDeleteValueW(hKey, L"b");
            ok_long(result, ERROR_SUCCESS);

            result = RegDeleteValueW(hKey, L"c");
            ok_long(result, ERROR_SUCCESS);

            result = RegDeleteValueW(hKey, L"d");
            ok_long(result, ERROR_SUCCESS);

            result = RegDeleteValueW(hKey, L"e");
            ok_long(result, ERROR_SUCCESS);
        }
        result = RegCloseKey(hKey);
        ok_long(result, ERROR_SUCCESS);

        result = RegDeleteKeyW(HKEY_CURRENT_USER, s_szMRUTest);
        ok_long(result, ERROR_SUCCESS);
    }
}

INT CALLBACK MyCompareString(LPCWSTR lhs, LPCWSTR rhs)
{
    return lstrcmpiW(lhs, rhs);
}

static void JustDoIt(void)
{
    MRUINFOW info;
    INT i, ret;
    WCHAR szText[64];
    HANDLE hList;

    ZeroMemory(&info, sizeof(info));
    info.cbSize = sizeof(info);
    info.uMax = 26;
    info.fFlags = 0;
    info.hKey = HKEY_CURRENT_USER;
    info.lpszSubKey = s_szMRUTest;
    info.u.string_cmpfn = MyCompareString;

    hList = (*s_pCreateMRUListW)(&info);
    ok(hList != NULL, "hList was NULL\n");

    i = 0;
    ret = (*s_pEnumMRUListW)(hList, i, szText, sizeof(szText));
    ok(ret > 0, "ret was %d\n", ret);
    ok(lstrcmpiW(szText, s_szC) == 0, "szText was %ls\n", szText);

    ++i;
    ret = (*s_pEnumMRUListW)(hList, i, szText, sizeof(szText));
    ok(ret > 0, "ret was %d\n", ret);
    ok(lstrcmpiW(szText, s_szA) == 0, "szText was %ls\n", szText);

    ++i;
    ret = (*s_pEnumMRUListW)(hList, i, szText, sizeof(szText));
    ok(ret > 0, "ret was %d\n", ret);
    ok(lstrcmpiW(szText, s_szD) == 0, "szText was %ls\n", szText);

    ++i;
    ret = (*s_pEnumMRUListW)(hList, i, szText, sizeof(szText));
    ok(ret > 0, "ret was %d\n", ret);
    ok(lstrcmpiW(szText, s_szB) == 0, "szText was %ls\n", szText);

    ++i;
    ret = (*s_pEnumMRUListW)(hList, i, szText, sizeof(szText));
    ok(ret == -1, "ret was %d\n", ret);

    ret = (*s_pAddMRUStringW)(hList, s_szNewItem);

    (*s_pFreeMRUList)(hList);

    hList = (*s_pCreateMRUListW)(&info);
    ok(hList != NULL, "hList was NULL\n");

    i = 0;
    ret = (*s_pEnumMRUListW)(hList, i, szText, sizeof(szText));
    ok(ret > 0, "ret was %d\n", ret);
    ok(lstrcmpiW(szText, s_szNewItem) == 0, "szText was %ls\n", szText);

    ++i;
    ret = (*s_pEnumMRUListW)(hList, i, szText, sizeof(szText));
    ok(ret > 0, "ret was %d\n", ret);
    ok(lstrcmpiW(szText, s_szC) == 0, "szText was %ls\n", szText);

    i = 0xFFFF;
    ret = (*s_pFindMRUStringW)(hList, L"b", &i);
    ok_int(ret, -1);
    ok_int(i, 0xFFFF);

    i = 0xFFFF;
    ret = (*s_pFindMRUStringW)(hList, L"XXX", &i);
    ok_int(ret, -1);
    ok_int(i, 0xFFFF);

    i = 0xFFFF;
    ret = (*s_pFindMRUStringW)(hList, s_szA, &i);
    ok_int(ret, 2);
    ok_int(i, 0);

    i = 0xFFFF;
    ret = (*s_pFindMRUStringW)(hList, s_szC, &i);
    ok_int(ret, 1);
    ok_int(i, 2);

    i = 0xFFFF;
    ret = (*s_pFindMRUStringW)(hList, s_szNewItem, &i);
    ok_int(ret, 0);
    ok_int(i, 4);

    (*s_pFreeMRUList)(hList);
}

START_TEST(MRUList)
{
    HINSTANCE hComCtl32 = GetModuleHandleA("comctl32.dll");
    if (!hComCtl32)
    {
        skip("comctl32 not loaded\n");
        return;
    }

    s_pCreateMRUListW = (CREATEMRULISTW)GetProcAddress(hComCtl32, (LPCSTR)400);
    s_pAddMRUStringW = (ADDMRUSTRINGW)GetProcAddress(hComCtl32, (LPCSTR)401);
    s_pFindMRUStringW = (FINDMRUSTRINGW)GetProcAddress(hComCtl32, (LPCSTR)402);
    s_pEnumMRUListW = (ENUMMRULISTW)GetProcAddress(hComCtl32, (LPCSTR)403);
    s_pFreeMRUList = (FREEMRULIST)GetProcAddress(hComCtl32, (LPCSTR)152);
    if (!s_pCreateMRUListW ||
        !s_pAddMRUStringW ||
        !s_pFindMRUStringW ||
        !s_pEnumMRUListW ||
        !s_pFreeMRUList)
    {
        skip("MRUList API not found %p, %p, %p, %p, %p\n",
            s_pCreateMRUListW,
            s_pAddMRUStringW,
            s_pFindMRUStringW,
            s_pEnumMRUListW,
            s_pFreeMRUList);
        return;
    }

    SetupRegistryforMRUListTest(TRUE);

    JustDoIt();

    SetupRegistryforMRUListTest(FALSE);
}
