/* comwsock.hh -- Private header file for winsock communications driver module
 *
 *  Copyright 1996 by Hilgraeve Inc. -- Monroe, MI
 *  All rights reserved
 *
 *  $Revision: 2 $
 *  $Date: 2/05/99 3:20p $
 */

// -=-=-=-=-=-=-=-=-=-=-=- EXPORTED PROTOTYPES -=-=-=-=-=-=-=-=-=-=-=-=-=-=-

int    WINAPI WsckDeviceInitialize(HCOM hCom, unsigned usInterfaceVersion,
                    void **ppvDriverData);
int    WINAPI WsckDeviceClose(void *pstPrivate);
int    WINAPI WsckDeviceDialog(void *pstPrivate, HWND hwndParent);
int    WINAPI WsckDeviceSpecial(void *pstPrivate,
                    const TCHAR *pszInstructions,
                    TCHAR *pszResult,
                    int   nBufrSize);
int    WINAPI WsckDeviceLoadHdl(void *pstPrivate, SF_HANDLE sfHdl);
int    WINAPI WsckDeviceSaveHdl(void *pstPrivate, SF_HANDLE sfHdl);
int    WINAPI WsckDeviceStub(void *pstPrivate);
int    WINAPI WsckPortActivate(void *pstPrivate,
                    TCHAR *pszPortName,
                    DWORD_PTR dwMediaHdl);
int    WINAPI WsckPortDeactivate(void *pstPrivate);
int    WINAPI WsckPortConnected(void *pstPrivate);


int    WINAPI WsckRcvRefill(void *pstPrivate);
int    WINAPI WsckRcvClear(void *pstPrivate);

int    WINAPI WsckSndBufrSend(void *pstPrivate, void *pvBufr,
                      int nSize);
int    WINAPI WsckSndBufrIsBusy(void *pstPrivate);
int    WINAPI WsckSndBufrQuery(void *pstPrivate, unsigned *pafStatus,
                      long *plHandshakeDelay);
int    WINAPI WsckSndBufrClear(void *pstPrivate);
int    WINAPI WsckPortConfigure(void *pstPrivate);
DWORD  WINAPI WsckComConnectThread(void *pvData);
DWORD  WINAPI WsckComReadThread(void *pvData);
DWORD  WINAPI WsckComWriteThread(void *pvData);


#if defined(INCL_WINSOCK)
/*
 * This block of defines is used by the Telnet / NVT code,
 * and is taken from RFC1143
 */
#define NO              1
#define YES             2
#define WANTNO          3
#define WANTYES         4

#define EMPTY           1
#define OPPOSITE        2
#define NONE            3

#define NVT_THRU        0
#define NVT_IAC         1
#define NVT_WILL        2
#define NVT_WONT        3
#define NVT_DO          4
#define NVT_DONT        5

#define NVT_SB          6
#define NVT_SB_TT       7
#define NVT_SB_TT_S     8
#define NVT_SB_TT_S_I   9

#define ECHO_MODE   0
#define SGA_MODE    1
#define TTYPE_MODE  2
#define BINARY_MODE 3
#define NAWS_MODE   4
// If any new modes are added, be sure to update MODE_MAX in comstd.

#define NVT_DISCARD 1
#define NVT_KEEP    2

/*
 * Definitions for the TELNET protocol.
 */
#define IAC     255     /* interpret as command: */
#define DONT    254     /* you are not to use option */
#define DO      253     /* please, you use option */
#define WONT    252     /* I won't use option */
#define WILL    251     /* I will use option */
#define SB      250     /* interpret as subnegotiation */
#define GA      249     /* you may reverse the line */
#define EL      248     /* erase the current line */
#define EC      247     /* erase the current character */
#define AYT     246     /* are you there */
#define AO      245     /* abort output--but let prog finish */
#define IP      244     /* interrupt process--permanently */
#define BREAK   243     /* break */
#define DM      242     /* data mark--for connect. cleaning */
#define NOP     241     /* nop */
#define SE      240     /* end sub negotiation */
#define EOR     239     /* end of record (transparent mode) */

