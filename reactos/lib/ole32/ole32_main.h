/*
 * Copyright 2000 Huw D M Davies for CodeWeavers
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

#ifndef __WINE_OLE32_MAIN_H
#define __WINE_OLE32_MAIN_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

extern HINSTANCE OLE32_hInstance;

void COMPOBJ_InitProcess( void );
void COMPOBJ_UninitProcess( void );

#endif /* __WINE_OLE32_MAIN_H */
