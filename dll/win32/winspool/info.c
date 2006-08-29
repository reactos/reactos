/*
 * WINSPOOL functions
 *
 * Copyright 1996 John Harvey
 * Copyright 1998 Andreas Mohr
 * Copyright 1999 Klaas van Gend
 * Copyright 1999, 2000 Huw D M Davies
 * Copyright 2001 Marcus Meissner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wine/config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winspool.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "winnls.h"

WINE_DEFAULT_DEBUG_CHANNEL(winspool);

/******************************************************************************
 *		GetDefaultPrinterA   (WINSPOOL.@)
 */
BOOL WINAPI GetDefaultPrinterA(LPSTR name, LPDWORD namesize)
{
   char *ptr;

   if (*namesize < 1)
   {
      SetLastError (ERROR_INSUFFICIENT_BUFFER);
      return FALSE;
   }

   if (!GetProfileStringA ("windows", "device", "", name, *namesize))
   {
      SetLastError (ERROR_FILE_NOT_FOUND);
      return FALSE;
   }

   if ((ptr = strchr (name, ',')) == NULL)
   {
      SetLastError (ERROR_FILE_NOT_FOUND);
      return FALSE;
   }

   *ptr = '\0';
   *namesize = strlen (name) + 1;
   return TRUE;
}


/******************************************************************************
 *		GetDefaultPrinterW   (WINSPOOL.@)
 */
BOOL WINAPI GetDefaultPrinterW(LPWSTR name, LPDWORD namesize)
{
   char *buf;
   BOOL  ret;

   if (*namesize < 1)
   {
      SetLastError (ERROR_INSUFFICIENT_BUFFER);
      return FALSE;
   }

   buf = HeapAlloc (GetProcessHeap (), 0, *namesize);
   ret = GetDefaultPrinterA (buf, namesize);
   if (ret)
   {
       DWORD len = MultiByteToWideChar (CP_ACP, 0, buf, -1, name, *namesize);
       if (!len)
       {
           SetLastError (ERROR_INSUFFICIENT_BUFFER);
           ret = FALSE;
       }
       else *namesize = len;
   }

   HeapFree (GetProcessHeap (), 0, buf);
   return ret;
}
