/*
 * dllmain.c
 *
 * A stub DllMain function which will be called by DLLs which do not
 * have a user supplied DllMain.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRENTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warrenties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.3 $
 * $Author: ariadne $
 * $Date: 1999/04/23 18:42:21 $
 *
 */

#include <windows.h>
#include <stdarg.h>
#include <string.h>

void debug_printf(char* fmt, ...)
{
   va_list args;
   char buffer[255];
   HANDLE OutputHandle;
   
   AllocConsole();
   OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
   va_start(args,fmt);
   vsprintf(buffer,fmt,args);
   WriteConsoleA(OutputHandle, buffer, strlen(buffer), NULL, NULL);
   va_end(args);
}

BOOL WINAPI DllMain (HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
	return TRUE;
}

