/*
 * PROJECT:     ReactOS netstat utility
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        apps/utils/net/netstat/netstat.c
 * PURPOSE:     display IP stack statistics
 * COPYRIGHT:   Copyright 2005 Ged Murphy <gedmurphy@gmail.com>
 */
/*
 * TODO:
 * sort function return values.
 * implement -b, -o and -v
 * clean up GetIpHostName
 * command line parser needs more work
 */

#include <windows.h>
#include <winsock.h>
#include <tchar.h>
#include <stdio.h>
#include <iphlpapi.h>
#include "netstat.h"


enum ProtoType {IP, TCP, UDP, ICMP} Protocol;
DWORD Interval; /* time to pause between printing output */

/* TCP endpoint states */
TCHAR TcpState[][32] = {
    _T("???"),
    _T("CLOSED"),
    _T("LISTENING"),
    _T("SYN_SENT"),
    _T("SYN_RCVD"),
    _T("ESTABLISHED"),
    _T("FIN_WAIT1"),
    _T("FIN_WAIT2"),
    _T("CLOSE_WAIT"),
    _T("CLOSING"),
    _T("LAST_ACK"),
    _T("TIME_WAIT"),
    _T("DELETE_TCB")
};


/*
 * format message string and display output
 */
DWORD DoFormatMessage(DWORD ErrorCode)
{
    LPVOID lpMsgBuf;
    DWORD RetVal;

    if ((RetVal = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            ErrorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
            (LPTSTR) &lpMsgBuf,
            0,
            NULL )))
    {
        _tprintf(_T("%s"), (LPTSTR)lpMsgBuf);

        LocalFree(lpMsgBuf);
        /* return number of TCHAR's stored in output buffer
         * excluding '\0' - as FormatMessage does*/
        return RetVal;
    }
    else
        return 0;
}


/*
 *
 * Parse command line parameters and set any options
 *
 */
BOOL ParseCmdline(int argc, char* argv[])
{
    INT i;

    TCHAR Proto[5];

    if ((argc == 1) || (_istdigit(*argv[1])))
        bNoOptions = TRUE;

    /* Parse command line for options we have been given. */
    for (i = 1; i < argc; i++)
    {
        if ( (argc > 1)&&(argv[i][0] == '-') )
        {
            TCHAR c;

            while ((c = *++argv[i]) != '\0')
            {
                switch (tolower(c))
                {
                    case 'a' :
                        bDoShowAllCons = TRUE;
                        break;
                    case 'b' :
                        bDoShowProcName = TRUE;
                        break;
                    case 'e' :
                        bDoShowEthStats = TRUE;
                        break;
                    case 'n' :
                        bDoShowNumbers = TRUE;
                        break;
                    case 's' :
                        bDoShowProtoStats = TRUE;
                        break;
                    case 'p' :
                        bDoShowProtoCons = TRUE;

                        strncpy(Proto, (++argv)[i], sizeof(Proto));
                        if (!_tcsicmp( "IP", Proto ))
                            Protocol = IP;
                        else if (!_tcsicmp( "ICMP", Proto ))
                            Protocol = ICMP;
                        else if (!_tcsicmp( "TCP", Proto ))
                            Protocol = TCP;
                        else if (!_tcsicmp( "UDP", Proto ))
                            Protocol = UDP;
                        else
                        {
                            Usage();
                            return EXIT_FAILURE;
                        }
                        --i; /* move pointer back down to previous argv */
                        break;
                    case 'r' :
                        bDoShowRouteTable = TRUE;
                        break;
                    case 'v' :
                        _tprintf(_T("got v\n"));
                        bDoDispSeqComp = TRUE;
                        break;
                    default :
                        Usage();
                        return EXIT_FAILURE;
                }
            }
        }
        else if (_istdigit(*argv[i]))
        {
            if (_stscanf(argv[i], "%lu", &Interval) != EOF)
                bLoopOutput = TRUE;
            else
                return EXIT_FAILURE;
        }
//        else
//        {
//            Usage();
//            EXIT_FAILURE;
//        }
    }

    return EXIT_SUCCESS;
}


/*
 * Simulate Microsofts netstat utility output
 */
