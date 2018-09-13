/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    servcomm.c

Abstract:

    Provides socket commmunications functionality for Proxy Agent.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <windows.h>

#include <winsvc.h>
#include <winsock2.h>

#include <wsipx.h>

#include <errno.h>
#include <stdio.h>
#include <process.h>
#include <string.h>


//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <snmputil.h>

#include "regconf.h"
#include "..\common\wellknow.h"
#include "..\common\evtlog.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

DWORD  timeZeroReference;

extern SERVICE_STATUS_HANDLE hService;
extern SERVICE_STATUS status;
extern BOOL noservice;
extern HANDLE g_hTerminationEvent;
extern HINSTANCE g_hInstance;
extern HWND hWnd;

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define bzero(lp, size)         (void)memset(lp, 0, size)
#define bcopy(slp, dlp, size)   (void)memcpy(dlp, slp, size)
#define bcmp(slp, dlp, size)    memcmp(dlp, slp, size)

#define CTAMTimeout ((DWORD)30000)
#define MAX_IPX_ADDR_LEN 64

//--------------------------- PRIVATE STRUCTS -------------------------------

typedef struct {
    int family;
    int type;
    int protocol;
    struct sockaddr localAddress;
} Session;


//--------------------------- PRIVATE VARIABLES -----------------------------

#define RECVBUFSIZE 4096

#define NPOLLFILE 2     // UDP and IPX

static SOCKET fdarray[NPOLLFILE];
static INT      fdarrayLen;

CRITICAL_SECTION g_commThreadCritSec;

#define SNMP_AGENT_WND_CLASS    "Microsoft SNMP Agent"
#define SNMP_AGENT_WND_MESSAGE  (WM_USER+100)

static HWND g_hwndAgent;
static BYTE * g_recvBuf = NULL;

//--------------------------- PRIVATE PROTOTYPES ----------------------------

VOID trapThread(VOID *threadParam);

VOID agentCommThread(VOID *threadParam);

BOOL filtmgrs(struct sockaddr *source, INT sourceLen);

SNMPAPI SnmpServiceProcessMessage(
    IN OUT BYTE **pBuf,
    IN OUT UINT *length);

//--------------------------- PRIVATE PROCEDURES ----------------------------

LPSTR
AddrToString(
    struct sockaddr * pSockAddr
    )
{
    static CHAR ipxAddr[MAX_IPX_ADDR_LEN];

    // determine family
    if (pSockAddr->sa_family == AF_INET) {

        struct sockaddr_in * pSockAddrIn;

        // obtain pointer to protocol specific structure
        pSockAddrIn = (struct sockaddr_in * )pSockAddr;

        // forward to winsock conversion function
        return inet_ntoa(pSockAddrIn->sin_addr);

    } else if (pSockAddr->sa_family == AF_IPX) {

        struct sockaddr_ipx * pSockAddrIpx;

        // obtain pointer to protocol specific structure
        pSockAddrIpx = (struct sockaddr_ipx * )pSockAddr;

        // transfer ipx address to static buffer
        sprintf(ipxAddr, 
            "%02x%02x%02x%02x.%02x%02x%02x%02x%02x%02x",
            (BYTE)pSockAddrIpx->sa_netnum[0],
            (BYTE)pSockAddrIpx->sa_netnum[1],
            (BYTE)pSockAddrIpx->sa_netnum[2],
            (BYTE)pSockAddrIpx->sa_netnum[3],
            (BYTE)pSockAddrIpx->sa_nodenum[0],
            (BYTE)pSockAddrIpx->sa_nodenum[1],
            (BYTE)pSockAddrIpx->sa_nodenum[2],
            (BYTE)pSockAddrIpx->sa_nodenum[3],
            (BYTE)pSockAddrIpx->sa_nodenum[4],
            (BYTE)pSockAddrIpx->sa_nodenum[5]
            );

        // return addr
        return ipxAddr;
    }

    // failure
    return NULL;
}

