/*
 * netstat - display IP stack statistics.
 *
 * This source code is in the PUBLIC DOMAIN and has NO WARRANTY.
 *
 * Robert Dickenson <robd@reactos.org>, August 15, 2002.
 */

// Extensive reference made and use of source to netstatp by:
// Copyright (C) 1998-2002 Mark Russinovich
// www.sysinternals.com


#include <windows.h>
#include <winsock.h>
#include <tchar.h>
#include <stdio.h>
#include <time.h>

#ifdef __GNUC__
#undef WINAPI
#define WINAPI
#endif

#include <iptypes.h>
#include <ipexport.h>
#include <tlhelp32.h>
#include <iphlpapi.h>
#include <snmp.h>

//#include "windows.h"
//#include "stdio.h"
//#include "winsock.h"
//#include "iprtrmib.h"
//#include "tlhelp32.h"
//#include "iphlpapi.h"
//#include "netstatp.h"

#include "trace.h"
#include "resource.h"


#define MAX_RESLEN 4000

//
// Possible TCP endpoint states
//
static char TcpState[][32] = {
	"???",
	"CLOSED",
	"LISTENING",
	"SYN_SENT",
	"SYN_RCVD",
	"ESTABLISHED",
	"FIN_WAIT1",
	"FIN_WAIT2",
	"CLOSE_WAIT",
	"CLOSING",
	"LAST_ACK",
	"TIME_WAIT",
	"DELETE_TCB"
};

VOID PrintError(DWORD ErrorCode)
{
	LPVOID lpMsgBuf;
 
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				  NULL, ErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				  (LPTSTR)&lpMsgBuf, 0, NULL);
	printf("%s\n", lpMsgBuf);
	LocalFree(lpMsgBuf);
}

static void ShowTcpStatistics()
{
    MIB_TCPSTATS TcpStatsMIB;
    GetTcpStatistics(&TcpStatsMIB);

    _tprintf(_T("TCP/IP Statistics\t\n"));
    _tprintf(_T("  time-out algorithm:\t\t%d\n"), TcpStatsMIB.dwRtoAlgorithm);    
    _tprintf(_T("  minimum time-out:\t\t%d\n"), TcpStatsMIB.dwRtoMin);         
    _tprintf(_T("  maximum time-out:\t\t%d\n"), TcpStatsMIB.dwRtoMax);         
    _tprintf(_T("  maximum connections:\t\t%d\n"), TcpStatsMIB.dwMaxConn);        
    _tprintf(_T("  active opens:\t\t\t%d\n"), TcpStatsMIB.dwActiveOpens);    
    _tprintf(_T("  passive opens:\t\t\t%d\n"), TcpStatsMIB.dwPassiveOpens);   
    _tprintf(_T("  failed attempts:\t\t%d\n"), TcpStatsMIB.dwAttemptFails);   
    _tprintf(_T("  established connections reset:\t%d\n"), TcpStatsMIB.dwEstabResets);    
    _tprintf(_T("  established connections:\t%d\n"), TcpStatsMIB.dwCurrEstab);      
    _tprintf(_T("  segments received:\t\t%d\n"), TcpStatsMIB.dwInSegs);         
    _tprintf(_T("  segment sent:\t\t\t%d\n"), TcpStatsMIB.dwOutSegs);        
    _tprintf(_T("  segments retransmitted:\t\t%d\n"), TcpStatsMIB.dwRetransSegs);    
    _tprintf(_T("  incoming errors:\t\t%d\n"), TcpStatsMIB.dwInErrs);         
    _tprintf(_T("  outgoing resets:\t\t%d\n"), TcpStatsMIB.dwOutRsts);        
    _tprintf(_T("  cumulative connections:\t\t%d\n"), TcpStatsMIB.dwNumConns);       
}

static void ShowUdpStatistics()
{
    MIB_UDPSTATS UDPStatsMIB;
    GetUdpStatistics(&UDPStatsMIB);

    _tprintf(_T("UDP Statistics\t\n"));
    _tprintf(_T("  received datagrams:\t\t\t%d\n"), UDPStatsMIB.dwInDatagrams);
    _tprintf(_T("  datagrams for which no port exists:\t%d\n"), UDPStatsMIB.dwNoPorts); 
    _tprintf(_T("  errors on received datagrams:\t\t%d\n"), UDPStatsMIB.dwInErrors);
    _tprintf(_T("  sent datagrams:\t\t\t\t%d\n"), UDPStatsMIB.dwOutDatagrams);
    _tprintf(_T("  number of entries in listener table:\t%d\n"), UDPStatsMIB.dwNumAddrs);
}

