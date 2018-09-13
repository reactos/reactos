/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    trapapi.c

Abstract:

    Provides SNMP message authentication functionality for Proxy Agent.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <windows.h>

#include <winsock.h>
#include <wsipx.h>

#include <errno.h>
#include <stdio.h>
#include <process.h>
#include <string.h>


//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <snmputil.h>
#include "wellknow.h"

#include "..\authapi.\berapi.h"
#include "..\authapi.\pduapi.h"
#include "..\authapi.\auth1157.h"
#include "..\authapi.\authxxxx.h"
#include "..\authapi.\authapi.h"

#include "regconf.h"


//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

extern DWORD g_platformId;
extern AsnObjectIdentifier * g_enterprise;

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SGTTimeout ((DWORD)3600000)


//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

static RFC1157VarBindList noVarBinds = { NULL, 0 };

//--------------------------- PRIVATE PROTOTYPES ----------------------------

int gethostname(OUT char *,IN int );
void dp_ipx(int, char *, SOCKADDR_IPX *, char *);

//--------------------------- PRIVATE PROCEDURES ----------------------------

#define bzero(lp, size)         (void)memset(lp, 0, size)

//--------------------------- PUBLIC PROCEDURES -----------------------------

SNMPAPI 
SNMP_FUNC_TYPE
SnmpSvcGenerateTrap(
    IN AsnObjectIdentifier * enterprise,
    IN AsnInteger            genericTrap,
    IN AsnInteger            specificTrap,
    IN AsnTimeticks          timeStamp,
    IN RFC1157VarBindList *  variableBindings)
    {
    static BOOL   fFirstTime = TRUE;
    static HANDLE hGenerateTrapMutex;
    static char   myname[128];
    static struct sockaddr mysocket;
    static SnmpMgmtCom request;
    static SOCKET    fd_inet, fd_ipx, fd;
    static unsigned long nul_s_addr = 0L;

    struct sockaddr dest;
    BYTE   *pBuf;
    UINT   length;
    INT    i, j;


    if (fFirstTime)
        {
        struct sockaddr localAddress;
        BOOL fSuccess;

        fFirstTime = FALSE;
        fSuccess = FALSE;

        // initialize trap destinations
        if (!tdConfig(&trapDests, &trapDestsLen))
            {
            SnmpUtilVarBindListFree(variableBindings);
            return FALSE;
            }

        // setup 2 trap generation sockets, one for inet, one for ipx

        // block...
            {
            struct sockaddr_in localAddress_in;

            localAddress_in.sin_family = AF_INET;
            localAddress_in.sin_port = htons(0);
            localAddress_in.sin_addr.s_addr = ntohl(INADDR_ANY);
            bcopy(&localAddress_in, &localAddress, sizeof(localAddress_in));
            } // end block.

        if ((fd_inet = socket(AF_INET, SOCK_DGRAM, 0)) == (SOCKET)-1)
            {
            SNMPDBG((SNMP_LOG_TRACE, "SNMP: TRAP: error %d creating udp trap generation socket.\n", GetLastError()));
            }
        else if (bind(fd_inet, &localAddress, sizeof(localAddress)) != 0)
            {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: TRAP: error %d binding udp trap generation socket.\n", GetLastError()));
            }
        else
            {
            fSuccess = TRUE;
            }

            {
            struct sockaddr_ipx localAddress_ipx;

            bzero(&localAddress_ipx,sizeof(localAddress_ipx));
            localAddress_ipx.sa_family = AF_IPX;
            bcopy(&localAddress_ipx, &localAddress, sizeof(localAddress_ipx));
            }

        if ((fd_ipx = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX)) == (SOCKET)-1)
            {
            SNMPDBG((SNMP_LOG_TRACE, "SNMP: TRAP: error %d creating ipx trap generation socket.\n", GetLastError()));
            }
        else if (bind(fd_ipx, &localAddress, sizeof(localAddress)) != 0)
            {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: TRAP: error %d binding ipx trap generation socket.\n", GetLastError()));
            }
        else
            {
            fSuccess = TRUE;
            }

        if (!fSuccess) {
            SnmpUtilVarBindListFree(variableBindings);
            return FALSE;
        }

        // create mutex...

        if ((hGenerateTrapMutex = CreateMutex(NULL, FALSE, NULL)) == NULL)
            {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: TRAP: error %d creating trap generaion mutex.\n", GetLastError()));

            SnmpUtilVarBindListFree(variableBindings);
            return FALSE;
            }


        // other one-time initialization...

        request.pdu.pduType                        = ASN_RFC1157_TRAP;

        if (fd_inet != (SOCKET)-1)
            {

            if (gethostname(myname, sizeof(myname)) == -1)
                {
                SNMPDBG((SNMP_LOG_ERROR, "SNMP: TRAP: error on gethostname %d.\n", GetLastError()));

                SnmpUtilVarBindListFree(variableBindings);
                return FALSE;
                }

            if (!SnmpSvcAddrToSocket(myname, &mysocket) == -1)
                {
                SNMPDBG((SNMP_LOG_ERROR, "SNMP: TRAP: error on SnmpSvcAddrToSocket %d.\n", GetLastError()));

                SnmpUtilVarBindListFree(variableBindings);
                return FALSE;
                }
            }

        } // end if (fFirstTime)

    // take mutex
    if (WaitForSingleObject(hGenerateTrapMutex, SGTTimeout)
        == 0xffffffff)
        {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: TRAP: error %d waiting for trap generation mutex.\n", GetLastError()));

        SnmpUtilVarBindListFree(variableBindings);
        return FALSE;
        }

    // build pdu
    request.pdu.pduValue.trap.genericTrap   = genericTrap;
    request.pdu.pduValue.trap.specificTrap  = specificTrap;
    request.pdu.pduValue.trap.timeStamp     = timeStamp;
    request.pdu.pduValue.trap.varBinds.list = variableBindings->list;
    request.pdu.pduValue.trap.varBinds.len  = variableBindings->len;

    // check if default enterprise oid has been overridden
    if (enterprise && enterprise->idLength && enterprise->ids) 
        {
        request.pdu.pduValue.trap.enterprise.idLength = enterprise->idLength;
        request.pdu.pduValue.trap.enterprise.ids      = enterprise->ids;
        }
    else 
        {
        request.pdu.pduValue.trap.enterprise.idLength = g_enterprise->idLength;
        request.pdu.pduValue.trap.enterprise.ids      = g_enterprise->ids;
        }

    for (i=0; i<trapDestsLen; i++)
        {
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: TRAP: processing trap destinations for community %s.\n",
                          trapDests[i].communityName));
        for (j=0; j<trapDests[i].addrLen; j++)
            {
            request.community.stream  = trapDests[i].communityName;
            request.community.length  = strlen(trapDests[i].communityName);
            request.community.dynamic = FALSE;

            switch (trapDests[i].addrList[j].addrEncoding.sa_family)
                {
                case AF_INET:
                    {
                    struct sockaddr_in destAddress_in;
                    struct servent *serv;

                    if (fd_inet == (SOCKET)-1)  // don't have an IP socket
                        {
                        SNMPDBG((SNMP_LOG_TRACE, "SNMP: TRAP: cannot send udp trap because no socket available.\n"));
                        continue;
                        }

                    destAddress_in.sin_family = AF_INET;
                    if ((serv = getservbyname( "snmp-trap", "udp" )) == NULL) {
                        destAddress_in.sin_port = htons(162);
                    } else {
                        destAddress_in.sin_port = (SHORT)serv->s_port;
                    }
                    destAddress_in.sin_addr.s_addr = ((struct sockaddr_in *)(&trapDests[i].addrList[j].addrEncoding))->sin_addr.s_addr;
                    SNMPDBG((SNMP_LOG_TRACE, "SNMP: TRAP: processing ip trap destination %u.%u.%u.%u.\n",
                                  destAddress_in.sin_addr.S_un.S_un_b.s_b1,
                                  destAddress_in.sin_addr.S_un.S_un_b.s_b2,
                                  destAddress_in.sin_addr.S_un.S_un_b.s_b3,
                                  destAddress_in.sin_addr.S_un.S_un_b.s_b4));
                    bcopy(&destAddress_in, &dest, sizeof(destAddress_in));
                    }

                    // include local agent addr in pdu (ip only)
                    request.pdu.pduValue.trap.agentAddr.stream =
                        (BYTE *)&((struct sockaddr_in *)(&mysocket))->sin_addr.s_addr;
                    request.pdu.pduValue.trap.agentAddr.length = 4;

                    fd = fd_inet;
                    break;

                case AF_IPX:
                    {
                    SOCKADDR_IPX *pdest = (SOCKADDR_IPX *) &dest;

                    if (fd_ipx == (SOCKET)-1)  // don't have an IPX socket
                        {
                        SNMPDBG((SNMP_LOG_TRACE, "SNMP: TRAP: cannot send ipx trap because no socket available.\n"));
                        continue;
                        }

                    // sa_family, sa_netnum, and sa_nodenum are already set
                    bcopy(&trapDests[i].addrList[j].addrEncoding, &dest,
                          sizeof(SOCKADDR_IPX));
                    pdest->sa_socket = htons(WKSN_IPX_TRAP);

                    dp_ipx(SNMP_LOG_TRACE, "SNMP: TRAP: processing ipx trap destination ", pdest, ".\n");
                    }

                    // include empty agent addr in pdu (ipx only)
                    request.pdu.pduValue.trap.agentAddr.stream = (BYTE *)&nul_s_addr;
                    request.pdu.pduValue.trap.agentAddr.length = 4;

                    fd = fd_ipx;
                    break;

                default:
                    {
                    SNMPDBG((SNMP_LOG_TRACE, "SNMP: TRAP: cannot send trap because sa_family invalid.\n"));
                    continue;
                    }
                }

            pBuf   = NULL;
            length = 0;
            if (!SnmpSvcEncodeMessage(ASN_SEQUENCE, &request, &pBuf, &length))
                {
                SNMPDBG((SNMP_LOG_TRACE, "SNMP: TRAP: error on SnmpSvcEncodeMessage %d.\n",
                          GetLastError()));

                SnmpUtilMemFree(pBuf);

                continue;
                }

            // transmit trap pdu
            if ((length = sendto(fd, pBuf, length, 0, &dest, sizeof(dest)))
                == -1)
                {
                SNMPDBG((SNMP_LOG_ERROR, "SNMP: TRAP: error %d sending trap.\n", GetLastError()));

                //a serious error?
                }
            else
                {
                SNMPDBG((SNMP_LOG_TRACE, "SNMP: TRAP: trap sent successfully.\n"));
                }

            SnmpUtilMemFree(pBuf);

            } // end for ()

        } // end for ()

    SnmpUtilVarBindListFree(variableBindings);

    // release mutex
    if (!ReleaseMutex(hGenerateTrapMutex))
        {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: TRAP: error %d releasing trap generation mutex.\n", GetLastError()));

        return FALSE;
        }

    return TRUE;

    } // end SnmpSvcGenerateTrap()


