/* $Id: windc.c,v 1.1 2002/07/04 20:12:27 dwelch Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/class.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <win32k/userobj.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>
#include <include/msgqueue.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

HRGN 
DceGetVisRgn(HWND hWnd, ULONG Flags, HWND hWndChild, ULONG CFlags)
{

}

INT STDCALL
NtUserReleaseDC(HWND hWnd, HDC hDc)
{
  
}

HDC STDCALL
NtUserGetDCEx(HWND hWnd, HANDLE hRegion, ULONG Flags)
{
  
}

/* EOF */
