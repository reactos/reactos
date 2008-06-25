#ifndef __TNERROR_H
#define __TNERROR_H

#ifndef __WINDOWS_H
#include <windows.h>
#endif

#include "tnmsg.h"

extern int Telnet_Redir;

int printm(LPTSTR szModule, BOOL fSystem, DWORD dwMessageId, ...);
void LogErrorConsole(LPTSTR szError);
int printit(const char * it);

#endif