static void ShowIpStatistics()
{
    MIB_IPSTATS IPStatsMIB;
    GetIpStatistics(&IPStatsMIB);

    _tprintf(_T("IP Statistics\t\n"));
    _tprintf(_T("  IP forwarding enabled or disabled:\t%d\n"), IPStatsMIB.dwForwarding);      
    _tprintf(_T("  default time-to-live:\t\t\t%d\n"), IPStatsMIB.dwDefaultTTL);      
    _tprintf(_T("  datagrams received:\t\t\t%d\n"), IPStatsMIB.dwInReceives);      
    _tprintf(_T("  received header errors:\t\t\t%d\n"), IPStatsMIB.dwInHdrErrors);     
    _tprintf(_T("  received address errors:\t\t%d\n"), IPStatsMIB.dwInAddrErrors);    
    _tprintf(_T("  datagrams forwarded:\t\t\t%d\n"), IPStatsMIB.dwForwDatagrams);   
    _tprintf(_T("  datagrams with unknown protocol:\t%d\n"), IPStatsMIB.dwInUnknownProtos); 
    _tprintf(_T("  received datagrams discarded:\t\t%d\n"), IPStatsMIB.dwInDiscards);      
    _tprintf(_T("  received datagrams delivered:\t\t%d\n"), IPStatsMIB.dwInDelivers);      
    _tprintf(_T("  sent datagrams discarded:\t\t%d\n"), IPStatsMIB.dwOutDiscards);     
    _tprintf(_T("  datagrams for which no route exists:\t%d\n"), IPStatsMIB.dwOutNoRoutes);     
    _tprintf(_T("  datagrams for which frags didn't arrive:%d\n"), IPStatsMIB.dwReasmTimeout);    
    _tprintf(_T("  datagrams requiring reassembly:\t\t%d\n"), IPStatsMIB.dwReasmReqds);      
    _tprintf(_T("  successful reassemblies:\t\t%d\n"), IPStatsMIB.dwReasmOks);        
    _tprintf(_T("  failed reassemblies:\t\t\t%d\n"), IPStatsMIB.dwReasmFails);      
    _tprintf(_T("  successful fragmentations:\t\t%d\n"), IPStatsMIB.dwFragOks);         
    _tprintf(_T("  failed fragmentations:\t\t\t%d\n"), IPStatsMIB.dwFragFails);       
    _tprintf(_T("  datagrams fragmented:\t\t\t%d\n"), IPStatsMIB.dwFragCreates);     
    _tprintf(_T("  number of interfaces on computer:\t%d\n"), IPStatsMIB.dwNumIf);           
    _tprintf(_T("  number of IP address on computer:\t%d\n"), IPStatsMIB.dwNumAddr);         
    _tprintf(_T("  number of routes in routing table:\t%d\n"), IPStatsMIB.dwNumRoutes);       
}

static void ShowNetworkParams()
{
    FIXED_INFO* FixedInfo;
    IP_ADDR_STRING* pIPAddr;
    ULONG ulOutBufLen;
    DWORD dwRetVal;

    _tprintf(_T("Network Parameters\t\n"));

    FixedInfo = (FIXED_INFO*)GlobalAlloc(GPTR, sizeof(FIXED_INFO));
    ulOutBufLen = sizeof(FIXED_INFO);
    if (ERROR_BUFFER_OVERFLOW == GetNetworkParams(FixedInfo, &ulOutBufLen)) {
        GlobalFree(FixedInfo);
        FixedInfo =(FIXED_INFO*)GlobalAlloc(GPTR, ulOutBufLen);
    }
    if (dwRetVal = GetNetworkParams(FixedInfo, &ulOutBufLen)) {
        _tprintf(_T("Call to GetNetworkParams failed. Return Value: %08x\n"), dwRetVal);
    } else {
        printf("  Host Name: %s", FixedInfo->HostName);
        printf("\n  Domain Name: %s", FixedInfo->DomainName);
        printf("\n  DNS Servers:\t%s\n", FixedInfo->DnsServerList.IpAddress.String);
        pIPAddr = FixedInfo->DnsServerList.Next;
        while (pIPAddr) {
            printf("\t\t\t%s\n", pIPAddr->IpAddress.String);
            pIPAddr = pIPAddr->Next;
        }
    }
}

