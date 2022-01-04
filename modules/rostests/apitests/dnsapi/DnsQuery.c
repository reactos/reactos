/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for DnsQuery_A, DnsQuery_UTF8
 * PROGRAMMER:      Victor Martinez Calvo <victor.martinez@reactos.org>
 */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <windns.h>
#include <apitest.h>
#include <iphlpapi.h>


void TestHostName(void)
{

    DNS_STATUS dns_status;
    char host_name[255];
    char test_name[255];
    char host_nameUTF8[255];
    char test_nameUTF8[255];
    PDNS_RECORD dp;
    WCHAR host_nameW[255];
    WCHAR test_nameW[255];
    PFIXED_INFO network_info;
    ULONG network_info_blen = 0;
    DWORD network_info_result;

    network_info_result = GetNetworkParams(NULL, &network_info_blen);
    network_info = (PFIXED_INFO)HeapAlloc(GetProcessHeap(), 0, (size_t)network_info_blen);
    if (NULL == network_info)
    {
        skip("Not enough memory. Can't continue!\n");
        return;
    }

    network_info_result = GetNetworkParams(network_info, &network_info_blen);
    if (network_info_result != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, network_info);
        skip("Can't get network info. Some results may be wrong.\n");
        return;
    }
    else
    {
        strcpy(host_name, network_info->HostName);
        if (strlen(network_info->DomainName))
        {
            strcat(host_name, ".");
            strcat(host_name, network_info->DomainName);
        }
        HeapFree(GetProcessHeap(), 0, network_info);
        mbstowcs(host_nameW, host_name, 255);
        wcstombs(host_nameUTF8, host_nameW, 255);
    }

    //DnsQuery_A:
    //NULL
    dp = InvalidPointer;
    dns_status = DnsQuery_A(NULL, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == ERROR_INVALID_PARAMETER, "DnsQuery_A failed with error %lu\n", dns_status);
    ok(dp == InvalidPointer, "dp = %p\n", dp);

    //NULL dp
    dns_status = DnsQuery_A(host_name, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, NULL, 0);
    ok(dns_status == ERROR_INVALID_PARAMETER, "DnsQuery_A failed with error %lu\n", dns_status);

    //Testing HostName
    dp = InvalidPointer;
    dns_status = DnsQuery_A(host_name, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_A failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        ok(strcmp(dp->pName, host_name) == 0, "DnsQuery_A returned wrong answer '%s' expected '%s'\n", dp->pName, host_name);
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_A returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_A returned wrong data size %d\n", dp->wDataLength);
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    //127.0.0.1
    dp = InvalidPointer;
    dns_status = DnsQuery_A("127.0.0.1", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_A failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        ok(strcmp(dp->pName, "127.0.0.1") == 0, "DnsQuery_A returned wrong answer '%s' expected '%s'\n", dp->pName, "127.0.0.1");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_A returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_A returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_A returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    //Localhost strings
    dp = InvalidPointer;
    dns_status = DnsQuery_A("LocalHost", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_A failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        /* On Windows 7 is unchanged on XP is lowercased */
        ok(strcmp(dp->pName, "localhost") == 0 || broken(strcmp(dp->pName, "LocalHost") == 0), "DnsQuery_A returned wrong answer '%s' expected '%s'\n", dp->pName, "localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_A returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_A returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_A returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    dns_status = DnsQuery_A("Localhost", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_A failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        /* On Windows 7 is unchanged on XP is lowercased */
        ok(strcmp(dp->pName, "localhost") == 0 || broken(strcmp(dp->pName, "Localhost") == 0), "DnsQuery_A returned wrong answer '%s' expected '%s'\n", dp->pName, "localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_A returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_A returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_A returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    dns_status = DnsQuery_A("localhost", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_A failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        ok(strcmp(dp->pName, "localhost") == 0, "DnsQuery_A returned wrong answer '%s' expected '%s'\n", dp->pName, "localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_A returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_A returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_A returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    dns_status = DnsQuery_A("", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_A failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        /* On Windows 7 is the host on XP is dot ??? */
        ok(strcmp(dp->pName, ".") == 0 || broken(strcmp(dp->pName, host_name) == 0), "DnsQuery_A returned wrong answer '%s' expected '%s' or '.'\n", dp->pName, host_name);
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_A returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_A returned wrong data size %d\n", dp->wDataLength);
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    dns_status = DnsQuery_A(" ", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    /* On Windows 7 is DNS_ERROR_INVALID_NAME_CHAR on XP is ERROR_TIMEOUT on Win 2k3 is ERROR_INVALID_NAME*/
    ok(dns_status == ERROR_INVALID_NAME || broken(dns_status == ERROR_TIMEOUT) || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_A failed with error %lu expected %u or %u or %u\n", dns_status, ERROR_INVALID_NAME, ERROR_TIMEOUT, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(strcmp(dp->pName, host_name) == 0, "DnsQuery_A returned wrong answer '%s' expected '%s'\n", dp->pName, host_name);
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_A returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_A returned wrong data size %d\n", dp->wDataLength);
    }
    ok(dp == InvalidPointer || broken(dp == NULL), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    dns_status = DnsQuery_A("0.0.0.0", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_A failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        ok(strcmp(dp->pName, "0.0.0.0") == 0, "DnsQuery_A returned wrong answer '%s' expected '%s'\n", dp->pName, "0.0.0.0");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_A returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_A returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_ANY), "DnsQuery_A returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_ANY));
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    dns_status = DnsQuery_A("0.0.0.0 ", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    /* On windows 7 fails with DNS_ERROR_INVALID_NAME_CHAR on XP no error */
    ok(dns_status == NO_ERROR || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_A wrong status %lu expected %u or %u\n", dns_status, NO_ERROR, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(strcmp(dp->pName, "0.0.0.0") == 0 || broken(strcmp(dp->pName, "0.0.0.0 ") == 0), "DnsQuery_A returned wrong answer '%s' expected '%s' or '%s'\n", dp->pName, "0.0.0.0", "0.0.0.0 ");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_A returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_A returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_ANY), "DnsQuery_A returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_ANY));
    }
    ok(dp != InvalidPointer || broken(dp == InvalidPointer) || broken(dp == NULL), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    dns_status = DnsQuery_A("127.0.0.1 ", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    /* On windows 7 fails with DNS_ERROR_INVALID_NAME_CHAR on XP no error */
    ok(dns_status == NO_ERROR || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_A wrong status %lu expected %u or %u\n", dns_status, NO_ERROR, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(strcmp(dp->pName, "127.0.0.1") == 0 || strcmp(dp->pName, "127.0.0.1 ") == 0, "DnsQuery_A returned wrong answer '%s' expected '%s' or '%s'\n", dp->pName, "127.0.0.1", "127.0.0.1 ");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_A returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_A returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_A returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp != InvalidPointer || broken(dp == InvalidPointer) || broken(dp == NULL), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    dns_status = DnsQuery_A(" 127.0.0.1 ", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == DNS_ERROR_RCODE_NAME_ERROR || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_A wrong status %lu expected %u or %u\n", dns_status, DNS_ERROR_RCODE_NAME_ERROR, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(strcmp(dp->pName, "127.0.0.1") == 0, "DnsQuery_A returned wrong answer '%s' expected '%s'\n", dp->pName, "127.0.0.1");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_A returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_A returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_A returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp == InvalidPointer || broken(dp == NULL), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    dns_status = DnsQuery_A(" 127.0. 0.1 ", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == DNS_ERROR_RCODE_NAME_ERROR || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_A wrong status %lu expected %u or %u\n", dns_status, DNS_ERROR_RCODE_NAME_ERROR, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp == InvalidPointer && dns_status == NO_ERROR)
    {
        ok(strcmp(dp->pName, "127.0.0.1") == 0, "DnsQuery_A returned wrong answer '%s' expected '%s'\n", dp->pName, "127.0.0.1");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_A returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_A returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_A returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp == InvalidPointer || broken(dp == NULL), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    dns_status = DnsQuery_A("localhost ", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == ERROR_INVALID_NAME || broken(dns_status == ERROR_TIMEOUT) || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_A wrong status %lu expected %u or %u or %u\n", dns_status, ERROR_INVALID_NAME, ERROR_TIMEOUT, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(strcmp(dp->pName, "localhost") == 0, "DnsQuery_A returned wrong answer '%s' expected '%s'\n", dp->pName, "localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_A returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_A returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_A returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp == InvalidPointer || broken(dp == NULL), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    dns_status = DnsQuery_A(" localhost", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == ERROR_INVALID_NAME || broken(dns_status == ERROR_TIMEOUT) || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_A wrong status %lu expected %u or %u or %u\n", dns_status, ERROR_INVALID_NAME, ERROR_TIMEOUT, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(strcmp(dp->pName, "localhost") == 0, "DnsQuery_A returned wrong answer '%s' expected '%s'\n", dp->pName, "localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_A returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_A returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_A returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp == InvalidPointer || broken(dp == NULL), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    strcpy(test_name, " local host ");
    dns_status = DnsQuery_A(test_name, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == ERROR_INVALID_NAME || broken(dns_status == ERROR_TIMEOUT) || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_A wrong status %lu expected %u or %u or %u\n", dns_status, ERROR_INVALID_NAME, ERROR_TIMEOUT, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(strcmp(dp->pName, "localhost") == 0, "DnsQuery_A returned wrong answer '%s' expected '%s'\n", dp->pName, "localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_A returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_A returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_A returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp == InvalidPointer || broken(dp == NULL), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    //DnsQuery_UTF8:
    //NULL
    dp = InvalidPointer;
    dns_status = DnsQuery_UTF8(NULL, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == ERROR_INVALID_PARAMETER, "DnsQuery_UTF8 failed with error %lu\n", dns_status);
    ok(dp == InvalidPointer, "dp = %p\n", dp);

    //NULL dp
    dns_status = DnsQuery_UTF8(host_nameUTF8, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, NULL, 0);
    ok(dns_status == ERROR_INVALID_PARAMETER, "DnsQuery_UTF8 failed with error %lu\n", dns_status);

    //Testing HostName
    dp = InvalidPointer;
    dns_status = DnsQuery_UTF8(host_nameUTF8, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_UTF8 failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        ok(strcmp(dp->pName, host_name) == 0, "DnsQuery_UTF8 returned wrong answer '%s' expected '%s'\n", dp->pName, host_name);
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_UTF8 returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_UTF8 returned wrong data size %d\n", dp->wDataLength);
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    //127.0.0.1
    dp = InvalidPointer;
    wcscpy(test_nameW, L"127.0.0.1");
    wcstombs(test_nameUTF8, test_nameW, 255);
    dns_status = DnsQuery_UTF8(test_nameUTF8, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_UTF8 failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        ok(strcmp(dp->pName, "127.0.0.1") == 0, "DnsQuery_UTF8 returned wrong answer '%s' expected '%s'\n", dp->pName, "127.0.0.1");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_UTF8 returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_UTF8 returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_UTF8 returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    //Localhost strings
    dp = InvalidPointer;
    wcscpy(test_nameW, L"LocalHost");
    wcstombs(test_nameUTF8, test_nameW, 255);
    dns_status = DnsQuery_UTF8(test_nameUTF8, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_UTF8 failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        /* On Windows 7 is unchanged on XP is lowercased */
        ok(strcmp(dp->pName, "localhost") == 0 || broken(strcmp(dp->pName, "LocalHost") == 0), "DnsQuery_UTF8 returned wrong answer '%s' expected '%s'\n", dp->pName, "localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_UTF8 returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_UTF8 returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_UTF8 returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    wcscpy(test_nameW, L"Localhost");
    wcstombs(test_nameUTF8, test_nameW, 255);
    dns_status = DnsQuery_UTF8(test_nameUTF8, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_UTF8 failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        /* On Windows 7 is unchanged on XP is lowercased */
        ok(strcmp(dp->pName, "localhost") == 0 || broken(strcmp(dp->pName, "Localhost") == 0), "DnsQuery_UTF8 returned wrong answer '%s' expected '%s'\n", dp->pName, "localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_UTF8 returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_UTF8 returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_UTF8 returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    wcscpy(test_nameW, L"localhost");
    wcstombs(test_nameUTF8, test_nameW, 255);
    dns_status = DnsQuery_UTF8(test_nameUTF8, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_UTF8 failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        ok(strcmp(dp->pName, "localhost") == 0, "DnsQuery_UTF8 returned wrong answer '%s' expected '%s'\n", dp->pName, "localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_UTF8 returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_UTF8 returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_UTF8 returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    wcscpy(test_nameW, L"");
    wcstombs(test_nameUTF8, test_nameW, 255);
    dns_status = DnsQuery_UTF8(test_nameUTF8, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_UTF8 failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        /* On Windows 7 is the host on XP is dot ??? */
        ok(strcmp(dp->pName, ".") == 0 || broken(strcmp(dp->pName, host_name) == 0), "DnsQuery_UTF8 returned wrong answer '%s' expected '%s' or '.'\n", dp->pName, host_name);
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_UTF8 returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_UTF8 returned wrong data size %d\n", dp->wDataLength);
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    wcscpy(test_nameW, L" ");
    wcstombs(test_nameUTF8, test_nameW, 255);
    dns_status = DnsQuery_UTF8(test_nameUTF8, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    /* On Windows 7 is DNS_ERROR_INVALID_NAME_CHAR on XP is ERROR_TIMEOUT on Win 2k3 is ERROR_INVALID_NAME*/
    ok(dns_status == ERROR_INVALID_NAME || broken(dns_status == ERROR_TIMEOUT) || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_UTF8 failed with error %lu expected %u or %u or %u\n", dns_status, ERROR_INVALID_NAME, ERROR_TIMEOUT, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(strcmp(dp->pName, host_name) == 0, "DnsQuery_UTF8 returned wrong answer '%s' expected '%s'\n", dp->pName, host_name);
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_UTF8 returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_UTF8 returned wrong data size %d\n", dp->wDataLength);
    }
    ok(dp == InvalidPointer || broken(dp == NULL), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    wcscpy(test_nameW, L"0.0.0.0");
    wcstombs(test_nameUTF8, test_nameW, 255);
    dns_status = DnsQuery_UTF8(test_nameUTF8, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_UTF8 failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        ok(strcmp(dp->pName, "0.0.0.0") == 0, "DnsQuery_UTF8 returned wrong answer '%s' expected '%s'\n", dp->pName, "0.0.0.0");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_UTF8 returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_UTF8 returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_ANY), "DnsQuery_UTF8 returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_ANY));
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    wcscpy(test_nameW, L"0.0.0.0 ");
    wcstombs(test_nameUTF8, test_nameW, 255);
    dns_status = DnsQuery_UTF8(test_nameUTF8, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    /* On windows 7 fails with DNS_ERROR_INVALID_NAME_CHAR on XP no error */
    ok(dns_status == NO_ERROR || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_UTF8 wrong status %lu expected %u or %u\n", dns_status, NO_ERROR, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(strcmp(dp->pName, "0.0.0.0") == 0 || broken(strcmp(dp->pName, "0.0.0.0 ") == 0), "DnsQuery_UTF8 returned wrong answer '%s' expected '%s' or '%s'\n", dp->pName, "0.0.0.0", "0.0.0.0 ");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_UTF8 returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_UTF8 returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_ANY), "DnsQuery_UTF8 returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_ANY));
    }
    ok(dp != InvalidPointer || broken(dp == InvalidPointer) || broken(dp == NULL), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    wcscpy(test_nameW, L"127.0.0.1 ");
    wcstombs(test_nameUTF8, test_nameW, 255);
    dns_status = DnsQuery_UTF8(test_nameUTF8, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    /* On windows 7 fails with DNS_ERROR_INVALID_NAME_CHAR on XP no error */
    ok(dns_status == NO_ERROR || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_UTF8 wrong status %lu expected %u or %u\n", dns_status, NO_ERROR, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(strcmp(dp->pName, "127.0.0.1") == 0 || strcmp(dp->pName, "127.0.0.1 ") == 0, "DnsQuery_UTF8 returned wrong answer '%s' expected '%s' or '%s'\n", dp->pName, "127.0.0.1", "127.0.0.1 ");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_UTF8 returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_UTF8 returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_UTF8 returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp != InvalidPointer || broken(dp == InvalidPointer) || broken(dp == NULL), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    wcscpy(test_nameW, L" 127.0.0.1 ");
    wcstombs(test_nameUTF8, test_nameW, 255);
    dns_status = DnsQuery_UTF8(test_nameUTF8, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == DNS_ERROR_RCODE_NAME_ERROR || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_UTF8 wrong status %lu expected %u or %u\n", dns_status, DNS_ERROR_RCODE_NAME_ERROR, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(strcmp(dp->pName, "127.0.0.1") == 0, "DnsQuery_UTF8 returned wrong answer '%s' expected '%s'\n", dp->pName, "127.0.0.1");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_UTF8 returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_UTF8 returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_UTF8 returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp == InvalidPointer || broken(dp == NULL), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    wcscpy(test_nameW, L" 127.0. 0.1 ");
    wcstombs(test_nameUTF8, test_nameW, 255);
    dns_status = DnsQuery_UTF8(test_nameUTF8, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == DNS_ERROR_RCODE_NAME_ERROR || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_UTF8 wrong status %lu expected %u or %u\n", dns_status, DNS_ERROR_RCODE_NAME_ERROR, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp == InvalidPointer && dns_status == NO_ERROR)
    {
        ok(strcmp(dp->pName, "127.0.0.1") == 0, "DnsQuery_UTF8 returned wrong answer '%s' expected '%s'\n", dp->pName, "127.0.0.1");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_UTF8 returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_UTF8 returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_UTF8 returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp == InvalidPointer || broken(dp == NULL), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    wcscpy(test_nameW, L"localhost ");
    wcstombs(test_nameUTF8, test_nameW, 255);
    dns_status = DnsQuery_UTF8(test_nameUTF8, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == ERROR_INVALID_NAME || broken(dns_status == ERROR_TIMEOUT) || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_UTF8 wrong status %lu expected %u or %u or %u\n", dns_status, ERROR_INVALID_NAME, ERROR_TIMEOUT, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(strcmp(dp->pName, "localhost") == 0, "DnsQuery_UTF8 returned wrong answer '%s' expected '%s'\n", dp->pName, "localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_UTF8 returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_UTF8 returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_UTF8 returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp == InvalidPointer || broken(dp == NULL), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    wcscpy(test_nameW, L" localhost");
    wcstombs(test_nameUTF8, test_nameW, 255);
    dns_status = DnsQuery_UTF8(test_nameUTF8, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == ERROR_INVALID_NAME || broken(dns_status == ERROR_TIMEOUT) || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_UTF8 wrong status %lu expected %u or %u or %u\n", dns_status, ERROR_INVALID_NAME, ERROR_TIMEOUT, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(strcmp(dp->pName, "localhost") == 0, "DnsQuery_UTF8 returned wrong answer '%s' expected '%s'\n", dp->pName, "localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_UTF8 returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_UTF8 returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_UTF8 returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp == InvalidPointer || broken(dp == NULL), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    wcscpy(test_nameW, L" local host ");
    wcstombs(test_nameUTF8, test_nameW, 255);
    dns_status = DnsQuery_UTF8(test_nameUTF8, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == ERROR_INVALID_NAME || broken(dns_status == ERROR_TIMEOUT) || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_UTF8 wrong status %lu expected %u or %u or %u\n", dns_status, ERROR_INVALID_NAME, ERROR_TIMEOUT, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(strcmp(dp->pName, "localhost") == 0, "DnsQuery_UTF8 returned wrong answer '%s' expected '%s'\n", dp->pName, "localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_UTF8 returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_UTF8 returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_UTF8 returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp == InvalidPointer || broken(dp == NULL), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    //DnsQuery_W:
    //NULL
    dp = InvalidPointer;
    dns_status = DnsQuery_W(NULL, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    if (dns_status == NO_ERROR)
    {
        /* Win2003 */
        ok(dns_status == NO_ERROR, "DnsQuery_W failed with error %lu\n", dns_status);
        ok(dp != NULL && dp != InvalidPointer, "dp = %p\n", dp);
    }
    else
    {
        /* Win7 */
        ok(dns_status == ERROR_INVALID_PARAMETER, "DnsQuery_W failed with error %lu\n", dns_status);
        ok(dp == InvalidPointer, "dp = %p\n", dp);
    }
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    //NULL dp
    dns_status = DnsQuery_W(host_nameW, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, NULL, 0);
    ok(dns_status == ERROR_INVALID_PARAMETER, "DnsQuery_W failed with error %lu\n", dns_status);

    //Testing HostName
    dns_status = DnsQuery_W(host_nameW, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_W failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        ok(wcscmp((LPCWSTR)dp->pName, host_nameW) == 0, "DnsQuery_w returned wrong answer '%ls' expected '%ls'\n", (LPCWSTR)dp->pName, host_nameW);
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_W returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_w returned wrong data size %d\n", dp->wDataLength);
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    //127.0.0.1
    dns_status = DnsQuery_W(L"127.0.0.1", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_W failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        ok(wcscmp((LPCWSTR)dp->pName, L"127.0.0.1") == 0, "DnsQuery_W returned wrong answer '%ls' expected '%ls'\n", (LPCWSTR)dp->pName, L"127.0.0.1");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_W returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_W returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_W returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    //Localhost strings
    dns_status = DnsQuery_W(L"LocalHost", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_W failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        ok(wcscmp((LPCWSTR)dp->pName, L"localhost") == 0 || broken(wcscmp((LPCWSTR)dp->pName, L"LocalHost") == 0), "DnsQuery_W returned wrong answer '%ls' expected '%ls'\n", (LPCWSTR)dp->pName, L"localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_W returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_W returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_W returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    dns_status = DnsQuery_W(L"Localhost", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_W failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        ok(wcscmp((LPCWSTR)dp->pName, L"localhost") == 0 || broken(wcscmp((LPCWSTR)dp->pName, L"Localhost") == 0), "DnsQuery_W returned wrong answer '%ls' expected '%ls'\n", (LPCWSTR)dp->pName, L"localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_W returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_W returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_W returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    dns_status = DnsQuery_W(L"localhost", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_W failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        ok(wcscmp((LPCWSTR)dp->pName, L"localhost") == 0, "DnsQuery_W returned wrong answer '%ls' expected '%ls'\n", (LPCWSTR)dp->pName, L"localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_W returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_W returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_W returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    dns_status = DnsQuery_W(L"", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_W failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        /* On Windows 7 is the host on XP is dot ??? */
        ok(wcscmp((LPCWSTR)dp->pName, L".") == 0 || broken(wcscmp((LPCWSTR)dp->pName, host_nameW) == 0), "DnsQuery_W returned wrong answer '%ls' expected '%ls'\n", (LPCWSTR)dp->pName, host_nameW);
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_W returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_W returned wrong data size %d\n", dp->wDataLength);
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    dns_status = DnsQuery_W(L" ", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == ERROR_INVALID_NAME || broken(dns_status == ERROR_TIMEOUT) || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_W wrong status %lu expected %u or %u or %d\n", dns_status, ERROR_INVALID_NAME, ERROR_TIMEOUT, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(wcscmp((LPCWSTR)dp->pName, L"localhost") == 0, "DnsQuery_W returned wrong answer '%ls' expected '%ls'\n", (LPCWSTR)dp->pName, L"localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_W returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_W returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_W returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp == NULL || broken(dp == InvalidPointer), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dns_status = DnsQuery_W(L"0.0.0.0", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_W failed with error %lu\n", dns_status);
    if (dp != InvalidPointer && dp)
    {
        ok(wcscmp((LPCWSTR)dp->pName, L"0.0.0.0") == 0, "DnsQuery_W returned wrong answer '%ls' expected '%ls'\n", (LPCWSTR)dp->pName, L"0.0.0.0");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_W returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_W returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_ANY), "DnsQuery_W returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_ANY));
    }
    ok(dp != InvalidPointer && dp != NULL, "dp = %p\n", dp);
    if (dp != InvalidPointer) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    dns_status = DnsQuery_W(L"0.0.0.0 ", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_W wrong status %lu expected %u or %u\n", dns_status, NO_ERROR, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(wcscmp((LPCWSTR)dp->pName, L"0.0.0.0") == 0 || broken(wcscmp((LPCWSTR)dp->pName, L"0.0.0.0 ") == 0), "DnsQuery_W returned wrong answer '%ls' expected '%ls'\n", (LPCWSTR)dp->pName, L"0.0.0.0");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_W returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_W returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_ANY), "DnsQuery_W returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_ANY));
    }
    ok(dp != InvalidPointer || broken(dp == InvalidPointer) || broken(dp == NULL), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    dns_status = DnsQuery_W(L"127.0.0.1 ", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_W wrong status %lu expected %u or %u\n", dns_status, NO_ERROR, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(wcscmp((LPCWSTR)dp->pName, L"127.0.0.1") == 0 || broken(wcscmp((LPCWSTR)dp->pName, L"127.0.0.1 ") == 0), "DnsQuery_W returned wrong answer '%ls' expected '%ls'\n", (LPCWSTR)dp->pName, L"127.0.0.1");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_W returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_W returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_W returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp != InvalidPointer || broken(dp == InvalidPointer) || broken(dp == NULL), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    dns_status = DnsQuery_W(L" 127.0.0.1 ", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == DNS_ERROR_RCODE_NAME_ERROR || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_W wrong status %lu expected %u or %u\n", dns_status, DNS_ERROR_RCODE_NAME_ERROR, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(wcscmp((LPCWSTR)dp->pName, L"localhost") == 0, "DnsQuery_W returned wrong answer '%ls' expected '%ls'\n", (LPCWSTR)dp->pName, L"localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_W returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_W returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_W returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp == NULL || broken(dp == InvalidPointer), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    dns_status = DnsQuery_W(L" 127.0. 0.1 ", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == DNS_ERROR_RCODE_NAME_ERROR || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_W wrong status %lu expected %u or %u\n", dns_status, DNS_ERROR_RCODE_NAME_ERROR, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(wcscmp((LPCWSTR)dp->pName, L"localhost") == 0, "DnsQuery_W returned wrong answer '%ls' expected '%ls'\n", (LPCWSTR)dp->pName, L"localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_W returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_W returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_W returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp == NULL || broken(dp == InvalidPointer), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    dns_status = DnsQuery_W(L"localhost ", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == ERROR_INVALID_NAME || broken(dns_status == ERROR_TIMEOUT) || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_W wrong status %lu expected %u or %u or %u\n", dns_status, ERROR_INVALID_NAME, ERROR_TIMEOUT, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(wcscmp((LPCWSTR)dp->pName, L"localhost") == 0, "DnsQuery_W returned wrong answer '%ls' expected '%ls'\n", (LPCWSTR)dp->pName, L"localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_W returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_W returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_W returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp == NULL || broken(dp == InvalidPointer), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    dns_status = DnsQuery_W(L" localhost", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == ERROR_INVALID_NAME || broken(dns_status == ERROR_TIMEOUT) || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_W wrong status %lu expected %u or %u or %u\n", dns_status, ERROR_INVALID_NAME, ERROR_TIMEOUT, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(wcscmp((LPCWSTR)dp->pName, L"localhost") == 0, "DnsQuery_W returned wrong answer '%ls' expected '%ls'\n", (LPCWSTR)dp->pName, L"localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_W returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_W returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_W returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp == NULL || broken(dp == InvalidPointer), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);

    dp = InvalidPointer;
    wcscpy(test_nameW, L" local host ");
    dns_status = DnsQuery_W(test_nameW, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == ERROR_INVALID_NAME || broken(dns_status == ERROR_TIMEOUT) || broken(dns_status == DNS_ERROR_INVALID_NAME_CHAR), "DnsQuery_W wrong status %lu expected %u or %u or %u\n", dns_status, ERROR_INVALID_NAME, ERROR_TIMEOUT, DNS_ERROR_INVALID_NAME_CHAR);
    if (dp != InvalidPointer && dns_status == NO_ERROR)
    {
        ok(wcscmp((LPCWSTR)dp->pName, L"localhost") == 0, "DnsQuery_W returned wrong answer '%ls' expected '%ls'\n", (LPCWSTR)dp->pName, L"localhost");
        ok(dp->wType == DNS_TYPE_A, "DnsQuery_W returned wrong type %d expected %d\n", dp->wType, DNS_TYPE_A);
        ok(dp->wDataLength == sizeof(IP4_ADDRESS), "DnsQuery_W returned wrong data size %d\n", dp->wDataLength);
        ok(dp->Data.A.IpAddress == ntohl(INADDR_LOOPBACK), "DnsQuery_W returned wrong data %ld expected %ld\n", dp->Data.A.IpAddress, ntohl(INADDR_LOOPBACK));
    }
    ok(dp == NULL || broken(dp == InvalidPointer), "dp = %p\n", dp);
    if (dp != InvalidPointer && dns_status == NO_ERROR) DnsRecordListFree(dp, DnsFreeRecordList);
}

START_TEST(DnsQuery)
{
    WSADATA wsaData;
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    ok(iResult == 0, "WSAStartup failed: %d\n", iResult);
    if (iResult != 0) return;

    // Tests
    TestHostName();

    WSACleanup();

    return;
}