LRESULT
CALLBACK
AgentWndProc(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    struct sockaddr source;
    int             sourceLen;
    int             length;
    struct sockaddr_in *saddr;
    SOCKET          commSocket = (SOCKET)wParam;
    BYTE *          sendBuf;

    // see if agent message specified
    if (uMsg == SNMP_AGENT_WND_MESSAGE) {

        // see if read event is signalled
        if ((WSAGETSELECTERROR(lParam) == NOERROR) &&
            (WSAGETSELECTEVENT(lParam) & FD_READ)) {

            EnterCriticalSection(&g_commThreadCritSec);

            sourceLen = sizeof(source);
            if ((length = recvfrom(commSocket, g_recvBuf, RECVBUFSIZE,
                0, &source, &sourceLen)) == -1)
                {
                SNMPDBG((SNMP_LOG_ERROR, "SNMP: PDU: error on recvfrom %d.\n",GetLastError()));

                LeaveCriticalSection(&g_commThreadCritSec);
                return (1);
                }

            if (length == RECVBUFSIZE)
                {
                SNMPDBG((SNMP_LOG_TRACE,
                          "SNMP: PDU: recvfrom exceeded %d octets.\n", RECVBUFSIZE));

                LeaveCriticalSection(&g_commThreadCritSec);
                return (1);
                }

            // verify permittedManagers
            if (!filtmgrs(&source, sourceLen))
                {
                LeaveCriticalSection(&g_commThreadCritSec);
                return (1);
                }

            sendBuf = g_recvBuf;
            saddr = (struct sockaddr_in *)&source;
            SNMPDBG((SNMP_LOG_TRACE, "SNMP: PDU: request received from %s (%d octets).\n",
                AddrToString(&source), length));

            if (!SnmpServiceProcessMessage(&sendBuf, &length))
                {
                SNMPDBG((SNMP_LOG_TRACE, "SNMP: PDU: error on SnmpServiceProcessMessage %d\n",
                          GetLastError()));

                LeaveCriticalSection(&g_commThreadCritSec);
                return (1);
                }

            if ((length = sendto(commSocket, sendBuf, length,
                                 0, &source, sizeof(source))) == -1)
                {
                SNMPDBG((SNMP_LOG_ERROR, "SNMP: PDU: error %d sending response pdu.\n",
                          GetLastError()));

                SnmpUtilMemFree(sendBuf);

                LeaveCriticalSection(&g_commThreadCritSec);
                return (1);
                }

            // receive send buffer
            if (sendBuf != g_recvBuf) {
                SnmpUtilMemFree(sendBuf);
            }

            SNMPDBG((SNMP_LOG_TRACE, "SNMP: PDU: response sent to %s (%d octets).\n",
                AddrToString(&source), length));

            LeaveCriticalSection(&g_commThreadCritSec);
        }

        return (0);

    } else if (uMsg == WM_DESTROY) {

        PostQuitMessage(0);
        return(0);
    }

    return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

BOOL
RegisterAgentWndClass(
    )
{
    BOOL fOk;
    WNDCLASS wc;

    // initialize notification window class
    wc.lpfnWndProc   = (WNDPROC)AgentWndProc;
    wc.lpszClassName = SNMP_AGENT_WND_CLASS;
    wc.lpszMenuName  = NULL;
    wc.hInstance     = g_hInstance;
    wc.hIcon         = NULL;
    wc.hCursor       = NULL;
    wc.hbrBackground = NULL;
    wc.cbWndExtra    = 0;
    wc.cbClsExtra    = 0;
    wc.style         = 0;

    // register class
    fOk = RegisterClass(&wc);

    if (!fOk) {

        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: INIT: RegisterClass returned %d.\n",
            GetLastError()
            ));
    }

    return fOk;
}

BOOL
UnregisterAgentWndClass(
    )
{
    BOOL fOk;

    // unregister notification window class
    fOk = UnregisterClass(
            SNMP_AGENT_WND_CLASS,
            g_hInstance
            );

    if (!fOk) {

        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: INIT: UnregisterClass returned %d.\n",
            GetLastError()
            ));
    }

    return fOk;
}

BOOL
CreateAgentWnd(
    )
{
    BOOL fOk;
    HWND hwnd;
    
    // create notification window
    hwnd = CreateWindow(
                SNMP_AGENT_WND_CLASS,
                NULL,       // pointer to window name
                0,          // window style
                0,          // horizontal position of window
                0,          // vertical position of window
                0,          // window width
                0,          // window height
                NULL,       // handle to parent or owner window
                NULL,       // handle to menu or child-window identifier
                g_hInstance,// handle to application instance
                NULL        // pointer to window-creation data
                );

    // validate window handle
    if (hwnd != NULL) {

        // transfer
        g_hwndAgent = hwnd;

        // success
        fOk = TRUE;

    } else {
        
        SNMPDBG((   
            SNMP_LOG_ERROR,
            "SNMP: INIT: CreateWindow returned %d.\n",
            GetLastError()
            ));

        // transfer
        g_hwndAgent = NULL;

        // failure
        fOk = FALSE;
    }

    return fOk;
}

