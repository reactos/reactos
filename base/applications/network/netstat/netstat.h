
/* Maximum string lengths for ASCII ip address and port names */
#define HOSTNAMELEN     256
#define PORTNAMELEN     256
#define ADDRESSLEN      HOSTNAMELEN+PORTNAMELEN

/* command line options */
BOOL bNoOptions        = FALSE; // print default
BOOL bDoShowAllCons    = FALSE; // -a
BOOL bDoShowProcName   = FALSE; // -b
BOOL bDoShowEthStats   = FALSE; // -e
BOOL bDoShowNumbers    = FALSE; // -n
BOOL bDoShowProtoCons  = FALSE; // -p
BOOL bDoShowRouteTable = FALSE; // -r
BOOL bDoShowProtoStats = FALSE; // -s
BOOL bDoDispSeqComp    = FALSE; // -v
BOOL bLoopOutput       = FALSE; // interval


/* Undocumented extended information structures available only on XP and higher */
typedef struct {
  DWORD dwState;        // state of the connection
  DWORD dwLocalAddr;    // address on local computer
  DWORD dwLocalPort;    // port number on local computer
  DWORD dwRemoteAddr;   // address on remote computer
  DWORD dwRemotePort;   // port number on remote computer
  DWORD dwProcessId;
} MIB_TCPEXROW, *PMIB_TCPEXROW;

typedef struct {
    DWORD dwNumEntries;
    MIB_TCPEXROW table;
} MIB_TCPEXTABLE, *PMIB_TCPEXTABLE;

typedef struct {
  DWORD   dwLocalAddr;    // address on local computer
  DWORD   dwLocalPort;    // port number on local computer
  DWORD   dwProcessId;
} MIB_UDPEXROW, *PMIB_UDPEXROW;

typedef struct {
    DWORD dwNumEntries;
    MIB_UDPEXROW table;
} MIB_UDPEXTABLE, *PMIB_UDPEXTABLE;


/* function declerations */
BOOL ParseCmdline(int argc, char* argv[]);
BOOL DisplayOutput(VOID);
DWORD DoFormatMessage(DWORD ErrorCode);
VOID ShowIpStatistics(VOID);
VOID ShowIcmpStatistics(VOID);
VOID ShowTcpStatistics(VOID);
VOID ShowUdpStatistics(VOID);
VOID ShowEthernetStatistics(VOID);
VOID ShowTcpTable(VOID);
VOID ShowUdpTable(VOID);
PCHAR GetPortName(UINT Port, PCSTR Proto, CHAR Name[PORTNAMELEN], INT NameLen);
PCHAR GetIpHostName(BOOL local, UINT ipaddr, CHAR name[HOSTNAMELEN], int namelen);
VOID Usage(VOID);

