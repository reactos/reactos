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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <windows.h>

#include "wine/test.h"

static UINT (WINAPI *pPrivateExtractIconsA)(LPCTSTR, int, int, int, HICON *, UINT *, UINT, UINT) = NULL;

static void init_function_pointers(void) 
{
    HMODULE hmod = GetModuleHandleA("user32.dll");
    if (hmod) {
        pPrivateExtractIconsA = (void*)GetProcAddress(hmod, "PrivateExtractIconsA");
    }
}

static void test_LoadStringA (void)
{
    HINSTANCE hInst = GetModuleHandle (NULL);
    static const char str[] = "String resource"; /* same in resource.rc */
    char buf[128];
    struct string_test {
        int bufsiz;
        int expected;
    };
    struct string_test tests[] = {{sizeof buf, sizeof str - 1},
                                  {sizeof str, sizeof str - 1},
                                  {sizeof str - 1, sizeof str - 2}};
    unsigned int i;

    assert (sizeof str < sizeof buf);
    for (i = 0; i < sizeof tests / sizeof tests[0]; i++) {
        const int bufsiz = tests[i].bufsiz;
        const int expected = tests[i].expected;
        const int len = LoadStringA (hInst, 0, buf, bufsiz);

        ok (len == expected, "bufsiz=%d: got %d, expected %d\n",
            bufsiz, len, expected);
        ok (!memcmp (buf, str, len),
            "bufsiz=%d: got '%s', expected '%.*s'\n",
            bufsiz, buf, len, str);
        ok (buf[len] == 0, "bufsiz=%d: NUL termination missing\n",
            bufsiz);
    }
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

    hAccel = CreateAcceleratorTable( &ac[0], n );
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
    ac[n].key = 0xffff;
    ac[n++].fVirt = (SHORT) 0xfff0;

    ac[n].cmd = 0xfff0;
    ac[n].key = 0xffff;
    ac[n++].fVirt = (SHORT) 0x0000;

    ac[n].cmd = 0xfff0;
    ac[n].key = 0xffff;
    ac[n++].fVirt = (SHORT) 0x0001;

    hAccel = CreateAcceleratorTable( &ac[0], n );
    ok( hAccel != NULL, "create accelerator table\n");

    r = CopyAcceleratorTable( hAccel, NULL, 0 );
    ok( r == n, "two entries in table\n");

    r = CopyAcceleratorTable( hAccel, &ac[0], r );
    ok( r == n, "still should be two entries in table\n");

    n=0;
    ok( ac[n].cmd == 1000, "cmd 0 not preserved\n");
    ok( ac[n].key == 'A', "key 0 not preserved\n");
    ok( ac[n].fVirt == (FVIRTKEY | FNOINVERT), "fVirt 0 not preserved\n");

    n++;
    ok( ac[n].cmd == 0xffff, "cmd 1 not preserved\n");
    ok( ac[n].key == 0xffff, "key 1 not preserved\n");
    ok( ac[n].fVirt == 0x007f, "fVirt 1 not changed\n");

    n++;
    ok( ac[n].cmd == 0xfff0, "cmd 2 not preserved\n");
    ok( ac[n].key == 0x00ff, "key 2 not preserved\n");
    ok( ac[n].fVirt == 0x0070, "fVirt 2 not changed\n");

    n++;
    ok( ac[n].cmd == 0xfff0, "cmd 3 not preserved\n");
    ok( ac[n].key == 0x00ff, "key 3 not preserved\n");
    ok( ac[n].fVirt == 0x0000, "fVirt 3 not changed\n");

    n++;
    ok( ac[n].cmd == 0xfff0, "cmd 4 not preserved\n");
    ok( ac[n].key == 0xffff, "key 4 not preserved\n");
    ok( ac[n].fVirt == 0x0001, "fVirt 4 not changed\n");

    r = DestroyAcceleratorTable( hAccel );
    ok( r, "destroy accelerator table\n");

    hAccel = CreateAcceleratorTable( &ac[0], 0 );
    ok( !hAccel, "zero elements should fail\n");

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
    hac = CreateAcceleratorTable( &ac[0], 0 );
    ok( !hac , "fail\n");
    ok( !DestroyAcceleratorTable( hac ), "destroy failed\n");

    /* creating one accelerator should work */
    hac = CreateAcceleratorTable( &ac[0], 1 );
    ok( hac != NULL , "fail\n");
    ok( 1 == CopyAcceleratorTable( hac, out, 1 ), "copy failed\n");
    ok( DestroyAcceleratorTable( hac ), "destroy failed\n");

    /* how about two of the same type? */
    hac = CreateAcceleratorTable( &ac[0], 2);
    ok( hac != NULL , "fail\n");
    ok( 2 == CopyAcceleratorTable( hac, NULL, 100 ), "copy null failed\n");
    ok( 2 == CopyAcceleratorTable( hac, NULL, 0 ), "copy null failed\n");
    ok( 2 == CopyAcceleratorTable( hac, NULL, 1 ), "copy null failed\n");
    ok( 1 == CopyAcceleratorTable( hac, out, 1 ), "copy 1 failed\n");
    ok( 2 == CopyAcceleratorTable( hac, out, 2 ), "copy 2 failed\n");
    ok( DestroyAcceleratorTable( hac ), "destroy failed\n");
    /* ok( !memcmp( ac, out, sizeof ac ), "tables different\n"); */

    /* how about two of the same type with a non-zero key? */
    ac[0].key = 0x20;
    ac[1].key = 0x20;
    hac = CreateAcceleratorTable( &ac[0], 2);
    ok( hac != NULL , "fail\n");
    ok( 2 == CopyAcceleratorTable( hac, out, 2 ), "copy 2 failed\n");
    ok( DestroyAcceleratorTable( hac ), "destroy failed\n");
    /* ok( !memcmp( ac, out, sizeof ac ), "tables different\n"); */

    /* how about two of the same type with a non-zero virtual key? */
    ac[0].fVirt = FVIRTKEY;
    ac[0].key = 0x40;
    ac[1].fVirt = FVIRTKEY;
    ac[1].key = 0x40;
    hac = CreateAcceleratorTable( &ac[0], 2);
    ok( hac != NULL , "fail\n");
    ok( 2 == CopyAcceleratorTable( hac, out, 2 ), "copy 2 failed\n");
    /* ok( !memcmp( ac, out, sizeof ac ), "tables different\n"); */
    ok( DestroyAcceleratorTable( hac ), "destroy failed\n");

    /* how virtual key codes */
    ac[0].fVirt = FVIRTKEY;
    hac = CreateAcceleratorTable( &ac[0], 1);
    ok( hac != NULL , "fail\n");
    ok( 1 == CopyAcceleratorTable( hac, out, 2 ), "copy 2 failed\n");
    /* ok( !memcmp( ac, out, sizeof ac/2 ), "tables different\n"); */
    ok( DestroyAcceleratorTable( hac ), "destroy failed\n");

    /* how turning on all bits? */
    ac[0].cmd   = 0xffff;
    ac[0].fVirt = 0xff;
    ac[0].key   = 0xffff;
    hac = CreateAcceleratorTable( &ac[0], 1);
    ok( hac != NULL , "fail\n");
    ok( 1 == CopyAcceleratorTable( hac, out, 1 ), "copy 1 failed\n");
    /* ok( memcmp( ac, out, sizeof ac/2 ), "tables not different\n"); */
    ok( out[0].cmd == ac[0].cmd, "cmd modified\n");
    ok( out[0].fVirt == (ac[0].fVirt&0x7f), "fVirt not modified\n");
    ok( out[0].key == ac[0].key, "key modified\n");
    ok( DestroyAcceleratorTable( hac ), "destroy failed\n");

    /* how turning on all bits? */
    memset( ac, 0xff, sizeof ac );
    hac = CreateAcceleratorTable( &ac[0], 2);
    ok( hac != NULL , "fail\n");
    ok( 2 == CopyAcceleratorTable( hac, out, 2 ), "copy 2 failed\n");
    /* ok( memcmp( ac, out, sizeof ac ), "tables not different\n"); */
    ok( out[0].cmd == ac[0].cmd, "cmd modified\n");
    ok( out[0].fVirt == (ac[0].fVirt&0x7f), "fVirt not modified\n");
    ok( out[0].key == ac[0].key, "key modified\n");
    ok( out[1].cmd == ac[1].cmd, "cmd modified\n");
    ok( out[1].fVirt == (ac[1].fVirt&0x7f), "fVirt not modified\n");
    ok( out[1].key == ac[1].key, "key modified\n");
    ok( DestroyAcceleratorTable( hac ), "destroy failed\n");
}

