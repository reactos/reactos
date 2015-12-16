/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Tests for ICMP functions
 * PROGRAMMERS:     Tim Crawford
 */

#include <apitest.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>

static
void
test_IcmpCreateFile(void)
{
    HANDLE hIcmp;

    SetLastError(0xDEADBEEF);
    hIcmp = IcmpCreateFile();
    ok(hIcmp != INVALID_HANDLE_VALUE, "IcmpCreateFile failed unexpectedly: %lu\n", GetLastError());

    if (hIcmp != INVALID_HANDLE_VALUE)
        IcmpCloseHandle(hIcmp);
}

static
void
test_Icmp6CreateFile(void)
{
    HANDLE hIcmp;

    SetLastError(0xDEADBEEF);
    hIcmp = Icmp6CreateFile();

    if (GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        /* On Windows Server 2003, the IPv6 protocol must be installed. */
        skip("IPv6 is not available.\n");
        return;
    }

    ok(hIcmp != INVALID_HANDLE_VALUE, "Icmp6CreateFile failed unexpectedly: %lu\n", GetLastError());

    if (hIcmp != INVALID_HANDLE_VALUE)
        IcmpCloseHandle(hIcmp);
}

static
void
test_IcmpCloseHandle(void)
{
    HANDLE hIcmp;
    BOOL bRet;

    SetLastError(0xDEADBEEF);
    hIcmp = IcmpCreateFile();
    if (hIcmp != INVALID_HANDLE_VALUE)
    {
        bRet = IcmpCloseHandle(hIcmp);
        ok(bRet, "IcmpCloseHandle failed unexpectedly: %lu\n", GetLastError());
    }

    SetLastError(0xDEADBEEF);
    hIcmp = Icmp6CreateFile();
    if (hIcmp != INVALID_HANDLE_VALUE)
    {
        bRet = IcmpCloseHandle(hIcmp);
        ok(bRet, "IcmpCloseHandle failed unexpectedly: %lu\n", GetLastError());
    }

    hIcmp = INVALID_HANDLE_VALUE;
    SetLastError(0xDEADBEEF);
    bRet = IcmpCloseHandle(hIcmp);
    ok(!bRet, "IcmpCloseHandle succeeded unexpectedly\n");
    ok_err(ERROR_INVALID_HANDLE);

    hIcmp = NULL;
    SetLastError(0xDEADBEEF);
    bRet = IcmpCloseHandle(hIcmp);
    ok(!bRet, "IcmpCloseHandle succeeded unexpectedly\n");
    ok_err(ERROR_INVALID_HANDLE);
}

START_TEST(icmp)
{
    test_IcmpCreateFile();
    test_Icmp6CreateFile();
    test_IcmpCloseHandle();
}
