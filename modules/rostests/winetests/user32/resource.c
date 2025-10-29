/* Unit test suite for resources.
 *
 * Copyright 2004 Ferenc Wagner
 * Copyright 2003, 2004 Mike McCormack
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

#include <assert.h>
#include <windows.h>

#include "wine/test.h"

static UINT (WINAPI *pPrivateExtractIconsA)(LPCSTR, int, int, int, HICON *, UINT *, UINT, UINT) = NULL;

static void init_function_pointers(void) 
{
    HMODULE hmod = GetModuleHandleA("user32.dll");
    pPrivateExtractIconsA = (void*)GetProcAddress(hmod, "PrivateExtractIconsA");
}

static void test_LoadStringW(void)
{
    HINSTANCE hInst = GetModuleHandleA(NULL);
    WCHAR copiedstringw[128], returnedstringw[128], *resourcepointer = NULL;
    char copiedstring[128], returnedstring[128];
    int length1, length2, retvalue, i;
    static struct
    {
        int id;
        const WCHAR *string;
    } string_table_tests[] =
    {
        { 2,   L"Error" },
        { 13,  L"&More Windows..." },
        { 800, L"OK" },
        { 801, L"Cancel" },
        { 802, L"&Abort" },
        { 803, L"&Retry" },
        { 804, L"&Ignore" },
        { 805, L"&Yes" },
        { 806, L"&No" },
        { 807, L"&Close" },
        { 808, L"Help" },
        { 809, L"&Try Again" },
        { 810, L"&Continue" },
    };

    /* Check that the string which is returned by LoadStringW matches
       the string at the pointer returned by LoadStringW when called with buflen = 0 */
    SetLastError(0xdeadbeef);
    length1 = LoadStringW(hInst, 2, (WCHAR *) &resourcepointer, 0); /* get pointer to resource. */
    if (!length1)
    {
        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
            win_skip( "LoadStringW not implemented\n" );
        else
            win_skip( "LoadStringW does not return a pointer to the resource\n" );
        return;
    }
    length2 = LoadStringW(hInst, 2, returnedstringw, ARRAY_SIZE(returnedstringw)); /* get resource string */
    ok(length2 > 0, "LoadStringW failed to load resource 2, ret %d, err %ld\n", length2, GetLastError());
    ok(length1 == length2, "LoadStringW returned different values dependent on buflen. ret1 %d, ret2 %d\n",
        length1, length2);
    ok(length1 > 0 && resourcepointer != NULL, "LoadStringW failed to get pointer to resource 2, ret %d, err %ld\n",
        length1, GetLastError());

    /* Copy the resource since it is not '\0' terminated, and add '\0' to the end */
    if(resourcepointer != NULL) /* Check that the resource pointer was loaded to avoid access violation */
    {
        memcpy(copiedstringw, resourcepointer, length1 * sizeof(WCHAR));
        copiedstringw[length1] = '\0';
        /* check that strings match */
        WideCharToMultiByte( CP_ACP, 0, returnedstringw, -1, returnedstring, 128, NULL, NULL );
        WideCharToMultiByte( CP_ACP, 0, copiedstringw, -1, copiedstring, 128, NULL, NULL );
        ok(!memcmp(copiedstringw, returnedstringw, (length2 + 1)*sizeof(WCHAR)),
           "strings don't match: returnedstring = %s, copiedstring = %s\n", returnedstring, copiedstring);
    }

    /* check that calling LoadStringW with buffer = NULL returns zero */
    retvalue = LoadStringW(hInst, 2, NULL, 0);
    ok(!retvalue, "LoadStringW returned a non-zero value when called with buffer = NULL, retvalue = %d\n", retvalue);
    /* check again, with a different buflen value, that calling LoadStringW with buffer = NULL returns zero */
    retvalue = LoadStringW(hInst, 2, NULL, 128);
    ok(!retvalue, "LoadStringW returned a non-zero value when called with buffer = NULL, retvalue = %d\n", retvalue);

    /* Test builtin string table in user32. */
    if ((PRIMARYLANGID(LANGIDFROMLCID(GetSystemDefaultLCID())) != LANG_ENGLISH)
            || (PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) != LANG_ENGLISH))
    {
        skip("Skip for Non-English system.\n");
        return;
    }
    else
    {
        hInst = GetModuleHandleW(L"user32.dll");
        ok(!!hInst, "Can't get module %#lx.\n", GetLastError());

        for (i = 0; i < ARRAYSIZE(string_table_tests); i++)
        {
            winetest_push_context("Test %u", i);

            length1 = LoadStringW(hInst, string_table_tests[i].id, returnedstringw, ARRAY_SIZE(returnedstringw));
            ok(length1 == wcslen(string_table_tests[i].string), "Got wrong length %d.\n", length1);
            ok(!wcscmp(returnedstringw, string_table_tests[i].string), "Got wrong string %s.\n",
                    debugstr_w(returnedstringw));

            winetest_pop_context();
        }
    }

    /* Test missing resource. */
    SetLastError(0xdeadbeef);
    memset(returnedstringw, 0xcc, sizeof(returnedstringw));
    length1 = LoadStringW(hInst, 0xdeadbeef, returnedstringw, ARRAY_SIZE(returnedstringw));
    ok(!length1, "got %d.\n", length1);
    ok(GetLastError() == ERROR_RESOURCE_NAME_NOT_FOUND || broken(GetLastError() == ERROR_MUI_FILE_NOT_FOUND) /* Win7 */,
            "got %lu.\n", GetLastError());
    ok(!returnedstringw[0], "got %#x.\n", returnedstringw[0]);
    ok(returnedstringw[1] == 0xcccc, "got %#x.\n", returnedstringw[1]);

    SetLastError(0xdeadbeef);
    memset(returnedstringw, 0xcc, sizeof(returnedstringw));
    length1 = LoadStringW(hInst, 0xdeadbeef, returnedstringw, 0);
    ok(!length1, "got %d.\n", length1);
    ok(GetLastError() == ERROR_RESOURCE_NAME_NOT_FOUND || broken(GetLastError() == ERROR_MUI_FILE_NOT_LOADED) /* Win7 */,
            "got %lu.\n", GetLastError());
    ok(returnedstringw[0] == 0xcccc, "got %#x.\n", returnedstringw[1]);

    SetLastError(0xdeadbeef);
    memset(returnedstringw, 0xcc, sizeof(returnedstringw));
    length1 = LoadStringW(hInst, 0xdeadbeef, returnedstringw, 1);
    ok(!length1, "got %d.\n", length1);
    ok(GetLastError() == ERROR_RESOURCE_NAME_NOT_FOUND || broken(GetLastError() == ERROR_MUI_FILE_NOT_LOADED) /* Win7 */,
            "got %lu.\n", GetLastError());
    ok(!returnedstringw[0], "got %#x.\n", returnedstringw[0]);
    ok(returnedstringw[1] == 0xcccc, "got %#x.\n", returnedstringw[1]);

    /* Test short buffer */
    SetLastError(0xdeadbeef);
    memset(returnedstringw, 0xcc, sizeof(returnedstringw));
    length1 = LoadStringW(hInst, 2, returnedstringw, 1); /* get resource string */
    ok(!length1, "got %d.\n", length1);
    ok(GetLastError() == 0xdeadbeef, "got %lu.\n", GetLastError());
    ok(!returnedstringw[0], "got %#x.\n", returnedstringw[0]);
    ok(returnedstringw[1] == 0xcccc, "got %#x.\n", returnedstringw[1]);
}

