/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
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
/* $Id: bit.c,v 1.1 2004/02/02 00:36:36 ekohl Exp $
 *
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Runtime code
 * FILE:              ntoskrnl/rtl/bit.c
 * PROGRAMER:         Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>


/* FUNCTIONS ****************************************************************/

CCHAR STDCALL
RtlFindLeastSignificantBit (IN ULONGLONG Set)
{
  int i;

  if (Set == 0ULL)
    return -1;

  for (i = 0; i < 64; i++)
    {
      if (Set & (1 << i))
        return (CCHAR)i;
    }

  return -1;
}


CCHAR STDCALL
RtlFindMostSignificantBit (IN ULONGLONG Set)
{
  int i;

  if (Set == 0ULL)
    return -1;

  for (i = 63; i >= 0; i--)
    {
      if (Set & (1 << i))
        return (CCHAR)i;
    }

  return -1;
}

/* EOF */
