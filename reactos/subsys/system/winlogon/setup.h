/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
/* $Id: setup.h,v 1.1 2003/03/25 19:26:33 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS winlogon
 * FILE:            subsys/system/winlogon/setup.h
 * PURPOSE:         Setup support functions
 * PROGRAMMER:      Eric Kohl
 */

#ifndef __SETUP_H__
#define __SETUP_H__

DWORD GetSetupType (VOID);
BOOL SetSetupType (DWORD dwSetupType);
BOOL RunSetup (VOID);

#endif /* __SETUP_H__ */

/* EOF */