#define SYNCH   242     /* for telfunc calls */

/* telnet options */
#define TELOPT_BINARY   0   /* 8-bit data path */
#define TELOPT_ECHO     1   /* echo */
#define TELOPT_RCP      2   /* prepare to reconnect */
#define TELOPT_SGA      3   /* suppress go ahead */
#define TELOPT_NAMS     4   /* approximate message size */
#define TELOPT_STATUS   5   /* give status */
#define TELOPT_TM       6   /* timing mark */
#define TELOPT_RCTE     7   /* remote controlled transmission and echo */
#define TELOPT_NAOL     8   /* negotiate about output line width */
#define TELOPT_NAOP     9   /* negotiate about output page size */
#define TELOPT_NAOCRD   10  /* negotiate about CR disposition */
#define TELOPT_NAOHTS   11  /* negotiate about horizontal tabstops */
#define TELOPT_NAOHTD   12  /* negotiate about horizontal tab disposition */
#define TELOPT_NAOFFD   13  /* negotiate about formfeed disposition */
#define TELOPT_NAOVTS   14  /* negotiate about vertical tab stops */
#define TELOPT_NAOVTD   15  /* negotiate about vertical tab disposition */
#define TELOPT_NAOLFD   16  /* negotiate about output LF disposition */
#define TELOPT_XASCII   17  /* extended ascic character set */
#define TELOPT_LOGOUT   18  /* force logout */
#define TELOPT_BM       19  /* byte macro */
#define TELOPT_DET      20  /* data entry terminal */
#define TELOPT_SUPDUP   21  /* supdup protocol */
#define TELOPT_SUPDUPOUTPUT 22  /* supdup output */
#define TELOPT_SNDLOC   23  /* send location */
#define TELOPT_TTYPE    24  /* terminal type */
#define TELOPT_EOR      25  /* end or record */
#define TELOPT_TACACS   26  /* TACACS user identification */
#define TELOPT_OUTMARK  27  /* output marking */
#define TELOPT_TERMLOC  28  /* terminal location number */
#define TELOPT_X3_PAD   30
#define TELOPT_NAWS     31  /* negotiate about terminal size */
#define TELOPT_SPEED    32  /* negotiate terminal speed */
#define TELOPT_TOGGLEFLOW 33    /* toggle flow control */
#define TELOPT_XDISPLOC 35  /* X display location */
#define TELOPT_EXOPL    255 /* extended-options-list */

/* sub-option qualifiers */
#define TELQUAL_IS      0   /* option is... */
#define TELQUAL_SEND    1   /* send option */
#define TELQUAL_OFF     0   /* turn off option */
#define TELQUAL_ON      1   /* turn on option */


/* --- These implement the Telnet NVT(network virtual terminal) --- */

VOID WinSockCreateNVT(ST_STDCOM * pstWS);
VOID WinSockReleaseNVT(ST_STDCOM * pstWS);
int FAR PASCAL WinSockNetworkVirtualTerminal(ECHAR mc, void *pD);

VOID WinSockSendMessage(ST_STDCOM * pstWS, INT nMsg, INT nChar);
VOID WinSockSendBuffer(ST_STDCOM * pstWS, INT nSize, LPSTR pszBuffer);

VOID WinSockGotDO  (ST_STDCOM * pstWS, const PSTOPT pstO);
VOID WinSockGotWILL(ST_STDCOM * pstWS, const PSTOPT pstO);
VOID WinSockGotDONT(ST_STDCOM * pstWS, const PSTOPT pstO);
VOID WinSockGotWONT(ST_STDCOM * pstWS, const PSTOPT pstO);

VOID WinSockSendNAWS( ST_STDCOM * hhDriver );
#endif