static void test_PrivateExtractIcons(void) {
    CONST CHAR szShell32Dll[] = "shell32.dll";
    HICON ahIcon[256];
    UINT aIconId[256];
    UINT cIcons, cIcons2;

    if (!pPrivateExtractIconsA) return;
    
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

    cIcons = pPrivateExtractIconsA(szShell32Dll, 0, MAKELONG(32,16), MAKELONG(32,16), 
                                  ahIcon, aIconId, 3, 0);
    ok(cIcons == 4, "Three icons requested, four expected, got cIcons=%d\n", cIcons);
}

static void test_LoadImage(void) {
    HBITMAP bmp;
    
    bmp = LoadBitmapA(NULL, MAKEINTRESOURCE(OBM_CHECK));
    ok(bmp != NULL, "Could not load the OBM_CHECK bitmap\n");
    if (bmp) DeleteObject(bmp);
    
    bmp = LoadBitmapA(NULL, "#32760"); /* Value of OBM_CHECK */
    ok(bmp != NULL, "Could not load the OBM_CHECK bitmap\n");
    if (bmp) DeleteObject(bmp);
}    

START_TEST(resource)
{
    init_function_pointers();
    test_LoadStringA ();
    test_accel1();
    test_accel2();
    test_PrivateExtractIcons();
    test_LoadImage();
}