static void test_LoadStringA (void)
{
    HINSTANCE hInst = GetModuleHandleA(NULL);
    static const char str[] = "String resource"; /* same in resource.rc */
    char buf[128];
    struct string_test {
        unsigned int bufsiz;
        unsigned int expected;
    };
    struct string_test tests[] = {{sizeof buf, sizeof str - 1},
                                  {sizeof str, sizeof str - 1},
                                  {sizeof str - 1, sizeof str - 2}};
    unsigned int i;
    int ret, ret2;

    assert (sizeof str < sizeof buf);
    for (i = 0; i < ARRAY_SIZE(tests); i++) {
        const unsigned int bufsiz = tests[i].bufsiz;
        const unsigned int expected = tests[i].expected;
        const int len = LoadStringA (hInst, 0, buf, bufsiz);

        ok (len == expected, "bufsiz=%d: got %d, expected %d\n",
            bufsiz, len, expected);
        if (len != expected) continue;
        ok (!memcmp (buf, str, len),
            "bufsiz=%d: got '%s', expected '%.*s'\n",
            bufsiz, buf, len, str);
        ok (buf[len] == 0, "bufsiz=%d: NUL termination missing\n",
            bufsiz);
    }

    ret = LoadStringA(hInst, 1, buf, sizeof(buf) );
    ok( ret > 0, "LoadString failed: ret %d err %ld\n", ret, GetLastError());
    ret2 = LoadStringA( hInst, MAKELONG( 1, 0x8000 ), buf, sizeof(buf));
    ok( ret2 == ret, "LoadString failed: ret %d err %ld\n", ret, GetLastError());
    ret2 = LoadStringA( hInst, MAKELONG( 1, 0xffff ), buf, sizeof(buf));
    ok( ret2 == ret, "LoadString failed: ret %d err %ld\n", ret, GetLastError());

    ret = LoadStringA(hInst, 65534, buf, sizeof(buf) );
    ok( ret > 0, "LoadString failed: ret %d err %ld\n", ret, GetLastError());
    ret2 = LoadStringA( hInst, MAKELONG( 65534, 0x8000 ), buf, sizeof(buf));
    ok( ret2 == ret, "LoadString failed: ret %d err %ld\n", ret, GetLastError());
    ret2 = LoadStringA( hInst, MAKELONG( 65534, 0xffff ), buf, sizeof(buf));
    ok( ret2 == ret, "LoadString failed: ret %d err %ld\n", ret, GetLastError());

    ret = LoadStringA(hInst, 0, buf, 0);
    ok( ret == -1 || broken(ret == 0),
        "LoadStringA did not return -1 when called with buflen = 0, got %d, err %ld\n",
        ret, GetLastError());

    SetLastError(0xdeadbeef);
    buf[0] = 'a';
    ret = LoadStringA(hInst, 1, buf, 1);
    ok( !ret, "LoadString returned %d\n", ret);
    ok( buf[0] == 0, "buf[0] = %c (%x)\n", buf[0], buf[0]);
    ok( GetLastError() == 0xdeadbeef, "GetLastError() = %ld\n", GetLastError());
}

