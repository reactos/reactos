/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    ipx.c

Abstract:

    Contains routines to manipulate ipx addresses.

        SnmpSvcAddrToSocket

Environment:

    User Mode - Win32

Revision History:

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <snmp.h>
#include <snmputil.h>
#include <winsock.h>
#include <wsipx.h>

#define bcopy(slp, dlp, size)   (void)memcpy(dlp, slp, size)

BOOL isHex(LPSTR str, int strLen)
    {
    int ii;

    for (ii=0; ii < strLen; ii++)
        if (isxdigit(*str))
            str++;
        else
            return FALSE;

    return TRUE;
    }

unsigned int toHex(unsigned char x)
    {
    if (x >= '0' && x <= '9')
        return x - '0';
    else if (x >= 'A' && x <= 'F')
        return x - 'A' + 10;
    else if (x >= 'a' && x <= 'f')
        return x - 'a' + 10;
    else
        return 0;
    }

// convert str to hex number of NumDigits (must be even) into pNum
void atohex(IN LPSTR str, IN int NumDigits, OUT unsigned char *pNum)
    {
    int i, j;

    j=0;
    for (i=0; i < (NumDigits>>1) ; i++)
        {
        pNum[i] = (toHex(str[j]) << 4) + toHex(str[j+1]);
        j+=2;
        }
    }

// return true if addrText is of the form 123456789ABC or
// 000001.123456789abc
// if pNetNum is not null, upon successful return, pNetNum = network number
// if pNodeNum is not null, upon successful return, pNodeNum = node number

BOOL 
SNMP_FUNC_TYPE 
SnmpSvcAddrIsIpx(
    IN  LPSTR addrText,
    OUT char pNetNum[4],
    OUT char pNodeNum[6])
    {
    int addrTextLen;

    addrTextLen = strlen(addrText);
    if (addrTextLen == 12 && isHex(addrText, 12))
        {
            if (pNetNum)
                *((UNALIGNED unsigned long *) pNetNum) = 0L;
            if (pNodeNum)
                atohex(addrText, 12, pNodeNum);
            return TRUE;
        }
    else if (addrTextLen == 21 && addrText[8] == '.' && isHex(addrText, 8) &&
            isHex(addrText+9, 12))
        {
            if (pNetNum)
                atohex(addrText, 8, pNetNum);
            if (pNodeNum)
                atohex(addrText+9, 12, pNodeNum);
            return TRUE;
        }
    else
        return FALSE;
    }

BOOL 
SNMP_FUNC_TYPE
SnmpSvcAddrToSocket(
    LPSTR addrText,
    struct sockaddr *addrEncoding
    )
{
    struct hostent * hp;
    struct sockaddr_in * pAddr_in = (struct sockaddr_in *)addrEncoding;
    struct sockaddr_ipx * pAddr_ipx = (struct sockaddr_ipx *)addrEncoding;
    unsigned long addr;

    // check for ipx addr
    if (SnmpSvcAddrIsIpx(
           addrText,
           pAddr_ipx->sa_netnum,
           pAddr_ipx->sa_nodenum
           )) {

        // see if ip host name which looks like ipx
        if ((hp = gethostbyname(addrText)) == NULL) {

            // host really is ipx machine
            pAddr_ipx->sa_family = AF_IPX;
            pAddr_ipx->sa_socket = htons(DEFAULT_SNMPTRAP_PORT_IPX);

            // address transferred above...

        } else {

            // host is really ip machine
            pAddr_in->sin_family = AF_INET;
            pAddr_in->sin_port = htons(DEFAULT_SNMPTRAP_PORT_UDP);
            pAddr_in->sin_addr.s_addr = *(unsigned long *)hp->h_addr;
        }

    } else if (strncmp(addrText, "255.255.255.255", 15) == 0) {

        // host is a broadcast address
        pAddr_in->sin_family = AF_INET;
        pAddr_in->sin_port = htons(DEFAULT_SNMPTRAP_PORT_UDP);
        pAddr_in->sin_addr.s_addr = 0xffffffff;

    } else if ((long)(addr = inet_addr(addrText)) != -1) {

        // host is ip machine
        pAddr_in->sin_family = AF_INET;
        pAddr_in->sin_port = htons(DEFAULT_SNMPTRAP_PORT_UDP);
        pAddr_in->sin_addr.s_addr = addr;

    } else if ((hp = gethostbyname(addrText)) != NULL) {

        // host is really ip machine
        pAddr_in->sin_family = AF_INET;
        pAddr_in->sin_port = htons(DEFAULT_SNMPTRAP_PORT_UDP);
        pAddr_in->sin_addr.s_addr = *(unsigned long *)hp->h_addr;

    } else {

        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: API: could not convert %s to socket.\n",
            addrText
            ));

        // failure
        return FALSE;
    }

    // success
    return TRUE;
}