SNMPAPI 
SNMP_FUNC_TYPE
SnmpSvcGenerateColdStartTrap(
    IN AsnInteger timeStamp)
    {
    if (!SnmpSvcGenerateTrap(
            NULL, // use default oid
            SNMP_GENERICTRAP_COLDSTART, 
            0, 
            timeStamp, 
            &noVarBinds))
        {
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: TRAP: error on SnmpServiceGenerateTrap %d.\n", GetLastError()));

        return FALSE;
        }

    return TRUE;

    } // end SnmpSvcGenerateColdStartTrap()


SNMPAPI 
SNMP_FUNC_TYPE
SnmpSvcGenerateWarmStartTrap(
    IN AsnInteger timeStamp)
    {
    if (!SnmpSvcGenerateTrap(
            NULL, // use default oid
            SNMP_GENERICTRAP_WARMSTART, 
            0, 
            timeStamp, 
            &noVarBinds))
        {
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: TRAP: error on SnmpServiceGenerateTrap %d.\n", GetLastError()));

        return FALSE;
        }

    return TRUE;

    } // end SnmpSvcGenerateWarmStartTrap()


SNMPAPI 
SNMP_FUNC_TYPE
SnmpSvcGenerateLinkUpTrap(
    IN AsnInteger           timeStamp,
    IN RFC1157VarBindList * variableBindings)
    {
    if (!SnmpSvcGenerateTrap(
            NULL, // use default oid
            SNMP_GENERICTRAP_LINKUP, 
            0, 
            timeStamp, 
            variableBindings))
        {
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: TRAP: error on SnmpServiceGenerateTrap %d.\n", GetLastError()));

        return FALSE;
        }

    return TRUE;

    } // end SnmpSvcGenerateLinkUpTrap()


