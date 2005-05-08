/* $Id: cctypes.h,v 1.1.2.1 2004/10/25 01:24:07 ion Exp $
 *
 *  ReactOS Headers
 *  Copyright (C) 1998-2004 ReactOS Team
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
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/cctypes.h
 * PURPOSE:         Definitions for exported Cache Manager Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */

#ifndef _CCTYPES_H
#define _CCTYPES_H

#include <reactos/helper.h>

#ifdef __NTOSKRNL__
extern ULONG EXPORTED CcFastMdlReadWait;
extern ULONG EXPORTED CcFastReadNotPossible;
extern ULONG EXPORTED CcFastReadWait;
#else
extern ULONG IMPORTED CcFastMdlReadWait;
extern ULONG IMPORTED CcFastReadNotPossible;
extern ULONG IMPORTED CcFastReadWait;
#endif

#endif