static void test_accel1(void)
{
    UINT r, n;
    HACCEL hAccel;
    ACCEL ac[10];

    /* now create our own valid accelerator table */
    n = 0;
    ac[n].cmd = 1000;
    ac[n].key = 'A';
    ac[n++].fVirt = FVIRTKEY | FNOINVERT;

    ac[n].cmd = 1001;
    ac[n].key = 'B';
    ac[n++].fVirt = FNOINVERT;

    ac[n].cmd = 0;
    ac[n].key = 0;
    ac[n++].fVirt = 0;

    hAccel = CreateAcceleratorTableA( &ac[0], n );
    ok( hAccel != NULL, "create accelerator table\n");

    r = DestroyAcceleratorTable( hAccel );
    ok( r, "destroy accelerator table\n");

    /* now try create an invalid one */
    n = 0;
    ac[n].cmd = 1000;
    ac[n].key = 'A';
    ac[n++].fVirt = FVIRTKEY | FNOINVERT;

    ac[n].cmd = 0xffff;
    ac[n].key = 0xffff;
    ac[n++].fVirt = (SHORT) 0xffff;

    ac[n].cmd = 0xfff0;
    ac[n].key = 'B';
    ac[n++].fVirt = (SHORT) 0xfff0;

    ac[n].cmd = 0xfff0;
    ac[n].key = 'C';
    ac[n++].fVirt = 0x0000;

    ac[n].cmd = 0xfff0;
    ac[n].key = 0xffff;
    ac[n++].fVirt = 0x0001;

    hAccel = CreateAcceleratorTableA( &ac[0], n );
    ok( hAccel != NULL, "create accelerator table\n");

    r = CopyAcceleratorTableA( hAccel, NULL, 0 );
    ok( r == n, "two entries in table %u/%u\n", r, n);

    r = CopyAcceleratorTableA( hAccel, &ac[0], n );
    ok( r == n, "still should be two entries in table %u/%u\n", r, n);

    n=0;
    ok( ac[n].cmd == 1000, "cmd 0 not preserved got %x\n", ac[n].cmd);
    ok( ac[n].key == 'A', "key 0 not preserved got %x\n", ac[n].key);
    ok( ac[n].fVirt == (FVIRTKEY | FNOINVERT), "fVirt 0 not preserved got %x\n", ac[n].fVirt);

    if (++n == r) goto done;
    ok( ac[n].cmd == 0xffff, "cmd 1 not preserved got %x\n", ac[n].cmd);
    ok( ac[n].key == 0xffff, "key 1 not preserved got %x\n", ac[n].key);
    ok( ac[n].fVirt == 0x007f, "fVirt 1 wrong got %x\n", ac[n].fVirt);

    if (++n == r) goto done;
    ok( ac[n].cmd == 0xfff0, "cmd 2 not preserved got %x\n", ac[n].cmd);
    ok( ac[n].key == 'B', "key 2 not preserved got %x\n", ac[n].key);
    ok( ac[n].fVirt == 0x0070, "fVirt 2 wrong got %x\n", ac[n].fVirt);

    if (++n == r) goto done;
    ok( ac[n].cmd == 0xfff0, "cmd 3 not preserved got %x\n", ac[n].cmd);
    ok( ac[n].key == 'C', "key 3 not preserved got %x\n", ac[n].key);
    ok( ac[n].fVirt == 0x0000, "fVirt 3 wrong got %x\n", ac[n].fVirt);

    if (++n == r) goto done;
    ok( ac[n].cmd == 0xfff0, "cmd 4 not preserved got %x\n", ac[n].cmd);
    ok( ac[n].key == 0xffff, "key 4 not preserved got %x\n", ac[n].key);
    ok( ac[n].fVirt == 0x0001, "fVirt 4 wrong  got %x\n", ac[n].fVirt);
done:
    r = DestroyAcceleratorTable( hAccel );
    ok( r, "destroy accelerator table\n");

    hAccel = CreateAcceleratorTableA( &ac[0], 0 );
    ok( !hAccel || broken(hAccel != NULL), /* nt4 */ "zero elements should fail\n");

    /* these will on crash win2k
    hAccel = CreateAcceleratorTable( NULL, 1 );
    hAccel = CreateAcceleratorTable( &ac[0], -1 );
    */
}

