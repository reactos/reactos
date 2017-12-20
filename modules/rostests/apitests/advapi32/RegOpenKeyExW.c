/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for the RegOpenKeyExW alignment
 * PROGRAMMER:      Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

#define TEST_STR    L".exe"

START_TEST(RegOpenKeyExW)
{
    char GccShouldNotAlignThis[20 * 2];
    char GccShouldNotAlignThis2[20];
    PVOID Alias = GccShouldNotAlignThis + 1;
    PVOID UnalignedKey = GccShouldNotAlignThis2 + 1;
    HKEY hk;
    LONG lRes;

    memcpy(Alias, TEST_STR, sizeof(TEST_STR));

    lRes = RegOpenKeyExW(HKEY_CLASSES_ROOT, TEST_STR, 0, KEY_READ, &hk);
    ok_int(lRes, ERROR_SUCCESS);
    if (lRes)
        return;
    RegCloseKey(hk);

    ok_hex(((ULONG_PTR)Alias) % 2, 1);
    lRes = RegOpenKeyExW(HKEY_CLASSES_ROOT, Alias, 0, KEY_READ, &hk);
    ok_int(lRes, ERROR_SUCCESS);
    if (!lRes)
        RegCloseKey(hk);

    ok_hex(((ULONG_PTR)UnalignedKey) % 2, 1);
    lRes = RegOpenKeyExW(HKEY_CLASSES_ROOT, TEST_STR, 0, KEY_READ, UnalignedKey);
    ok_int(lRes, ERROR_SUCCESS);
    if (!lRes)
    {
        RegCloseKey(*(HKEY*)(UnalignedKey));
    }
}