BOOL
DestroyAgentWnd(
    )
{
    BOOL fOk;

    // validate handle
    if (g_hwndAgent != NULL) {

        // destroy notification window
        fOk = DestroyWindow(g_hwndAgent);

        if (!fOk) {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: INIT: DestroyWindow returned %d.\n",
                GetLastError()
                ));
        }
    }

    return fOk;
}

//--------------------------- PUBLIC PROCEDURES -----------------------------

BOOL agentConfigInit(VOID)
    {
    Session  session;
    SOCKET sd;
    DWORD    threadId;
    HANDLE   hCommThread;
    DWORD    dwResult;
    INT      i;
    WSADATA  WSAData;
    WORD     wVersionRequested;
    BOOL     fSuccess;
    INT      pseudoAgentsLen;
    INT      j;
    AsnObjectIdentifier tmpView;

    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: initializing master agent.\n"));

    wVersionRequested = MAKEWORD( 1, 1 );
    if (i = WSAStartup(wVersionRequested, &WSAData))
        {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error on WSAStartup %d\n", i));

        return FALSE;
        }

    // initialize configuration from registry...
    if (!regconf())
        {
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: error on regconf %d\n", GetLastError()));

        return FALSE;
        }

    if (!SnmpSvcGenerateColdStartTrap(0))
        {
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: error on SnmpSvcGenerateColdStartTrap %d\n", GetLastError()));
        //not serious error
        }

    timeZeroReference = SnmpSvcInitUptime() / 10;

    pseudoAgentsLen = extAgentsLen;
    for (i=0; i < extAgentsLen; i++)
        {
            extAgents[i].fInitedOk = TRUE;
            // load extension DLL (if not already...

            SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: %s loading.\n", extAgents[i].pathName));

            if ((extAgents[i].hExtension = GetModuleHandle(extAgents[i].pathName)) == NULL)
            {
                if ((extAgents[i].hExtension =
                    LoadLibrary(extAgents[i].pathName)) == NULL)
                {
                    SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d loading %s\n", GetLastError(), extAgents[i].pathName));
                    SnmpSvcReportEvent(SNMP_EVENT_INVALID_EXTENSION_AGENT_DLL, 1, &extAgents[i].pathName, GetLastError());

                    extAgents[i].fInitedOk = FALSE;
                }
            }
           
            if (extAgents[i].fInitedOk)               
            {
                if ((extAgents[i].initAddr = GetProcAddress(extAgents[i].hExtension,"SnmpExtensionInit")) == NULL)
                {
                    SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d resolving SnmpExtensionInit in %s\n", GetLastError(), extAgents[i].pathName));
                    SnmpSvcReportEvent(SNMP_EVENT_INVALID_EXTENSION_AGENT_DLL, 1, &extAgents[i].pathName, GetLastError());

                    extAgents[i].fInitedOk = FALSE;
                }
                else if ((extAgents[i].queryAddr = 
                            GetProcAddress(extAgents[i].hExtension,"SnmpExtensionQuery")) == NULL)
                {
                    SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d resolving SnmpExtensionQuery in %s\n", GetLastError(), extAgents[i].pathName));
                    SnmpSvcReportEvent(SNMP_EVENT_INVALID_EXTENSION_AGENT_DLL, 1, &extAgents[i].pathName, GetLastError());

                    extAgents[i].fInitedOk = FALSE;
                }
                else if ((extAgents[i].trapAddr =
                            GetProcAddress(extAgents[i].hExtension,"SnmpExtensionTrap")) == NULL)
                {
                    SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d resolving SnmpExtensionTrap in %s\n", GetLastError(), extAgents[i].pathName));
                    SnmpSvcReportEvent(SNMP_EVENT_INVALID_EXTENSION_AGENT_DLL, 1, &extAgents[i].pathName, GetLastError());

                    extAgents[i].fInitedOk = FALSE;
                }
                else if (!(*extAgents[i].initAddr)(timeZeroReference,
                                            &extAgents[i].hPollForTrapEvent,
                                            &(extAgents[i].supportedView)))
                {
                    SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: SnmpExtensionInit failed in %s\n", extAgents[i].pathName));
                    SnmpSvcReportEvent(SNMP_EVENT_INVALID_EXTENSION_AGENT_DLL, 1, &extAgents[i].pathName, GetLastError());

                    extAgents[i].fInitedOk = FALSE;
                }
                else
                {
                    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: %s supports %s.\n",
                            extAgents[i].pathName, SnmpUtilOidToA(&extAgents[i].supportedView)));

                    if ((extAgents[i].initAddrEx =
                           GetProcAddress(extAgents[i].hExtension,"SnmpExtensionInitEx")) != NULL) 
                    {
                        j = 1;
                        while ((*extAgents[i].initAddrEx)(&tmpView)) 
                        {
                            pseudoAgentsLen++;
                            extAgents = (CfgExtensionAgents *) SnmpUtilMemReAlloc(extAgents,
                                (pseudoAgentsLen * sizeof(CfgExtensionAgents)));
                            extAgents[pseudoAgentsLen-1].supportedView.ids =
                                                         tmpView.ids;
                            extAgents[pseudoAgentsLen-1].supportedView.idLength =
                                                         tmpView.idLength;
                            extAgents[pseudoAgentsLen-1].initAddr =
                                                        extAgents[i].initAddr;
                            extAgents[pseudoAgentsLen-1].queryAddr =
                                                        extAgents[i].queryAddr;
                            extAgents[pseudoAgentsLen-1].trapAddr =
                                                        extAgents[i].trapAddr;
                            extAgents[pseudoAgentsLen-1].pathName =
                                                        extAgents[i].pathName;
                            extAgents[pseudoAgentsLen-1].hExtension =
                                                        extAgents[i].hExtension;
                            extAgents[pseudoAgentsLen-1].fInitedOk = TRUE;
                            extAgents[pseudoAgentsLen-1].hPollForTrapEvent = NULL;
                            SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: %s supports %s.\n",
                                extAgents[pseudoAgentsLen-1].pathName, SnmpUtilOidToA(&tmpView)));
                            j++;
                        }
                    } 
                }

            } // end if fIntedOk

            SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: %s loaded %s.\n",extAgents[i].pathName,
                extAgents[i].fInitedOk ? "successfully" : "unsuccessfully"));

        } // end for ()

    extAgentsLen = pseudoAgentsLen;

    fdarrayLen = 0;

                {
                struct sockaddr_in localAddress_in;
                struct sockaddr_ipx localAddress_ipx;
                struct servent *serv;

                session.family   = AF_INET;
                session.type     = SOCK_DGRAM;
                session.protocol = 0;

                localAddress_in.sin_family = AF_INET;
                if ((serv = getservbyname( "snmp", "udp" )) == NULL) {
                    localAddress_in.sin_port =
                        htons(WKSN_UDP_GETSET /*extract address.TAddress*/ );
                } else {
                    localAddress_in.sin_port = (SHORT)serv->s_port;
                }
                localAddress_in.sin_addr.s_addr = ntohl(INADDR_ANY);
                bcopy(&localAddress_in,
                    &session.localAddress,
                    sizeof(localAddress_in));

                fSuccess = FALSE;
                if      ((sd = socket(session.family, session.type,
                                      session.protocol)) == (SOCKET)-1)
                    {
                    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: error %d creating udp socket.\n", GetLastError()));
                    }
                else if (bind(sd, &session.localAddress,
                              sizeof(session.localAddress)) != 0)
                    {
                    SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d binding udp socket.\n", GetLastError()));
                    }
                else  // successfully opened an UDP socket
                    {
                    fdarray[fdarrayLen] = sd;
                    fdarrayLen += 1;
                    fSuccess = TRUE;
                    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: setup udp listen port.\n"));
                    }

                // now setup IPX socket

                session.family  = PF_IPX;
                session.type    = SOCK_DGRAM;
                session.protocol = NSPROTO_IPX;

                bzero(&localAddress_ipx, sizeof(localAddress_ipx));
                localAddress_ipx.sa_family = PF_IPX;
                localAddress_ipx.sa_socket = htons(WKSN_IPX_GETSET);
                bcopy(&localAddress_ipx, &session.localAddress,
                      sizeof(localAddress_ipx));

                if      ((sd = socket(session.family, session.type,
                                      session.protocol)) == (SOCKET)-1)
                    {
                    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: error %d creating ipx socket.\n", GetLastError()));
                    }
                else if (bind(sd, &session.localAddress,
                              sizeof(session.localAddress)) != 0)
                    {
                    SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d binding ipx socket.\n", GetLastError()));
                    }
                else
                    {
                    fdarray[fdarrayLen] = sd;
                    fdarrayLen += 1;
                    fSuccess = TRUE;
                    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: setup ipx listen port.\n"));
                    }

                if (!fSuccess)
                    return FALSE;       // can't open either socket
                }