static void ShowAdapterInfo()
{
    IP_ADAPTER_INFO* pAdaptorInfo;
    ULONG ulOutBufLen;
    DWORD dwRetVal;

    _tprintf(_T("\nAdaptor Information\t\n"));
    pAdaptorInfo = (IP_ADAPTER_INFO*)GlobalAlloc(GPTR, sizeof(IP_ADAPTER_INFO));
    ulOutBufLen = sizeof(IP_ADAPTER_INFO);

    if (ERROR_BUFFER_OVERFLOW == GetAdaptersInfo(pAdaptorInfo, &ulOutBufLen)) {
        GlobalFree(pAdaptorInfo);
        pAdaptorInfo = (IP_ADAPTER_INFO*)GlobalAlloc(GPTR, ulOutBufLen);
    }
    if (dwRetVal = GetAdaptersInfo(pAdaptorInfo, &ulOutBufLen)) {
        _tprintf(_T("Call to GetAdaptersInfo failed. Return Value: %08x\n"), dwRetVal);
    } else {
        while (pAdaptorInfo) {
            printf("  AdapterName: %s\n", pAdaptorInfo->AdapterName);
            printf("  Description: %s\n", pAdaptorInfo->Description);
            pAdaptorInfo = pAdaptorInfo->Next;
        }
    }
}

/*
typedef struct {
    UINT idLength;
    UINT* ids;
} AsnObjectIdentifier;

VOID SnmpUtilPrintAsnAny(AsnAny* pAny);  // pointer to value to print
VOID SnmpUtilPrintOid(AsnObjectIdentifier* Oid);  // object identifier to print

 */
void test_snmp(void)
{
    int nBytes = 500;
    BYTE* pCache;

    pCache = (BYTE*)SnmpUtilMemAlloc(nBytes);
    if (pCache != NULL) {
        AsnObjectIdentifier* pOidSrc = NULL;
        AsnObjectIdentifier AsnObId;
        if (SnmpUtilOidCpy(&AsnObId, pOidSrc)) {
            //
            //
            //
            SnmpUtilOidFree(&AsnObId);
        }
        SnmpUtilMemFree(pCache);
    } else {
        _tprintf(_T("ERROR: call to SnmpUtilMemAlloc() failed\n"));
    }
}

// Maximum string lengths for ASCII ip address and port names
//
#define HOSTNAMELEN		256
#define PORTNAMELEN		256
#define ADDRESSLEN		HOSTNAMELEN+PORTNAMELEN

//
// Our option flags
//
#define FLAG_SHOW_ALL_ENDPOINTS    1
#define FLAG_SHOW_ETH_STATS        2
#define FLAG_SHOW_NUMBERS          3
#define FLAG_SHOW_PROT_CONNECTIONS 4
#define FLAG_SHOW_ROUTE_TABLE      5
#define FLAG_SHOW_PROT_STATS       6
#define FLAG_SHOW_INTERVAL         7


// Undocumented extended information structures available only on XP and higher

typedef struct {
  DWORD dwState;        // state of the connection
  DWORD dwLocalAddr;    // address on local computer
  DWORD dwLocalPort;    // port number on local computer
  DWORD dwRemoteAddr;   // address on remote computer
  DWORD dwRemotePort;   // port number on remote computer
  DWORD	dwProcessId;
} MIB_TCPEXROW, *PMIB_TCPEXROW;

typedef struct {
	DWORD dwNumEntries;
	MIB_TCPEXROW table[ANY_SIZE];
} MIB_TCPEXTABLE, *PMIB_TCPEXTABLE;

typedef struct {
  DWORD   dwLocalAddr;    // address on local computer
  DWORD   dwLocalPort;    // port number on local computer
  DWORD	  dwProcessId;
} MIB_UDPEXROW, *PMIB_UDPEXROW;

typedef struct {
	DWORD dwNumEntries;
	MIB_UDPEXROW table[ANY_SIZE];
} MIB_UDPEXTABLE, *PMIB_UDPEXTABLE;


