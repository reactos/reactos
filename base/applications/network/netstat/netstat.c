/*
 * PROJECT:     ReactOS netstat utility
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/network/netstat/netstat.c
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

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#define _INC_WINDOWS
#include <winsock2.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <iphlpapi.h>

#include <conutils.h>

#include "netstat.h"
#include "resource.h"

enum ProtoType {IP, TCP, UDP, ICMP} Protocol;
DWORD Interval; /* time to pause between printing output */

/* TCP endpoint states */
PCWSTR TcpState[] = {
    L"???",
    L"CLOSED",
    L"LISTENING",
    L"SYN_SENT",
    L"SYN_RCVD",
    L"ESTABLISHED",
    L"FIN_WAIT1",
    L"FIN_WAIT2",
    L"CLOSE_WAIT",
    L"CLOSING",
    L"LAST_ACK",
    L"TIME_WAIT",
    L"DELETE_TCB"
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
            (LPWSTR) &lpMsgBuf,
            0,
            NULL )))
    {
        wprintf(L"%s", (LPWSTR)lpMsgBuf);

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
BOOL ParseCmdline(int argc, wchar_t* argv[])
{
    LPWSTR Proto;
    WCHAR c;
    INT i;

    if ((argc == 1) || (iswdigit(*argv[1])))
        bNoOptions = TRUE;

    /* Parse command line for options we have been given. */
    for (i = 1; i < argc; i++)
    {
        if ((argc > 1) && (argv[i][0] == L'-' || argv[i][0] == L'/'))
        {
            while ((c = *++argv[i]) != L'\0')
            {
                switch (towlower(c))
                {
                    case L'a':
                        bDoShowAllCons = TRUE;
                        break;
                    case L'b':
                        bDoShowProcName = TRUE;
                        break;
                    case L'e':
                        bDoShowEthStats = TRUE;
                        break;
                    case L'n':
                        bDoShowNumbers = TRUE;
                        break;
                    case L'p':
                        bDoShowProtoCons = TRUE;
                        Proto = argv[i+1];
                        if (!_wcsicmp(L"IP", Proto))
                            Protocol = IP;
                        else if (!_wcsicmp(L"ICMP", Proto))
                            Protocol = ICMP;
                        else if (!_wcsicmp(L"TCP", Proto))
                            Protocol = TCP;
                        else if (!_wcsicmp(L"UDP", Proto))
                            Protocol = UDP;
                        else
                        {
                            ConResPuts(StdOut, IDS_USAGE);
                            return EXIT_FAILURE;
                        }
                        break;
                    case L'r':
                        bDoShowRouteTable = TRUE;
                        break;
                    case L's':
                        bDoShowProtoStats = TRUE;
                        break;
                    case L'o':
                        bDoShowProcessId = TRUE;
                        break;
                    case L'v':
                        // FIXME!
                        ConPuts(StdOut, L"got v\n");
                        bDoDispSeqComp = TRUE;
                        break;
                    default :
                        ConResPuts(StdOut, IDS_USAGE);
                        return EXIT_FAILURE;
                }
            }
        }
        else if (iswdigit(*argv[i]) != 0)
        {
            if (swscanf(argv[i], L"%lu", &Interval) != EOF)
                bLoopOutput = TRUE;
            else
                return EXIT_FAILURE;
        }
//        else
//        {
//            ConResPrintf(StdOut, IDS_USAGE);
//            return EXIT_FAILURE;
//        }
    }

    return EXIT_SUCCESS;
}

/*
 * Display table header
 */
VOID DisplayTableHeader()
{
    ConResPuts(StdOut, IDS_DISPLAY_THEADER);
    if (bDoShowProcessId)
        ConResPuts(StdOut, IDS_DISPLAY_PROCESS);
    else
        ConPuts(StdOut, L"\n");
}


/*
 * Simulate Microsofts netstat utility output
 */
BOOL DisplayOutput()
{
    if (bNoOptions)
    {
        DisplayTableHeader();
        ShowTcpTable();
        return EXIT_SUCCESS;
    }

    if (bDoShowRouteTable)
    {
        /* mingw doesn't have lib for _tsystem */
        if (system("route print") == -1)
        {
            ConResPuts(StdErr, IDS_ERROR_ROUTE);
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
                case IP:
                    if (bDoShowProtoStats)
                    {
                        ShowIpStatistics();
                        return EXIT_SUCCESS;
                    }
                    break;
                case ICMP:
                    if (bDoShowProtoStats)
                    {
                        ShowIcmpStatistics();
                        return EXIT_SUCCESS;
                    }
                    break;
                case TCP:
                    if (bDoShowProtoStats)
                        ShowTcpStatistics();
                    ConResPuts(StdOut, IDS_ACTIVE_CONNECT);
                    DisplayTableHeader();
                    ShowTcpTable();
                    break;
                case UDP:
                    if (bDoShowProtoStats)
                        ShowUdpStatistics();
                    ConResPuts(StdOut, IDS_ACTIVE_CONNECT);
                    DisplayTableHeader();
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
    else
    {
        ConResPuts(StdOut, IDS_ACTIVE_CONNECT);
        DisplayTableHeader();
        ShowTcpTable();
        if (bDoShowAllCons)
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
        ConResPuts(StdOut, IDS_IP4_STAT_HEADER);
        ConResPrintf(StdOut, IDS_IP_PACK_REC, pIpStats->dwInReceives);
        ConResPrintf(StdOut, IDS_IP_HEAD_REC_ERROR, pIpStats->dwInHdrErrors);
        ConResPrintf(StdOut, IDS_IP_ADDR_REC_ERROR, pIpStats->dwInAddrErrors);
        ConResPrintf(StdOut, IDS_IP_DATAG_FWD, pIpStats->dwForwDatagrams);
        ConResPrintf(StdOut, IDS_IP_UNKNOWN_PRO_REC, pIpStats->dwInUnknownProtos);
        ConResPrintf(StdOut, IDS_IP_REC_PACK_DISCARD, pIpStats->dwInDiscards);
        ConResPrintf(StdOut, IDS_IP_REC_PACK_DELIVER, pIpStats->dwInDelivers);
        ConResPrintf(StdOut, IDS_IP_OUT_REQUEST, pIpStats->dwOutRequests);
        ConResPrintf(StdOut, IDS_IP_ROUTE_DISCARD, pIpStats->dwRoutingDiscards);
        ConResPrintf(StdOut, IDS_IP_DISCARD_OUT_PACK, pIpStats->dwOutDiscards);
        ConResPrintf(StdOut, IDS_IP_OUT_PACKET_NO_ROUTE, pIpStats->dwOutNoRoutes);
        ConResPrintf(StdOut, IDS_IP_REASSEMBLE_REQUIRED, pIpStats->dwReasmReqds);
        ConResPrintf(StdOut, IDS_IP_REASSEMBLE_SUCCESS, pIpStats->dwReasmOks);
        ConResPrintf(StdOut, IDS_IP_REASSEMBLE_FAILURE, pIpStats->dwReasmFails);
        ConResPrintf(StdOut, IDS_IP_DATAG_FRAG_SUCCESS, pIpStats->dwFragOks);
        ConResPrintf(StdOut, IDS_IP_DATAG_FRAG_FAILURE, pIpStats->dwFragFails);
        ConResPrintf(StdOut, IDS_IP_DATAG_FRAG_CREATE, pIpStats->dwFragCreates);
    }
    else
    {
        DoFormatMessage(dwRetVal);
    }

    HeapFree(GetProcessHeap(), 0, pIpStats);
}

VOID ShowIcmpStatistics()
{
    PMIB_ICMP pIcmpStats;
    DWORD dwRetVal;

    pIcmpStats = (MIB_ICMP*) HeapAlloc(GetProcessHeap(), 0, sizeof(MIB_ICMP));

    if ((dwRetVal = GetIcmpStatistics(pIcmpStats)) == NO_ERROR)
    {
        ConResPuts(StdOut, IDS_ICMP4_STAT_HEADER);
        ConResPuts(StdOut, IDS_ICMP_THEADER);
        ConResPrintf(StdOut, IDS_ICMP_MSG,
            pIcmpStats->stats.icmpInStats.dwMsgs, pIcmpStats->stats.icmpOutStats.dwMsgs);
        ConResPrintf(StdOut, IDS_ICMP_ERROR,
            pIcmpStats->stats.icmpInStats.dwErrors, pIcmpStats->stats.icmpOutStats.dwErrors);
        ConResPrintf(StdOut, IDS_ICMP_DEST_UNREACH,
            pIcmpStats->stats.icmpInStats.dwDestUnreachs, pIcmpStats->stats.icmpOutStats.dwDestUnreachs);
        ConResPrintf(StdOut, IDS_ICMP_TIME_EXCEED,
            pIcmpStats->stats.icmpInStats.dwTimeExcds, pIcmpStats->stats.icmpOutStats.dwTimeExcds);
        ConResPrintf(StdOut, IDS_ICMP_PARAM_PROBLEM,
            pIcmpStats->stats.icmpInStats.dwParmProbs, pIcmpStats->stats.icmpOutStats.dwParmProbs);
        ConResPrintf(StdOut, IDS_ICMP_SRC_QUENCHES,
            pIcmpStats->stats.icmpInStats.dwSrcQuenchs, pIcmpStats->stats.icmpOutStats.dwSrcQuenchs);
        ConResPrintf(StdOut, IDS_ICMP_REDIRECT,
            pIcmpStats->stats.icmpInStats.dwRedirects, pIcmpStats->stats.icmpOutStats.dwRedirects);
        ConResPrintf(StdOut, IDS_ICMP_ECHO,
            pIcmpStats->stats.icmpInStats.dwEchos, pIcmpStats->stats.icmpOutStats.dwEchos);
        ConResPrintf(StdOut, IDS_ICMP_ECHO_REPLY,
            pIcmpStats->stats.icmpInStats.dwEchoReps, pIcmpStats->stats.icmpOutStats.dwEchoReps);
        ConResPrintf(StdOut, IDS_ICMP_TIMESTAMP,
            pIcmpStats->stats.icmpInStats.dwTimestamps, pIcmpStats->stats.icmpOutStats.dwTimestamps);
        ConResPrintf(StdOut, IDS_ICMP_TIMESTAMP_REPLY,
            pIcmpStats->stats.icmpInStats.dwTimestampReps, pIcmpStats->stats.icmpOutStats.dwTimestampReps);
        ConResPrintf(StdOut, IDS_ICMP_ADDRESSS_MASK,
            pIcmpStats->stats.icmpInStats.dwAddrMasks, pIcmpStats->stats.icmpOutStats.dwAddrMasks);
        ConResPrintf(StdOut, IDS_ICMP_ADDRESSS_MASK_REPLY,
            pIcmpStats->stats.icmpInStats.dwAddrMaskReps, pIcmpStats->stats.icmpOutStats.dwAddrMaskReps);
    }
    else
    {
        DoFormatMessage(dwRetVal);
    }

    HeapFree(GetProcessHeap(), 0, pIcmpStats);

}

VOID ShowTcpStatistics()
{
    PMIB_TCPSTATS pTcpStats;
    DWORD dwRetVal;

    pTcpStats = (MIB_TCPSTATS*) HeapAlloc(GetProcessHeap(), 0, sizeof(MIB_TCPSTATS));

    if ((dwRetVal = GetTcpStatistics(pTcpStats)) == NO_ERROR)
    {
        ConResPuts(StdOut, IDS_TCP4_HEADER);
        ConResPrintf(StdOut, IDS_TCP_ACTIVE_OPEN, pTcpStats->dwActiveOpens);
        ConResPrintf(StdOut, IDS_TCP_PASS_OPEN, pTcpStats->dwPassiveOpens);
        ConResPrintf(StdOut, IDS_TCP_FAIL_CONNECT, pTcpStats->dwAttemptFails);
        ConResPrintf(StdOut, IDS_TCP_RESET_CONNECT, pTcpStats->dwEstabResets);
        ConResPrintf(StdOut, IDS_TCP_CURRENT_CONNECT, pTcpStats->dwCurrEstab);
        ConResPrintf(StdOut, IDS_TCP_SEG_RECEIVE, pTcpStats->dwInSegs);
        ConResPrintf(StdOut, IDS_TCP_SEG_SENT, pTcpStats->dwOutSegs);
        ConResPrintf(StdOut, IDS_TCP_SEG_RETRANSMIT, pTcpStats->dwRetransSegs);
    }
    else
    {
        DoFormatMessage(dwRetVal);
    }

    HeapFree(GetProcessHeap(), 0, pTcpStats);
}

VOID ShowUdpStatistics()
{
    PMIB_UDPSTATS pUdpStats;
    DWORD dwRetVal;

    pUdpStats = (MIB_UDPSTATS*) HeapAlloc(GetProcessHeap(), 0, sizeof(MIB_UDPSTATS));

    if ((dwRetVal = GetUdpStatistics(pUdpStats)) == NO_ERROR)
    {
        ConResPuts(StdOut, IDS_UDP_IP4_HEADER);
        ConResPrintf(StdOut, IDS_UDP_DATAG_RECEIVE, pUdpStats->dwInDatagrams);
        ConResPrintf(StdOut, IDS_UDP_NO_PORT, pUdpStats->dwNoPorts);
        ConResPrintf(StdOut, IDS_UDP_RECEIVE_ERROR, pUdpStats->dwInErrors);
        ConResPrintf(StdOut, IDS_UDP_DATAG_SEND, pUdpStats->dwOutDatagrams);
    }
    else
    {
        DoFormatMessage(dwRetVal);
    }

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
            ConResPuts(StdOut, IDS_ETHERNET_INTERFACE_STAT);
            ConResPuts(StdOut, IDS_ETHERNET_THEADER);
            ConResPrintf(StdOut, IDS_ETHERNET_BYTES,
                pIfTable->table[0].dwInOctets, pIfTable->table[0].dwOutOctets);
            ConResPrintf(StdOut, IDS_ETHERNET_UNICAST_PACKET,
                pIfTable->table[0].dwInUcastPkts, pIfTable->table[0].dwOutUcastPkts);
            ConResPrintf(StdOut, IDS_ETHERNET_NON_UNICAST_PACKET,
                pIfTable->table[0].dwInNUcastPkts, pIfTable->table[0].dwOutNUcastPkts);
            ConResPrintf(StdOut, IDS_ETHERNET_DISCARD,
                pIfTable->table[0].dwInDiscards, pIfTable->table[0].dwOutDiscards);
            ConResPrintf(StdOut, IDS_ETHERNET_ERROR,
                pIfTable->table[0].dwInErrors, pIfTable->table[0].dwOutErrors);
            ConResPrintf(StdOut, IDS_ETHERNET_UNKNOWN,
                pIfTable->table[0].dwInUnknownProtos);
        }
        else
        {
            DoFormatMessage(dwRetVal);
        }
    }
    HeapFree(GetProcessHeap(), 0, pIfTable);
}

VOID ShowTcpTable()
{
    PMIB_TCPTABLE_OWNER_PID tcpTable;
    DWORD error, dwSize;
    DWORD i;
    CHAR HostIp[HOSTNAMELEN], HostPort[PORTNAMELEN];
    CHAR RemoteIp[HOSTNAMELEN], RemotePort[PORTNAMELEN];
    CHAR Host[ADDRESSLEN];
    CHAR Remote[ADDRESSLEN];
    CHAR PID[64];

    /* Get the table of TCP endpoints */
    dwSize = sizeof (MIB_TCPTABLE_OWNER_PID);
    /* Should also work when we get new connections between 2 GetTcpTable()
     * calls: */
    do
    {
        tcpTable = (PMIB_TCPTABLE_OWNER_PID) HeapAlloc(GetProcessHeap(), 0, dwSize);
        error = GetExtendedTcpTable(tcpTable, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
        if ( error != NO_ERROR )
            HeapFree(GetProcessHeap(), 0, tcpTable);
    }
    while  ( error == ERROR_INSUFFICIENT_BUFFER );

    if (error != NO_ERROR)
    {
        ConResPrintf(StdErr, IDS_ERROR_TCP_SNAPSHOT);
        DoFormatMessage(error);
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
            sprintf(Host, "%s:%s", HostIp, HostPort);

            if (tcpTable->table[i].dwState ==  MIB_TCP_STATE_LISTEN)
            {
                sprintf(Remote, "%s:0", HostIp);
            }
            else
            {
                GetIpHostName(FALSE, tcpTable->table[i].dwRemoteAddr, RemoteIp, HOSTNAMELEN);
                GetPortName(tcpTable->table[i].dwRemotePort, "tcp", RemotePort, PORTNAMELEN);
                sprintf(Remote, "%s:%s", RemoteIp, RemotePort);
            }

            if (bDoShowProcessId)
            {
                sprintf(PID, "%ld", tcpTable->table[i].dwOwningPid);
            }
            else
            {
                PID[0] = 0;
            }

            ConPuts(StdOut, L"  %-6s %-22s %-22s %-11s %s\n", L"TCP",
                    Host, Remote, TcpState[tcpTable->table[i].dwState], PID);
        }
    }
    HeapFree(GetProcessHeap(), 0, tcpTable);
}

VOID ShowUdpTable()
{
    PMIB_UDPTABLE_OWNER_PID udpTable;
    DWORD error, dwSize;
    DWORD i;
    CHAR HostIp[HOSTNAMELEN], HostPort[PORTNAMELEN];
    CHAR Host[ADDRESSLEN];
    CHAR PID[64];

    /* Get the table of UDP endpoints */
    dwSize = 0;
    error = GetExtendedUdpTable(NULL, &dwSize, TRUE, AF_INET, UDP_TABLE_OWNER_PID, 0);
    if (error != ERROR_INSUFFICIENT_BUFFER)
    {
        ConResPuts(StdErr, IDS_ERROR_UDP_ENDPOINT);
        DoFormatMessage(error);
        exit(EXIT_FAILURE);
    }
    udpTable = (PMIB_UDPTABLE_OWNER_PID) HeapAlloc(GetProcessHeap(), 0, dwSize);
    error = GetExtendedUdpTable(udpTable, &dwSize, TRUE, AF_INET, UDP_TABLE_OWNER_PID, 0);
    if (error)
    {
        ConResPuts(StdErr, IDS_ERROR_UDP_ENDPOINT_TABLE);
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

        if (bDoShowProcessId)
        {
            sprintf(PID, "%ld", udpTable->table[i].dwOwningPid);
        }
        else
        {
            PID[0] = 0;
        }

        ConPuts(StdOut, L"  %-6s %-22s %-34s %s\n", L"UDP", Host, L"*:*", PID);
    }

    HeapFree(GetProcessHeap(), 0, udpTable);
}

/*
 * Translate port numbers into their text equivalent if there is one
 */
PCHAR
GetPortName(UINT Port, PCSTR Proto, CHAR Name[], INT NameLen)
{
    struct servent *pServent;

    if (bDoShowNumbers)
    {
        sprintf(Name, "%d", htons((WORD)Port));
        return Name;
    }
    /* Try to translate to a name */
    if ((pServent = getservbyport(Port, Proto)))
        strcpy(Name, pServent->s_name );
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

    Name[0] = '\0';

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
            strncpy(Name, "localhost", 10);
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

/*
 *
 * Parse command line parameters and set any options
 * Run display output, looping over set intervals if a number is given
 *
 */
int wmain(int argc, wchar_t *argv[])
{
    WSADATA wsaData;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        ConResPrintf(StdErr, IDS_ERROR_WSA_START, WSAGetLastError());
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
