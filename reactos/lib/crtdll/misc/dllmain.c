/* $Id: dllmain.c,v 1.5 2002/11/24 18:42:15 robd Exp $
 *
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
 * $Revision: 1.5 $
 * $Author: robd $
 * $Date: 2002/11/24 18:42:15 $
 *
 */

#include <windows.h>
#include <stdarg.h>
#include <msvcrt/stdio.h>
#include <string.h>


/* EXTERNAL PROTOTYPES ********************************************************/

void debug_printf(char* fmt, ...);


/* LIBRARY GLOBAL VARIABLES ***************************************************/

int __mb_cur_max_dll = 1;
int _commode_dll = _IOCOMMIT;


/* LIBRARY ENTRY POINT ********************************************************/

BOOL
WINAPI
DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
	return TRUE;
}

/* EOF */