/*
 *  memcmp on the tables works in Windows, but does not work in wine, as
 *  there is an extra undefined and unused byte between fVirt and the key
 */
static void test_accel2(void)
{
    ACCEL ac[2], out[2];
    HACCEL hac;
    int res;

    ac[0].cmd   = 0;
    ac[0].fVirt = 0;
    ac[0].key   = 0;

    ac[1].cmd   = 0;
    ac[1].fVirt = 0;
    ac[1].key   = 0;

    /*
     * crashes on win2k
     * hac = CreateAcceleratorTable( NULL, 1 );
     */

    /* try a zero count */
    hac = CreateAcceleratorTableA( &ac[0], 0 );
    ok( !hac || broken(hac != NULL), /* nt4 */ "fail\n");
    if (!hac) ok( !DestroyAcceleratorTable( hac ), "destroy failed\n");

    /* creating one accelerator should work */
    hac = CreateAcceleratorTableA( &ac[0], 1 );
    ok( hac != NULL , "fail\n");
    ok( 1 == CopyAcceleratorTableA( hac, out, 1 ), "copy failed\n");
    ok( DestroyAcceleratorTable( hac ), "destroy failed\n");

    /* how about two of the same type? */
    hac = CreateAcceleratorTableA( &ac[0], 2);
    ok( hac != NULL , "fail\n");
    res = CopyAcceleratorTableA( hac, NULL, 100 );
    ok( res == 2, "copy null failed %d\n", res);
    res = CopyAcceleratorTableA( hac, NULL, 0 );
    ok( res == 2, "copy null failed %d\n", res);
    res = CopyAcceleratorTableA( hac, NULL, 1 );
    ok( res == 2, "copy null failed %d\n", res);
    ok( 1 == CopyAcceleratorTableA( hac, out, 1 ), "copy 1 failed\n");
    ok( 2 == CopyAcceleratorTableA( hac, out, 2 ), "copy 2 failed\n");
    ok( DestroyAcceleratorTable( hac ), "destroy failed\n");
    /* ok( !memcmp( ac, out, sizeof ac ), "tables different\n"); */

    /* how about two of the same type with a non-zero key? */
    ac[0].key = 0x20;
    ac[1].key = 0x20;
    hac = CreateAcceleratorTableA( &ac[0], 2);
    ok( hac != NULL , "fail\n");
    ok( 2 == CopyAcceleratorTableA( hac, out, 2 ), "copy 2 failed\n");
    ok( DestroyAcceleratorTable( hac ), "destroy failed\n");
    /* ok( !memcmp( ac, out, sizeof ac ), "tables different\n"); */

    /* how about two of the same type with a non-zero virtual key? */
    ac[0].fVirt = FVIRTKEY;
    ac[0].key = 0x40;
    ac[1].fVirt = FVIRTKEY;
    ac[1].key = 0x40;
    hac = CreateAcceleratorTableA( &ac[0], 2);
    ok( hac != NULL , "fail\n");
    ok( 2 == CopyAcceleratorTableA( hac, out, 2 ), "copy 2 failed\n");
    /* ok( !memcmp( ac, out, sizeof ac ), "tables different\n"); */
    ok( DestroyAcceleratorTable( hac ), "destroy failed\n");

    /* how virtual key codes */
    ac[0].fVirt = FVIRTKEY;
    hac = CreateAcceleratorTableA( &ac[0], 1);
    ok( hac != NULL , "fail\n");
    ok( 1 == CopyAcceleratorTableA( hac, out, 2 ), "copy 2 failed\n");
    /* ok( !memcmp( ac, out, sizeof ac/2 ), "tables different\n"); */
    ok( DestroyAcceleratorTable( hac ), "destroy failed\n");

    /* how turning on all bits? */
    ac[0].cmd   = 0xffff;
    ac[0].fVirt = 0xff;
    ac[0].key   = 0xffff;
    hac = CreateAcceleratorTableA( &ac[0], 1);
    ok( hac != NULL , "fail\n");
    ok( 1 == CopyAcceleratorTableA( hac, out, 1 ), "copy 1 failed\n");
    /* ok( memcmp( ac, out, sizeof ac/2 ), "tables not different\n"); */
    ok( out[0].cmd == ac[0].cmd, "cmd modified\n");
    ok( out[0].fVirt == (ac[0].fVirt&0x7f), "fVirt not modified\n");
    ok( out[0].key == ac[0].key, "key modified\n");
    ok( DestroyAcceleratorTable( hac ), "destroy failed\n");

    /* how turning on all bits? */
    memset( ac, 0xff, sizeof ac );
    hac = CreateAcceleratorTableA( &ac[0], 2);
    ok( hac != NULL , "fail\n");
    res = CopyAcceleratorTableA( hac, out, 2 );
    ok( res == 2, "copy 2 failed %d\n", res);
    /* ok( memcmp( ac, out, sizeof ac ), "tables not different\n"); */
    ok( out[0].cmd == ac[0].cmd, "cmd modified\n");
    ok( out[0].fVirt == (ac[0].fVirt&0x7f), "fVirt not modified\n");
    ok( out[0].key == ac[0].key, "key modified\n");
    if (res == 2)
    {
        ok( out[1].cmd == ac[1].cmd, "cmd modified\n");
        ok( out[1].fVirt == (ac[1].fVirt&0x7f), "fVirt not modified\n");
        ok( out[1].key == ac[1].key, "key modified\n");
    }
    ok( DestroyAcceleratorTable( hac ), "destroy failed\n");
}

