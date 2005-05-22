/* $Id: init.c 13449 2005-02-06 21:55:07Z ea $
 *
 * initmv.c - Process the file rename list
 * 
 * ReactOS Operating System
 * 
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
 

#include "smss.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
SmProcessFileRenameList(VOID)
{
  DPRINT("SmProcessFileRenameList() called\n");

  /* FIXME: implement it! */
/*
 * open HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\FileRenameOperations
 * for each item in its value
 *     clone the old file in the new name,
 *     delete the source.
 *
 */

  DPRINT("SmProcessFileRenameList() done\n");

  return(STATUS_SUCCESS);
}

/* EOF */
