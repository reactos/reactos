#pragma once

#ifndef __WINDOWS_H
#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wincon.h>
#define _INC_WINDOWS
#include <winsock.h>
#endif

#include "tnmsg.h"

extern int Telnet_Redir;

int printm(LPTSTR szModule, BOOL fSystem, DWORD dwMessageId, ...);
void LogErrorConsole(LPTSTR szError);
int printit(const char * it);