static void test_PrivateExtractIcons(void) {
    const CHAR szShell32Dll[] = "shell32.dll";
    HICON ahIcon[256];
    UINT i, aIconId[256], cIcons, cIcons2;

    if (!pPrivateExtractIconsA) return;

    cIcons = pPrivateExtractIconsA("", 0, 16, 16, ahIcon, aIconId, 1, 0);
    ok(cIcons == ~0u, "got %u\n", cIcons);

    cIcons = pPrivateExtractIconsA("notepad.exe", 0, 16, 16, NULL, NULL, 1, 0);
    ok(cIcons == 1 || broken(cIcons == 4) /* win11 */ ||
       broken(cIcons == 2) /* win2k */, "got %u\n", cIcons);

    ahIcon[0] = (HICON)0xdeadbeef;
    cIcons = pPrivateExtractIconsA("notepad.exe", 0, 16, 16, ahIcon, NULL, 1, 0);
    ok(cIcons == 1, "got %u\n", cIcons);
    ok(ahIcon[0] != (HICON)0xdeadbeef, "icon not set\n");
    DestroyIcon(ahIcon[0]);

    ahIcon[0] = (HICON)0xdeadbeef;
    aIconId[0] = 0xdeadbeef;
    cIcons = pPrivateExtractIconsA("notepad.exe", 0, 16, 16, ahIcon, aIconId, 1, 0);
    ok(cIcons == 1, "got %u\n", cIcons);
    ok(ahIcon[0] != (HICON)0xdeadbeef, "icon not set\n");
    ok(aIconId[0] != 0xdeadbeef, "id not set\n");
    DestroyIcon(ahIcon[0]);

    cIcons = pPrivateExtractIconsA(szShell32Dll, 0, 16, 16, NULL, NULL, 0, 0);
    cIcons2 = pPrivateExtractIconsA(szShell32Dll, 4, MAKELONG(32,16), MAKELONG(32,16), 
                                   NULL, NULL, 256, 0);
    ok((cIcons == cIcons2) && (cIcons > 0), 
       "Icon count should be independent of requested icon sizes and base icon index! "
       "(cIcons=%d, cIcons2=%d)\n", cIcons, cIcons2);

    cIcons = pPrivateExtractIconsA(szShell32Dll, 0, 16, 16, ahIcon, aIconId, 0, 0);
    ok(cIcons == 0, "Zero icons requested, got cIcons=%d\n", cIcons);

    cIcons = pPrivateExtractIconsA(szShell32Dll, 0, 16, 16, ahIcon, aIconId, 3, 0);
    ok(cIcons == 3, "Three icons requested got cIcons=%d\n", cIcons);
    for (i = 0; i < cIcons; i++) DestroyIcon(ahIcon[i]);

    /* count must be a multiple of two when getting two sizes */
    cIcons = pPrivateExtractIconsA(szShell32Dll, 0, MAKELONG(16,32), MAKELONG(16,32),
                                   ahIcon, aIconId, 3, 0);
    ok(cIcons == 0 /* vista */ || cIcons == 4, "Three icons requested got cIcons=%d\n", cIcons);
    for (i = 0; i < cIcons; i++) DestroyIcon(ahIcon[i]);

    cIcons = pPrivateExtractIconsA(szShell32Dll, 0, MAKELONG(16,32), MAKELONG(16,32),
                                   ahIcon, aIconId, 4, 0);
    ok(cIcons == 4, "Four icons requested got cIcons=%d\n", cIcons);
    for (i = 0; i < cIcons; i++) DestroyIcon(ahIcon[i]);
}

static void test_LoadImage(void)
{
    HBITMAP bmp;
    HRSRC hres;

    bmp = LoadBitmapA(GetModuleHandleA(NULL), MAKEINTRESOURCEA(100));
    ok(bmp != NULL, "Could not load a bitmap resource\n");
    if (bmp) DeleteObject(bmp);

    hres = FindResourceA(GetModuleHandleA(NULL), "#100", (LPCSTR)RT_BITMAP);
    ok(hres != NULL, "Could not find a bitmap resource with a numeric string\n");

    bmp = LoadBitmapA(GetModuleHandleA(NULL), "#100");
    ok(bmp != NULL, "Could not load a bitmap resource with a numeric string\n");
    if (bmp) DeleteObject(bmp);
}

START_TEST(resource)
{
    init_function_pointers();
#if !defined(__REACTOS__) || !defined(_M_AMD64)
    test_LoadStringA();
#else
    skip("LoadStringA test broken on amd64!\n");
#endif
    test_LoadStringW();
    test_accel1();
    test_accel2();
    test_PrivateExtractIcons();
    test_LoadImage();
}