BOOL DisplayOutput()
{
    if (bNoOptions)
    {
        _tprintf(_T("\n  Proto  Local Address          Foreign Address        State\n"));
        ShowTcpTable();
        return EXIT_SUCCESS;
    }

    if (bDoShowRouteTable)
    {
        /* mingw doesn't have lib for _tsystem */
        if (system("route print") == -1)
        {
            _tprintf(_T("cannot find 'route.exe'\n"));
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    if (bDoShowEthStats)
    {
        ShowEthernetStatistics();
        return EXIT_SUCCESS;
    }

    if (bDoShowProtoCons)
    {
        switch (Protocol)
        {
                case IP :
                    if (bDoShowProtoStats)
                    {
                        ShowIpStatistics();
                        return EXIT_SUCCESS;
                    }
                    break;
                case ICMP :
                    if (bDoShowProtoStats)
                    {
                        ShowIcmpStatistics();
                        return EXIT_SUCCESS;
                    }
                    break;
                case TCP :
                    if (bDoShowProtoStats)
                        ShowTcpStatistics();
                    _tprintf(_T("\nActive Connections\n"));
                    _tprintf(_T("\n  Proto  Local Address          Foreign Address        State\n"));
                    ShowTcpTable();
                    break;
                case UDP :
                    if (bDoShowProtoStats)
                        ShowUdpStatistics();
                    _tprintf(_T("\nActive Connections\n"));
                    _tprintf(_T("\n  Proto  Local Address          Foreign Address        State\n"));
                    ShowUdpTable();
                    break;
                default :
                    break;
        }
    }
    else if (bDoShowProtoStats)
    {
        ShowIpStatistics();
        ShowIcmpStatistics();
        ShowTcpStatistics();
        ShowUdpStatistics();
        return EXIT_SUCCESS;
    }
    else //if (bDoShowAllCons)
    {
        _tprintf(_T("\nActive Connections\n"));
        _tprintf(_T("\n  Proto  Local Address          Foreign Address        State\n"));
        ShowTcpTable();
        ShowUdpTable();
    }
    return EXIT_SUCCESS;
}




VOID ShowIpStatistics()
{
    PMIB_IPSTATS pIpStats;
    DWORD dwRetVal;

    pIpStats = (MIB_IPSTATS*) HeapAlloc(GetProcessHeap(), 0, sizeof(MIB_IPSTATS));

    if ((dwRetVal = GetIpStatistics(pIpStats)) == NO_ERROR)
    {
        _tprintf(_T("\nIPv4 Statistics\n\n"));
        _tprintf(_T("  %-34s = %lu\n"), _T("Packets Recieved"), pIpStats->dwInReceives);
        _tprintf(_T("  %-34s = %lu\n"), _T("Received Header Errors"), pIpStats->dwInHdrErrors);
        _tprintf(_T("  %-34s = %lu\n"), _T("Received Address Errors"), pIpStats->dwInAddrErrors);
        _tprintf(_T("  %-34s = %lu\n"), _T("Datagrams Forwarded"), pIpStats->dwForwDatagrams);
        _tprintf(_T("  %-34s = %lu\n"), _T("Unknown Protocols Recieved"), pIpStats->dwInUnknownProtos);
        _tprintf(_T("  %-34s = %lu\n"), _T("Received Packets Discarded"), pIpStats->dwInDiscards);
        _tprintf(_T("  %-34s = %lu\n"), _T("Recieved Packets Delivered"), pIpStats->dwInDelivers);
        _tprintf(_T("  %-34s = %lu\n"), _T("Output Requests"), pIpStats->dwOutRequests);
        _tprintf(_T("  %-34s = %lu\n"), _T("Routing Discards"), pIpStats->dwRoutingDiscards);
        _tprintf(_T("  %-34s = %lu\n"), _T("Discarded Output Packets"), pIpStats->dwOutDiscards);
        _tprintf(_T("  %-34s = %lu\n"), _T("Output Packets No Route"), pIpStats->dwOutNoRoutes);
        _tprintf(_T("  %-34s = %lu\n"), _T("Reassembly Required"), pIpStats->dwReasmReqds);
        _tprintf(_T("  %-34s = %lu\n"), _T("Reassembly Succesful"), pIpStats->dwReasmOks);
        _tprintf(_T("  %-34s = %lu\n"), _T("Reassembly Failures"), pIpStats->dwReasmFails);
       // _tprintf(_T("  %-34s = %lu\n"), _T("Datagrams succesfully fragmented"), NULL); /* FIXME: what is this one? */
        _tprintf(_T("  %-34s = %lu\n"), _T("Datagrams Failing Fragmentation"), pIpStats->dwFragFails);
        _tprintf(_T("  %-34s = %lu\n"), _T("Fragments Created"), pIpStats->dwFragCreates);
    }
    else
        DoFormatMessage(dwRetVal);

    HeapFree(GetProcessHeap(), 0, pIpStats);
}

VOID ShowIcmpStatistics()
{
    PMIB_ICMP pIcmpStats;
    DWORD dwRetVal;

    pIcmpStats = (MIB_ICMP*) HeapAlloc(GetProcessHeap(), 0, sizeof(MIB_ICMP));

    if ((dwRetVal = GetIcmpStatistics(pIcmpStats)) == NO_ERROR)
    {
        _tprintf(_T("\nICMPv4 Statistics\n\n"));
        _tprintf(_T("                            Received    Sent\n"));
        _tprintf(_T("  %-25s %-11lu %lu\n"), _T("Messages"),
            pIcmpStats->stats.icmpInStats.dwMsgs, pIcmpStats->stats.icmpOutStats.dwMsgs);
        _tprintf(_T("  %-25s %-11lu %lu\n"), _T("Errors"),
            pIcmpStats->stats.icmpInStats.dwErrors, pIcmpStats->stats.icmpOutStats.dwErrors);
        _tprintf(_T("  %-25s %-11lu %lu\n"), _T("Destination Unreachable"),
            pIcmpStats->stats.icmpInStats.dwDestUnreachs, pIcmpStats->stats.icmpOutStats.dwDestUnreachs);
        _tprintf(_T("  %-25s %-11lu %lu\n"), _T("Time Exceeded"),
            pIcmpStats->stats.icmpInStats.dwTimeExcds, pIcmpStats->stats.icmpOutStats.dwTimeExcds);
        _tprintf(_T("  %-25s %-11lu %lu\n"), _T("Parameter Problems"),
            pIcmpStats->stats.icmpInStats.dwParmProbs, pIcmpStats->stats.icmpOutStats.dwParmProbs);
        _tprintf(_T("  %-25s %-11lu %lu\n"), _T("Source Quenches"),
            pIcmpStats->stats.icmpInStats.dwSrcQuenchs, pIcmpStats->stats.icmpOutStats.dwSrcQuenchs);
        _tprintf(_T("  %-25s %-11lu %lu\n"), _T("Redirects"),
            pIcmpStats->stats.icmpInStats.dwRedirects, pIcmpStats->stats.icmpOutStats.dwRedirects);
        _tprintf(_T("  %-25s %-11lu %lu\n"), _T("Echos"),
            pIcmpStats->stats.icmpInStats.dwEchos, pIcmpStats->stats.icmpOutStats.dwEchos);
        _tprintf(_T("  %-25s %-11lu %lu\n"), _T("Echo Replies"),
            pIcmpStats->stats.icmpInStats.dwEchoReps, pIcmpStats->stats.icmpOutStats.dwEchoReps);
        _tprintf(_T("  %-25s %-11lu %lu\n"), _T("Timestamps"),
            pIcmpStats->stats.icmpInStats.dwTimestamps, pIcmpStats->stats.icmpOutStats.dwTimestamps);
        _tprintf(_T("  %-25s %-11lu %lu\n"), _T("Timestamp Replies"),
            pIcmpStats->stats.icmpInStats.dwTimestampReps, pIcmpStats->stats.icmpOutStats.dwTimestampReps);
        _tprintf(_T("  %-25s %-11lu %lu\n"), _T("Address Masks"),
            pIcmpStats->stats.icmpInStats.dwAddrMasks, pIcmpStats->stats.icmpOutStats.dwAddrMasks);
        _tprintf(_T("  %-25s %-11lu %lu\n"), _T("Address Mask Replies"),
            pIcmpStats->stats.icmpInStats.dwAddrMaskReps, pIcmpStats->stats.icmpOutStats.dwAddrMaskReps);
    }
    else
        DoFormatMessage(dwRetVal);

    HeapFree(GetProcessHeap(), 0, pIcmpStats);

}

VOID ShowTcpStatistics()
{
    PMIB_TCPSTATS pTcpStats;
    DWORD dwRetVal;

    pTcpStats = (MIB_TCPSTATS*) HeapAlloc(GetProcessHeap(), 0, sizeof(MIB_TCPSTATS));

    if ((dwRetVal = GetTcpStatistics(pTcpStats)) == NO_ERROR)
    {
        _tprintf(_T("\nTCP Statistics for IPv4\n\n"));
        _tprintf(_T("  %-35s = %lu\n"), _T("Active Opens"), pTcpStats->dwActiveOpens);
        _tprintf(_T("  %-35s = %lu\n"), _T("Passive Opens"), pTcpStats->dwPassiveOpens);
        _tprintf(_T("  %-35s = %lu\n"), _T("Failed Connection Attempts"), pTcpStats->dwAttemptFails);
        _tprintf(_T("  %-35s = %lu\n"), _T("Reset Connections"), pTcpStats->dwEstabResets);
        _tprintf(_T("  %-35s = %lu\n"), _T("Current Connections"), pTcpStats->dwCurrEstab);
        _tprintf(_T("  %-35s = %lu\n"), _T("Segments Recieved"), pTcpStats->dwInSegs);
        _tprintf(_T("  %-35s = %lu\n"), _T("Segments Sent"), pTcpStats->dwOutSegs);
        _tprintf(_T("  %-35s = %lu\n"), _T("Segments Retransmitted"), pTcpStats->dwRetransSegs);
    }
    else
        DoFormatMessage(dwRetVal);

    HeapFree(GetProcessHeap(), 0, pTcpStats);
}

VOID ShowUdpStatistics()
{
    PMIB_UDPSTATS pUdpStats;
    DWORD dwRetVal;

    pUdpStats = (MIB_UDPSTATS*) HeapAlloc(GetProcessHeap(), 0, sizeof(MIB_UDPSTATS));

    if ((dwRetVal = GetUdpStatistics(pUdpStats)) == NO_ERROR)
    {
        _tprintf(_T("\nUDP Statistics for IPv4\n\n"));
        _tprintf(_T("  %-21s = %lu\n"), _T("Datagrams Recieved"), pUdpStats->dwInDatagrams);
        _tprintf(_T("  %-21s = %lu\n"), _T("No Ports"), pUdpStats->dwNoPorts);
        _tprintf(_T("  %-21s = %lu\n"), _T("Recieve Errors"), pUdpStats->dwInErrors);
        _tprintf(_T("  %-21s = %lu\n"), _T("Datagrams Sent"), pUdpStats->dwOutDatagrams);
    }
    else
        DoFormatMessage(dwRetVal);

    HeapFree(GetProcessHeap(), 0, pUdpStats);
}

VOID ShowEthernetStatistics()
{
    PMIB_IFTABLE pIfTable;
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;

    pIfTable = (MIB_IFTABLE*) HeapAlloc(GetProcessHeap(), 0, sizeof(MIB_IFTABLE));

    if (GetIfTable(pIfTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER)
    {
        HeapFree(GetProcessHeap(), 0, pIfTable);
        pIfTable = (MIB_IFTABLE*) HeapAlloc(GetProcessHeap(), 0, dwSize);

        if ((dwRetVal = GetIfTable(pIfTable, &dwSize, 0)) == NO_ERROR)
        {
            _tprintf(_T("Interface Statistics\n\n"));
            _tprintf(_T("                           Received            Sent\n\n"));
            _tprintf(_T("%-20s %14lu %15lu\n"), _T("Bytes"),
                pIfTable->table[0].dwInOctets, pIfTable->table[0].dwOutOctets);
            _tprintf(_T("%-20s %14lu %15lu\n"), _T("Unicast packets"),
                pIfTable->table[0].dwInUcastPkts, pIfTable->table[0].dwOutUcastPkts);
            _tprintf(_T("%-20s %14lu %15lu\n"), _T("Non-unicast packets"),
                pIfTable->table[0].dwInNUcastPkts, pIfTable->table[0].dwOutNUcastPkts);
            _tprintf(_T("%-20s %14lu %15lu\n"), _T("Discards"),
                pIfTable->table[0].dwInDiscards, pIfTable->table[0].dwOutDiscards);
            _tprintf(_T("%-20s %14lu %15lu\n"), _T("Errors"),
                pIfTable->table[0].dwInErrors, pIfTable->table[0].dwOutErrors);
            _tprintf(_T("%-20s %14lu\n"), _T("Unknown Protocols"),
                pIfTable->table[0].dwInUnknownProtos);
        }
        else
            DoFormatMessage(dwRetVal);
    }
    HeapFree(GetProcessHeap(), 0, pIfTable);
}

VOID ShowTcpTable()
{
    PMIB_TCPTABLE tcpTable;
    DWORD error, dwSize;
    DWORD i;
    CHAR HostIp[HOSTNAMELEN], HostPort[PORTNAMELEN];
    CHAR RemoteIp[HOSTNAMELEN], RemotePort[PORTNAMELEN];
    CHAR Host[ADDRESSLEN];
    CHAR Remote[ADDRESSLEN];

    /* Get the table of TCP endpoints */
    dwSize = 0;
    error = GetTcpTable(NULL, &dwSize, TRUE);
    if (error != ERROR_INSUFFICIENT_BUFFER)
    {
        printf("Failed to snapshot TCP endpoints.\n");
        DoFormatMessage(error);
        exit(EXIT_FAILURE);
    }
    tcpTable = (PMIB_TCPTABLE) HeapAlloc(GetProcessHeap(), 0, dwSize);
    error = GetTcpTable(tcpTable, &dwSize, TRUE );
    if (error)
    {
        printf("Failed to snapshot TCP endpoints table.\n");
        DoFormatMessage(error);
        HeapFree(GetProcessHeap(), 0, tcpTable);
        exit(EXIT_FAILURE);
    }

    /* Dump the TCP table */
    for (i = 0; i < tcpTable->dwNumEntries; i++)
    {
        /* If we aren't showing all connections, only display established, close wait
         * and time wait. This is the default output for netstat */
        if (bDoShowAllCons || (tcpTable->table[i].dwState ==  MIB_TCP_STATE_ESTAB)
            || (tcpTable->table[i].dwState ==  MIB_TCP_STATE_CLOSE_WAIT)
            || (tcpTable->table[i].dwState ==  MIB_TCP_STATE_TIME_WAIT))
        {
            /* I've split this up so it's easier to follow */
            GetIpHostName(TRUE, tcpTable->table[i].dwLocalAddr, HostIp, HOSTNAMELEN);
            GetPortName(tcpTable->table[i].dwLocalPort, "tcp", HostPort, PORTNAMELEN);
            GetIpHostName(FALSE, tcpTable->table[i].dwRemoteAddr, RemoteIp, HOSTNAMELEN);
            GetPortName(tcpTable->table[i].dwRemotePort, "tcp", RemotePort, PORTNAMELEN);

            sprintf(Host, "%s:%s", HostIp, HostPort);
            sprintf(Remote, "%s:%s", RemoteIp, RemotePort);

            _tprintf(_T("  %-6s %-22s %-22s %s\n"), _T("TCP"),
            Host, Remote, TcpState[tcpTable->table[i].dwState]);
        }
    }
    HeapFree(GetProcessHeap(), 0, tcpTable);
}


VOID ShowUdpTable()
{
    PMIB_UDPTABLE udpTable;
    DWORD error, dwSize;
    DWORD i;
    CHAR HostIp[HOSTNAMELEN], HostPort[PORTNAMELEN];
    CHAR Host[ADDRESSLEN];

    /* Get the table of UDP endpoints */
    dwSize = 0;
    error = GetUdpTable(NULL, &dwSize, TRUE);
    if (error != ERROR_INSUFFICIENT_BUFFER)
    {
        printf("Failed to snapshot UDP endpoints.\n");
        DoFormatMessage(error);
        exit(EXIT_FAILURE);
    }
    udpTable = (PMIB_UDPTABLE) HeapAlloc(GetProcessHeap(), 0, dwSize);
    error = GetUdpTable(udpTable, &dwSize, TRUE);
    if (error)
    {
        printf("Failed to snapshot UDP endpoints table.\n");
        DoFormatMessage(error);
        HeapFree(GetProcessHeap(), 0, udpTable);
        exit(EXIT_FAILURE);
    }

    /* Dump the UDP table */
    for (i = 0; i < udpTable->dwNumEntries; i++)
    {

        /* I've split this up so it's easier to follow */
        GetIpHostName(TRUE, udpTable->table[i].dwLocalAddr, HostIp, HOSTNAMELEN);
        GetPortName(udpTable->table[i].dwLocalPort, "tcp", HostPort, PORTNAMELEN);

        sprintf(Host, "%s:%s", HostIp, HostPort);

        _tprintf(_T("  %-6s %-22s %-22s\n"), _T("UDP"), Host,  _T("*:*"));
    }

    HeapFree(GetProcessHeap(), 0, udpTable);
}


/*
 * Translate port numbers into their text equivalent if there is one
 */
PCHAR
GetPortName(UINT Port, PCSTR Proto, CHAR Name[], INT NameLen)
{
    struct servent *pSrvent;

    if (bDoShowNumbers)
    {
        sprintf(Name, "%d", htons((WORD)Port));
        return Name;
    }
    /* Try to translate to a name */
    if ((pSrvent = getservbyport(Port, Proto)))
        strcpy(Name, pSrvent->s_name );
    else
        sprintf(Name, "%d", htons((WORD)Port));
    return Name;
}


/*
 * convert addresses into dotted decimal or hostname
 */
PCHAR
GetIpHostName(BOOL Local, UINT IpAddr, CHAR Name[], int NameLen)
{
//  struct hostent *phostent;
    UINT nIpAddr;

    /* display dotted decimal */
    nIpAddr = htonl(IpAddr);
    if (bDoShowNumbers) {
        sprintf(Name, "%d.%d.%d.%d",
            (nIpAddr >> 24) & 0xFF,
            (nIpAddr >> 16) & 0xFF,
            (nIpAddr >> 8) & 0xFF,
            (nIpAddr) & 0xFF);
        return Name;
    }

    Name[0] = _T('\0');

    /* try to resolve the name */
    if (!IpAddr) {
        if (!Local) {
            sprintf(Name, "%d.%d.%d.%d",
                (nIpAddr >> 24) & 0xFF,
                (nIpAddr >> 16) & 0xFF,
                (nIpAddr >> 8) & 0xFF,
                (nIpAddr) & 0xFF);
        } else {
            if (gethostname(Name, NameLen) != 0)
                DoFormatMessage(WSAGetLastError());
        }
    } else if (IpAddr == 0x0100007f) {
        if (Local) {
            if (gethostname(Name, NameLen) != 0)
                DoFormatMessage(WSAGetLastError());
        } else {
            _tcsncpy(Name, _T("localhost"), 10);
        }
//  } else if (phostent = gethostbyaddr((char*)&ipaddr, sizeof(nipaddr), PF_INET)) {
//      strcpy(name, phostent->h_name);
    } else {
        sprintf(Name, "%d.%d.%d.%d",
            ((nIpAddr >> 24) & 0x000000FF),
            ((nIpAddr >> 16) & 0x000000FF),
            ((nIpAddr >> 8) & 0x000000FF),
            ((nIpAddr) & 0x000000FF));
    }
    return Name;
}

VOID Usage()
{
    _tprintf(_T("\nDisplays current TCP/IP protocol statistics and network connections.\n\n"
    "NETSTAT [-a] [-e] [-n] [-s] [-p proto] [-r] [interval]\n\n"
    "  -a            Displays all connections and listening ports.\n"
    "  -e            Displays Ethernet statistics. May be combined with -s\n"
    "                option\n"
    "  -n            Displays address and port numbers in numeric form.\n"
    "  -p proto      Shows connections for protocol 'proto' TCP or UDP.\n"
    "                If used with the -s option to display\n"
    "                per-protocol statistics, 'proto' may be TCP, UDP, or IP.\n"
    "  -r            Displays the current routing table.\n"
    "  -s            Displays per-protocol statistics. By default, Statistics are\n"
    "                shown for IP, ICMP, TCP and UDP;\n"
    "                the -p option may be used to specify a subset of the default.\n"
    "  interval      Redisplays selected statistics every 'interval' seconds.\n"
    "                Press CTRL+C to stop redisplaying. By default netstat will\n"
    "                print the current information only once.\n"));
}



/*
 *
 * Parse command line parameters and set any options
 * Run display output, looping over set intervals if a number is given
 *
 */
int main(int argc, char *argv[])
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        _tprintf(_T("WSAStartup() failed : %d\n"), WSAGetLastError());
        return -1;
    }

    if (ParseCmdline(argc, argv))
        return -1;

    if (bLoopOutput)
    {
        while (1)
        {
            if (DisplayOutput())
                return -1;
            Sleep(Interval*1000);
        }
    }

    if (DisplayOutput())
        return -1;
    else
        return 0;
}