// --------- END: PROTOCOL SPECIFIC SOCKET CODE END. ---------------

    // allocate receive buffer for incoming snmp messages
    if ((g_recvBuf = (BYTE *)SnmpUtilMemAlloc(RECVBUFSIZE)) == NULL) {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: could not allocate receive buffer.\n"));
        SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    // initialize critical used with comm thread
    InitializeCriticalSection(&g_commThreadCritSec);

    // create communication thread
    hCommThread = CreateThread(
                        NULL,           // lpThreadAttributes
                        0,              // dwStackSize
                        (LPTHREAD_START_ROUTINE)agentCommThread,
                        NULL,           // lpParameters
                        0,              // dwCreationFlags
                        &threadId
                        );

    // validate thread handle
    if (hCommThread == NULL) {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d creating agentCommThread.\n", GetLastError()));
        SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, GetLastError());
        goto cleanup;
    }

    if (!noservice) {

        SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: setting service status to running.\n"));

        status.dwCurrentState = SERVICE_RUNNING;
        status.dwCheckPoint   = 0;
        status.dwWaitHint     = 0;
        if (!SetServiceStatus(hService, &status))
            {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error on SetServiceStatus %d\n", GetLastError()));
            SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, GetLastError());
            goto cleanup;
            }
        else
            {
            SnmpSvcReportEvent(SNMP_EVENT_SERVICE_STARTED, 0, NULL, NO_ERROR);
            }
    }

    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: agentTrapThread entered.\n"));

    // become the trap thread...
    trapThread(NULL);

    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: agentTrapThread terminated.\n"));
    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: waiting for agentCommThread to terminate.\n"));