//
// GetPortName
//
// Translate port numbers into their text equivalent if there is one
//
PCHAR
GetPortName(DWORD Flags, UINT port, PCHAR proto, PCHAR name, int namelen) 
{
	struct servent *psrvent;

	if (Flags & FLAG_SHOW_NUMBERS) {
		sprintf(name, "%d", htons((WORD)port));
		return name;
	} 
	// Try to translate to a name
	if (psrvent = getservbyport(port, proto)) {
		strcpy(name, psrvent->s_name );
	} else {
		sprintf(name, "%d", htons((WORD)port));
	}		
	return name;
}


//
// GetIpHostName
//
// Translate IP addresses into their name-resolved form if possible.
//
PCHAR
GetIpHostName(DWORD Flags, BOOL local, UINT ipaddr, PCHAR name, int namelen) 
{
//	struct hostent *phostent;
	UINT nipaddr;

	// Does the user want raw numbers?
	nipaddr = htonl(ipaddr);
	if (Flags & FLAG_SHOW_NUMBERS ) {
		sprintf(name, "%d.%d.%d.%d", 
			(nipaddr >> 24) & 0xFF,
			(nipaddr >> 16) & 0xFF,
			(nipaddr >> 8) & 0xFF,
			(nipaddr) & 0xFF);
		return name;
	}

   	name[0] = _T('\0');

	// Try to translate to a name
	if (!ipaddr) {
		if (!local) {
			sprintf(name, "%d.%d.%d.%d", 
				(nipaddr >> 24) & 0xFF,
				(nipaddr >> 16) & 0xFF,
				(nipaddr >> 8) & 0xFF,
				(nipaddr) & 0xFF);
		} else {
			//gethostname(name, namelen);
		}
	} else if (ipaddr == 0x0100007f) {
		if (local) {
			//gethostname(name, namelen);
		} else {
			strcpy(name, "localhost");
		}
//	} else if (phostent = gethostbyaddr((char*)&ipaddr, sizeof(nipaddr), PF_INET)) {
//		strcpy(name, phostent->h_name);
	} else {
#if 0
        int i1, i2, i3, i4;

		i1 = (nipaddr >> 24) & 0x000000FF;
		i2 = (nipaddr >> 16) & 0x000000FF;
		i3 = (nipaddr >> 8) & 0x000000FF;
		i4 = (nipaddr) & 0x000000FF;

		i1 = 10;
		i2 = 20;
		i3 = 30;
		i4 = 40;

		sprintf(name, "%d.%d.%d.%d", i1,i2,i3,i4);
#else
		sprintf(name, "%d.%d.%d.%d", 
			((nipaddr >> 24) & 0x000000FF),
			((nipaddr >> 16) & 0x000000FF),
			((nipaddr >> 8) & 0x000000FF),
			((nipaddr) & 0x000000FF));
#endif
	}
	return name;
}

BOOLEAN usage(void)
{
    TCHAR buffer[MAX_RESLEN];

    int length = LoadString(GetModuleHandle(NULL), IDS_APP_USAGE, buffer, sizeof(buffer)/sizeof(buffer[0]));
	_fputts(buffer, stderr);
	return FALSE;
}

//
// GetOptions
// 
// Parses the command line arguments.
//
BOOLEAN 
GetOptions(int argc, char *argv[], PDWORD pFlags)
{
	int		i, j;
	BOOLEAN	skipArgument;

	*pFlags = 0;
	for (i = 1; i < argc; i++) {
		skipArgument = FALSE;
		switch (argv[i][0]) {
		case '-':
		case '/':
			j = 1;
			while (argv[i][j]) {
				switch (toupper(argv[i][j])) {
				case 'A':
					*pFlags |= FLAG_SHOW_ALL_ENDPOINTS;
					break;
				case 'E':
					*pFlags |= FLAG_SHOW_ETH_STATS;
					break;
				case 'N':
					*pFlags |= FLAG_SHOW_NUMBERS;
					break;
				case 'P':
					*pFlags |= FLAG_SHOW_PROT_CONNECTIONS;
					break;
				case 'R':
					*pFlags |= FLAG_SHOW_ROUTE_TABLE;
					break;
				case 'S':
					*pFlags |= FLAG_SHOW_PROT_STATS;
					break;
				default:
					return usage();
				}
				if (skipArgument) break;
				j++;
			}
			break;
		case 'i':
			*pFlags |= FLAG_SHOW_INTERVAL;
			break;
		default:
			return usage();
		}
	}
	return TRUE;
}

