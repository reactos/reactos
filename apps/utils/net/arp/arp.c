/*
 * arp - display ARP cache from the IP stack parameters.
 *
 * This source code is in the PUBLIC DOMAIN and has NO WARRANTY.
 *
 * Robert Dickenson <robd@reactos.org>, August 15, 2002.
 */
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <time.h>

#include <iptypes.h>
#include <ipexport.h>
#include <iphlpapi.h>
#include <snmp.h>

#include "trace.h"


VOID SNMP_FUNC_TYPE SnmpSvcInitUptime();
DWORD SNMP_FUNC_TYPE SnmpSvcGetUptime();

////////////////////////////////////////////////////////////////////////////////

const char szUsage[] = { "\n" \
    "Displays and modifies the IP Protocol to physical address translation tables\n" \
    "used by address resolution protocol (ARP).\n" \
    "\n" \
    "ARP -s inet_addr eth_addr [if_addr]\n" \
    "ARP -d inet_addr [if_addr]\n" \
    "ARP -a [inet_addr] [-N if_addr]\n" \
    "\n" \
    "  -a            Displays the active ARP table by querying the current protocol\n" \
    "                data. If inet_addr is specified, the IP and physical addresses\n" \
    "                for the specified address are displayed. If more than one\n" \
    "                network interface is using ARP, each interfaces ARP table is\n" \
    "                displayed.\n" \
    "  -g            Indentical to -a.\n" \
    "  inet_addr     Specifies the IP address.\n" \
    "  -N if_addr    Displays the ARP table for the specified interface only\n" \
    "  -d            Deletes the host entry specified by inet_addr. inet_addr may be\n" \
    "                wildcarded with * to delete all host entries in the ARP table.\n" \
    "  -s            Adds the host and associates the IP address inet_addr with the\n" \
    "                physical address eth_addr. The physical address must be specified\n" \
    "                as 6 hexadecimal characters delimited by hyphens. The new entry\n" \
    "                will become permanent in the ARP table.\n" \
    "  eth_addr      Specifies the interface physical address.\n" \
    "  if_addr       If present, this specifies the IP address of the interface whose\n" \
    "                address translation table should be modified. If not present, the\n" \
    "                first applicable interface will be used.\n" \
    "Example:\n" \
    "  > arp -s 192.168.0.12   55-AA-55-01-02-03   .... Static entry creation.\n" \
    "  > arp -a                                    .... ARP table display.\n" \
    "  > arp -d *                                  .... Delete all ARP table entries.\n"
};

void usage(void)
{
//	fprintf(stderr,"USAGE:\n");
	fputs(szUsage, stderr);
}

int main(int argc, char *argv[])
{
    TCHAR szComputerName[50];
    DWORD dwSize = 50;

    int nBytes = 500;
    BYTE* pCache;

    if (argc > 1) {
        usage();
        return 1;
    }

    SnmpSvcInitUptime();

    GetComputerName(szComputerName, &dwSize);
    _tprintf(_T("ReactOS ARP cache on Computer Name: %s\n"), szComputerName);

    pCache = (BYTE*)SnmpUtilMemAlloc(nBytes);

        Sleep(2500);

    if (pCache != NULL) {

        DWORD dwUptime = SnmpSvcGetUptime();

        _tprintf(_T("SNMP uptime: %d\n"), dwUptime);

        SnmpUtilMemFree(pCache);
    } else {
        _tprintf(_T("ERROR: call to SnmpUtilMemAlloc() failed\n"));
        return 1;
    }
	return 0;
}

