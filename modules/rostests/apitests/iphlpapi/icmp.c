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

static
void
test_IcmpSendEcho(void)
{
    HANDLE hIcmp;
    unsigned long ipaddr = INADDR_NONE;
    DWORD bRet = 0, error = 0;
    char SendData[32] = "Data Buffer";
    PVOID ReplyBuffer;
    DWORD ReplySize = 0;

    SetLastError(0xDEADBEEF);
    hIcmp = IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE)
    {
        skip("IcmpCreateFile failed unexpectedly: %lu\n", GetLastError());
        return;
    }

    ipaddr = 0x08080808; // 8.8.8.8
    ReplyBuffer = malloc(sizeof(ICMP_ECHO_REPLY) + sizeof(SendData));

    ReplySize = sizeof(ICMP_ECHO_REPLY);
    SetLastError(0xDEADBEEF);
    bRet = IcmpSendEcho(hIcmp, ipaddr, SendData, sizeof(SendData),
        NULL, ReplyBuffer, ReplySize, 5000);

    ok(!bRet, "IcmpSendEcho succeeded unexpectedly\n");
    error = GetLastError();
    ok(error == IP_BUF_TOO_SMALL /* Win2003 */ ||
       error == IP_GENERAL_FAILURE /* Win10 */,
       "IcmpSendEcho returned unexpected error: %lu\n", error);

    ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
    SetLastError(0xDEADBEEF);
    bRet = IcmpSendEcho(hIcmp, ipaddr, SendData, sizeof(SendData),
        NULL, ReplyBuffer, ReplySize, 5000);

    ok(bRet, "IcmpSendEcho failed unexpectedly: %lu\n", GetLastError());

    free(ReplyBuffer);
    IcmpCloseHandle(hIcmp);
}

START_TEST(icmp)
{
    test_IcmpCreateFile();
    test_Icmp6CreateFile();
    test_IcmpCloseHandle();
    test_IcmpSendEcho();
}