#if 1

	CHAR localname[HOSTNAMELEN], remotename[HOSTNAMELEN];
	CHAR remoteport[PORTNAMELEN], localport[PORTNAMELEN];
	CHAR localaddr[ADDRESSLEN], remoteaddr[ADDRESSLEN];

int main(int argc, char *argv[])
{
	PMIB_TCPTABLE tcpTable;
	PMIB_UDPTABLE udpTable;
	DWORD error, dwSize;
	DWORD i, flags;

	// Get options
	if (!GetOptions(argc, argv, &flags)) {
		return -1;
    } else {
		// Get the table of TCP endpoints
		dwSize = 0;
		error = GetTcpTable(NULL, &dwSize, TRUE);
		if (error != ERROR_INSUFFICIENT_BUFFER) {
			printf("Failed to snapshot TCP endpoints.\n");
			PrintError(error);
			return -1;
		}
		tcpTable = (PMIB_TCPTABLE)malloc(dwSize);
		error = GetTcpTable(tcpTable, &dwSize, TRUE );
		if (error) {
			printf("Failed to snapshot TCP endpoints table.\n");
			PrintError(error);
			return -1;
		}

		// Get the table of UDP endpoints
		dwSize = 0;
		error = GetUdpTable(NULL, &dwSize, TRUE);
		if (error != ERROR_INSUFFICIENT_BUFFER) {
			printf("Failed to snapshot UDP endpoints.\n");
			PrintError(error);
			return -1;
		}
		udpTable = (PMIB_UDPTABLE)malloc(dwSize);
		error = GetUdpTable(udpTable, &dwSize, TRUE);
		if (error) {
			printf("Failed to snapshot UDP endpoints table.\n");
			PrintError(error);
			return -1;
		}

		// Dump the TCP table
		for (i = 0; i < tcpTable->dwNumEntries; i++) {
			if (flags & FLAG_SHOW_ALL_ENDPOINTS ||
				tcpTable->table[i].dwState == MIB_TCP_STATE_ESTAB) {
				sprintf(localaddr, "%s:%s", 
					GetIpHostName(flags, TRUE, tcpTable->table[i].dwLocalAddr, localname, HOSTNAMELEN), 
				    GetPortName(flags, tcpTable->table[i].dwLocalPort, "tcp", localport, PORTNAMELEN));
				sprintf(remoteaddr, "%s:%s",
					GetIpHostName(flags, FALSE, tcpTable->table[i].dwRemoteAddr, remotename, HOSTNAMELEN), 
				    tcpTable->table[i].dwRemoteAddr ? 
					GetPortName(flags, tcpTable->table[i].dwRemotePort, "tcp", remoteport, PORTNAMELEN):
					"0");
				printf("%4s\tState:   %s\n", "[TCP]", TcpState[tcpTable->table[i].dwState]);
				printf("       Local:   %s\n       Remote:  %s\n", localaddr, remoteaddr);
			}
		}
		// Dump the UDP table
		if (flags & FLAG_SHOW_ALL_ENDPOINTS) {
			for (i = 0; i < udpTable->dwNumEntries; i++) {
				sprintf(localaddr, "%s:%s", 
					GetIpHostName(flags, TRUE, udpTable->table[i].dwLocalAddr, localname, HOSTNAMELEN), 
					GetPortName(flags, udpTable->table[i].dwLocalPort, "tcp", localport, PORTNAMELEN));
				printf("%4s", "[UDP]");
				printf("       Local:   %s\n       Remote:  %s\n", localaddr, "*.*.*.*:*");
			}
		}
	}	
	printf("\n");
	return 0;
}

#else

int main(int argc, char *argv[])
{
    if (argc > 1) {
        usage();
        return 1;
    }

    _tprintf(_T("\nActive Connections\n\n")\
             _T("  Proto  Local Address          Foreign Address        State\n\n"));
    test_snmp();

    ShowTcpStatistics();
    ShowUdpStatistics();
    ShowIpStatistics();
    ShowNetworkParams();
    ShowAdapterInfo();

	return 0;
}

#endif