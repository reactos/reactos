/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

HANDLE SockSanCleanUpCompleteEvent;
BOOLEAN SockSanEnabled;

WSAPROTOCOL_INFOW SockTcpProviderInfo =
{
    XP1_GUARANTEED_DELIVERY |
    XP1_GUARANTEED_ORDER |
    XP1_GRACEFUL_CLOSE |
    XP1_EXPEDITED_DATA |
    XP1_IFS_HANDLES,
    0,
    0,
    0,
    PFL_MATCHES_PROTOCOL_ZERO,
    {
        0xe70f1aa0,
        0xab8b,
        0x11cf,
        {0x8c, 0xa3, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}
    },
    0,
    {
        BASE_PROTOCOL,
        { 0, 0, 0, 0, 0, 0, 0 }
    },
    2,
    AF_INET,
    sizeof(SOCKADDR_IN),
    sizeof(SOCKADDR_IN),
    SOCK_STREAM,
    IPPROTO_TCP,
    0,
    BIGENDIAN,
    SECURITY_PROTOCOL_NONE,
    0,
    0,
    L"MSAFD Tcpip [TCP/IP]"
};

/* FUNCTIONS *****************************************************************/

VOID
WSPAPI
SockSanInitialize(VOID)
{

}