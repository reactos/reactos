/* $Id: helpsock.cpp,v 1.1 2001/01/27 22:38:43 ea Exp $
 *
 * FILE       : helpsock.cpp
 * AUTHOR     : unknown (sources found on www.telnet.org)
 * PROJECT    : ReactOS Operating System
 * DESCRIPTION: telnet client for the W32 subsystem
 * DATE       : 2001-01-21
 * REVISIONS
 *	2001-02-21 ea	Modified to compile under 0.0.16 src tree
 */
#include <winsock.h>
#include <windows.h>
#include <process.h>

#include "telnet.h"

char const* sockmsg(int ecode)
{
  switch(ecode)
  {
// programming errors
// (should never occour in release code?)
  case WSASYSNOTREADY: return "tcp/ip network not ready";
  case WSAEINVAL: return "invalid winsock version";
  case WSAVERNOTSUPPORTED: return "wrong winsock version";
  case WSANOTINITIALISED: return "winsock not initialized";
  case WSAEINTR: "The call was canceled";
  case WSAEINPROGRESS: "A blocking winsock operation is in progress";
  default: return "unknown winsock error";
// general TCP problems
  case WSAENETDOWN: return "winsock has detected that the network subsystem has failed";
// GetXbyY related errors:
  case WSAHOST_NOT_FOUND: return "Authoritative Answer Host not found";
  case WSATRY_AGAIN: return "Non-Authoritative Host not found, or SERVERFAIL";
  case WSANO_RECOVERY: "Nonrecoverable errors: FORMERR, REFUSED, NOTIMP";
  case WSANO_DATA: "Valid name, no data record of requested type";
  }
}

/* EOF */
