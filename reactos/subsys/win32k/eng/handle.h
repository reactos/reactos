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
/* $Id: handle.h,v 1.5 2003/05/18 17:16:17 ea Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Manage GDI Handle definitions
 * FILE:              subsys/win32k/eng/handle.h
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 29/8/1999: Created
 */
#ifndef __ENG_HANDLE_H
#define __ENG_HANDLE_H

#include "objects.h"
#include <include/object.h>

typedef struct _GDI_HANDLE {
  PENGOBJ		pEngObj;
} GDI_HANDLE, *PGDI_HANDLE;

#define INVALID_HANDLE  0
#define MAX_GDI_HANDLES 4096

GDI_HANDLE GDIHandles[MAX_GDI_HANDLES];

#define ValidEngHandle( x )  (!( (x) == INVALID_HANDLE ))

#endif
