/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        misc/bsd.c
 * PURPOSE:     Legacy BSD sockets APIs
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 15/06-2001 Created
 */
#include <ws2_32.h>

ULONG
EXPORT
htonl(
  IN  ULONG hostlong)
{
  return DH2N(hostlong);
}

USHORT
EXPORT
htons(
  IN  USHORT hostshort)
{
  return WH2N(hostshort);
}

ULONG
EXPORT
ntohl(
  IN  ULONG netlong)
{
  return DN2H(netlong);
}

USHORT
EXPORT
ntohs(
  IN  USHORT netshort)
{
  return WN2H(netshort);
}

/* EOF */
