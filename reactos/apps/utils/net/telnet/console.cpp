/* $Id: console.cpp,v 1.1 2001/01/27 22:38:43 ea Exp $
 *
 * FILE       : console.cpp
 * AUTHOR     : E.Aliberti
 * PROJECT    : ReactOS Operating System
 * DESCRIPTION: telnet client for the W32 subsystem
 * DATE       : 2001-01-21
 * REVISIONS
 *	2001-02-21 ea	added to group console-related methods
 */
#include <windows.h>

const char * title = "telnet - ";

// Set console's title
void console_title_connecting (
	char const* pszHostName,
	const int nPort
	)
{
  char t[256];
  wsprintf(t,"%sconnecting to %s:%i", title, pszHostName, nPort);
  SetConsoleTitle(t);
}

void console_title_connected (
	char const* pszHostName,
	const int nPort
	)
{
  char t[256];
  wsprintf(t,"%sconnected to %s:%i", title, pszHostName, nPort);
  SetConsoleTitle(t);
}

void console_title_not_connected (void)
{
  char t[256];
  wsprintf(t,"%snot connected", title);
  SetConsoleTitle(t);
}

/* EOF */
