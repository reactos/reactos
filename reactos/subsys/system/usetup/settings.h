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
/* $Id: settings.h,v 1.1 2004/05/30 14:54:02 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/settings.h
 * PURPOSE:         Device settings support functions
 * PROGRAMMER:      Eric Kohl
 */

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

PGENERIC_LIST
CreateComputerTypeList(HINF InfFile);

PGENERIC_LIST
CreateDisplayDriverList(HINF InfFile);

PGENERIC_LIST
CreateKeyboardDriverList(HINF InfFile);

PGENERIC_LIST
CreateKeyboardLayoutList(HINF InfFile);

BOOLEAN
ProcessKeyboardLayoutRegistry(PGENERIC_LIST List);

BOOLEAN
ProcessKeyboardLayoutFiles(PGENERIC_LIST List);

PGENERIC_LIST
CreateMouseDriverList(HINF InfFile);

#endif /* __SETTINGS_H__ */

/* EOF */
