/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/misc/object.c
 * PURPOSE:         DDE
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(user32);

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
GetUserObjectInformationA(
  HANDLE hObj,
  int nIndex,
  PVOID pvInfo,
  DWORD nLength,
  LPDWORD lpnLengthNeeded)
{
  LPWSTR buffer;
  BOOL ret = FALSE;
  DWORD LengthNeeded;

  TRACE("GetUserObjectInformationA(%x %d %x %d %x)\n", hObj, nIndex,
         pvInfo, nLength, lpnLengthNeeded);

  if (nIndex != UOI_NAME && nIndex != UOI_TYPE)
    return GetUserObjectInformationW(hObj, nIndex, pvInfo, nLength, lpnLengthNeeded);

  /* allocate unicode buffer */
  buffer = HeapAlloc(GetProcessHeap(), 0, nLength*2);
  if (buffer == NULL)
  {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }

  /* get unicode string */
  if (GetUserObjectInformationW(hObj, nIndex, buffer, nLength*2, lpnLengthNeeded))
  {
    /* convert string */
    LengthNeeded = WideCharToMultiByte(CP_THREAD_ACP, 0, buffer, -1,
                                       pvInfo, nLength, NULL, NULL);

    if (LengthNeeded != 0)
    {
        if (lpnLengthNeeded)
            *lpnLengthNeeded = LengthNeeded;
        ret = TRUE;
    }
  }

  /* free resources */
  HeapFree(GetProcessHeap(), 0, buffer);
  return ret;
}