cleanup:

    // wait for threads to terminate
    dwResult = WaitForSingleObject(
                hCommThread,
                CTAMTimeout     // dwMilliseconds
                );

    // evaluate return status
    if (dwResult == WAIT_FAILED) {

        SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d waiting for agentCommThread to terminate.\n", GetLastError()));

    } else if (dwResult == WAIT_TIMEOUT) {

        SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: timeout waiting for agentCommThread to terminate.\n"));

        // continue, and try to terminate comm thread anyway
    } else {

        SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: agentCommThread terminated.\n"));
    }

    // shutdown
    WSACleanup();

    // initialize critical used with comm thread
    DeleteCriticalSection(&g_commThreadCritSec);

    return TRUE;

    } // end agentConfigInit()

VOID agentCommThread(VOID *threadParam)
    {
    INT i;

    SNMPDBG((SNMP_LOG_TRACE, "SNMP: PDU: agentCommThread entered.\n"));

    // register window class
    RegisterAgentWndClass();

    // create window
    CreateAgentWnd();

    // associate each socket with window
    for (i = 0; i < fdarrayLen; i++) {

        // associate event with comm thread socket
        if (WSAAsyncSelect(fdarray[i],g_hwndAgent,SNMP_AGENT_WND_MESSAGE,FD_READ) == SOCKET_ERROR) {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: PDU: error %d associating window with socket.\n",WSAGetLastError()));
            SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, WSAGetLastError());
            SetEvent(g_hTerminationEvent);
            goto cleanup;
        }
    }

    while(1) {

        int nResult;
        DWORD dwResult;
#define COMM_EVENT_TERMINATE    0
#define COMM_EVENT_GET_MESSAGE  1

        // wait for termination or event
        dwResult = MsgWaitForMultipleObjects(
                        1,
                        &g_hTerminationEvent,
                        FALSE,          // bWaitAll
                        INFINITE,       // dwMilliseconds
                        QS_ALLINPUT
                        );

        // see if socket ready for reading
        if (dwResult == COMM_EVENT_GET_MESSAGE) {

            MSG msg;

            // retrieve next item in thread message queue
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

                // translate message
                TranslateMessage(&msg);

                // dispatch message
                DispatchMessage(&msg);
            }

        } else if (dwResult == COMM_EVENT_TERMINATE) {

            SNMPDBG((SNMP_LOG_ERROR, "SNMP: PDU: agentCommThread exiting.\n"));
            break;

        } else if (dwResult == WAIT_FAILED){

            SNMPDBG((SNMP_LOG_ERROR, "SNMP: PDU: error %d waiting for socket.\n",dwResult));
            SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, dwResult);
            SetEvent(g_hTerminationEvent);
            break;
        }
    }

cleanup:

    // destroy window
    DestroyAgentWnd();

    // register window class
    UnregisterAgentWndClass();

} // end agentCommThread()


//-------------------------------- END --------------------------------------