SNMPAPI 
SNMP_FUNC_TYPE
SnmpSvcGenerateLinkDownTrap(
    IN AsnInteger           timeStamp,
    IN RFC1157VarBindList * variableBindings)
    {
    if (!SnmpSvcGenerateTrap(
            NULL, // use default oid
            SNMP_GENERICTRAP_LINKDOWN, 
            0, 
            timeStamp, 
            variableBindings))
        {
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: TRAP: error on SnmpServiceGenerateTrap %d.\n", GetLastError()));

        return FALSE;
        }

    return TRUE;

    } // end SnmpSvcGenerateLinkDownTrap()


SNMPAPI 
SNMP_FUNC_TYPE
SnmpSvcGenerateAuthFailTrap(
    AsnInteger timeStamp)
    {
    if (!SnmpSvcGenerateTrap(
            NULL, // use default oid
            SNMP_GENERICTRAP_AUTHFAILURE, 
            0, 
            timeStamp, 
            &noVarBinds))
        {
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: TRAP: error on SnmpServiceGenerateTrap %d.\n", GetLastError()));

        return FALSE;
        }

    return TRUE;

    } // end SnmpSvcGenerateAuthFailTrap()


// authenticate community of rfc1157 message with valid communities in registry
BOOL commauth(RFC1157Message *message)
    {
    static BOOL fFirstTime = TRUE;

    BOOL fFound = FALSE;
    INT  i;

    if (fFirstTime)
        {
        fFirstTime = FALSE;
        vcConfig(&validComms, &validCommsLen);
        }

    if (validCommsLen > 0)
        {
        for(i=0; i < validCommsLen; i++)
            {
            if ((strlen(validComms[i].communityName) == message->community.length) && 
                !strncmp(message->community.stream, validComms[i].communityName, message->community.length))
                {
                fFound = TRUE;
                break;
                }
            } // end for ()
        }
    else
        {
        fFound = TRUE; // no entries means all communities allowed
        } // end if

    if (!fFound)
        {
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: PDU: invalid community filtered.\n"));

        if (enableAuthTraps)
            {
            if (!SnmpSvcGenerateAuthFailTrap(SnmpSvcGetUptime()))
                {
                SNMPDBG((SNMP_LOG_TRACE, "SNMP: PDU: error on SnmpSvcGenerateAuthFailTrap %d.\n",
                          GetLastError()));

                }
            } // end if
        } // end if

    return fFound;

    } // end commauth()


//-------------------------------- END --------------------------------------

// display IPX address in 00000001.123456789ABC form

void dp_ipx(int level, char *msg1, SOCKADDR_IPX* addr, char *msg2)
    {
    SNMPDBG((level, "%s%02X%02X%02X%02X.%02X%02X%02X%02X%02X%02X%s",
              msg1,
              (unsigned char)addr->sa_netnum[0],
              (unsigned char)addr->sa_netnum[1],
              (unsigned char)addr->sa_netnum[2],
              (unsigned char)addr->sa_netnum[3],
              (unsigned char)addr->sa_nodenum[0],
              (unsigned char)addr->sa_nodenum[1],
              (unsigned char)addr->sa_nodenum[2],
              (unsigned char)addr->sa_nodenum[3],
              (unsigned char)addr->sa_nodenum[4],
              (unsigned char)addr->sa_nodenum[5],
              msg2));
    }
