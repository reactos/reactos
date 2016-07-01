/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for DnsQuery_A
 * PROGRAMMER:      Victor Martinez Calvo <victor.martinez@reactos.org>
 */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <windns.h>
#include <apitest.h>


void TestHostName(void)
{

    DNS_STATUS dns_status = NO_ERROR;
    char host_name[255];
    PDNS_RECORD dp = NULL;
    WCHAR host_nameW[255];

    gethostname(host_name, sizeof(host_name));
    mbstowcs(host_nameW, host_name, 255);

    //DnsQuery_A:
    //NULL
    dp = InvalidPointer;
    dns_status = DnsQuery_A(NULL, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == ERROR_INVALID_PARAMETER, "DnsQuery_A failed with error %lu\n", dns_status);
    ok(dp == InvalidPointer, "dp = %p\n", dp);
    
    //Testing HostName 
    dns_status = DnsQuery_A(host_name, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_A failed with error %lu\n", dns_status);
    DnsRecordListFree(dp, DnsFreeRecordList);
    
    //127.0.0.1
    dns_status = DnsQuery_A("127.0.0.1", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_A failed with error %lu\n", dns_status);
    DnsRecordListFree(dp, DnsFreeRecordList);

    //Localhost strings
    dns_status = DnsQuery_A("LocalHost", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_A failed with error %lu\n", dns_status);
    DnsRecordListFree(dp, DnsFreeRecordList);

    dns_status = DnsQuery_A("Localhost", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_A failed with error %lu\n", dns_status);
    DnsRecordListFree(dp, DnsFreeRecordList);

    dns_status = DnsQuery_A("localhost", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_A failed with error %lu\n", dns_status);
    DnsRecordListFree(dp, DnsFreeRecordList);

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

    //Testing HostName 
    dns_status = DnsQuery_W(host_nameW, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_W failed with error %lu\n", dns_status);
    DnsRecordListFree(dp, DnsFreeRecordList);
    
    //127.0.0.1
    dns_status = DnsQuery_W(L"127.0.0.1", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_W failed with error %lu\n", dns_status);
    DnsRecordListFree(dp, DnsFreeRecordList);

    //Localhost strings
    dns_status = DnsQuery_W(L"LocalHost", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_W failed with error %lu\n", dns_status);
    DnsRecordListFree(dp, DnsFreeRecordList);

    dns_status = DnsQuery_W(L"Localhost", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_W failed with error %lu\n", dns_status);
    DnsRecordListFree(dp, DnsFreeRecordList);

    dns_status = DnsQuery_W(L"localhost", DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &dp, 0);
    ok(dns_status == NO_ERROR, "DnsQuery_W failed with error %lu\n", dns_status);
    DnsRecordListFree(dp, DnsFreeRecordList);
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

    return;
}
