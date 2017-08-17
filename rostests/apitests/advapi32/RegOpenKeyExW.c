/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for the RegOpenKeyExW alignment
 * PROGRAMMER:      Mark Jansen <mark.jansen@reactos.org>
 */
#include <apitest.h>

#define WIN32_NO_STATUS
#include <winreg.h>

#include <pshpack1.h>
struct Unalignment1
{
    char dum;
    WCHAR buffer[20];
} Unalignment1;
struct Unalignment2
{
    char dum;
    HKEY hk;
} Unalignment2;
#include <poppack.h>


#define TEST_STR    L".exe"


START_TEST(RegOpenKeyExW)
{
    struct Unalignment1 un;
    struct Unalignment2 un2;
    HKEY hk;
    LONG lRes;

    memcpy(un.buffer, TEST_STR, sizeof(TEST_STR));
    un2.hk = 0;

    lRes = RegOpenKeyExW(HKEY_CLASSES_ROOT, TEST_STR, 0, KEY_READ, &hk);
    ok_int(lRes, ERROR_SUCCESS);
    if (lRes)
        return;
    RegCloseKey(hk);

    ok_hex(((ULONG_PTR)un.buffer) % 2, 1);
    lRes = RegOpenKeyExW(HKEY_CLASSES_ROOT, un.buffer, 0, KEY_READ, &hk);
    ok_int(lRes, ERROR_SUCCESS);
    if (!lRes)
        RegCloseKey(hk);

    ok_hex(((ULONG_PTR)&un2.hk) % 2, 1);
    lRes = RegOpenKeyExW(HKEY_CLASSES_ROOT, TEST_STR, 0, KEY_READ, &un2.hk);
    ok_int(lRes, ERROR_SUCCESS);
    if (!lRes)
    {
        hk = un2.hk;
        RegCloseKey(hk);
    }
}
